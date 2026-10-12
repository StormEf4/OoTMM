#include <combo.h>
#include <combo/player.h>
#include <combo/config.h>
#include <combo/mask.h>
#include <combo/math.h>
#include <combo/sfx.h>
#include <combo/collision.h>
#include <combo/age.h>
#include <combo/oot/fierce_deity.h>
#include <combo/oot/transform_mask.h>

/*
 * MM transformation masks (OoT): Deku Mask, MM Goron Transformation Mask, MM Zora Transformation Mask
 *
 * Built like the OoT Fierce Deity's Mask (fierce_deity.c), with one state machine for the three forms:
 * - Model: MM's form object (object_link_nuts / goron / zora) is loaded as a foreign object and its skeleton
 *   is drawn with Link's joint table (TransformOot_DrawSkeleton, from Player_DrawFlexLod). The limb layout is the
 *   same in both games. Hands, waist and sheath come from per-form tables, like MM's Player_OverrideLimbDraw.
 *   Goron and Zora eyes and mouth use the form's own textures.
 * - Size and movement: MM's age properties and boot data for each form (actor scale stays 0.01 like in MM).
 * - Items: like MM, only the ocarina, lens, bottles, trade items and the transformation masks. Deku also has Deku
 *   Nuts and Goron the Powder Keg. Goron and Zora always punch with B (a sword is lent during the player update,
 *   like Fierce Deity, and drawn as a fist).
 * - Rules: Goron is heat proof, Zora never drowns, Goron and Deku sink and void out in deep water, Deku and Zora
 *   void out when set on fire, Goron and Zora can't crawl.
 * - The form stays on through loading zones, like in MM.
 *
 * See docs/custom/transform-masks.md.
 */

#define TRANSFORM_FRAMES        20
#define TRANSFORM_SWAP_FRAME    10
#define SINK_FRAMES             20
#define SPAWN_GRACE_FRAMES      60

#define AGE_PROPERTIES_SIZE     0xd4
#define AGE_PROPERTIES_FLOATS   17

#define TUNIC_KOKIRI            0
#define TUNIC_GORON             1
#define TUNIC_ZORA              2

/* Vanilla player override function for first person (its entry is patched to an OoTMM wrapper) */
#define OVERRIDE_FIRST_PERSON   ((void*)0x8007ad80)

/* sMeleeWeaponLengths: Master, Kokiri (rewritten every frame by Player.c), Biggoron */
#define SWORD_LENGTH_COUNT      3
static f32* const kSwordLengths[SWORD_LENGTH_COUNT] = { (f32*)0x800f7bb4, (f32*)0x800f7bb8, (f32*)0x800f7bbc };

typedef enum
{
    TF_STATE_NORMAL,
    TF_STATE_PUTTING_ON,
    TF_STATE_FORM,
    TF_STATE_TAKING_OFF,
}
TransformState;

typedef struct
{
    u16 objectId;
    u32 skelHeader;
    u16 maskObjectId;
    u32 maskDList;
    u16 maskItem;
    u8  maskId;
    u8  xitem;
    u8  childProperties;
    u8  tunic;
    f32 height;
    f32 swordLength;
    f32 props[AGE_PROPERTIES_FLOATS];
    s16 bootRegs[17];
    u32 waist;
    u32 sheath;
    u32 lHandOpen;
    u32 lHandClosed;
    u32 lHandBottle;
    u32 rHandOpen;
    u32 rHandClosed;
    u8  eyeCount;
    u8  mouthCount;
    u32 eyeTex[8];
    u32 mouthTex[4];
}
TransformFormInfo;

/*
 * props: OoT PlayerAgeProperties floats (ceilingCheckHeight .. unk_40), values from MM's sPlayerAgeProperties
 * bootRegs: REG(19), 30, 32, 34, 35, 36, 37, 38, R_DECELERATE_RATE, R_RUN_SPEED_LIMIT, REG(68), 69,
 *           IREG(66..69), MREG(95), values from MM's boot data
 */
static const TransformFormInfo kForms[] = {
    /* TRANSFORM_OOT_FORM_NONE */
    { 0 },
    /* TRANSFORM_OOT_FORM_DEKU: object_link_nuts */
    {
        0x0154 | MASK_FOREIGN_OBJECT, 0xba24,
        0x01e3 | MASK_FOREIGN_OBJECT, 0x0a001d90,
        ITEM_OOT_MASK_DEKU, MASK_DEKU, XITEM_OOT_CHILD_MASK_DEKU,
        1, TUNIC_KOKIRI, 36.0f, 0.0f,
        { 35.0f, 50.0f, 0.3f, 71.0f, 50.0f, 49.0f, 39.0f, 27.0f, 19.0f, 8.0f, 13.6f, 24.0f, 24.0f, 70.0f, 14.0f, 12.0f, 55.0f },
        { 200, 1000, 300, 700, 550, 270, 600, 1000, 800, 600, -100, 600, 590, 750, 125, 200, 130 },
        0x06002c20, 0x060035f0,
        0x06003bc0, 0x06003bc0, 0x06003bc0,
        0x060038c0, 0x060038c0,
        0, 0,
        { 0 },
        { 0 },
    },
    /* TRANSFORM_OOT_FORM_GORON: object_link_goron */
    {
        0x014c | MASK_FOREIGN_OBJECT, 0x17a84,
        0x01e1 | MASK_FOREIGN_OBJECT, 0x0a0014a0,
        ITEM_OOT_MASK_GORON_MM_TRANSFORM, MASK_GORON_MM_TRANSFORM, XITEM_OOT_CHILD_MASK_GORON_MM_TRANSFORM,
        0, TUNIC_GORON, 80.0f, 2000.0f,
        { 70.0f, 90.0f, 0.74f, 111.0f, 70.0f, 79.4f, 59.0f, 41.0f, 19.0f, 36.0f, 50.0f, 56.0f, 68.0f, 70.0f, 19.5f, 18.2f, 80.0f },
        { 200, 1000, 300, 700, 550, 270, 700, 200, 800, 600, -140, 600, 590, 750, 125, 200, 130 },
        0x06008c00, 0,
        0x0600a500, 0x0600de68, 0x06011468,
        0x06009ef8, 0x0600e7e8,
        4, 0,
        { 0x06000000, 0x06000800, 0x06001000, 0x06001800 },
        { 0 },
    },
    /* TRANSFORM_OOT_FORM_ZORA: object_link_zora */
    {
        0x014d | MASK_FOREIGN_OBJECT, 0x12c34,
        0x01e2 | MASK_FOREIGN_OBJECT, 0x0a000db0,
        ITEM_OOT_MASK_ZORA_MM_TRANSFORM, MASK_ZORA_MM_TRANSFORM, XITEM_OOT_CHILD_MASK_ZORA_MM_TRANSFORM,
        0, TUNIC_ZORA, 68.0f, 2500.0f,
        { 56.0f, 90.0f, 1.0f, 111.0f, 70.0f, 79.4f, 59.0f, 41.0f, 19.0f, 36.0f, 50.0f, 56.0f, 68.0f, 70.0f, 18.0f, 23.0f, 70.0f },
        { 200, 1000, 300, 700, 550, 270, 700, 300, 800, 600, -100, 600, 590, 750, 125, 200, 130 },
        0x0600ab40, 0,
        0x0600fdf0, 0x0600c270, 0x06010d08,
        0x0600fbb8, 0x0600bbe0,
        8, 4,
        { 0x06000000, 0x06000800, 0x06001000, 0x06001800, 0x06002000, 0x06002800, 0x06003000, 0x06003800 },
        { 0x06004000, 0x06004400, 0x06004800, 0x06004c00 },
    },
};

/* OoT's sPlayerFaces: eyes and mouth for each face index */
static const u8 kFaces[][2] = {
    { 0, 0 }, { 1, 0 }, { 2, 0 }, { 0, 0 }, { 1, 0 }, { 2, 0 }, { 4, 0 }, { 5, 1 },
    { 7, 2 }, { 0, 2 }, { 3, 0 }, { 4, 0 }, { 2, 2 }, { 1, 1 }, { 0, 2 }, { 0, 0 },
};

static s32 sState;
static s32 sTimer;
static s32 sForm;           /* The form being worn (or put on) */
static s32 sDrawnForm;      /* The form Link currently is, switched at TRANSFORM_SWAP_FRAME */
static s32 sFrozePlayer;
static s32 sSinkTimer;
static s32 sVoidTriggered;
static s32 sFramesSinceInit;

static ALIGNED(8) u8 sAgeProperties[AGE_PROPERTIES_SIZE];
static u8* sOriginalAgeProperties;

static s32 sSwordLengthsSaved;
static f32 sSwordLengthsBackup[SWORD_LENGTH_COUNT];

static u8  sLentSword;
static u8  sLentSwordSavedB;
static s8  sSavedTunic;
static s8  sSavedBoots;
static s8  sSavedShield;
static s32 sLentEquipment;
static u8  sSavedSaveTunic;
static s32 sLentSaveTunic;

static void* sVanillaOverride;
static s32 sDrawFirstPerson;
static Gfx* sOpaAfterSkeleton;
static u32 sOpaAfterSkeletonFrame;

static const TransformFormInfo* TransformOot_Info(s32 form)
{
    if (form <= TRANSFORM_OOT_FORM_NONE || form >= ARRAY_COUNT(kForms))
        return NULL;
    return &kForms[form];
}

s32 TransformOot_IsActive(void)
{
    return sState != TF_STATE_NORMAL;
}

s32 TransformOot_IsTransforming(void)
{
    return sState == TF_STATE_PUTTING_ON || sState == TF_STATE_TAKING_OFF;
}

s32 TransformOot_GetForm(void)
{
    return sDrawnForm;
}

static s32 TransformOot_FormFromMaskItem(s16 itemId)
{
    switch (itemId)
    {
    case ITEM_OOT_MASK_DEKU: return TRANSFORM_OOT_FORM_DEKU;
    case ITEM_OOT_MASK_GORON_MM_TRANSFORM: return TRANSFORM_OOT_FORM_GORON;
    case ITEM_OOT_MASK_ZORA_MM_TRANSFORM: return TRANSFORM_OOT_FORM_ZORA;
    default: return TRANSFORM_OOT_FORM_NONE;
    }
}

s32 TransformOot_IsFormMaskItem(s16 itemId)
{
    return TransformOot_FormFromMaskItem(itemId) != TRANSFORM_OOT_FORM_NONE;
}

static s32 TransformOot_IsSwordItem(s16 itemId)
{
    switch (itemId)
    {
    case ITEM_OOT_SWORD_KOKIRI:
    case ITEM_OOT_SWORD_MASTER:
    case ITEM_OOT_SWORD_KNIFE_BIGGORON:
    case ITEM_OOT_SWORD_KNIFE_BROKEN:
        return 1;
    default:
        return 0;
    }
}

/* What a form can use, like MM's gPlayerFormItemRestrictions */
s32 TransformOot_CanUseItem(s16 itemId)
{
    if (sState == TF_STATE_NORMAL)
        return 1;
    if (TransformOot_IsTransforming())
        return 0;

    switch (itemId)
    {
    case ITEM_NONE:
        return 1;
    case ITEM_OOT_SWORD_KOKIRI:
    case ITEM_OOT_SWORD_MASTER:
    case ITEM_OOT_SWORD_KNIFE_BIGGORON:
    case ITEM_OOT_SWORD_KNIFE_BROKEN:
        /* Goron and Zora punch with B */
        return sForm == TRANSFORM_OOT_FORM_GORON || sForm == TRANSFORM_OOT_FORM_ZORA;
    case ITEM_OOT_NUT:
        return sForm == TRANSFORM_OOT_FORM_DEKU;
    case ITEM_OOT_POWDER_KEG:
        return sForm == TRANSFORM_OOT_FORM_GORON;
    case ITEM_OOT_OCARINA_FAIRY:
    case ITEM_OOT_OCARINA_TIME:
    case ITEM_OOT_LENS:
    case ITEM_OOT_MASK_DEKU:
    case ITEM_OOT_MASK_GORON_MM_TRANSFORM:
    case ITEM_OOT_MASK_ZORA_MM_TRANSFORM:
    case ITEM_OOT_MASK_FIERCE_DEITY:
        return 1;
    default:
        break;
    }

    /* Bottles */
    if (itemId >= ITEM_OOT_BOTTLE_EMPTY && itemId <= ITEM_OOT_POE)
        return 1;
    if (itemId >= ITEM_OOT_MAGIC_MUSHROOM && itemId <= ITEM_OOT_ZORA_EGG)
        return 1;

    /* Trade items, but not the regular masks */
    if (itemId >= ITEM_OOT_WEIRD_EGG && itemId <= ITEM_OOT_ZELDA_LETTER)
        return 1;
    if (itemId >= ITEM_OOT_POCKET_EGG && itemId <= ITEM_OOT_CLAIM_CHECK)
        return 1;

    return 0;
}

static s32 TransformOot_CanTransform(Player* player)
{
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

/* Age properties: OoT's adult (Goron, Zora) or child (Deku) entry, with MM's form values */
static void TransformOot_ApplyAgeProperties(Player* player, const TransformFormInfo* info)
{
    u8* base;
    f32* props;
    s32 i;

    if (!player->ageProperties)
        return;

    if (player->ageProperties != sAgeProperties)
    {
        /* OoT's table is { adult, child } */
        sOriginalAgeProperties = player->ageProperties;
    }
    if (!sOriginalAgeProperties)
        return;

    base = sOriginalAgeProperties;
    if (gSave.age != AGE_ADULT)
        base -= AGE_PROPERTIES_SIZE;
    if (info->childProperties)
        base += AGE_PROPERTIES_SIZE;
    memcpy(sAgeProperties, base, AGE_PROPERTIES_SIZE);

    props = (f32*)sAgeProperties;
    for (i = 0; i < AGE_PROPERTIES_FLOATS; ++i)
        props[i] = info->props[i];

    player->ageProperties = sAgeProperties;
}

static void TransformOot_RestoreAgeProperties(Player* player)
{
    if (sOriginalAgeProperties && player->ageProperties == sAgeProperties)
        player->ageProperties = sOriginalAgeProperties;
    sOriginalAgeProperties = NULL;
}

/* Same as Player_SetBootData, with the form's values. Written before and after the player update. */
static void TransformOot_ApplyBootRegs(PlayState* play, const TransformFormInfo* info)
{
    const s16* r;

    r = info->bootRegs;
    REG(27) = 2000;
    REG(48) = (sDrawnForm == TRANSFORM_OOT_FORM_GORON) ? 200 : 370;
    REG(19) = r[0];
    REG(30) = r[1];
    REG(32) = r[2];
    REG(34) = r[3];
    REG(35) = r[4];
    REG(36) = r[5];
    REG(37) = r[6];
    REG(38) = r[7];
    REG(43) = r[8];
    REG(45) = r[9];
    REG(68) = r[10];
    REG(69) = r[11];
    IREG(66) = r[12];
    IREG(67) = r[13];
    IREG(68) = r[14];
    IREG(69) = r[15];
    MREG(95) = r[16];

    /* ROOM_TYPE_INDOORS */
    if (play->roomCtx.curRoom.behaviorType1 == 2)
        REG(45) = 500;
}

static void TransformOot_SetSwordLengths(const TransformFormInfo* info)
{
    s32 i;

    if (!sSwordLengthsSaved)
    {
        for (i = 0; i < SWORD_LENGTH_COUNT; ++i)
            sSwordLengthsBackup[i] = *kSwordLengths[i];
        sSwordLengthsSaved = 1;
    }

    for (i = 0; i < SWORD_LENGTH_COUNT; ++i)
        *kSwordLengths[i] = info->swordLength;
}

static void TransformOot_RestoreSwordLengths(void)
{
    s32 i;

    if (!sSwordLengthsSaved)
        return;

    /* The Kokiri Sword length is set again by Player.c every frame */
    for (i = 0; i < SWORD_LENGTH_COUNT; ++i)
        *kSwordLengths[i] = sSwordLengthsBackup[i];
    sSwordLengthsSaved = 0;
}

static void TransformOot_ApplyForm(PlayState* play, Player* player)
{
    const TransformFormInfo* info;

    info = TransformOot_Info(sDrawnForm);
    if (!info)
        return;
    TransformOot_ApplyAgeProperties(player, info);
    TransformOot_ApplyBootRegs(play, info);
}

static void TransformOot_LeaveForm(PlayState* play, Player* player)
{
    if (sDrawnForm == TRANSFORM_OOT_FORM_NONE)
        return;

    sDrawnForm = TRANSFORM_OOT_FORM_NONE;
    TransformOot_RestoreAgeProperties(player);
    TransformOot_RestoreSwordLengths();

    /* Restores the movement REGs through Player_SetBootData */
    if (play)
        UpdateEquipment(play, player);
}

static void TransformOot_Freeze(PlayState* play, Player* player)
{
    if (player->csAction == PLAYER_CSACTION_NONE)
    {
        Player_SetCsActionWithHaltedActors(play, NULL, PLAYER_CSACTION_1);
        sFrozePlayer = 1;
    }
}

static void TransformOot_Unfreeze(PlayState* play, Player* player)
{
    if (sFrozePlayer && player->csAction == PLAYER_CSACTION_1)
        Player_SetCsActionWithHaltedActors(play, NULL, PLAYER_CSACTION_7);
    sFrozePlayer = 0;
}

static void TransformOot_StartPuttingOn(PlayState* play, Player* player, s32 form)
{
    const TransformFormInfo* info;

    info = TransformOot_Info(form);
    sForm = form;
    sState = TF_STATE_PUTTING_ON;
    sTimer = 0;
    player->mask = info->maskId;
    TransformOot_Freeze(play, player);
    PlaySound(NA_SE_PL_MAGIC_WIND_WARP);

    /* Start loading the models now */
    comboGetObject(info->objectId);
    comboGetObject(info->maskObjectId);
}

static void TransformOot_StartTakingOff(PlayState* play, Player* player)
{
    sState = TF_STATE_TAKING_OFF;
    sTimer = 0;
    TransformOot_Freeze(play, player);
    PlaySound(NA_SE_PL_MAGIC_WIND_WARP);
}

static void TransformOot_RemoveMask(PlayState* play, Player* player)
{
    if (player->mask == MASK_DEKU || player->mask == MASK_GORON_MM_TRANSFORM || player->mask == MASK_ZORA_MM_TRANSFORM)
    {
        player->mask = MASK_NONE;
        Interface_LoadItemIconImpl(play, 0);
    }
}

void TransformOot_UseMask(PlayState* play, Player* player, s16 itemId)
{
    s32 form;

    form = TransformOot_FormFromMaskItem(itemId);
    if (form == TRANSFORM_OOT_FORM_NONE || TransformOot_IsTransforming())
        return;

    /* Taking the mask off */
    if (sState == TF_STATE_FORM && form == sForm)
    {
        PlaySound(0x835);
        TransformOot_StartTakingOff(play, player);
        return;
    }

    /* Switching to another form, like in MM */
    if (sState == TF_STATE_FORM)
    {
        PlaySound(0x835);
        TransformOot_StartPuttingOn(play, player, form);
        return;
    }

    /* From Fierce Deity: back to Link first */
    if (FierceDeityOot_IsActive())
        FierceDeityOot_Reset(play, player);

    if (!TransformOot_CanTransform(player))
    {
        PlaySound(NA_SE_SY_ERROR);
        return;
    }

    /* Any regular mask is replaced */
    PlaySound(0x835);
    TransformOot_StartPuttingOn(play, player, form);
}

/* Back to Link right away, used before putting on the Fierce Deity's Mask */
void TransformOot_ForceLeave(PlayState* play, Player* player)
{
    TransformOot_Unfreeze(play, player);
    TransformOot_LeaveForm(play, player);
    TransformOot_RemoveMask(play, player);
    sState = TF_STATE_NORMAL;
    sForm = TRANSFORM_OOT_FORM_NONE;
    sTimer = 0;
    sSinkTimer = 0;
}

/* Player_AfterInit: the form stays on through loading zones, like in MM */
void TransformOot_AfterPlayerInit(PlayState* play, Player* player)
{
    const TransformFormInfo* info;
    s32 keep;

    info = TransformOot_Info(sForm);
    keep = (info != NULL) && (sState == TF_STATE_FORM || sState == TF_STATE_PUTTING_ON) && OotTradeChild_Has(info->xitem);

    sTimer = 0;
    sFrozePlayer = 0;
    sSinkTimer = 0;
    sVoidTriggered = 0;
    sFramesSinceInit = 0;
    sOriginalAgeProperties = NULL;
    sSwordLengthsSaved = 0;
    sLentSword = 0;
    sLentEquipment = 0;
    sLentSaveTunic = 0;

    if (keep)
    {
        sState = TF_STATE_FORM;
        sDrawnForm = sForm;
        if (player)
        {
            player->mask = info->maskId;
            comboGetObject(info->objectId);
        }
    }
    else
    {
        sState = TF_STATE_NORMAL;
        sForm = TRANSFORM_OOT_FORM_NONE;
        sDrawnForm = TRANSFORM_OOT_FORM_NONE;
    }
}

static void TransformOot_UpdateState(Player* player, PlayState* play)
{
    switch (sState)
    {
    case TF_STATE_NORMAL:
        break;

    case TF_STATE_PUTTING_ON:
        sTimer++;
        if (sTimer == TRANSFORM_SWAP_FRAME)
        {
            /* Switching forms: leave the old one first */
            if (sDrawnForm != TRANSFORM_OOT_FORM_NONE && sDrawnForm != sForm)
                TransformOot_LeaveForm(play, player);
            sDrawnForm = sForm;
            PlaySound(NA_SE_IT_ROLLING_CUT_LV1);
        }
        if (sTimer >= TRANSFORM_FRAMES)
        {
            sState = TF_STATE_FORM;
            TransformOot_Unfreeze(play, player);
        }
        break;

    case TF_STATE_FORM:
        /* The mask was removed by something else (cutscene, item, ...) */
        if (player->mask != kForms[sForm].maskId)
        {
            player->mask = kForms[sForm].maskId;
            TransformOot_StartTakingOff(play, player);
        }
        break;

    case TF_STATE_TAKING_OFF:
        sTimer++;
        if (sTimer == TRANSFORM_SWAP_FRAME)
        {
            TransformOot_RemoveMask(play, player);
            TransformOot_LeaveForm(play, player);
        }
        if (sTimer >= TRANSFORM_FRAMES)
        {
            sState = TF_STATE_NORMAL;
            sForm = TRANSFORM_OOT_FORM_NONE;
            TransformOot_Unfreeze(play, player);
        }
        break;
    }
}

/*
 * Like Fierce Deity, Goron and Zora always punch with B. When B holds no sword, a sword is put on B for the duration
 * of the player update only (that is where the B button is read). It is drawn as a fist.
 */
static void TransformOot_LendSword(void)
{
    u8* bItem;

    if (sState != TF_STATE_FORM || (sForm != TRANSFORM_OOT_FORM_GORON && sForm != TRANSFORM_OOT_FORM_ZORA))
        return;

    bItem = &gSave.info.equips.buttonItems[0];
    if (TransformOot_IsSwordItem(*bItem))
        return;

    sLentSwordSavedB = *bItem;
    sLentSword = (gSave.age == AGE_ADULT) ? ITEM_OOT_SWORD_MASTER : ITEM_OOT_SWORD_KOKIRI;
    *bItem = sLentSword;
}

static void TransformOot_ReturnSword(void)
{
    if (!sLentSword)
        return;

    /* Only put the old item back if nothing else changed B during the update */
    if (gSave.info.equips.buttonItems[0] == sLentSword)
        gSave.info.equips.buttonItems[0] = sLentSwordSavedB;
    sLentSword = 0;
}

/* Forms don't wear Link's tunic, boots or shield. The form's own tunic gives the Goron heat and Zora water perks. */
static void TransformOot_LendEquipment(Player* player)
{
    const TransformFormInfo* info;

    info = TransformOot_Info(sDrawnForm);
    if (!info)
        return;

    sSavedTunic = player->currentTunic;
    sSavedBoots = player->currentBoots;
    sSavedShield = player->currentShield;
    player->currentTunic = info->tunic;
    player->currentBoots = 0;
    player->currentShield = 0;
    sLentEquipment = 1;
}

static void TransformOot_ReturnEquipment(Player* player)
{
    if (!sLentEquipment)
        return;

    player->currentTunic = sSavedTunic;
    player->currentBoots = sSavedBoots;
    player->currentShield = sSavedShield;
    sLentEquipment = 0;
}

/* Interface_Update reads the equipped tunic for the hot room and underwater timers */
void TransformOot_BeforeInterfaceUpdate(void)
{
    sLentSaveTunic = 0;
    if (sDrawnForm != TRANSFORM_OOT_FORM_GORON && sDrawnForm != TRANSFORM_OOT_FORM_ZORA)
        return;

    sSavedSaveTunic = gSave.info.equips.equipment.tunics;
    gSave.info.equips.equipment.tunics = (sDrawnForm == TRANSFORM_OOT_FORM_GORON) ? 2 : 3;
    sLentSaveTunic = 1;
}

void TransformOot_AfterInterfaceUpdate(void)
{
    if (!sLentSaveTunic)
        return;

    gSave.info.equips.equipment.tunics = sSavedSaveTunic;
    sLentSaveTunic = 0;
}

/* Same as Play_TriggerVoidOut */
static void TransformOot_VoidOut(PlayState* play)
{
    if (sVoidTriggered || play->transitionTrigger != 0)
        return;

    sVoidTriggered = 1;
    gSaveContext.respawn[RESPAWN_MODE_DOWN].tempSwitchFlags = play->actorCtx.flags.tempSwch;
    gSaveContext.respawn[RESPAWN_MODE_DOWN].tempCollectFlags = play->actorCtx.flags.tempCollect;
    gSaveContext.respawnFlag = 1;
    play->transitionTrigger = 20; /* TRANS_TRIGGER_START */
    play->nextEntrance = gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance;
    play->transitionType = TRANS_TYPE_FADE_BLACK;
}

static void TransformOot_UpdateRules(PlayState* play, Player* player)
{
    if (sState != TF_STATE_FORM)
    {
        sSinkTimer = 0;
        return;
    }

    switch (sForm)
    {
    case TRANSFORM_OOT_FORM_ZORA:
        /* Zora never drowns */
        player->underwaterTimer = 0;
        break;

    case TRANSFORM_OOT_FORM_GORON:
    case TRANSFORM_OOT_FORM_DEKU:
        /* Goron and Deku can't swim: they sink and void out, like in MM */
        if (player->stateFlags1 & PLAYER_ACTOR_STATE_WATER)
        {
            sSinkTimer++;
            if (sSinkTimer == 1)
                PlaySound(NA_SE_EV_WATER_CONVECTION);
            if (sSinkTimer >= SINK_FRAMES)
            {
                /* Arriving in deep water as a Goron or Deku: take the mask off rather than looping void outs */
                if (sFramesSinceInit < SPAWN_GRACE_FRAMES)
                    TransformOot_ForceLeave(play, player);
                else
                    TransformOot_VoidOut(play);
            }
        }
        else
        {
            sSinkTimer = 0;
        }
        break;
    }

    /* Deku and Zora can't survive fire, like in MM */
    if ((sForm == TRANSFORM_OOT_FORM_DEKU || sForm == TRANSFORM_OOT_FORM_ZORA) && player->isBurning)
        TransformOot_VoidOut(play);
}

static void TransformOot_UpdateDamage(Player* player)
{
    u32 flags;
    s32 i;

    /*
     * Damage tables use the highest damage bit, so the flags are replaced rather than OR'd.
     * MM's form bits mean something else in OoT, so they are translated:
     * Goron punch -> hammer swing (2), Zora punch -> Master Sword slash (1, 2 for jump attacks).
     */
    for (i = 0; i < ARRAY_COUNT(player->meleeWeaponQuads); ++i)
    {
        flags = player->meleeWeaponQuads[i].elem.atDmgInfo.dmgFlags;
        if (!(flags & DMG_SWORD))
            continue;

        if (sDrawnForm == TRANSFORM_OOT_FORM_GORON)
        {
            if (flags & DMG_JUMP_SLASH)
            {
                player->meleeWeaponQuads[i].elem.atDmgInfo.dmgFlags = DMG_HAMMER_JUMP;
                player->meleeWeaponQuads[i].elem.atDmgInfo.damage = 4;
            }
            else
            {
                player->meleeWeaponQuads[i].elem.atDmgInfo.dmgFlags = DMG_HAMMER_SWING;
                player->meleeWeaponQuads[i].elem.atDmgInfo.damage = 2;
            }
        }
        else if (sDrawnForm == TRANSFORM_OOT_FORM_ZORA)
        {
            if (flags & DMG_JUMP_SLASH)
            {
                player->meleeWeaponQuads[i].elem.atDmgInfo.dmgFlags = DMG_JUMP_MASTER;
                player->meleeWeaponQuads[i].elem.atDmgInfo.damage = 2;
            }
            else if (flags & DMG_SPIN_ATTACK)
            {
                player->meleeWeaponQuads[i].elem.atDmgInfo.dmgFlags = DMG_SPIN_MASTER;
                player->meleeWeaponQuads[i].elem.atDmgInfo.damage = 2;
            }
            else
            {
                player->meleeWeaponQuads[i].elem.atDmgInfo.dmgFlags = DMG_SLASH_MASTER;
                player->meleeWeaponQuads[i].elem.atDmgInfo.damage = 1;
            }
        }
    }
}

void TransformOot_BeforePlayerUpdate(Player* player, PlayState* play)
{
    if (player != GET_PLAYER(play))
        return;

    if (sFramesSinceInit < SPAWN_GRACE_FRAMES)
        sFramesSinceInit++;

    TransformOot_UpdateState(player, play);
    if (sDrawnForm != TRANSFORM_OOT_FORM_NONE)
    {
        TransformOot_ApplyForm(play, player);
        TransformOot_LendEquipment(player);
    }
    TransformOot_LendSword();
}

void TransformOot_AfterPlayerUpdate(Player* player, PlayState* play)
{
    const TransformFormInfo* info;

    if (player != GET_PLAYER(play))
        return;

    TransformOot_ReturnSword();
    TransformOot_ReturnEquipment(player);

    info = TransformOot_Info(sDrawnForm);
    if (!info)
        return;

    /* The player may have reset its boot data during the update */
    TransformOot_ApplyForm(play, player);
    if (info->swordLength > 0.0f)
        TransformOot_SetSwordLengths(info);
    TransformOot_UpdateDamage(player);
    TransformOot_UpdateRules(play, player);
}

f32 TransformOot_GetHeight(Player* player)
{
    const TransformFormInfo* info;

    info = TransformOot_Info(sDrawnForm);
    if (!info)
        return 0.0f;
    return info->height;
}

/* Goron and Zora can't crawl (adult sized), Deku can */
static s32 TransformOot_TryEnteringCrawlspace(Player* player, PlayState* play, u32 interactWallFlags)
{
    s32 (*Player_TryEnteringCrawlspace)(Player*, PlayState*, u32);

    if (sDrawnForm == TRANSFORM_OOT_FORM_GORON || sDrawnForm == TRANSFORM_OOT_FORM_ZORA)
        return 0;

    Player_TryEnteringCrawlspace = OverlayAddr(0x8083d144);
    return Player_TryEnteringCrawlspace(player, play, interactWallFlags);
}

PATCH_CALL(0x8083d8b4, TransformOot_TryEnteringCrawlspace);

/* Draw */

static u32 TransformOot_HandDList(const TransformFormInfo* info, s32 limbIndex, Player* player)
{
    if (limbIndex == PLAYER_LIMB_L_HAND)
    {
        switch (player->leftHandType)
        {
        case PLAYER_MODELTYPE_LH_OPEN:
            return info->lHandOpen;
        case PLAYER_MODELTYPE_LH_BOTTLE:
            return info->lHandBottle;
        default:
            return info->lHandClosed;
        }
    }

    switch (player->rightHandType)
    {
    case PLAYER_MODELTYPE_RH_CLOSED:
    case PLAYER_MODELTYPE_RH_BOW_SLINGSHOT:
    case PLAYER_MODELTYPE_RH_BOW_SLINGSHOT_2:
    case PLAYER_MODELTYPE_RH_HOOKSHOT:
        return info->rHandClosed;
    default:
        return info->rHandOpen;
    }
}

static s32 TransformOot_OverrideLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot, void* arg)
{
    s32 (*vanillaOverride)(PlayState*, s32, Gfx**, Vec3f*, Vec3s*, void*);
    const TransformFormInfo* info;
    Player* player;
    Gfx* formList;
    u32 movementFlags;
    s32 ret;

    player = (Player*)arg;
    info = &kForms[sDrawnForm];
    vanillaOverride = sVanillaOverride;
    formList = *dList;

    /* MM scales the animation root for every form but Fierce Deity (the draw runs as adult, so no child scaling) */
    if (limbIndex == PLAYER_LIMB_ROOT)
    {
        movementFlags = player->skelAnime.moveFlags;
        if (!(movementFlags & 0x04) || (movementFlags & 0x01))
        {
            pos->x *= info->props[2];
            pos->z *= info->props[2];
        }
        if (!(movementFlags & 0x04) || (movementFlags & 0x02))
            pos->y *= info->props[2];
    }

    /* Head look, leg IK, body part positions... OoT's hand, waist and sheath models are replaced below */
    ret = vanillaOverride ? vanillaOverride(play, limbIndex, dList, pos, rot, arg) : 0;

    if (sDrawFirstPerson)
        *dList = NULL;
    else if (limbIndex == PLAYER_LIMB_L_HAND || limbIndex == PLAYER_LIMB_R_HAND)
        *dList = (Gfx*)TransformOot_HandDList(info, limbIndex, player);
    else if (limbIndex == PLAYER_LIMB_WAIST)
        *dList = (Gfx*)info->waist;
    else if (limbIndex == PLAYER_LIMB_SHEATH)
        *dList = (Gfx*)info->sheath;
    else
        *dList = formList;

    /* OoTMM tints some limbs with the tunic color */
    OPEN_DISPS(play->state.gfxCtx);
    gDPSetEnvColor(POLY_OPA_DISP++, 0xff, 0xff, 0xff, 0xff);
    CLOSE_DISPS();

    return ret;
}

static void TransformOot_SetFace(PlayState* play, const TransformFormInfo* info, u8* obj, Vec3s* jointTable, Player* player)
{
    s32 eyes;
    s32 mouth;
    s32 face;

    if (!info->eyeCount)
        return;

    face = player->actor.shape.face;
    if (face < 0 || face >= ARRAY_COUNT(kFaces))
        face = 0;

    eyes = (jointTable[PLAYER_LIMB_MAX].x & 0xf) - 1;
    mouth = (jointTable[PLAYER_LIMB_MAX].x >> 4) - 1;
    if (eyes < 0)
        eyes = kFaces[face][0];
    if (mouth < 0)
        mouth = kFaces[face][1];

    /* Goron has open, half, closed and surprised eyes */
    if (info->eyeCount < 8)
    {
        if (eyes == 5 || eyes == 7)
            eyes = 3;
        else if (eyes > 2)
            eyes = 0;
    }
    if (eyes >= info->eyeCount)
        eyes = 0;
    if (mouth >= info->mouthCount)
        mouth = 0;

    OPEN_DISPS(play->state.gfxCtx);
    gSPSegment(POLY_OPA_DISP++, 0x08, obj + (info->eyeTex[eyes] & 0xffffff));
    if (info->mouthCount)
        gSPSegment(POLY_OPA_DISP++, 0x09, obj + (info->mouthTex[mouth] & 0xffffff));
    CLOSE_DISPS();
}

s32 TransformOot_DrawSkeleton(PlayState* play, Vec3s* jointTable, s32 dListCount, void* overrideLimbDraw, void* postLimbDraw, void* arg, s32 lod)
{
    const TransformFormInfo* info;
    u8* obj;
    u8* header;
    void** limbs;
    s32 count;
    u32 savedSegment;
    s32 savedAge;

    info = TransformOot_Info(sDrawnForm);
    if (!info)
        return 0;

    obj = comboGetObject(info->objectId);
    if (!obj)
        return 0;

    header = obj + info->skelHeader;
    limbs = (void**)(obj + (*(u32*)header & 0xffffff));
    count = header[8];
    if (count < dListCount)
        count = dListCount;

    sVanillaOverride = overrideLimbDraw;
    sDrawFirstPerson = (overrideLimbDraw == OVERRIDE_FIRST_PERSON);

    /* The form skeleton is resolved through segment 6 on both the CPU and the RSP */
    savedSegment = gSegments[6];
    gSegments[6] = (u32)obj - K0BASE;
    OPEN_DISPS(play->state.gfxCtx);
    gSPSegment(POLY_OPA_DISP++, 0x06, obj);
    gDPSetEnvColor(POLY_OPA_DISP++, 0xff, 0xff, 0xff, 0xff);
    CLOSE_DISPS();
    TransformOot_SetFace(play, info, obj, jointTable, (Player*)arg);

    /* Draw as adult: no child root scaling, adult leg IK, like Fierce Deity */
    savedAge = gSave.age;
    gSave.age = AGE_ADULT;
    SkelAnime_DrawFlexLod(play, limbs, jointTable, count, TransformOot_OverrideLimbDraw, postLimbDraw, arg, lod);
    gSave.age = savedAge;

    gSegments[6] = savedSegment;
    OPEN_DISPS(play->state.gfxCtx);
    gSPSegment(POLY_OPA_DISP++, 0x06, (void*)(savedSegment + K0BASE));
    sOpaAfterSkeleton = POLY_OPA_DISP;
    sOpaAfterSkeletonFrame = play->gameplayFrames;
    CLOSE_DISPS();

    return 1;
}

/*
 * Called from the mask draw right after Player_DrawImpl.
 * Transformed: drops what Player_DrawImpl drew after the skeleton (adult gauntlets and boots), no mask on top.
 * Putting the mask on: the mask is drawn on Link's face, from MM's object_mask_* object.
 */
s32 TransformOot_DrawMask(PlayState* play, Player* player, s32 form)
{
    const TransformFormInfo* info;
    void* obj;

    if (sDrawnForm != TRANSFORM_OOT_FORM_NONE)
    {
        if (sOpaAfterSkeleton && sOpaAfterSkeletonFrame == play->gameplayFrames)
        {
            OPEN_DISPS(play->state.gfxCtx);
            POLY_OPA_DISP = sOpaAfterSkeleton;
            CLOSE_DISPS();
        }
        sOpaAfterSkeleton = NULL;
        return 1;
    }

    info = TransformOot_Info(form);
    if (!info)
        return 0;
    obj = comboGetObject(info->maskObjectId);
    if (!obj)
        return 1;

    OPEN_DISPS(play->state.gfxCtx);
    gSPMatrix(POLY_OPA_DISP++, 0x0d0001c0, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPSegment(POLY_OPA_DISP++, 0x0a, obj);
    gSPDisplayList(POLY_OPA_DISP++, info->maskDList);
    CLOSE_DISPS();
    return 1;
}
