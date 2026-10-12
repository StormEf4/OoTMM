#include <combo.h>
#include <combo/player.h>
#include <combo/config.h>
#include <combo/mask.h>
#include <combo/magic.h>
#include <combo/math.h>
#include <combo/sfx.h>
#include <combo/collision.h>
#include <combo/age.h>
#include <combo/oot/giant_mask.h>
#include <combo/oot/fierce_deity.h>
#include <combo/oot/transform_mask.h>

/*
 * Giant's Mask (OoT)
 *
 * An OoT port of Giant's Mask Anywhere (see src/mm/giant_mask.c). Wearing the mask makes Link
 * grow 10x, drains magic like in Twinmold's Lair, and the mask comes off when leaving the area.
 *
 * What scales:
 * - Link's model, sword hitbox and body collider
 * - ceiling, wall and ledge heights (player age properties)
 * - Player_GetHeight, which the camera and many actors use
 * - movement: the player code keeps normal speeds, jump velocities and gravity, and the asm hooks
 *   in src/oot/patches/giant_mask_oot.S multiply the step applied to Link's position, so he runs,
 *   jumps and falls like normal Link at 10x the size. The same hooks scale the collision distances
 *   the player code hard-codes (wall height, floor check, ledge walk-off) and convert the world
 *   distances it reads back (floor distance, fall distance, ledge heights) into normal units.
 */

#define GIANT_SCALE_FACTOR          10.0f
#define GIANT_PLAYER_SCALE          0.1f
#define NORMAL_PLAYER_SCALE         0.01f

#define GIANT_GROW_FRAMES           40
#define GIANT_SHRINK_FRAMES         30
#define GIANT_MAGIC_DRAIN_FRAMES    20

/* The first fields of OoT's PlayerAgeProperties, which hold the heights we scale */
typedef struct
{
    f32 ceilingCheckHeight;
    f32 unk_04;
    f32 unk_08;
    f32 unk_0C;
    f32 unk_10;
    f32 unk_14;
    f32 unk_18;
    f32 unk_1C;
    f32 unk_20;
    f32 unk_24;
    f32 unk_28;
    f32 unk_2C;
    f32 unk_30;
    f32 unk_34;
    f32 wallCheckRadius;
    f32 unk_3C;
    f32 unk_40;
}
GiantMaskAgeProperties;

typedef enum
{
    GIANT_STATE_NORMAL,
    GIANT_STATE_GROWING,
    GIANT_STATE_GIANT,
    GIANT_STATE_SHRINKING,
}
GiantState;

static s32 sState;
static s32 sTimer;
static s32 sMagicTimer;
static s32 sFrozePlayer;
static f32 sPlayerScale = NORMAL_PLAYER_SCALE;

static GiantMaskAgeProperties* sGrownAgeProperties;
static GiantMaskAgeProperties sAgePropertiesBackup;

static s16 sCylinderRadiusBase;
static s16 sCylinderRadiusWritten;

s32 GiantMaskOot_IsGiant(void)
{
    return sState != GIANT_STATE_NORMAL;
}

s32 GiantMaskOot_IsTransforming(void)
{
    return sState == GIANT_STATE_GROWING || sState == GIANT_STATE_SHRINKING;
}

/* Fixed modifier: physics switch to giant values for the whole transformation */
static f32 GiantMaskOot_GetScaleModifier(void)
{
    return (sState == GIANT_STATE_NORMAL) ? 1.0f : GIANT_SCALE_FACTOR;
}

/* Smooth modifier: follows Link's size during the transformation */
static f32 GiantMaskOot_GetSmoothScaleModifier(void)
{
    return sPlayerScale * (1.0f / NORMAL_PLAYER_SCALE);
}

static s32 GiantMaskOot_HasMagic(void)
{
    return comboIsChateauActive() || gSave.info.playerData.magic > 0;
}

static s32 GiantMaskOot_HasRoomAbove(PlayState* play, Player* player)
{
    GiantMaskAgeProperties* props;
    CollisionPoly* poly;
    Vec3f from;
    Vec3f to;
    Vec3f result;
    s32 bgId;

    props = (GiantMaskAgeProperties*)player->ageProperties;
    from = player->actor.world.pos;
    from.y += 10.0f;
    to = player->actor.world.pos;
    to.y += props->ceilingCheckHeight * GIANT_SCALE_FACTOR;

    return !BgCheck_EntityLineTest1(&play->colCtx, &from, &to, &result, &poly, 0, 0, 1, 1, &bgId);
}

static s32 GiantMaskOot_CanGrow(PlayState* play, Player* player)
{
    if (sState != GIANT_STATE_NORMAL)
        return 0;
    if (!GiantMaskOot_HasMagic())
        return 0;
    if (!(player->actor.bgCheckFlags & BGCHECKFLAG_GROUND))
        return 0;
    if (player->rideActor != NULL)
        return 0;
    if (player->stateFlags1 & (PLAYER_ACTOR_STATE_WATER | PLAYER_STATE1_23 | PLAYER_STATE1_29))
        return 0;
    if (player->stateFlags2 & PLAYER_STATE2_CRAWLING)
        return 0;
    if (player->csAction != PLAYER_CSACTION_NONE)
        return 0;
    return GiantMaskOot_HasRoomAbove(play, player);
}

static void GiantMaskOot_GrowAgeProperties(GiantMaskAgeProperties* props)
{
    props->ceilingCheckHeight *= GIANT_SCALE_FACTOR;
    props->unk_0C *= GIANT_SCALE_FACTOR;
    props->unk_10 *= GIANT_SCALE_FACTOR;
    props->unk_14 *= GIANT_SCALE_FACTOR;
    props->unk_18 *= GIANT_SCALE_FACTOR;
    props->unk_1C *= GIANT_SCALE_FACTOR;
    props->unk_24 *= GIANT_SCALE_FACTOR;
    props->unk_28 *= GIANT_SCALE_FACTOR;
    props->unk_2C *= GIANT_SCALE_FACTOR;
    props->unk_30 *= GIANT_SCALE_FACTOR;
    /* unk_34 is compared to sYDistToFloor, which the hooks keep in normal units */
    props->wallCheckRadius *= GIANT_SCALE_FACTOR;
    props->unk_3C *= GIANT_SCALE_FACTOR;
    props->unk_40 *= GIANT_SCALE_FACTOR;
}

static void GiantMaskOot_RestoreAgeProperties(Player* player)
{
    /* Only restore through a pointer we know is still valid and still grown */
    if (sGrownAgeProperties && (void*)sGrownAgeProperties == player->ageProperties && sGrownAgeProperties->ceilingCheckHeight >= 200.0f)
        *sGrownAgeProperties = sAgePropertiesBackup;
    sGrownAgeProperties = NULL;
}

static void GiantMaskOot_UpdateAgeProperties(Player* player)
{
    GiantMaskAgeProperties* props;

    props = (GiantMaskAgeProperties*)player->ageProperties;
    if (!props)
        return;

    if (sState == GIANT_STATE_NORMAL)
    {
        GiantMaskOot_RestoreAgeProperties(player);
        return;
    }

    /* The player overlay (and its data) is reloaded when unpausing, so check the actual values */
    if (props->ceilingCheckHeight < 200.0f)
    {
        sAgePropertiesBackup = *props;
        sGrownAgeProperties = props;
        GiantMaskOot_GrowAgeProperties(props);
    }
}

static void GiantMaskOot_ApplyPhysics(Player* player)
{
    GiantMaskOot_UpdateAgeProperties(player);

    if (sState != GIANT_STATE_NORMAL)
        Actor_SetScale(&player->actor, sPlayerScale);
}

static void GiantMaskOot_UpdateColliders(Player* player)
{
    s32 i;

    /* The body cylinder radius is only set once, the height follows the model on its own */
    if (player->cylinder.dim.radius != sCylinderRadiusWritten)
        sCylinderRadiusBase = player->cylinder.dim.radius;
    player->cylinder.dim.radius = (s16)(sCylinderRadiusBase * GiantMaskOot_GetSmoothScaleModifier());
    sCylinderRadiusWritten = player->cylinder.dim.radius;

    /* A giant's sword hits like a hammer and an explosion */
    if (sState == GIANT_STATE_GIANT)
    {
        for (i = 0; i < ARRAY_COUNT(player->meleeWeaponQuads); ++i)
            player->meleeWeaponQuads[i].elem.atDmgInfo.dmgFlags |= (DMG_EXPLOSIVE | DMG_HAMMER_SWING);
    }
}

static void GiantMaskOot_Freeze(PlayState* play, Player* player)
{
    if (player->csAction == PLAYER_CSACTION_NONE)
    {
        Player_SetCsActionWithHaltedActors(play, NULL, PLAYER_CSACTION_1);
        sFrozePlayer = 1;
    }
}

static void GiantMaskOot_Unfreeze(PlayState* play, Player* player)
{
    if (sFrozePlayer && player->csAction == PLAYER_CSACTION_1)
        Player_SetCsActionWithHaltedActors(play, NULL, PLAYER_CSACTION_7);
    sFrozePlayer = 0;
}

static void GiantMaskOot_StartGrowing(PlayState* play, Player* player)
{
    sState = GIANT_STATE_GROWING;
    sTimer = 0;
    sMagicTimer = 0;
    sPlayerScale = NORMAL_PLAYER_SCALE;
    GiantMaskOot_Freeze(play, player);
    PlaySound(NA_SE_PL_MAGIC_WIND_WARP);
}

static void GiantMaskOot_StartShrinking(PlayState* play, Player* player)
{
    sState = GIANT_STATE_SHRINKING;
    sTimer = 0;
    GiantMaskOot_Freeze(play, player);
    PlaySound(NA_SE_PL_MAGIC_WIND_WARP);
}

static void GiantMaskOot_RemoveMask(PlayState* play, Player* player)
{
    if (player->mask == MASK_GIANT)
    {
        player->mask = MASK_NONE;
        Interface_LoadItemIconImpl(play, 0);
    }
}

void GiantMaskOot_UseMask(PlayState* play, Player* player)
{
    if (GiantMaskOot_IsTransforming())
        return;

    /* Taking the mask off */
    if (player->mask == MASK_GIANT)
    {
        player->mask = MASK_NONE;
        PlaySound(0x835);
        if (sState == GIANT_STATE_GIANT)
            GiantMaskOot_StartShrinking(play, player);
        return;
    }

    /* Wearing another mask: same as vanilla, the mask is removed */
    if (player->mask != MASK_NONE)
    {
        player->mask = MASK_NONE;
        PlaySound(0x835);
        return;
    }

    if (!GiantMaskOot_CanGrow(play, player))
    {
        PlaySound(NA_SE_SY_ERROR);
        return;
    }

    player->mask = MASK_GIANT;
    PlaySound(0x835);
    GiantMaskOot_StartGrowing(play, player);
}

void GiantMaskOot_Reset(PlayState* play, Player* player)
{
    if (player)
    {
        GiantMaskOot_RestoreAgeProperties(player);
        GiantMaskOot_RemoveMask(play, player);
    }

    sState = GIANT_STATE_NORMAL;
    sTimer = 0;
    sMagicTimer = 0;
    sFrozePlayer = 0;
    sPlayerScale = NORMAL_PLAYER_SCALE;
    sCylinderRadiusBase = 0;
    sCylinderRadiusWritten = 0;
}

static void GiantMaskOot_UpdateState(Player* player, PlayState* play)
{
    switch (sState)
    {
    case GIANT_STATE_NORMAL:
        break;

    case GIANT_STATE_GROWING:
        sTimer++;
        Math_ApproachF(&sPlayerScale, GIANT_PLAYER_SCALE, 0.15f, 0.006f);
        if (sTimer >= GIANT_GROW_FRAMES)
        {
            sPlayerScale = GIANT_PLAYER_SCALE;
            sState = GIANT_STATE_GIANT;
            GiantMaskOot_Unfreeze(play, player);
        }
        break;

    case GIANT_STATE_GIANT:
        /* The mask was removed by something else (cutscene, item, ...) */
        if (player->mask != MASK_GIANT)
        {
            GiantMaskOot_StartShrinking(play, player);
            break;
        }

        /* Same magic drain as in Twinmold's Lair */
        if (!comboIsChateauActive() && gSaveContext.magicState == MAGIC_STATE_IDLE)
        {
            if (++sMagicTimer >= GIANT_MAGIC_DRAIN_FRAMES)
            {
                sMagicTimer = 0;
                if (gSave.info.playerData.magic > 0)
                    gSave.info.playerData.magic--;
            }

            if (gSave.info.playerData.magic <= 0)
            {
                GiantMaskOot_RemoveMask(play, player);
                GiantMaskOot_StartShrinking(play, player);
            }
        }
        break;

    case GIANT_STATE_SHRINKING:
        sTimer++;
        Math_ApproachF(&sPlayerScale, NORMAL_PLAYER_SCALE, 0.15f, 0.006f);
        if (sTimer >= GIANT_SHRINK_FRAMES)
        {
            sPlayerScale = NORMAL_PLAYER_SCALE;
            sState = GIANT_STATE_NORMAL;
            GiantMaskOot_Unfreeze(play, player);

            Actor_SetScale(&player->actor, NORMAL_PLAYER_SCALE);
        }
        break;
    }
}

void GiantMaskOot_BeforePlayerUpdate(Player* player, PlayState* play)
{
    if (player != GET_PLAYER(play))
        return;

    GiantMaskOot_UpdateState(player, play);
    GiantMaskOot_ApplyPhysics(player);
}

void GiantMaskOot_AfterPlayerUpdate(Player* player, PlayState* play)
{
    if (player != GET_PLAYER(play))
        return;

    /* The player may have reset its scale or boot data during the update */
    GiantMaskOot_ApplyPhysics(player);
    if (sState != GIANT_STATE_NORMAL || sCylinderRadiusWritten != 0)
        GiantMaskOot_UpdateColliders(player);
    if (sState == GIANT_STATE_NORMAL)
        sCylinderRadiusWritten = 0;
}

/* Replaces Player_GetHeight, used by the camera and many actors */
float GiantMaskOot_PlayerGetHeight(Player* player)
{
    f32 height;

    /* Fierce Deity's Mask uses MM's Fierce Deity height */
    if (player->actor.id == ACTOR_PLAYER && FierceDeityOot_IsFdForm())
        return ((player->stateFlags1 & PLAYER_STATE1_23) ? 32.0f : 0.0f) + FierceDeityOot_GetHeight();

    /* Deku, Goron and Zora use MM's heights */
    if (player->actor.id == ACTOR_PLAYER && TransformOot_GetForm() != TRANSFORM_OOT_FORM_NONE)
        return ((player->stateFlags1 & PLAYER_STATE1_23) ? 32.0f : 0.0f) + TransformOot_GetHeight(player);

    height = (player->stateFlags1 & PLAYER_STATE1_23) ? 32.0f : 0.0f;
    height += (gSave.age == AGE_ADULT) ? 68.0f : 44.0f;

    if (player->actor.id == ACTOR_PLAYER)
        height *= GiantMaskOot_GetSmoothScaleModifier();
    return height;
}

PATCH_FUNC(0x80021600, GiantMaskOot_PlayerGetHeight);

/*
 * Movement physics, called from src/oot/patches/giant_mask_oot.S
 * Those hooks are only in the ROM when the Giant's Mask is in OoT (patch group PG_OOT_GIANT_MASK).
 */
static f32 GiantMaskOot_GetActorModifier(Actor* actor)
{
    if (sState == GIANT_STATE_NORMAL || gPlay == NULL || actor != &GET_PLAYER(gPlay)->actor)
        return 1.0f;
    return GIANT_SCALE_FACTOR;
}

/* Player_UpdateCommon: moves Link by his velocity, scaled so a normal jump or run becomes a giant one */
void GiantMaskOot_PlayerUpdatePos(Actor* actor)
{
    Vec3f velocity;
    f32 modifier;

    modifier = GiantMaskOot_GetActorModifier(actor);
    if (modifier == 1.0f)
    {
        Actor_UpdatePos(actor);
        return;
    }

    velocity = actor->velocity;
    actor->velocity.x *= modifier;
    actor->velocity.y *= modifier;
    actor->velocity.z *= modifier;
    Actor_UpdatePos(actor);
    actor->velocity = velocity;
}

/* Player_ProcessSceneCollision and climbing: the wall height (26) is hard-coded */
void GiantMaskOot_PlayerUpdateBgCheckInfo(PlayState* play, Actor* actor, float wallCheckHeight, float wallCheckRadius, float ceilingCheckHeight, u32 flags)
{
    Actor_UpdateBgCheckInfo(play, actor, wallCheckHeight * GiantMaskOot_GetActorModifier(actor), wallCheckRadius, ceilingCheckHeight, flags);
}

/* Actor floor check: how far above the actor the floor is looked up */
f32 GiantMaskOot_GetFloorCheckOffset(Actor* actor)
{
    return 50.0f * GiantMaskOot_GetActorModifier(actor);
}

/* Replaces func_8002E234: how far above the floor an actor can be and still stand on it */
s32 GiantMaskOot_FloorLeaveCheck(Actor* actor, f32 floorHeightDiff, s32 flags)
{
    if ((actor->bgCheckFlags & BGCHECKFLAG_GROUND) && (floorHeightDiff < -11.0f * GiantMaskOot_GetActorModifier(actor)))
    {
        actor->bgCheckFlags &= ~BGCHECKFLAG_GROUND;
        actor->bgCheckFlags |= BGCHECKFLAG_GROUND_LEAVE;

        if ((actor->velocity.y < 0.0f) && (flags & UPDBGCHECKINFO_FLAG_OOT_4))
            actor->velocity.y = 0.0f;

        return 0;
    }

    return 1;
}

/* Converts a player world distance (floor distance) into normal units */
f32 GiantMaskOot_ScaleDownDistance(f32 distance)
{
    if (sState == GIANT_STATE_NORMAL)
        return distance;
    return distance * (1.0f / GIANT_SCALE_FACTOR);
}

/* func_8083AA10: returns the fall start height that gives the fall distance in normal units */
s32 GiantMaskOot_ScaleFallStart(s32 fallStartHeight, s32 posY)
{
    if (sState == GIANT_STATE_NORMAL)
        return fallStartHeight;
    return posY + (s32)((fallStartHeight - posY) * (1.0f / GIANT_SCALE_FACTOR));
}

/*
 * Player_ActionHandler_12: the climb animation raises Link by a fixed height (in model units), and
 * shape.yOffset (also in model units) covers the rest of the ledge height.
 * offset = yDistToLedge - animationHeight, the result is multiplied by 100 (1 / normal scale).
 */
f32 GiantMaskOot_GetLedgeClimbOffset(Player* player, f32 offset)
{
    f32 modifier;
    f32 animationHeight;

    modifier = GiantMaskOot_GetActorModifier(&player->actor);
    if (modifier == 1.0f)
        return offset;

    /* At giant scale the animation covers modifier times more, and yOffset counts modifier times more */
    animationHeight = (player->yDistToLedge - offset) * modifier;
    return (player->yDistToLedge - animationHeight) / modifier;
}

/* Jumps whose velocity comes from the ledge height use it in normal units */
f32 GiantMaskOot_GetJumpYDistToLedge(Player* player)
{
    return player->yDistToLedge / GiantMaskOot_GetActorModifier(&player->actor);
}
