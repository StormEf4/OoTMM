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

/*
 * Giant's Mask (OoT)
 *
 * An OoT port of Giant's Mask Anywhere (see src/mm/giant_mask.c). Wearing the mask makes Link
 * grow 10x, drains magic like in Twinmold's Lair, and the mask comes off when leaving the area.
 *
 * Unlike the MM version, there are no assembly hooks into the OoT player overlay (finding the
 * hook sites needs the ROM). What scales:
 * - Link's model, sword hitbox and body collider
 * - movement speed, acceleration and deceleration (player REGs)
 * - ceiling, wall and ledge heights (player age properties)
 * - Player_GetHeight, which the camera and many actors use
 * Jump height, gravity and fall speed keep their normal values.
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

static void GiantMaskOot_GrowRegs(void)
{
    REG(48) *= 10;          /* Slow backwalk animation threshold */
    REG(19) *= 10;          /* Run acceleration */
    REG(32) /= 10;          /* Sidewalk animation speed multiplier */
    REG(36) /= 10;          /* Slow backwalk animation speed multiplier */
    REG(37) /= 10;          /* Walk speed threshold */
    REG(38) /= 10;          /* Walk animation speed multiplier */
    R_DECELERATE_RATE *= 10;
    R_RUN_SPEED_LIMIT *= 10;
    IREG(66) *= 10;         /* Baby jump threshold */
    IREG(69) /= 10;         /* Baby jump speed multiplier */
    MREG(95) /= 10;         /* Bow sidewalk animation */
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
    props->unk_34 *= GIANT_SCALE_FACTOR;
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

    /* Player_SetBootData resets these on boot changes and room changes */
    if (sState != GIANT_STATE_NORMAL && R_RUN_SPEED_LIMIT < 2000)
        GiantMaskOot_GrowRegs();

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

        /* Don't carry a giant's running speed into the next area */
        if (sState != GIANT_STATE_NORMAL && player->speedXZ > 6.0f)
            player->speedXZ = 6.0f;
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

            /* Restores the movement REGs through Player_SetBootData */
            UpdateEquipment(play, player);
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

    height = (player->stateFlags1 & PLAYER_STATE1_23) ? 32.0f : 0.0f;
    height += (gSave.age == AGE_ADULT) ? 68.0f : 44.0f;

    if (player->actor.id == ACTOR_PLAYER)
        height *= GiantMaskOot_GetSmoothScaleModifier();
    return height;
}

PATCH_FUNC(0x80021600, GiantMaskOot_PlayerGetHeight);
