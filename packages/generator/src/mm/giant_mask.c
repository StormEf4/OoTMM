#include <combo.h>
#include <combo/player.h>
#include <combo/config.h>
#include <combo/souls.h>
#include <combo/magic.h>
#include <combo/math.h>
#include <combo/matrix.h>
#include <combo/collision.h>
#include <combo/controller.h>
#include <combo/mm/giant_mask.h>

/*
 * Giant's Mask Anywhere
 *
 * In vanilla, the Giant's Mask only works in Twinmold's Lair: the boss actor drives the
 * transformation cutscene, and the whole arena shrinks around Link instead of Link growing.
 * Outside of the lair, nothing ever ends the transformation and the game softlocks.
 *
 * Here we drive the transformation ourselves and actually scale Link up by 10x. Player
 * physics constants (speed, gravity, jump, ledge and wall heights, ...) are scaled by the
 * same factor through the hooks in patches/giant_mask_anywhere.S.
 *
 * Twinmold's Lair is left untouched and keeps the vanilla behavior.
 *
 * Based on the Giant's Mask Anywhere implementation from the MM Randomizer.
 */

#define GIANT_SCALE_FACTOR          10.0f
#define GIANT_PLAYER_SCALE          0.1f
#define NORMAL_PLAYER_SCALE         0.01f

#define GIANT_CS_SKIP_BUTTONS       (BTN_A | BTN_B | BTN_CUP | BTN_CDOWN | BTN_CLEFT | BTN_CRIGHT)

typedef enum
{
    GIANT_CS_STATE_WAITING_FOR_MASK = 0,
    GIANT_CS_STATE_MASK_ON = 1,
    GIANT_CS_STATE_MASK_ON_SKIPPED = 2,
    GIANT_CS_STATE_MASK_OFF = 10,
    GIANT_CS_STATE_MASK_OFF_SKIPPED = 11,
}
GiantCsState;

typedef enum
{
    GIANT_CS_FLASH_STATE_NOT_STARTED,
    GIANT_CS_FLASH_STATE_STARTED,
    GIANT_CS_FLASH_STATE_INCREASE_ALPHA,
    GIANT_CS_FLASH_STATE_DECREASE_ALPHA,
}
GiantCsFlashState;

typedef void (*GiantMask_PlayerFunc)(PlayState* play, Player* player);
typedef s32 (*GiantMask_MathAsymStepToF)(f32* pValue, f32 target, f32 incrStep, f32 decrStep);

extern void CutsceneManager_ClearNextCutscenes(void);

static s32 sIsGiant;
static f32 sPlayerScale = NORMAL_PLAYER_SCALE;

static s32 sCsState;
static u32 sCsTimer;
static s16 sSubCamId;
static Vec3f sSubCamEye;
static Vec3f sSubCamAt;
static Vec3f sSubCamUp;
static f32 sSubCamUpRotZScale;
static f32 sSubCamAtVel;
static f32 sSubCamDistZFromPlayer;
static f32 sSubCamAtOffsetY;
static f32 sSubCamAtOffsetTargetY;
static s32 sCsFlashState;
static s16 sCsFlashAlpha;
static s32 sCanSkipMaskOnCs;
static s32 sCanSkipMaskOffCs;

static PlayerAgeProperties* sGrownAgeProperties;
static PlayerAgeProperties sAgePropertiesBackup;
static s32 sAddedHeavySwitchFlag;

static s32 GiantMask_IsEnabled(PlayState* play)
{
    return Config_Flag(CFG_CUSTOM_GIANT_MASK_ANYWHERE) && play->sceneId != SCE_MM_LAIR_TWINMOLD;
}

s32 GiantMask_IsGiant(void)
{
    return sIsGiant;
}

static f32 GiantMask_GetSimpleScaleModifier(void)
{
    return sIsGiant ? GIANT_SCALE_FACTOR : 1.0f;
}

static f32 GiantMask_GetSimpleInvertedScaleModifier(void)
{
    return sIsGiant ? (1.0f / GIANT_SCALE_FACTOR) : 1.0f;
}

static f32 GiantMask_GetPlayerScaleModifier(Player* player)
{
    if (player->actor.id != ACTOR_PLAYER)
        return 1.0f;
    return GiantMask_GetSimpleScaleModifier();
}

/* Smoothly follows the player's size during the transformation */
static f32 GiantMask_GetSmoothScaleModifier(Player* player)
{
    if (player->actor.id != ACTOR_PLAYER)
        return 1.0f;
    return sPlayerScale * (1.0f / NORMAL_PLAYER_SCALE);
}

s32 GiantMask_CanPutOn(PlayState* play)
{
    /* Twinmold's lair requires the boss to drive the transformation */
    if (play->sceneId == SCE_MM_LAIR_TWINMOLD)
        return comboHasSoulMm(GI_MM_SOUL_BOSS_TWINMOLD);
    return Config_Flag(CFG_CUSTOM_GIANT_MASK_ANYWHERE) && !play->interfaceCtx.restrictions.masks;
}

s32 GiantMask_ShouldBlockUse(PlayState* play, Player* player)
{
    CollisionPoly* poly;
    s32 bgId;
    f32 ceilingY;

    if (!GiantMask_IsEnabled(play))
        return 0;

    /* Taking the mask off is always fine */
    if (sIsGiant || player->currentMask == PLAYER_MASK_GIANT)
        return 0;

    if (sCsState != GIANT_CS_STATE_WAITING_FOR_MASK)
        return 1;

    if (player->transformation != MM_PLAYER_FORM_HUMAN)
        return 0;

    /* Make sure there is enough room for Link to grow */
    if (!(player->stateFlags1 & PLAYER_ACTOR_STATE_WATER))
    {
        if (BgCheck_EntityCheckCeiling(&play->colCtx, &ceilingY, &player->actor.world.pos, player->ageProperties->ceilingCheckHeight * GIANT_SCALE_FACTOR, &poly, &bgId, &player->actor))
            return 1;
    }

    return 0;
}

s32 GiantMask_ShouldCheckItemUsabilityWhileSwimming(Player* player, u8 itemAction)
{
    /* Allow taking the mask off while swimming */
    if (itemAction == PLAYER_IA_MASK_GIANT && sIsGiant && player->transformation == MM_PLAYER_FORM_HUMAN)
        return 0;
    return 1;
}

u32 GiantMask_GetWeaponDamageFlags(Player* player, u32 dmgFlags)
{
    if (sIsGiant && player->actor.id == ACTOR_PLAYER)
        return DMG_POWDER_KEG | DMG_EXPLOSIVES | DMG_GORON_PUNCH | DMG_UNK_0x1E;
    return dmgFlags;
}

static void GiantMask_GrowRegs(void)
{
    REG(48) *= 10;          /* Slow backwalk animation threshold */
    REG(19) *= 10;          /* Run acceleration */
    REG(32) /= 10;          /* Sidewalk animation speed multiplier */
    REG(36) /= 10;          /* Slow backwalk animation speed multiplier */
    REG(37) /= 10;          /* Walk speed threshold */
    REG(38) /= 10;          /* Walk animation speed multiplier */
    REG(43) *= 10;          /* Deceleration rate */
    REG(45) *= 10;          /* Run speed limit */
    REG(68) *= 10;          /* Gravity */
    IREG(66) *= 10;         /* Baby jump threshold */
    IREG(69) /= 10;         /* Baby jump speed multiplier */
    MREG(95) /= 10;         /* Bow sidewalk animation */
}

static void GiantMask_GrowAgeProperties(PlayerAgeProperties* props)
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

static void GiantMask_UpdateAgeProperties(Player* player)
{
    PlayerAgeProperties* props;

    props = player->ageProperties;
    if (sIsGiant)
    {
        /* The table can be reloaded behind our back, so check the actual values */
        if (props->ceilingCheckHeight < 200.0f)
        {
            sAgePropertiesBackup = *props;
            sGrownAgeProperties = props;
            GiantMask_GrowAgeProperties(props);
        }
        if (!(player->actor.flags & ACTOR_FLAG_CAN_PRESS_HEAVY_SWITCHES))
        {
            player->actor.flags |= ACTOR_FLAG_CAN_PRESS_HEAVY_SWITCHES;
            sAddedHeavySwitchFlag = 1;
        }
    }
    else
    {
        if (sGrownAgeProperties == props && props->ceilingCheckHeight >= 200.0f)
            *props = sAgePropertiesBackup;
        sGrownAgeProperties = NULL;
        if (sAddedHeavySwitchFlag)
        {
            player->actor.flags &= ~ACTOR_FLAG_CAN_PRESS_HEAVY_SWITCHES;
            sAddedHeavySwitchFlag = 0;
        }
    }
}

static void GiantMask_ApplyScale(Player* player)
{
    if (!sIsGiant && sCsState == GIANT_CS_STATE_WAITING_FOR_MASK)
        return;

    if (player->transformation == MM_PLAYER_FORM_FIERCE_DEITY)
        Actor_SetScale(&player->actor, sPlayerScale * 1.5f);
    else
        Actor_SetScale(&player->actor, sPlayerScale);
}

static void GiantMask_ApplyPhysics(Player* player)
{
    GiantMask_UpdateAgeProperties(player);

    /* Player_SetBootData resets these regularly */
    if (sIsGiant && REG(68) > -200)
        GiantMask_GrowRegs();

    GiantMask_ApplyScale(player);
}

static void GiantMask_SetFlash(u8 alpha)
{
    R_PLAY_FILL_SCREEN_ON = 1;
    R_PLAY_FILL_SCREEN_R = 255;
    R_PLAY_FILL_SCREEN_G = 255;
    R_PLAY_FILL_SCREEN_B = 255;
    R_PLAY_FILL_SCREEN_ALPHA = alpha;
}

static void GiantMask_UpdateFlash(void)
{
    s16 alpha;

    switch (sCsFlashState)
    {
    case GIANT_CS_FLASH_STATE_NOT_STARTED:
        break;
    case GIANT_CS_FLASH_STATE_STARTED:
        sCsFlashAlpha = 0;
        GiantMask_SetFlash(0);
        sCsFlashState = GIANT_CS_FLASH_STATE_INCREASE_ALPHA;
        Audio_PlaySfx(NA_SE_SY_TRANSFORM_MASK_FLASH);
        __attribute__((fallthrough));
    case GIANT_CS_FLASH_STATE_INCREASE_ALPHA:
        sCsFlashAlpha += 40;
        if (sCsFlashAlpha >= 400)
            sCsFlashState = GIANT_CS_FLASH_STATE_DECREASE_ALPHA;
        alpha = sCsFlashAlpha;
        if (alpha > 255)
            alpha = 255;
        R_PLAY_FILL_SCREEN_ALPHA = alpha;
        break;
    case GIANT_CS_FLASH_STATE_DECREASE_ALPHA:
        sCsFlashAlpha -= 40;
        if (sCsFlashAlpha <= 0)
        {
            sCsFlashAlpha = 0;
            sCsFlashState = GIANT_CS_FLASH_STATE_NOT_STARTED;
            R_PLAY_FILL_SCREEN_ON = 0;
        }
        else
        {
            alpha = sCsFlashAlpha;
            if (alpha > 255)
                alpha = 255;
            R_PLAY_FILL_SCREEN_ALPHA = alpha;
        }
        break;
    }
}

static void GiantMask_UpdateCamera(Player* player, PlayState* play)
{
    Vec3f subCamEyeOffset;
    f32 subCamUpRotZ;

    if (sCsState == GIANT_CS_STATE_WAITING_FOR_MASK || sSubCamId <= CAM_ID_MAIN)
        return;

    /* Prevent other cutscenes from starting */
    CutsceneManager_ClearNextCutscenes();

    Matrix_RotateYS(player->actor.shape.rot.y, MTXMODE_NEW);
    Matrix_MultVecZ(sSubCamDistZFromPlayer, &subCamEyeOffset);

    sSubCamEye.x = player->actor.world.pos.x + subCamEyeOffset.x;
    sSubCamEye.y = player->actor.world.pos.y + subCamEyeOffset.y + 10.0f;
    sSubCamEye.z = player->actor.world.pos.z + subCamEyeOffset.z;

    sSubCamAt.x = player->actor.world.pos.x;
    sSubCamAt.y = player->actor.world.pos.y + sSubCamAtOffsetY;
    sSubCamAt.z = player->actor.world.pos.z;

    subCamUpRotZ = Math_SinS(sCsTimer * 1512) * sSubCamUpRotZScale;
    Matrix_RotateZF(subCamUpRotZ, MTXMODE_APPLY);
    Matrix_MultVecY(1.0f, &sSubCamUp);
    Play_SetCameraAtEyeUp(play, sSubCamId, &sSubCamAt, &sSubCamEye, &sSubCamUp);
    ShrinkWindow_Letterbox_SetSizeTarget(27);
}

static void GiantMask_StartCutscene(Player* player, PlayState* play, s32 growing)
{
    GiantMask_PlayerFunc restartAction;
    f32 playerHeight;
    s8 heldItemAction;
    s32 isShielding;

    /*
     * The transformation action was interrupted, but the player is still waiting for
     * the cutscene. Restart the matching action so the player does not get stuck.
     */
    isShielding = !!(player->stateFlags1 & PLAYER_STATE1_MM_400000);
    if (!(player->stateFlags1 & PLAYER_STATE1_MM_20000000) || isShielding)
    {
        restartAction = OverlayAddr(growing ? 0x808389BC : 0x80838A20);
        heldItemAction = player->heldItemAction;
        if (isShielding)
            player->heldItemAction = PLAYER_IA_NONE;
        restartAction(play, player);
        if (isShielding)
            player->heldItemAction = heldItemAction;
    }

    sSubCamId = Play_CreateSubCamera(play);
    if (sSubCamId > CAM_ID_MAIN)
    {
        Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STATUS_WAIT);
        Play_ChangeCameraStatus(play, sSubCamId, CAM_STATUS_ACTIVE);
    }
    Play_EnableMotionBlur(150);

    sCsTimer = 0;
    sSubCamAtVel = 0.0f;
    sSubCamUpRotZScale = 0.0f;
    playerHeight = Player_GetHeight(player);

    if (growing)
    {
        sCsState = GIANT_CS_STATE_MASK_ON;
        sSubCamDistZFromPlayer = 60.0f;
        sSubCamAtOffsetY = playerHeight * 0.53f;
        sSubCamAtOffsetTargetY = playerHeight * 6.2f;
        sPlayerScale = NORMAL_PLAYER_SCALE;
    }
    else
    {
        sCsState = GIANT_CS_STATE_MASK_OFF;
        sSubCamDistZFromPlayer = 200.0f;
        sSubCamAtOffsetY = playerHeight * 0.62f;
        sSubCamAtOffsetTargetY = playerHeight * 0.053f;
        sPlayerScale = GIANT_PLAYER_SCALE;
    }
}

static void GiantMask_EndCutscene(Player* player, PlayState* play, s32 growing)
{
    sCsState = GIANT_CS_STATE_WAITING_FOR_MASK;
    if (sSubCamId > CAM_ID_MAIN)
        func_80169AFC(play, sSubCamId, 0);
    sSubCamId = SUB_CAM_ID_DONE;
    Play_DisableMotionBlur();

    /* Lets the player's transformation action finish */
    player->stateFlags1 &= ~PLAYER_STATE1_MM_100;
    player->meleeWeaponState = 0;

    sIsGiant = growing;
    sPlayerScale = growing ? GIANT_PLAYER_SCALE : NORMAL_PLAYER_SCALE;
}

static void GiantMask_UpdateCutscene(Player* player, PlayState* play)
{
    u16 pressed;
    s32 growing;

    sCsTimer++;
    pressed = play->state.input[0].press.button;

    switch (sCsState)
    {
    case GIANT_CS_STATE_WAITING_FOR_MASK:
        if (!(player->stateFlags1 & PLAYER_STATE1_MM_100))
            break;
        growing = (player->currentMask == PLAYER_MASK_GIANT);
        GiantMask_StartCutscene(player, play, growing);
        if (growing)
            goto maskOn;
        goto maskOff;

    case GIANT_CS_STATE_MASK_ON:
        if (sCsTimer < 80 && sCanSkipMaskOnCs && CHECK_BTN_ANY(pressed, GIANT_CS_SKIP_BUTTONS))
        {
            sCsState = GIANT_CS_STATE_MASK_ON_SKIPPED;
            sCsFlashState = GIANT_CS_FLASH_STATE_STARTED;
            sCsTimer = 0;
            break;
        }
    maskOn:
        if (sCsTimer >= 50)
        {
            if (sCsTimer == 60)
                Audio_PlaySfx(NA_SE_PL_TRANSFORM_GIANT);
            Math_ApproachF(&sSubCamDistZFromPlayer, 200.0f, 0.1f, sSubCamAtVel * 640.0f);
            Math_ApproachF(&sSubCamAtOffsetY, sSubCamAtOffsetTargetY, 0.1f, sSubCamAtVel * 150.0f);
            Math_ApproachF(&sPlayerScale, GIANT_PLAYER_SCALE, 0.2f, sSubCamAtVel * 0.1f);
            Math_ApproachF(&sSubCamAtVel, 1.0f, 1.0f, 0.001f);
        }
        else
        {
            Math_ApproachF(&sSubCamDistZFromPlayer, 30.0f, 0.1f, 1.0f);
        }

        if (sCsTimer > 50)
            Math_ApproachZeroF(&sSubCamUpRotZScale, 1.0f, 0.06f);
        else
            Math_ApproachF(&sSubCamUpRotZScale, 0.4f, 1.0f, 0.02f);

        if (sCsTimer == 107)
            sCsFlashState = GIANT_CS_FLASH_STATE_STARTED;

        if (sCsTimer > 120)
        {
            sCanSkipMaskOnCs = 1;
            GiantMask_EndCutscene(player, play, 1);
        }
        break;

    case GIANT_CS_STATE_MASK_ON_SKIPPED:
        if (sCsTimer >= 8)
            GiantMask_EndCutscene(player, play, 1);
        break;

    case GIANT_CS_STATE_MASK_OFF:
        if (sCsTimer < 30 && sCanSkipMaskOffCs && CHECK_BTN_ANY(pressed, GIANT_CS_SKIP_BUTTONS))
        {
            sCsState = GIANT_CS_STATE_MASK_OFF_SKIPPED;
            sCsFlashState = GIANT_CS_FLASH_STATE_STARTED;
            sCsTimer = 0;
            break;
        }
    maskOff:
        if (sCsTimer != 0)
        {
            if (sCsTimer == 10)
                Audio_PlaySfx(NA_SE_PL_TRANSFORM_NORAML);
            Math_ApproachF(&sSubCamDistZFromPlayer, 60.0f, 0.1f, sSubCamAtVel * 640.0f);
            Math_ApproachF(&sSubCamAtOffsetY, sSubCamAtOffsetTargetY, 0.1f, sSubCamAtVel * 150.0f);
            Math_ApproachF(&sPlayerScale, NORMAL_PLAYER_SCALE, 0.1f, 0.003f);
            Math_ApproachF(&sSubCamAtVel, 2.0f, 1.0f, 0.01f);
        }

        if (sCsTimer == 42)
            sCsFlashState = GIANT_CS_FLASH_STATE_STARTED;

        if (sCsTimer > 50)
        {
            sCanSkipMaskOffCs = 1;
            GiantMask_EndCutscene(player, play, 0);
        }
        break;

    case GIANT_CS_STATE_MASK_OFF_SKIPPED:
        if (sCsTimer >= 8)
            GiantMask_EndCutscene(player, play, 0);
        break;
    }

    /* Freeze the world while transforming, the transformation action clears this when it ends */
    if (sCsState != GIANT_CS_STATE_WAITING_FOR_MASK && (player->stateFlags1 & PLAYER_STATE1_MM_100))
        player->stateFlags1 |= PLAYER_STATE1_MM_10000000;
}

void GiantMask_Reset(PlayState* play)
{
    Player* player;

    /* Only restore through a pointer we know is still valid */
    player = GET_PLAYER(play);
    if (player && sGrownAgeProperties && player->ageProperties == sGrownAgeProperties && sGrownAgeProperties->ceilingCheckHeight >= 200.0f)
        *sGrownAgeProperties = sAgePropertiesBackup;

    /* Scene changed in the middle of the transformation */
    if (sCsState != GIANT_CS_STATE_WAITING_FOR_MASK)
        Play_DisableMotionBlur();

    sIsGiant = 0;
    sPlayerScale = NORMAL_PLAYER_SCALE;
    sCsState = GIANT_CS_STATE_WAITING_FOR_MASK;
    sCsTimer = 0;
    sSubCamId = SUB_CAM_ID_DONE;
    sGrownAgeProperties = NULL;
    sAddedHeavySwitchFlag = 0;
    if (sCsFlashState != GIANT_CS_FLASH_STATE_NOT_STARTED)
    {
        sCsFlashState = GIANT_CS_FLASH_STATE_NOT_STARTED;
        R_PLAY_FILL_SCREEN_ON = 0;
    }
}

void GiantMask_BeforePlayerUpdate(Player* player, PlayState* play)
{
    if (!GiantMask_IsEnabled(play) || player != GET_PLAYER(play))
        return;

    /* Same magic drain as in Twinmold's Lair */
    R_MAGIC_CONSUME_TIMER_GIANTS_MASK = 20;

    /* The mask is removed when changing scenes, so is the magic drain */
    if (!sIsGiant && sCsState == GIANT_CS_STATE_WAITING_FOR_MASK && gSaveContext.magicState == MAGIC_STATE_CONSUME_GIANTS_MASK && player->currentMask != PLAYER_MASK_GIANT && !(player->stateFlags1 & PLAYER_STATE1_MM_100))
        Magic_Reset(play);

    GiantMask_UpdateCutscene(player, play);
    GiantMask_UpdateFlash();
    GiantMask_UpdateCamera(player, play);
    GiantMask_ApplyPhysics(player);
}

void GiantMask_AfterPlayerUpdate(Player* player, PlayState* play)
{
    if (!GiantMask_IsEnabled(play) || player != GET_PLAYER(play))
        return;

    /* The player may have reset its scale or boot data during the update */
    GiantMask_ApplyPhysics(player);
}

/*
 * Hooks called from patches/giant_mask_anywhere.S
 */

f32 GiantMask_ScalePlayerHeight(Player* player, f32 height)
{
    return height * GiantMask_GetSmoothScaleModifier(player);
}

f32 GiantMask_GetFloorHeightCheckDelta(PlayState* play, Actor* actor, Vec3f* pos, s32 flags)
{
    f32 result;

    result = (flags & UPDBGCHECKINFO_FLAG_MM_800) ? 10.0f : 50.0f;
    if (actor->id == ACTOR_PLAYER)
        result *= GiantMask_GetSimpleScaleModifier();
    return result;
}

f32 GiantMask_GetLedgeWalkOffHeight(Actor* actor)
{
    f32 result;

    result = -11.0f;
    if (actor->id == ACTOR_PLAYER)
        result *= GiantMask_GetSimpleScaleModifier();
    return result;
}

void GiantMask_HandleFormSpeed(PlayState* play, Player* player, f32* speed)
{
    if (player->transformation == MM_PLAYER_FORM_FIERCE_DEITY)
        *speed *= 1.5f;
    *speed *= GiantMask_GetPlayerScaleModifier(player);
}

f32 GiantMask_GetWallCollisionHeight(Player* player)
{
    return 26.8f * GiantMask_GetPlayerScaleModifier(player);
}

f32 GiantMask_GetDiveDepth(void)
{
    return 100.0f * GiantMask_GetSimpleScaleModifier();
}

f32 GiantMask_GetWaterSurfaceClimbHeight(Player* player)
{
    f32 result;

    result = (player->transformation == MM_PLAYER_FORM_FIERCE_DEITY) ? 80.0f : 50.0f;
    return result * GiantMask_GetPlayerScaleModifier(player);
}

f32 GiantMask_GetLedgeClimbFactorFromSwim(void)
{
    return 60.0f * GiantMask_GetSimpleScaleModifier();
}

f32 GiantMask_GetLedgeClimbFactor(void)
{
    return 41.0f * GiantMask_GetSimpleScaleModifier();
}

f32 GiantMask_GetLedgeClimbFactor2(void)
{
    return 59.0f * GiantMask_GetSimpleScaleModifier();
}

f32 GiantMask_GetInvertedLedgeClimbFactor(void)
{
    return 100.0f * GiantMask_GetSimpleInvertedScaleModifier();
}

void GiantMask_ModifyLedgeJumpWallHeight(f32* wallHeight)
{
    *wallHeight *= GiantMask_GetSimpleInvertedScaleModifier();
}

f32 GiantMask_GetMidAirJumpSlashHeight(f32* outAlternateHeight)
{
    f32 modifier;

    modifier = GiantMask_GetSimpleScaleModifier();
    *outAlternateHeight = 4.5f * modifier;
    return 3.0f * modifier;
}

void GiantMask_SetVelocityY(Player* player, f32 velocityY, f32 waterSpeedFactor)
{
    player->actor.velocity.y = velocityY * waterSpeedFactor * GiantMask_GetPlayerScaleModifier(player);
}

void GiantMask_ModifyJumpVelocity(Player* player)
{
    player->linearVelocity *= GiantMask_GetPlayerScaleModifier(player);
}

f32 GiantMask_GetJumpSlashHeight(void)
{
    return 5.0f * GiantMask_GetSimpleScaleModifier();
}

f32 GiantMask_GetNewMinVelocityY(void)
{
    return -20.0f * GiantMask_GetSimpleScaleModifier();
}

f32 GiantMask_GetRunDeceleration(void)
{
    return 1.5f * GiantMask_GetSimpleScaleModifier();
}

s32 GiantMask_Math_AsymStepToF(f32* pValue, f32 target, f32 incrStep, f32 decrStep)
{
    /* The vanilla function returns an s32, unlike our declaration */
    GiantMask_MathAsymStepToF asymStepToF = (void*)Math_AsymStepToF;
    f32 modifier;

    modifier = GiantMask_GetSimpleScaleModifier();
    return asymStepToF(pValue, target, incrStep * modifier, decrStep * modifier);
}

s32 GiantMask_IsAboveBonkThreshold(PlayState* play, Player* player, f32* velocity, f32 threshold)
{
    return threshold * GiantMask_GetPlayerScaleModifier(player) <= *velocity;
}

s32 GiantMask_Math_StepToF(f32* pValue, f32 target, f32 step)
{
    return Math_StepToF(pValue, target, step * GiantMask_GetSimpleScaleModifier());
}

void GiantMask_GetMidAirAcceleration(f32* increment, f32* decrement)
{
    f32 modifier;

    modifier = GiantMask_GetSimpleScaleModifier();
    *increment *= modifier;
    *decrement *= modifier;
}

f32 GiantMask_GetLedgeGrabDistance(void)
{
    return 150.0f * GiantMask_GetSimpleScaleModifier();
}

void GiantMask_AfterJumpSlashGravity(PlayState* play, Player* player)
{
    player->actor.gravity *= GiantMask_GetPlayerScaleModifier(player);
}

f32 GiantMask_GetSpinChargeWalkSpeedFactor(void)
{
    return 1.5f * GiantMask_GetSimpleInvertedScaleModifier();
}

f32 GiantMask_GetClimbXZDelta(void)
{
    return 50.0f * GiantMask_GetSimpleScaleModifier();
}

f32 GiantMask_GetClimbYDelta(void)
{
    return 26.8f * GiantMask_GetSimpleScaleModifier();
}

f32 GiantMask_GetDiveSpeed(void)
{
    return -2.0f * GiantMask_GetSimpleScaleModifier();
}

void GiantMask_AfterUpdateCylinder(Player* player)
{
    player->cylinder.dim.radius *= GiantMask_GetPlayerScaleModifier(player);
}
