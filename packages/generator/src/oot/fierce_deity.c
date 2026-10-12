#include <combo.h>
#include <combo/player.h>
#include <combo/config.h>
#include <combo/mask.h>
#include <combo/magic.h>
#include <combo/math.h>
#include <combo/matrix.h>
#include <combo/sfx.h>
#include <combo/collision.h>
#include <combo/age.h>
#include <combo/oot/fierce_deity.h>

/*
 * Fierce Deity's Mask (OoT)
 *
 * OoT has no transformation masks, so this is built from scratch, in C only, the same way as
 * the OoT Giant's Mask (giant_mask.c): a small state machine run around Player_Update, and
 * everything restored when the mask comes off or Link leaves the area.
 *
 * - Model: MM's object_link_boy (Fierce Deity) is loaded as a foreign object and its skeleton
 *   is drawn with Link's joint table instead of Link's own (FierceDeityOot_DrawSkeleton, called
 *   from Player_DrawFlexLod). The limb layout is the same in both games.
 * - Size: actor scale 0.015 and the adult age properties scaled by 1.5, which is exactly what
 *   MM uses for Fierce Deity. A child Link gets the adult properties too.
 * - Sword: giant-level damage (4, 8 for jump attacks, like MM), longer reach, and a sword beam
 *   (1 magic) on slashes while Z-targeting. The beam is drawn with MM's sword beam graphics.
 * - Other items can't be used (Player_UseItem, dpad.c).
 *
 * See docs/custom/fierce-deity.md.
 */

#define FD_OBJECT_ID            (0x0010 | MASK_FOREIGN_OBJECT) /* MM object_link_boy */
#define FD_SKEL_HEADER          0xd878
#define FD_DL_LHAND_SWORD       0x060080f0

#define FD_SCALE                0.015f
#define NORMAL_SCALE            0.01f
#define FD_SIZE_FACTOR          1.5f
#define FD_HEIGHT               124.0f
#define FD_SWORD_LENGTH         5500.0f

#define FD_TRANSFORM_FRAMES     20
#define FD_TRANSFORM_SWAP_FRAME 10

#define AGE_PROPERTIES_SIZE     0xd4

/* Vanilla player override functions (their entry is patched to OoTMM wrappers) */
#define OVERRIDE_FIRST_PERSON   ((void*)0x8007ad80)

/* sMeleeWeaponLengths: Master, Kokiri (rewritten every frame by Player.c), Biggoron */
#define SWORD_LENGTH_COUNT      3
static f32* const kSwordLengths[SWORD_LENGTH_COUNT] = { (f32*)0x800f7bb4, (f32*)0x800f7bb8, (f32*)0x800f7bbc };

/* MM gameplay_keep sword beam: two textures, vertices and gSwordBeamDL */
#define MM_KEEP_OBJECT_ID       (0x0001 | MASK_FOREIGN_OBJECT)
#define BEAM_DATA_START         0x269e0
#define BEAM_DATA_END           0x281dc
#define BEAM_DL                 0x04027ca0
#define BEAM_COUNT              3
#define BEAM_SPEED              80.0f
#define BEAM_SCALE_TARGET       12.0f
#define BEAM_DAMAGE             3

typedef enum
{
    FD_STATE_NORMAL,
    FD_STATE_PUTTING_ON,
    FD_STATE_FIERCE_DEITY,
    FD_STATE_TAKING_OFF,
}
FierceDeityState;

/* The first fields of OoT's PlayerAgeProperties, which hold the heights we scale (same as giant_mask.c) */
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
FierceDeityAgeProperties;

typedef struct
{
    s32     active;
    Vec3f   pos;
    Vec3f   velocity;
    Vec3s   rot;
    f32     scale;
    f32     life;
    s32     timer;
}
FierceDeityBeam;

static s32 sState;
static s32 sTimer;
static s32 sFrozePlayer;
static s32 sIsFdForm;

static ALIGNED(8) u8 sAgeProperties[AGE_PROPERTIES_SIZE];
static void* sOriginalAgeProperties;

static s16 sRunSpeedWritten;
static s16 sAccelWritten;

static s16 sCylinderRadiusBase;
static s16 sCylinderRadiusWritten;

static s32 sSwordLengthsSaved;
static f32 sSwordLengthsBackup[SWORD_LENGTH_COUNT];

static s8 sPrevMeleeWeaponState;

static void* sVanillaOverride;
static s32 sDrawFirstPerson;
static Gfx* sOpaAfterSkeleton;
static u32 sOpaAfterSkeletonFrame;

static Gfx sEmptyDL[] = { gsSPEndDisplayList() };

static FierceDeityBeam sBeams[BEAM_COUNT];
static ColliderCylinder sBeamColliders[BEAM_COUNT];
static Actor* sBeamCollidersOwner;
static u8* sBeamData;

static ColliderCylinderInit sBeamColliderInit = {
    {
        COL_MATERIAL_NONE,
        AT_ON | AT_TYPE_PLAYER,
        AC_NONE,
        OC1_NONE,
        OC2_TYPE_1,
        COLSHAPE_CYLINDER,
    },
    {
        ELEM_MATERIAL_UNK2,
        { DMG_SPIN_GIANT, 0x00, BEAM_DAMAGE },
        { 0xffcfffff, 0x00, 0x00 },
        ATELEM_ON | ATELEM_SFX_NONE,
        ACELEM_NONE,
        OCELEM_NONE,
    },
    { 0, 40, -20, { 0, 0, 0 } },
};

s32 FierceDeityOot_IsActive(void)
{
    return sState != FD_STATE_NORMAL;
}

s32 FierceDeityOot_IsTransforming(void)
{
    return sState == FD_STATE_PUTTING_ON || sState == FD_STATE_TAKING_OFF;
}

s32 FierceDeityOot_IsFdForm(void)
{
    return sIsFdForm;
}

f32 FierceDeityOot_GetHeight(void)
{
    return FD_HEIGHT;
}

static s32 FierceDeityOot_CanTransform(Player* player)
{
    if (sState != FD_STATE_NORMAL)
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
    return 1;
}

/* Age properties: the adult ones scaled by 1.5, like MM's Fierce Deity */
static void FierceDeityOot_ApplyAgeProperties(Player* player)
{
    FierceDeityAgeProperties* props;
    u8* adult;

    if (!player->ageProperties || player->ageProperties == sAgeProperties)
        return;

    /* OoT's table is { adult, child } */
    sOriginalAgeProperties = player->ageProperties;
    adult = (u8*)player->ageProperties;
    if (gSave.age != AGE_ADULT)
        adult -= AGE_PROPERTIES_SIZE;
    memcpy(sAgeProperties, adult, AGE_PROPERTIES_SIZE);

    props = (FierceDeityAgeProperties*)sAgeProperties;
    props->ceilingCheckHeight *= FD_SIZE_FACTOR;
    props->unk_0C *= FD_SIZE_FACTOR;
    props->unk_10 *= FD_SIZE_FACTOR;
    props->unk_14 *= FD_SIZE_FACTOR;
    props->unk_18 *= FD_SIZE_FACTOR;
    props->unk_1C *= FD_SIZE_FACTOR;
    props->unk_24 *= FD_SIZE_FACTOR;
    props->unk_28 *= FD_SIZE_FACTOR;
    props->unk_2C *= FD_SIZE_FACTOR;
    props->unk_30 *= FD_SIZE_FACTOR;
    props->unk_34 *= FD_SIZE_FACTOR;
    props->wallCheckRadius *= FD_SIZE_FACTOR;
    props->unk_3C *= FD_SIZE_FACTOR;
    props->unk_40 *= FD_SIZE_FACTOR;

    player->ageProperties = sAgeProperties;
}

static void FierceDeityOot_RestoreAgeProperties(Player* player)
{
    if (sOriginalAgeProperties && player->ageProperties == sAgeProperties)
        player->ageProperties = sOriginalAgeProperties;
    sOriginalAgeProperties = NULL;
}

/* Fierce Deity runs 1.5 times faster. Player_SetBootData resets these on boot and room changes. */
static void FierceDeityOot_ApplyRegs(void)
{
    if (R_RUN_SPEED_LIMIT != sRunSpeedWritten)
    {
        R_RUN_SPEED_LIMIT = (s16)(R_RUN_SPEED_LIMIT * FD_SIZE_FACTOR);
        sRunSpeedWritten = R_RUN_SPEED_LIMIT;
    }

    if (REG(19) != sAccelWritten)
    {
        REG(19) = (s16)(REG(19) * FD_SIZE_FACTOR);
        sAccelWritten = REG(19);
    }
}

static void FierceDeityOot_SetSwordLengths(void)
{
    s32 i;

    if (!sSwordLengthsSaved)
    {
        for (i = 0; i < SWORD_LENGTH_COUNT; ++i)
            sSwordLengthsBackup[i] = *kSwordLengths[i];
        sSwordLengthsSaved = 1;
    }

    for (i = 0; i < SWORD_LENGTH_COUNT; ++i)
        *kSwordLengths[i] = FD_SWORD_LENGTH;
}

static void FierceDeityOot_RestoreSwordLengths(void)
{
    s32 i;

    if (!sSwordLengthsSaved)
        return;

    /* The Kokiri Sword length is set again by Player.c every frame */
    for (i = 0; i < SWORD_LENGTH_COUNT; ++i)
        *kSwordLengths[i] = sSwordLengthsBackup[i];
    sSwordLengthsSaved = 0;
}

static void FierceDeityOot_ApplyForm(Player* player)
{
    FierceDeityOot_ApplyAgeProperties(player);
    FierceDeityOot_ApplyRegs();
    Actor_SetScale(&player->actor, FD_SCALE);
}

static void FierceDeityOot_LeaveForm(PlayState* play, Player* player)
{
    if (!sIsFdForm)
        return;

    sIsFdForm = 0;
    FierceDeityOot_RestoreAgeProperties(player);
    FierceDeityOot_RestoreSwordLengths();
    Actor_SetScale(&player->actor, NORMAL_SCALE);

    /* Restores the movement REGs through Player_SetBootData */
    sRunSpeedWritten = 0;
    sAccelWritten = 0;
    if (play)
        UpdateEquipment(play, player);
}

static void FierceDeityOot_Freeze(PlayState* play, Player* player)
{
    if (player->csAction == PLAYER_CSACTION_NONE)
    {
        Player_SetCsActionWithHaltedActors(play, NULL, PLAYER_CSACTION_1);
        sFrozePlayer = 1;
    }
}

static void FierceDeityOot_Unfreeze(PlayState* play, Player* player)
{
    if (sFrozePlayer && player->csAction == PLAYER_CSACTION_1)
        Player_SetCsActionWithHaltedActors(play, NULL, PLAYER_CSACTION_7);
    sFrozePlayer = 0;
}

static void FierceDeityOot_StartPuttingOn(PlayState* play, Player* player)
{
    sState = FD_STATE_PUTTING_ON;
    sTimer = 0;
    FierceDeityOot_Freeze(play, player);
    PlaySound(NA_SE_PL_MAGIC_WIND_WARP);

    /* Start loading the model now */
    comboGetObject(FD_OBJECT_ID);
}

static void FierceDeityOot_StartTakingOff(PlayState* play, Player* player)
{
    sState = FD_STATE_TAKING_OFF;
    sTimer = 0;
    FierceDeityOot_Freeze(play, player);
    PlaySound(NA_SE_PL_MAGIC_WIND_WARP);
}

static void FierceDeityOot_RemoveMask(PlayState* play, Player* player)
{
    if (player->mask == MASK_FIERCE_DEITY)
    {
        player->mask = MASK_NONE;
        Interface_LoadItemIconImpl(play, 0);
    }
}

void FierceDeityOot_UseMask(PlayState* play, Player* player)
{
    if (FierceDeityOot_IsTransforming())
        return;

    /* Taking the mask off */
    if (sState == FD_STATE_FIERCE_DEITY)
    {
        PlaySound(0x835);
        FierceDeityOot_StartTakingOff(play, player);
        return;
    }

    /* Wearing another mask: same as vanilla, the mask is removed */
    if (player->mask != MASK_NONE)
    {
        player->mask = MASK_NONE;
        PlaySound(0x835);
        return;
    }

    if (!FierceDeityOot_CanTransform(player))
    {
        PlaySound(NA_SE_SY_ERROR);
        return;
    }

    player->mask = MASK_FIERCE_DEITY;
    PlaySound(0x835);
    FierceDeityOot_StartPuttingOn(play, player);
}

static void FierceDeityOot_ClearBeams(void)
{
    s32 i;

    for (i = 0; i < BEAM_COUNT; ++i)
        sBeams[i].active = 0;
}

void FierceDeityOot_Reset(PlayState* play, Player* player)
{
    if (player)
    {
        FierceDeityOot_LeaveForm(NULL, player);
        FierceDeityOot_RemoveMask(play, player);

        /* Don't carry Fierce Deity's running speed into the next area */
        if (sState != FD_STATE_NORMAL && player->speedXZ > 6.0f)
            player->speedXZ = 6.0f;
    }

    FierceDeityOot_RestoreSwordLengths();
    FierceDeityOot_ClearBeams();
    sState = FD_STATE_NORMAL;
    sTimer = 0;
    sFrozePlayer = 0;
    sIsFdForm = 0;
    sOriginalAgeProperties = NULL;
    sRunSpeedWritten = 0;
    sAccelWritten = 0;
    sCylinderRadiusBase = 0;
    sCylinderRadiusWritten = 0;
    sPrevMeleeWeaponState = 0;
    sBeamCollidersOwner = NULL;
}

static void FierceDeityOot_UpdateState(Player* player, PlayState* play)
{
    switch (sState)
    {
    case FD_STATE_NORMAL:
        break;

    case FD_STATE_PUTTING_ON:
        sTimer++;
        if (sTimer == FD_TRANSFORM_SWAP_FRAME)
        {
            sIsFdForm = 1;
            PlaySound(NA_SE_IT_ROLLING_CUT_LV1);
        }
        if (sTimer >= FD_TRANSFORM_FRAMES)
        {
            sState = FD_STATE_FIERCE_DEITY;
            FierceDeityOot_Unfreeze(play, player);
        }
        break;

    case FD_STATE_FIERCE_DEITY:
        /* The mask was removed by something else (cutscene, item, ...) */
        if (player->mask != MASK_FIERCE_DEITY)
            FierceDeityOot_StartTakingOff(play, player);
        break;

    case FD_STATE_TAKING_OFF:
        sTimer++;
        if (sTimer == FD_TRANSFORM_SWAP_FRAME)
        {
            FierceDeityOot_RemoveMask(play, player);
            FierceDeityOot_LeaveForm(play, player);
        }
        if (sTimer >= FD_TRANSFORM_FRAMES)
        {
            sState = FD_STATE_NORMAL;
            FierceDeityOot_Unfreeze(play, player);
        }
        break;
    }
}

static void FierceDeityOot_UpdateColliders(Player* player)
{
    u32 flags;
    s32 i;

    /* Body cylinder, like giant_mask.c */
    if (player->cylinder.dim.radius != sCylinderRadiusWritten)
        sCylinderRadiusBase = player->cylinder.dim.radius;
    player->cylinder.dim.radius = (s16)(sCylinderRadiusBase * (sIsFdForm ? FD_SIZE_FACTOR : 1.0f));
    sCylinderRadiusWritten = sIsFdForm ? player->cylinder.dim.radius : 0;

    if (!sIsFdForm)
        return;

    /*
     * Damage tables use the highest damage bit, so the flags are replaced rather than OR'd.
     * Biggoron's Sword level damage gives MM's Fierce Deity numbers: 4, and 8 for jump attacks.
     */
    for (i = 0; i < ARRAY_COUNT(player->meleeWeaponQuads); ++i)
    {
        flags = player->meleeWeaponQuads[i].elem.atDmgInfo.dmgFlags;
        if (flags & DMG_JUMP_SLASH)
        {
            player->meleeWeaponQuads[i].elem.atDmgInfo.dmgFlags = DMG_JUMP_GIANT;
            player->meleeWeaponQuads[i].elem.atDmgInfo.damage = 8;
        }
        else if (flags & DMG_SPIN_ATTACK)
        {
            player->meleeWeaponQuads[i].elem.atDmgInfo.dmgFlags = DMG_SPIN_GIANT;
            player->meleeWeaponQuads[i].elem.atDmgInfo.damage = 4;
        }
        else if (flags & DMG_SLASH)
        {
            player->meleeWeaponQuads[i].elem.atDmgInfo.dmgFlags = DMG_SLASH_GIANT;
            player->meleeWeaponQuads[i].elem.atDmgInfo.damage = 4;
        }
    }
}

static s32 FierceDeityOot_IsZTargeting(Player* player)
{
    return !!(player->stateFlags1 & (PLAYER_STATE1_HOSTILE_LOCK_ON | PLAYER_STATE1_FRIENDLY_ACTOR_FOCUS | PLAYER_STATE1_PARALLEL));
}

static s32 FierceDeityOot_HoldsSword(Player* player)
{
    return player->leftHandType == PLAYER_MODELTYPE_LH_SWORD
        || player->leftHandType == PLAYER_MODELTYPE_LH_SWORD_2
        || player->leftHandType == PLAYER_MODELTYPE_LH_BGS;
}

/* Like in MM, Fierce Deity always has his sword in hand, even when OoT Link would have it sheathed */
static s32 FierceDeityOot_ShowsSword(Player* player)
{
    switch (player->leftHandType)
    {
    case PLAYER_MODELTYPE_LH_HAMMER:
    case PLAYER_MODELTYPE_LH_BOOMERANG:
    case PLAYER_MODELTYPE_LH_BOTTLE:
        return 0;
    default:
        return 1;
    }
}

static void FierceDeityOot_LoadBeamData(void)
{
    const ObjectData* keep;

    if (sBeamData)
        return;
    sBeamData = malloc(BEAM_DATA_END - BEAM_DATA_START);
    if (!sBeamData)
        return;
    keep = comboGetObjectData(MM_KEEP_OBJECT_ID);
    LoadFile(sBeamData, keep->vromStart + BEAM_DATA_START, BEAM_DATA_END - BEAM_DATA_START);
}

static void FierceDeityOot_SpawnBeam(PlayState* play, Player* player)
{
    FierceDeityBeam* beam;
    Vec3f diff;
    f32 distXZ;
    s16 pitch;
    s32 i;

    beam = NULL;
    for (i = 0; i < BEAM_COUNT; ++i)
    {
        if (!sBeams[i].active)
        {
            beam = &sBeams[i];
            break;
        }
    }
    if (!beam)
        return;

    /* 1 magic, like MM */
    if (!gSave.info.playerData.isMagicAcquired || gSaveContext.magicState != MAGIC_STATE_IDLE)
        return;
    if (!comboIsChateauActive() && !Magic_RequestChangeCustom(play, 1, MAGIC_CONSUME_NOW))
        return;

    FierceDeityOot_LoadBeamData();

    /* Aim at the lock-on target, otherwise straight ahead */
    pitch = 0;
    beam->pos = player->bodyPartsPos[PLAYER_BODYPART_WAIST];
    if (player->focusActor)
    {
        diff.x = player->focusActor->focus.pos.x - beam->pos.x;
        diff.y = player->focusActor->focus.pos.y - beam->pos.y;
        diff.z = player->focusActor->focus.pos.z - beam->pos.z;
        distXZ = sqrtf(diff.x * diff.x + diff.z * diff.z);
        pitch = Math_Atan2S(distXZ, diff.y);
    }

    beam->velocity.x = BEAM_SPEED * Math_CosS(pitch) * Math_SinS(player->actor.shape.rot.y);
    beam->velocity.y = BEAM_SPEED * Math_SinS(pitch);
    beam->velocity.z = BEAM_SPEED * Math_CosS(pitch) * Math_CosS(player->actor.shape.rot.y);
    beam->rot.x = pitch;
    beam->rot.y = player->actor.shape.rot.y + 0x8000;
    beam->rot.z = 0;
    beam->scale = 0.1f;
    beam->life = 1.0f;
    beam->timer = 0;
    beam->active = 1;

    Audio_PlaySfxGeneral(NA_SE_IT_ROLLING_CUT, &player->actor.projectedPos, 4, &gSfxDefaultFreqAndVolScale, &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
}

static void FierceDeityOot_UpdateBeams(PlayState* play, Player* player)
{
    FierceDeityBeam* beam;
    ColliderCylinder* col;
    s32 i;

    if (sBeamCollidersOwner != &player->actor)
    {
        for (i = 0; i < BEAM_COUNT; ++i)
        {
            Collider_InitCylinder(play, &sBeamColliders[i]);
            Collider_SetCylinder(play, &sBeamColliders[i], &player->actor, &sBeamColliderInit);
        }
        sBeamCollidersOwner = &player->actor;
    }

    for (i = 0; i < BEAM_COUNT; ++i)
    {
        beam = &sBeams[i];
        if (!beam->active)
            continue;

        /* Same timing and size as MM's EnMThunder_SwordBeam_Attack */
        if (Math_StepToF(&beam->life, 0.0f, 0.05f))
        {
            beam->active = 0;
            continue;
        }
        beam->timer++;
        beam->pos.x += beam->velocity.x;
        beam->pos.y += beam->velocity.y;
        beam->pos.z += beam->velocity.z;
        Math_SmoothStepToF(&beam->scale, BEAM_SCALE_TARGET, 0.6f, 2.0f, 0.0f);

        col = &sBeamColliders[i];
        col->dim.radius = (s16)(beam->scale * 5.0f);
        col->dim.height = 40;
        col->dim.yShift = -20;
        col->dim.pos.x = (s16)beam->pos.x;
        col->dim.pos.y = (s16)beam->pos.y;
        col->dim.pos.z = (s16)beam->pos.z;
        CollisionCheck_SetAT(play, &play->colChkCtx, &col->base);
    }
}

void FierceDeityOot_BeforePlayerUpdate(Player* player, PlayState* play)
{
    if (player != GET_PLAYER(play))
        return;

    FierceDeityOot_UpdateState(player, play);
    if (sIsFdForm)
        FierceDeityOot_ApplyForm(player);
}

void FierceDeityOot_AfterPlayerUpdate(Player* player, PlayState* play)
{
    s8 meleeWeaponState;

    if (player != GET_PLAYER(play))
        return;

    /* The player may have reset its scale or boot data during the update */
    if (sIsFdForm)
    {
        FierceDeityOot_ApplyForm(player);
        FierceDeityOot_SetSwordLengths();
    }

    if (sIsFdForm || sCylinderRadiusWritten != 0)
        FierceDeityOot_UpdateColliders(player);

    /* Sword beam: the first active frame of a regular slash while Z-targeting */
    meleeWeaponState = player->meleeWeaponState;
    if (sState == FD_STATE_FIERCE_DEITY && meleeWeaponState != 0 && sPrevMeleeWeaponState == 0
        && player->meleeWeaponAnimation < 22 && FierceDeityOot_IsZTargeting(player) && FierceDeityOot_HoldsSword(player))
    {
        FierceDeityOot_SpawnBeam(play, player);
    }
    sPrevMeleeWeaponState = meleeWeaponState;

    FierceDeityOot_UpdateBeams(play, player);
}

/* Draw */

static s32 FierceDeityOot_OverrideLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot, void* arg)
{
    s32 (*vanillaOverride)(PlayState*, s32, Gfx**, Vec3f*, Vec3s*, void*);
    Player* player;
    Gfx* fdList;
    s32 ret;

    player = (Player*)arg;
    vanillaOverride = sVanillaOverride;
    fdList = *dList;

    /* Head look, leg IK, body part positions... OoT's hand and sheath models are thrown away below */
    ret = vanillaOverride ? vanillaOverride(play, limbIndex, dList, pos, rot, arg) : 0;

    if (sDrawFirstPerson)
        *dList = NULL;
    else if (limbIndex == PLAYER_LIMB_L_HAND && fdList != NULL && FierceDeityOot_ShowsSword(player))
        *dList = (Gfx*)FD_DL_LHAND_SWORD;
    else
        *dList = fdList;

    /* OoTMM tints some limbs with the tunic color */
    OPEN_DISPS(play->state.gfxCtx);
    gDPSetEnvColor(POLY_OPA_DISP++, 0xff, 0xff, 0xff, 0xff);
    CLOSE_DISPS();

    return ret;
}

s32 FierceDeityOot_DrawSkeleton(PlayState* play, Vec3s* jointTable, s32 dListCount, void* overrideLimbDraw, void* postLimbDraw, void* arg, s32 lod)
{
    u8* obj;
    u8* header;
    void** limbs;
    s32 count;
    u32 savedSegment;
    s32 savedAge;

    if (!sIsFdForm)
        return 0;

    obj = comboGetObject(FD_OBJECT_ID);
    if (!obj)
        return 0;

    header = obj + FD_SKEL_HEADER;
    limbs = (void**)(obj + (*(u32*)header & 0xffffff));
    count = header[8];
    if (count < dListCount)
        count = dListCount;

    sVanillaOverride = overrideLimbDraw;
    sDrawFirstPerson = (overrideLimbDraw == OVERRIDE_FIRST_PERSON);

    /* The FD skeleton is resolved through segment 6 on both the CPU and the RSP */
    savedSegment = gSegments[6];
    gSegments[6] = (u32)obj - K0BASE;
    OPEN_DISPS(play->state.gfxCtx);
    gSPSegment(POLY_OPA_DISP++, 0x06, obj);
    gDPSetEnvColor(POLY_OPA_DISP++, 0xff, 0xff, 0xff, 0xff);
    CLOSE_DISPS();

    /* Draw as adult: no child root scaling, adult leg IK, like MM draws Fierce Deity */
    savedAge = gSave.age;
    gSave.age = AGE_ADULT;
    SkelAnime_DrawFlexLod(play, limbs, jointTable, count, FierceDeityOot_OverrideLimbDraw, postLimbDraw, arg, lod);
    gSave.age = savedAge;

    gSegments[6] = savedSegment;
    OPEN_DISPS(play->state.gfxCtx);
    gSPSegment(POLY_OPA_DISP++, 0x06, (void*)(savedSegment + K0BASE));
    sOpaAfterSkeleton = POLY_OPA_DISP;
    sOpaAfterSkeletonFrame = play->gameplayFrames;
    CLOSE_DISPS();

    return 1;
}

static void FierceDeityOot_DrawBeams(PlayState* play)
{
    FierceDeityBeam* beam;
    f32 alpha;
    s32 i;

    if (!sBeamData)
        return;

    OPEN_DISPS(play->state.gfxCtx);
    for (i = 0; i < BEAM_COUNT; ++i)
    {
        beam = &sBeams[i];
        if (!beam->active)
            continue;

        alpha = (beam->life > 0.9f) ? 1.0f : beam->life * (10.0f / 9.0f);

        Gfx_SetupDL25_Xlu(play->state.gfxCtx);
        Matrix_SetTranslateRotateYXZ(beam->pos.x, beam->pos.y, beam->pos.z, &beam->rot);
        Matrix_Scale(beam->scale * 0.02f, beam->scale * 0.02f, beam->scale * 0.02f, MTXMODE_APPLY);
        MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
        gSPSegment(POLY_XLU_DISP++, 0x04, sBeamData - BEAM_DATA_START);
        gSPSegment(POLY_XLU_DISP++, 0x08, Gfx_TwoTexScroll(play->state.gfxCtx, 0, 0, 0, 16, 64, 1, 0, 0x1ff - ((beam->timer * 20) & 0x1ff), 32, 128));
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, 170, 255, 255, (u8)(alpha * 255.0f));
        gDPSetEnvColor(POLY_XLU_DISP++, 0, 100, 255, 128);
        gSPDisplayList(POLY_XLU_DISP++, BEAM_DL);
        gSPSegment(POLY_XLU_DISP++, 0x04, (void*)(gSegments[4] + K0BASE));
    }
    CLOSE_DISPS();
}

/* Called from the mask draw right after Player_DrawImpl. Returns 1 when Fierce Deity is drawn. */
s32 FierceDeityOot_DrawMask(PlayState* play, Player* player)
{
    FierceDeityOot_DrawBeams(play);

    if (!sIsFdForm)
        return 0;

    /* Drop what Player_DrawImpl drew after the skeleton (adult gauntlets and boots) */
    if (sOpaAfterSkeleton && sOpaAfterSkeletonFrame == play->gameplayFrames)
    {
        OPEN_DISPS(play->state.gfxCtx);
        POLY_OPA_DISP = sOpaAfterSkeleton;
        CLOSE_DISPS();
    }
    sOpaAfterSkeleton = NULL;
    return 1;
}
