/*
 * Fork: the Pot God (Custom > Ocarina of Time > "Pot God").
 *
 * Spawned (invisible) in the child Market Pot House by PotGod_SpawnForScene.
 * It watches the pots in the room; once every pot is broken, the guard
 * screams, collapses and sinks into the floor, and the Pot God bursts out
 * where he went down: a giant pot, bleeding from its rim, with four of Dead
 * Hand's grasping arms writhing out of its mouth and pot shards circling it.
 * It hops at Link, slams him with its two front arms and throws pots.
 * Defeating it sets SAVE_FLAG_POT_GOD_DEFEATED, which makes 20 pots holding
 * checks appear in Hyrule Field. Everything is drawn from vanilla objects
 * (Dead Hand's hand from object_dh, object_tsubo and the guard's object_sd),
 * so no new asset is needed.
 */

#include "Custom_Pot_God.h"
#include <combo/rand.h>
#include <combo/math.h>
#include <combo/player.h>
#include <combo/effect.h>
#include <combo/sfx_source.h>
#include <combo/seqcmd.h>
#include <combo/audio.h>
#include <assets/oot/objects/object_dh.h>
#include <assets/oot/objects/object_tsubo.h>
#include <assets/oot/objects/object_sd.h>

#define FLAGS (ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED)

#define POT_GOD_HEALTH          16
#define POT_GOD_MIN_POTS        8
#define POT_GOD_SHARDS          6
#define POT_GOD_POT_GRAVITY     0.6f
#define POT_GOD_POT_FLIGHT      24

/* The body: a pot about 100 units tall (a room pot is 26 at scale 0.15) */
#define POT_GOD_BODY_SCALE      0.6f
#define POT_GOD_BODY_HEIGHT     100.0f
#define POT_GOD_BODY_RADIUS     36.0f
#define POT_GOD_RIM_RADIUS      14.0f

/* Dead Hand's hand (the one that grabs from the floor), a bit bigger */
#define POT_GOD_ARM_SCALE       0.014f
#define POT_GOD_ARM_TILT        0x1400

/* Hops */
#define POT_GOD_HOP_LENGTH      36
#define POT_GOD_HOP_AIR         20
#define POT_GOD_HOP_SPEED       3.0f

/* OoT voice sfx (not in sfx_id.h's OoT section) */
#define SFX_TALON_SURPRISE      0x6851
#define SFX_TALON_CRY           0x6852
#define SFX_INGO_CRY            0x6854

#define ANIM_DH_HAND            object_dh_Anim_0015B0
#define SKEL_DH_HAND            object_dh_Skel_000BD8

extern void* gExObjectsAddr[];

void PotGod_Init(Actor* thisx, PlayState* play);
void PotGod_Destroy(Actor* thisx, PlayState* play);
void PotGod_Update(Actor* thisx, PlayState* play);
void PotGod_Draw(Actor* thisx, PlayState* play);

static void PotGod_Watch(PotGod* this, PlayState* play);
static void PotGod_WaitCalm(PotGod* this, PlayState* play);
static void PotGod_Cutscene(PotGod* this, PlayState* play);
static void PotGod_SetupHop(PotGod* this);
static void PotGod_Hop(PotGod* this, PlayState* play);
static void PotGod_Slam(PotGod* this, PlayState* play);
static void PotGod_Summon(PotGod* this, PlayState* play);
static void PotGod_Damage(PotGod* this, PlayState* play);
static void PotGod_Death(PotGod* this, PlayState* play);

/* Each arm: where it comes out of the mouth (relative to the body's yaw) and how it writhes */
static const s16 kArmYaw[POT_GOD_ARMS] = { 0x1C00, -0x1C00, 0x6000, -0x6000 };
static const s16 kArmPhase[POT_GOD_ARMS] = { 0x0000, 0x5000, 0x9000, 0xD000 };

static ColliderCylinderInit sCylinderInit = {
    {
        COL_MATERIAL_HIT0,
        AT_NONE,
        AC_ON | AC_TYPE_PLAYER,
        OC1_ON | OC1_TYPE_PLAYER,
        OC2_TYPE_1,
        COLSHAPE_CYLINDER,
    },
    {
        ELEM_MATERIAL_UNK0,
        { 0x00000000, HIT_SPECIAL_EFFECT_NONE, 0x00 },
        { 0xFFCFFFFF, HIT_BACKLASH_NONE, 0x00 },
        ATELEM_NONE,
        ACELEM_ON,
        OCELEM_ON,
    },
    { 36, 100, 0, { 0, 0, 0 } },
};

/* The two front hands: they hit Link while slamming, and can be hit */
#define HAND_ELEMENT_INIT                                       \
    {                                                           \
        {                                                       \
            ELEM_MATERIAL_UNK0,                                 \
            { 0xFFCFFFFF, HIT_SPECIAL_EFFECT_NONE, 0x08 },      \
            { 0xFFCFFFFF, HIT_BACKLASH_NONE, 0x00 },            \
            ATELEM_NONE,                                        \
            ACELEM_ON,                                          \
            OCELEM_NONE,                                        \
        },                                                      \
        { 0, { { 0, 0, 0 }, 14 }, 100 },                        \
    }

static ColliderJntSphElementInit sHandsElementsInit[POT_GOD_SLAM_ARMS] = {
    HAND_ELEMENT_INIT,
    HAND_ELEMENT_INIT,
};

static ColliderJntSphInit sHandsInit = {
    {
        COL_MATERIAL_HIT0,
        AT_ON | AT_TYPE_ENEMY,
        AC_ON | AC_TYPE_PLAYER,
        OC1_NONE,
        OC2_NONE,
        COLSHAPE_JNTSPH,
    },
    ARRAY_COUNT(sHandsElementsInit),
    sHandsElementsInit,
};

#define POT_ELEMENT_INIT                                        \
    {                                                           \
        {                                                       \
            ELEM_MATERIAL_UNK0,                                 \
            { 0xFFCFFFFF, HIT_SPECIAL_EFFECT_NONE, 0x08 },      \
            { 0x00000000, HIT_BACKLASH_NONE, 0x00 },            \
            ATELEM_NONE | ATELEM_SFX_NORMAL,                    \
            ACELEM_NONE,                                        \
            OCELEM_NONE,                                        \
        },                                                      \
        { 0, { { 0, 0, 0 }, 14 }, 100 },                        \
    }

static ColliderJntSphElementInit sPotsElementsInit[POT_GOD_FLYING_POTS] = {
    POT_ELEMENT_INIT,
    POT_ELEMENT_INIT,
    POT_ELEMENT_INIT,
};

static ColliderJntSphInit sPotsInit = {
    {
        COL_MATERIAL_HARD,
        AT_ON | AT_TYPE_ENEMY,
        AC_NONE,
        OC1_NONE,
        OC2_NONE,
        COLSHAPE_JNTSPH,
    },
    ARRAY_COUNT(sPotsElementsInit),
    sPotsElementsInit,
};

/* Swords and sticks, like Dead Hand, plus bombs so a child has another option */
static DamageTable sDamageTable = {{
    /* Deku nut      */ DMG_ENTRY(0, 0x0),
    /* Deku stick    */ DMG_ENTRY(2, 0xF),
    /* Slingshot     */ DMG_ENTRY(0, 0x0),
    /* Explosive     */ DMG_ENTRY(2, 0xF),
    /* Boomerang     */ DMG_ENTRY(0, 0x0),
    /* Normal arrow  */ DMG_ENTRY(1, 0xF),
    /* Hammer swing  */ DMG_ENTRY(2, 0xF),
    /* Hookshot      */ DMG_ENTRY(0, 0x0),
    /* Kokiri sword  */ DMG_ENTRY(2, 0xF),
    /* Master sword  */ DMG_ENTRY(2, 0xF),
    /* Giant's Knife */ DMG_ENTRY(4, 0xF),
    /* Fire arrow    */ DMG_ENTRY(2, 0xF),
    /* Ice arrow     */ DMG_ENTRY(2, 0xF),
    /* Light arrow   */ DMG_ENTRY(2, 0xF),
    /* Unk arrow 1   */ DMG_ENTRY(2, 0xF),
    /* Unk arrow 2   */ DMG_ENTRY(2, 0xF),
    /* Unk arrow 3   */ DMG_ENTRY(2, 0xF),
    /* Fire magic    */ DMG_ENTRY(2, 0xF),
    /* Ice magic     */ DMG_ENTRY(0, 0x0),
    /* Light magic   */ DMG_ENTRY(0, 0x0),
    /* Shield        */ DMG_ENTRY(0, 0x0),
    /* Mirror Ray    */ DMG_ENTRY(0, 0x0),
    /* Kokiri spin   */ DMG_ENTRY(2, 0xF),
    /* Giant spin    */ DMG_ENTRY(4, 0xF),
    /* Master spin   */ DMG_ENTRY(2, 0xF),
    /* Kokiri jump   */ DMG_ENTRY(4, 0xF),
    /* Giant jump    */ DMG_ENTRY(8, 0xF),
    /* Master jump   */ DMG_ENTRY(4, 0xF),
    /* Unknown 1     */ DMG_ENTRY(0, 0x0),
    /* Unblockable   */ DMG_ENTRY(0, 0x0),
    /* Hammer jump   */ DMG_ENTRY(4, 0xF),
    /* Unknown 2     */ DMG_ENTRY(0, 0x0),
}};

static InitChainEntry sInitChain[] = {
    ICHAIN_S8(hintId, NAVI_ENEMY_DEAD_HAND, ICHAIN_CONTINUE),
    ICHAIN_F32(targetArrowOffset, 2000, ICHAIN_CONTINUE),
    ICHAIN_F32_DIV1000(gravity, -3500, ICHAIN_STOP),
};

/* Segment 6 points at one object at a time; the Pot God draws three */
static void* PotGod_ObjectSegment(int slot)
{
    return gExObjectsAddr[slot];
}

static void PotGod_SetCpuSegment(void* segment)
{
    gSegments[6] = OS_K0_TO_PHYSICAL(segment);
}

static int PotGod_IsFacingPlayer(PotGod* this, s16 maxAngle)
{
    s16 diff;

    diff = this->actor.yawTowardsPlayer - this->actor.shape.rot.y;
    return ABS(diff) < maxAngle;
}

static void PotGod_ShowText(PlayState* play, const char* text)
{
    char* b;

    Message_StartTextbox(play, 0x88c, NULL);
    b = play->msgCtx.font.msgBuf;
    comboTextAppendHeader(&b);
    comboTextAppendStr(&b, text);
}

static void PotGod_Quake(PlayState* play, s16 y, s32 duration)
{
    Camera_RequestQuake(GET_ACTIVE_CAM(play), 0, y, duration);
}

static void PotGod_SpawnBlood(PlayState* play, Vec3f* center, f32 spread, s16 scale)
{
    Vec3f pos;
    Vec3f vel;
    Vec3f accel;

    pos.x = center->x + Rand_CenteredFloat(spread);
    pos.y = center->y + Rand_CenteredFloat(spread);
    pos.z = center->z + Rand_CenteredFloat(spread);
    vel.x = Rand_CenteredFloat(2.0f);
    vel.y = Rand_ZeroOne() * 3.0f;
    vel.z = Rand_CenteredFloat(2.0f);
    accel.x = 0.0f;
    accel.y = -0.2f;
    accel.z = 0.0f;
    EffectSsDeadDb_Spawn(play, &pos, &vel, &accel, scale, -1, 200, 0, 0, 255, 60, 0, 0, 1, 9, 0);
}

static void PotGod_SpawnShards(PlayState* play, Vec3f* center, s32 count, f32 speed)
{
    Vec3f pos;
    Vec3f vel;
    s16 angle;

    for (s32 i = 0; i < count; ++i)
    {
        angle = (s16)(Rand_ZeroOne() * 0x10000);
        pos.x = center->x + Math_SinS(angle) * 10.0f;
        pos.y = center->y + Rand_ZeroOne() * 20.0f;
        pos.z = center->z + Math_CosS(angle) * 10.0f;
        vel.x = Math_SinS(angle) * speed;
        vel.y = (Rand_ZeroOne() * speed) + 2.0f;
        vel.z = Math_CosS(angle) * speed;
        EffectSsKakera_Spawn(play, &pos, &vel, center, -240, (Rand_ZeroOne() < 0.3f) ? 96 : 32, 10, 10, 0,
            (Rand_ZeroOne() * 95.0f) + 15.0f, 0, 32, 60, KAKERA_COLOR_NONE, OBJECT_TSUBO, (Gfx*)object_tsubo_DL_001960);
    }
}

static void PotGod_BreakPot(PotGod* this, PlayState* play, PotGodFlyingPot* pot)
{
    pot->active = FALSE;
    PotGod_SpawnShards(play, &pot->pos, 10, 4.0f);
    SfxSource_PlaySfxAtFixedWorldPos(play, &pot->pos, 20, NA_SE_EV_POT_BROKEN);
}

/* The middle of the pot's mouth, where the arms come out */
static void PotGod_GetMouthPos(PotGod* this, Vec3f* dst)
{
    *dst = this->actor.world.pos;
    dst->y += POT_GOD_BODY_HEIGHT - 4.0f + this->bodyRise + this->hopHeight;
}

static int PotGod_ObjectsLoaded(PotGod* this, PlayState* play)
{
    return Object_IsLoaded(&play->objectCtx, this->tsuboSlot) && Object_IsLoaded(&play->objectCtx, this->guardSlot);
}

void PotGod_Init(Actor* thisx, PlayState* play)
{
    PotGod* this = (PotGod*)thisx;

    this->tsuboSlot = Object_GetSlot(&play->objectCtx, OBJECT_TSUBO);
    this->guardSlot = Object_GetSlot(&play->objectCtx, OBJECT_SD);
    if (this->tsuboSlot < 0 || this->guardSlot < 0)
    {
        Actor_Kill(&this->actor);
        return;
    }

    Actor_ProcessInitChain(&this->actor, sInitChain);
    Actor_SetScale(&this->actor, 0.01f);
    this->actor.colChkInfo.damageTable = &sDamageTable;
    this->actor.colChkInfo.mass = MASS_HEAVY;
    this->actor.colChkInfo.health = POT_GOD_HEALTH;
    ActorShape_Init(&this->actor.shape, 0.0f, NULL, 0.0f);

    /* Dead Hand's hand: our own object, already in segment 6. The guard's skeleton is set up with the cutscene. */
    SkelAnime_InitFlex(play, &this->armSkelAnime, (void*)SKEL_DH_HAND, (void*)ANIM_DH_HAND, this->armJointTable, this->armMorphTable, 4);

    Collider_InitCylinder(play, &this->collider);
    Collider_SetCylinder(play, &this->collider, &this->actor, &sCylinderInit);
    Collider_InitJntSph(play, &this->colliderHands);
    Collider_SetJntSph(play, &this->colliderHands, &this->actor, &sHandsInit, this->colliderHandsElements);
    Collider_InitJntSph(play, &this->colliderPots);
    Collider_SetJntSph(play, &this->colliderPots, &this->actor, &sPotsInit, this->colliderPotsElements);

    this->subCamId = CAM_ID_MAIN;
    this->bodyRise = -(POT_GOD_BODY_HEIGHT + 20.0f);
    this->actor.draw = NULL;
    this->actionFunc = PotGod_Watch;
}

void PotGod_Destroy(Actor* thisx, PlayState* play)
{
    PotGod* this = (PotGod*)thisx;

    Collider_DestroyCylinder(play, &this->collider);
    Collider_DestroyJntSph(play, &this->colliderHands);
    Collider_DestroyJntSph(play, &this->colliderPots);
}

/* Count the pots left in the room */
static s32 PotGod_CountPots(PlayState* play)
{
    s32 count;

    count = 0;
    for (Actor* a = play->actorCtx.actors[ACTORCAT_PROP].first; a != NULL; a = a->next)
    {
        if (a->id == ACTOR_OBJ_TSUBO)
            count++;
    }
    return count;
}

static void PotGod_Watch(PotGod* this, PlayState* play)
{
    s32 count;

    count = PotGod_CountPots(play);
    if (count > this->potsSeen)
        this->potsSeen = (count > 0xff) ? 0xff : count;

    if (this->potsSeen >= POT_GOD_MIN_POTS && count == 0)
    {
        this->calmFrames = 0;
        this->actionFunc = PotGod_WaitCalm;
    }
}

static Actor* PotGod_FindGuard(PlayState* play)
{
    for (Actor* a = play->actorCtx.actors[ACTORCAT_NPC].first; a != NULL; a = a->next)
    {
        switch (a->id)
        {
        case ACTOR_EN_HEISHI1:
        case ACTOR_EN_HEISHI2:
        case ACTOR_EN_HEISHI3:
        case ACTOR_EN_HEISHI4:
            return a;
        }
    }
    return NULL;
}

static void PotGod_UpdateCsCamera(PotGod* this, PlayState* play, f32 dist, f32 eyeHeight, f32 atHeight)
{
    Vec3f at;
    Vec3f eye;

    at.x = this->guardPos.x;
    at.y = this->guardPos.y + atHeight;
    at.z = this->guardPos.z;
    eye.x = this->guardPos.x + Math_SinS(this->guardYaw) * dist;
    eye.y = this->guardPos.y + eyeHeight;
    eye.z = this->guardPos.z + Math_CosS(this->guardYaw) * dist;

    Math_ApproachF(&this->subCamAt.x, at.x, 0.2f, 20.0f);
    Math_ApproachF(&this->subCamAt.y, at.y, 0.2f, 20.0f);
    Math_ApproachF(&this->subCamAt.z, at.z, 0.2f, 20.0f);
    Math_ApproachF(&this->subCamEye.x, eye.x, 0.2f, 20.0f);
    Math_ApproachF(&this->subCamEye.y, eye.y, 0.2f, 20.0f);
    Math_ApproachF(&this->subCamEye.z, eye.z, 0.2f, 20.0f);
    Play_SetCameraAtEye(play, this->subCamId, &this->subCamAt, &this->subCamEye);
}

static void PotGod_StartCutscene(PotGod* this, PlayState* play)
{
    Player* player = GET_PLAYER(play);
    Camera* mainCam;
    Actor* guard;

    guard = PotGod_FindGuard(play);
    if (guard != NULL)
    {
        PotGod_SetCpuSegment(PotGod_ObjectSegment(this->guardSlot));
        SkelAnime_Init(play, &this->guardSkelAnime, (void*)gEnHeishiSkel, (void*)gEnHeishiIdleAnim, this->guardJointTable, this->guardMorphTable, 17);
        PotGod_SetCpuSegment(PotGod_ObjectSegment(this->actor.objectSlot));
        this->guardPos = guard->world.pos;
        Actor_Kill(guard);
    }
    else
    {
        /* No guard: the Pot God bursts out of the floor in front of Link */
        this->guardPos.x = player->actor.world.pos.x + Math_SinS(player->actor.shape.rot.y) * 150.0f;
        this->guardPos.y = player->actor.world.pos.y;
        this->guardPos.z = player->actor.world.pos.z + Math_CosS(player->actor.shape.rot.y) * 150.0f;
    }
    this->guardYaw = Math_Vec3f_Yaw(&this->guardPos, &player->actor.world.pos);
    this->actor.world.pos = this->guardPos;
    this->actor.home.pos = this->guardPos;
    this->actor.shape.rot.y = this->actor.world.rot.y = this->guardYaw;

    Cutscene_StartManual(play, &play->csCtx);
    this->subCamId = Play_CreateSubCamera(play);
    Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STATUS_WAIT);
    Play_ChangeCameraStatus(play, this->subCamId, CAM_STATUS_ACTIVE);
    Player_SetCsActionWithHaltedActors(play, &this->actor, PLAYER_CSACTION_1);
    mainCam = Play_GetCamera(play, CAM_ID_MAIN);
    this->subCamEye = mainCam->eye;
    this->subCamAt = mainCam->at;

    /* Silence, then horror */
    this->prevBgm = Audio_GetActiveSeqId(SEQ_PLAYER_BGM_MAIN);
    SEQCMD_STOP_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 1);

    this->drawGuard = (guard != NULL);
    this->actor.draw = PotGod_Draw;
    this->csTimer = 0;
    this->actionFunc = PotGod_Cutscene;
}

static void PotGod_WaitCalm(PotGod* this, PlayState* play)
{
    Player* player = GET_PLAYER(play);

    /* Wait for any item from the last pot and its text to be done */
    if (play->msgCtx.msgMode == MSGMODE_NONE && player->csAction == PLAYER_CSACTION_NONE && (player->actor.bgCheckFlags & BGCHECKFLAG_GROUND)
        && !(player->stateFlags1 & (PLAYER_ACTOR_STATE_HOLD_ITEM | PLAYER_ACTOR_STATE_GET_ITEM | PLAYER_ACTOR_STATE_CUTSCENE_FROZEN)))
    {
        this->calmFrames++;
    }
    else
    {
        this->calmFrames = 0;
    }

    /* The room objects load in the background; by now they are long done, but make sure */
    if (this->calmFrames >= 20 && PotGod_ObjectsLoaded(this, play))
        PotGod_StartCutscene(this, play);
}

static void PotGod_EndCutscene(PotGod* this, PlayState* play)
{
    Play_ReturnToMainCam(play, this->subCamId, 0);
    this->subCamId = CAM_ID_MAIN;
    Cutscene_StopManual(play, &play->csCtx);
    Player_SetCsActionWithHaltedActors(play, &this->actor, PLAYER_CSACTION_END);
    SEQCMD_PLAY_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 0, 0, NA_BGM_MINI_BOSS);
    this->actor.flags |= ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_HOSTILE;
    this->summonTimer = 60;
    PotGod_SetupHop(this);
}

/* Shout, collapse and sink, the pot bursts out, the arms tear out of it, roar */
static void PotGod_Cutscene(PotGod* this, PlayState* play)
{
    s16 t;
    Vec3f center;
    Vec3f mouth;

    t = this->csTimer++;

    center = this->guardPos;
    center.y += 30.0f;

    if (t == 0)
    {
        PotGod_ShowText(play, TEXT_COLOR_RED "OH GOD!" TEXT_CZ TEXT_FADE("\x28") TEXT_END);
        Actor_PlaySfx(&this->actor, SFX_TALON_SURPRISE);
        PotGod_Quake(play, 2, 20);
    }

    if (t < 60)
    {
        /* Shout: trembling in horror */
        PotGod_UpdateCsCamera(this, play, 110.0f, 50.0f, 45.0f);
    }
    else if (t < 120)
    {
        /* Collapse, in pain, and get dragged into the floor */
        if (t == 60)
        {
            if (this->drawGuard)
            {
                PotGod_SetCpuSegment(PotGod_ObjectSegment(this->guardSlot));
                Animation_Change(&this->guardSkelAnime, (void*)gEnHeishiDyingGuardDieAnim, 1.5f, 0.0f, Animation_GetLastFrame((void*)gEnHeishiDyingGuardDieAnim), ANIMMODE_ONCE, -6.0f);
                PotGod_SetCpuSegment(PotGod_ObjectSegment(this->actor.objectSlot));
            }
            Actor_PlaySfx(&this->actor, SFX_INGO_CRY);
            PotGod_Quake(play, 4, 30);
        }
        if (t == 90)
            Actor_PlaySfx(&this->actor, SFX_TALON_CRY);
        Math_ApproachF(&this->guardSink, (t < 100) ? 12.0f : 40.0f, 0.1f, 1.5f);
        if ((t & 3) == 0)
            PotGod_SpawnBlood(play, &center, 30.0f, 60);
        PotGod_UpdateCsCamera(this, play, 90.0f, 25.0f, 20.0f);
    }
    else if (t < 150)
    {
        /* The pot bursts out of the floor where he went down */
        if (t == 120)
        {
            this->drawBody = TRUE;
            Actor_PlaySfx(&this->actor, NA_SE_EN_DEADHAND_HIDE);
            PotGod_Quake(play, 6, 40);
        }
        if (t == 128)
            this->drawGuard = FALSE;
        Math_ApproachF(&this->bodyRise, 0.0f, 0.25f, 8.0f);
        if ((t & 1) == 0)
            PotGod_SpawnBlood(play, &center, 50.0f, 90);
        if ((t % 8) == 0)
        {
            PotGod_SpawnShards(play, &center, 4, 6.0f);
            SfxSource_PlaySfxAtFixedWorldPos(play, &center, 20, NA_SE_EV_POT_BROKEN);
        }
        PotGod_UpdateCsCamera(this, play, 200.0f, 90.0f, 70.0f);
    }
    else if (t < 190)
    {
        /* One by one, the arms tear out of its mouth */
        this->bodyRise = 0.0f;
        for (s32 i = 0; i < POT_GOD_ARMS; ++i)
        {
            if (t == 150 + i * 8)
            {
                PotGod_GetMouthPos(this, &mouth);
                Actor_PlaySfx(&this->actor, NA_SE_EN_DEADHAND_HAND_AT);
                for (s32 j = 0; j < 4; ++j)
                    PotGod_SpawnBlood(play, &mouth, 20.0f, 80);
                PotGod_Quake(play, 3, 8);
            }
            if (t >= 150 + i * 8)
                Math_ApproachF(&this->armGrow[i], 1.0f, 0.3f, 0.12f);
        }
        PotGod_UpdateCsCamera(this, play, 210.0f, 110.0f, 90.0f);
    }
    else if (t < 250)
    {
        /* Roar */
        if (t == 190)
        {
            Actor_PlaySfx(&this->actor, NA_SE_EN_DEADHAND_LAUGH);
            PotGod_Quake(play, 5, 30);
            PotGod_ShowText(play, TEXT_COLOR_RED "POT GOD" TEXT_CZ TEXT_NL "Keeper of Every Broken Pot" TEXT_FADE("\x3c") TEXT_END);
        }
        for (s32 i = 0; i < POT_GOD_ARMS; ++i)
            this->armGrow[i] = 1.0f;
        PotGod_UpdateCsCamera(this, play, 200.0f, 90.0f, 90.0f);
    }
    else if (play->msgCtx.msgMode == MSGMODE_NONE)
    {
        PotGod_EndCutscene(this, play);
        return;
    }

    if (this->drawGuard)
    {
        PotGod_SetCpuSegment(PotGod_ObjectSegment(this->guardSlot));
        SkelAnime_Update(&this->guardSkelAnime);
        PotGod_SetCpuSegment(PotGod_ObjectSegment(this->actor.objectSlot));
    }
    if (this->drawBody)
        SkelAnime_Update(&this->armSkelAnime);
}

static void PotGod_SetupHop(PotGod* this)
{
    this->timer = 0;
    this->slamTilt = 0;
    this->slamActive = FALSE;
    this->actionFunc = PotGod_Hop;
}

static void PotGod_SetupSlam(PotGod* this)
{
    this->timer = 0;
    this->actor.speed = 0.0f;
    this->hopHeight = 0.0f;
    this->actionFunc = PotGod_Slam;
}

static void PotGod_SetupSummon(PotGod* this)
{
    this->actor.speed = 0.0f;
    this->hopHeight = 0.0f;
    this->timer = 0;
    this->actionFunc = PotGod_Summon;
}

/* It has no legs: it hops at Link, landing with a thud */
static void PotGod_Hop(PotGod* this, PlayState* play)
{
    s16 t;

    Math_SmoothStepToS(&this->actor.shape.rot.y, this->actor.yawTowardsPlayer, 1, 0x300, 0);
    this->actor.world.rot.y = this->actor.shape.rot.y;
    SkelAnime_Update(&this->armSkelAnime);

    if (this->summonTimer > 0)
        this->summonTimer--;

    t = this->timer;
    if (t < POT_GOD_HOP_AIR)
    {
        this->hopHeight = Math_SinS((t * 0x8000) / POT_GOD_HOP_AIR) * 24.0f;
        this->actor.speed = (this->actor.xzDistToPlayer > 70.0f) ? POT_GOD_HOP_SPEED : 0.0f;
    }
    else
    {
        if (t == POT_GOD_HOP_AIR)
        {
            Actor_PlaySfx(&this->actor, NA_SE_EN_DEADHAND_WALK);
            PotGod_Quake(play, 2, 6);
        }
        this->hopHeight = 0.0f;
        this->actor.speed = 0.0f;
    }
    if ((play->gameplayFrames & 0x5F) == 0)
        Actor_PlaySfx(&this->actor, NA_SE_EN_DEADHAND_LAUGH);

    /* Only decide on the ground */
    if (++this->timer < POT_GOD_HOP_LENGTH)
        return;
    this->timer = 0;

    if (this->actor.xzDistToPlayer <= 110.0f && PotGod_IsFacingPlayer(this, 0x2AAA))
        PotGod_SetupSlam(this);
    else if (this->summonTimer == 0 && this->actor.xzDistToPlayer > 160.0f)
        PotGod_SetupSummon(this);
}

/* The two front arms rear back, then slam down in front of it */
static void PotGod_Slam(PotGod* this, PlayState* play)
{
    s16 t;

    SkelAnime_Update(&this->armSkelAnime);
    t = this->timer++;

    if (t < 16)
    {
        Math_SmoothStepToS(&this->actor.shape.rot.y, this->actor.yawTowardsPlayer, 1, 0x600, 0);
        this->actor.world.rot.y = this->actor.shape.rot.y;
        Math_SmoothStepToS(&this->slamTilt, -0x1800, 1, 0x400, 0);
        if (t == 4)
            Actor_PlaySfx(&this->actor, NA_SE_EN_DEADHAND_GRIP);
    }
    else if (t < 24)
    {
        if (t == 16)
        {
            Actor_PlaySfx(&this->actor, NA_SE_EN_DEADHAND_HAND_AT);
            this->slamActive = TRUE;
        }
        Math_SmoothStepToS(&this->slamTilt, 0x5800, 1, 0x1400, 0);
        if (t == 22)
        {
            PotGod_Quake(play, 4, 10);
            for (s32 i = 0; i < POT_GOD_SLAM_ARMS; ++i)
                PotGod_SpawnBlood(play, &this->handPos[i], 10.0f, 70);
        }
    }
    else if (t < 50)
    {
        this->slamActive = FALSE;
        Math_SmoothStepToS(&this->slamTilt, 0, 1, 0x300, 0);
    }
    else
    {
        PotGod_SetupHop(this);
    }

    if (this->colliderHands.base.atFlags & (AT_HIT | AT_BOUNCED))
    {
        this->colliderHands.base.atFlags &= ~(AT_HIT | AT_BOUNCED);
        this->slamActive = FALSE;
    }
}

static void PotGod_LaunchPot(PotGod* this, PlayState* play, s32 index)
{
    Player* player = GET_PLAYER(play);
    PotGodFlyingPot* pot = &this->pots[index];
    Vec3f target;
    f32 t;

    if (pot->active)
        return;

    PotGod_GetMouthPos(this, &pot->pos);
    pot->pos.x += Math_SinS(this->actor.shape.rot.y + 0x4000 * (index - 1)) * 30.0f;
    pot->pos.y += 25.0f;
    pot->pos.z += Math_CosS(this->actor.shape.rot.y + 0x4000 * (index - 1)) * 30.0f;
    target = player->actor.world.pos;
    target.y += 20.0f;

    t = (f32)POT_GOD_POT_FLIGHT;
    pot->vel.x = (target.x - pot->pos.x) / t;
    pot->vel.z = (target.z - pot->pos.z) / t;
    pot->vel.y = ((target.y - pot->pos.y) / t) + (0.5f * POT_GOD_POT_GRAVITY * t);
    pot->rotX = 0;
    pot->rotY = 0;
    pot->timer = 0;
    pot->active = TRUE;
    Actor_PlaySfx(&this->actor, NA_SE_EN_DEADHAND_HAND_AT);
}

/* The arms reach into its mouth and fling pots at Link */
static void PotGod_Summon(PotGod* this, PlayState* play)
{
    Math_SmoothStepToS(&this->actor.shape.rot.y, this->actor.yawTowardsPlayer, 1, 0x400, 0);
    this->actor.world.rot.y = this->actor.shape.rot.y;
    SkelAnime_Update(&this->armSkelAnime);
    this->slamTilt = (s16)(Math_SinS(this->timer * 0x1800) * 0x1800);

    switch (this->timer)
    {
    case 10:
        Actor_PlaySfx(&this->actor, NA_SE_EN_DEADHAND_LAUGH);
        break;
    case 18:
        PotGod_LaunchPot(this, play, 0);
        break;
    case 26:
        PotGod_LaunchPot(this, play, 1);
        break;
    case 34:
        PotGod_LaunchPot(this, play, 2);
        break;
    }

    if (++this->timer >= 50)
    {
        this->summonTimer = 100 + (s16)(Rand_ZeroOne() * 60.0f);
        PotGod_SetupHop(this);
    }
}

static void PotGod_SetupDamage(PotGod* this)
{
    this->actor.speed = 0.0f;
    this->hopHeight = 0.0f;
    this->slamActive = FALSE;
    this->timer = 0;
    Actor_PlaySfx(&this->actor, NA_SE_EN_DEADHAND_DAMAGE);
    this->isHurt = TRUE;
    this->actionFunc = PotGod_Damage;
}

static void PotGod_Damage(PotGod* this, PlayState* play)
{
    Vec3f mouth;

    SkelAnime_Update(&this->armSkelAnime);
    this->wobble = (s16)(Math_SinS(this->timer * 0x3000) * (0x800 - this->timer * 0x50));
    this->slamTilt = (s16)(Math_CosS(this->timer * 0x2000) * 0x1000);
    if (this->timer == 0)
    {
        PotGod_GetMouthPos(this, &mouth);
        for (s32 i = 0; i < 3; ++i)
            PotGod_SpawnBlood(play, &mouth, 30.0f, 80);
    }
    if (++this->timer >= 20)
    {
        this->isHurt = FALSE;
        this->wobble = 0;
        PotGod_SetupHop(this);
    }
}

static void PotGod_SetupDeath(PotGod* this, PlayState* play)
{
    this->actor.flags &= ~(ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_HOSTILE);
    this->actor.speed = 0.0f;
    this->hopHeight = 0.0f;
    this->slamActive = FALSE;
    this->timer = 0;
    Actor_PlaySfx(&this->actor, NA_SE_EN_DEADHAND_DEAD);
    for (s32 i = 0; i < POT_GOD_FLYING_POTS; ++i)
    {
        if (this->pots[i].active)
            PotGod_BreakPot(this, play, &this->pots[i]);
    }
    SaveFlag_Set(SAVE_FLAG_POT_GOD_DEFEATED);
    this->actionFunc = PotGod_Death;
}

static void PotGod_Death(PotGod* this, PlayState* play)
{
    Vec3f center;
    Vec3f mouth;

    center = this->actor.world.pos;
    center.y += POT_GOD_BODY_HEIGHT * 0.5f;
    PotGod_GetMouthPos(this, &mouth);
    this->timer++;

    if (this->timer < 60)
    {
        /* The arms go limp and droop over the rim while it shakes and bleeds */
        Math_SmoothStepToS(&this->slamTilt, 0x6000, 1, 0x200, 0);
        this->wobble = (s16)(Math_SinS(this->timer * 0x3800) * 0x600);
        if ((this->timer & 3) == 0)
            PotGod_SpawnBlood(play, &mouth, 40.0f, 80);
        if (this->timer == 30)
            Actor_PlaySfx(&this->actor, NA_SE_EN_DEADHAND_HAND_DEAD);
        return;
    }

    if (this->timer == 60)
    {
        /* Shatter into a hail of pottery */
        PotGod_SpawnShards(play, &mouth, 30, 8.0f);
        PotGod_SpawnShards(play, &center, 30, 6.0f);
        SfxSource_PlaySfxAtFixedWorldPos(play, &mouth, 40, NA_SE_EV_POT_BROKEN);
        SfxSource_PlaySfxAtFixedWorldPos(play, &center, 40, NA_SE_EV_POT_BROKEN);
        PotGod_Quake(play, 5, 20);
        this->drawBody = FALSE;
        Item_DropCollectible(play, &this->actor.world.pos, ITEM00_RECOVERY_HEART);
        Item_DropCollectible(play, &this->actor.world.pos, ITEM00_RECOVERY_HEART);
        if (this->prevBgm == NA_BGM_DISABLED || (this->prevBgm & 0xff) == 0 || (this->prevBgm & 0xff) == 0xff)
            SEQCMD_STOP_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 1);
        else
            SEQCMD_PLAY_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 0, 0, this->prevBgm & 0xff);
        Audio_PlaySfx_AtPosWithReverb(&gSfxDefaultPos, NA_SE_SY_CORRECT_CHIME, 0);
        return;
    }

    if (this->timer == 75)
    {
        PotGod_ShowText(play, "The Pot God shattered..." TEXT_NL "Its pots are scattered all over" TEXT_NL TEXT_COLOR_RED "Hyrule Field" TEXT_CZ "!" TEXT_END);
        Actor_Kill(&this->actor);
    }
}

static void PotGod_CollisionCheck(PotGod* this, PlayState* play)
{
    s32 hit;

    hit = FALSE;
    if (this->colliderHands.base.acFlags & AC_HIT)
    {
        this->colliderHands.base.acFlags &= ~AC_HIT;
        hit = TRUE;
    }
    if (this->collider.base.acFlags & AC_HIT)
    {
        this->collider.base.acFlags &= ~AC_HIT;
        hit = TRUE;
    }

    if (!hit || this->isHurt || this->actor.colChkInfo.health == 0)
        return;
    if (this->actionFunc == PotGod_Death || this->actionFunc == PotGod_Cutscene || this->actionFunc == PotGod_Watch || this->actionFunc == PotGod_WaitCalm)
        return;
    if (this->actor.colChkInfo.damageEffect == 0 || this->actor.colChkInfo.damage == 0)
        return;

    Actor_SetColorFilter(&this->actor, COLORFILTER_COLORFLAG_RED, 255, COLORFILTER_BUFFLAG_OPA, 8);
    if (Actor_ApplyDamage(&this->actor) == 0)
        PotGod_SetupDeath(this, play);
    else
        PotGod_SetupDamage(this);
}

static void PotGod_UpdatePots(PotGod* this, PlayState* play)
{
    PotGodFlyingPot* pot;
    ColliderJntSphElement* elem;

    for (s32 i = 0; i < POT_GOD_FLYING_POTS; ++i)
    {
        pot = &this->pots[i];
        elem = &this->colliderPots.elements[i];

        if (pot->active && (elem->base.atElemFlags & ATELEM_HIT))
            PotGod_BreakPot(this, play, pot);
        elem->base.atElemFlags &= ~ATELEM_HIT;

        if (pot->active)
        {
            pot->vel.y -= POT_GOD_POT_GRAVITY;
            pot->pos.x += pot->vel.x;
            pot->pos.y += pot->vel.y;
            pot->pos.z += pot->vel.z;
            pot->rotX += 0x900;
            pot->rotY += 0x500;
            pot->timer++;
            if (pot->pos.y <= this->actor.floorHeight + 5.0f || pot->timer > 60)
                PotGod_BreakPot(this, play, pot);
        }

        if (pot->active)
        {
            elem->base.atElemFlags = ATELEM_ON;
            elem->dim.worldSphere.center.x = pot->pos.x;
            elem->dim.worldSphere.center.y = pot->pos.y;
            elem->dim.worldSphere.center.z = pot->pos.z;
            elem->dim.worldSphere.radius = 14;
        }
        else
        {
            elem->base.atElemFlags = ATELEM_NONE;
            elem->dim.worldSphere.radius = 0;
        }
    }
    this->colliderPots.base.atFlags &= ~(AT_HIT | AT_BOUNCED);
}

/* The front hands' spheres follow the hands (positions from the last draw) */
static void PotGod_UpdateHands(PotGod* this)
{
    ColliderJntSphElement* elem;

    for (s32 i = 0; i < POT_GOD_SLAM_ARMS; ++i)
    {
        elem = &this->colliderHands.elements[i];
        elem->dim.worldSphere.center.x = this->handPos[i].x;
        elem->dim.worldSphere.center.y = this->handPos[i].y;
        elem->dim.worldSphere.center.z = this->handPos[i].z;
        elem->dim.worldSphere.radius = 14;
        elem->base.atElemFlags = this->slamActive ? ATELEM_ON : ATELEM_NONE;
        elem->base.atElemFlags &= ~ATELEM_HIT;
    }
}

static int PotGod_PotsActive(PotGod* this)
{
    for (s32 i = 0; i < POT_GOD_FLYING_POTS; ++i)
    {
        if (this->pots[i].active)
            return TRUE;
    }
    return FALSE;
}

static int PotGod_IsFighting(PotGod* this)
{
    return this->actionFunc == PotGod_Hop || this->actionFunc == PotGod_Slam || this->actionFunc == PotGod_Summon || this->actionFunc == PotGod_Damage;
}

void PotGod_Update(Actor* thisx, PlayState* play)
{
    PotGod* this = (PotGod*)thisx;

    PotGod_CollisionCheck(this, play);
    this->actionFunc(this, play);
    if (this->actor.update == NULL)
        return;
    if (this->actionFunc == PotGod_Watch || this->actionFunc == PotGod_WaitCalm)
        return;

    this->shardAngle += 0x300;
    Actor_MoveXZGravity(&this->actor);
    Actor_UpdateBgCheckInfo(play, &this->actor, 20.0f, POT_GOD_BODY_RADIUS, 45.0f, UPDBGCHECKINFO_FLAG_OOT_0 | UPDBGCHECKINFO_FLAG_OOT_2 | UPDBGCHECKINFO_FLAG_OOT_3 | UPDBGCHECKINFO_FLAG_OOT_4);
    PotGod_GetMouthPos(this, &this->actor.focus.pos);
    PotGod_UpdatePots(this, play);
    PotGod_UpdateHands(this);

    Collider_UpdateCylinder(&this->actor, &this->collider);
    this->collider.dim.yShift = (s16)this->hopHeight;
    if (PotGod_IsFighting(this))
    {
        CollisionCheck_SetOC(play, &play->colChkCtx, &this->collider.base);
        if (!this->isHurt)
        {
            CollisionCheck_SetAC(play, &play->colChkCtx, &this->collider.base);
            CollisionCheck_SetAC(play, &play->colChkCtx, &this->colliderHands.base);
        }
        if (this->slamActive)
            CollisionCheck_SetAT(play, &play->colChkCtx, &this->colliderHands.base);
    }
    if (PotGod_PotsActive(this))
        CollisionCheck_SetAT(play, &play->colChkCtx, &this->colliderPots.base);
}

/* Each arm writhes on its own; the front two also rear back and slam */
static s32 PotGod_ArmOverrideLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot, void* thisx)
{
    PotGod* this = (PotGod*)thisx;
    s16 phase;

    /* Like En_Dha: these rotations straighten the arm (pointing up), plus our writhing */
    phase = kArmPhase[this->drawArm] + this->shardAngle * 3;
    switch (limbIndex)
    {
    case 1:
        rot->y = (s16)(Math_CosS(phase) * 0x600);
        rot->z += (s16)(Math_SinS(phase) * 0x900);
        break;
    case 2:
        rot->z = (s16)(Math_SinS(phase + 0x3000) * 0xC00);
        break;
    case 3:
        rot->y = (s16)(Math_SinS(phase * 2) * 0x1000);
        rot->z = (s16)(Math_CosS(phase + 0x2000) * 0x800);
        break;
    }
    return FALSE;
}

static void PotGod_ArmPostLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3s* rot, void* thisx)
{
    static Vec3f sHandOffset = { 1100.0f, 0.0f, 0.0f };
    PotGod* this = (PotGod*)thisx;

    if (limbIndex == 3)
        Matrix_MultVec3f(&sHandOffset, &this->handPos[this->drawArm]);
}

static void PotGod_DrawPotDList(PlayState* play, Gfx* dl, Vec3f* pos, s16 rx, s16 ry, s16 rz, f32 scale)
{
    OPEN_DISPS(play->state.gfxCtx);
    Matrix_Push();
    Matrix_Translate(pos->x, pos->y, pos->z, MTXMODE_NEW);
    Matrix_RotateZYX(rx, ry, rz, MTXMODE_APPLY);
    Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
    gSPMatrix(POLY_OPA_DISP++, Matrix_Finalize(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPDisplayList(POLY_OPA_DISP++, dl);
    Matrix_Pop();
    CLOSE_DISPS();
}

static void PotGod_DrawGuard(PotGod* this, PlayState* play)
{
    void* seg;
    s16 shakeX;
    s16 shakeZ;

    seg = PotGod_ObjectSegment(this->guardSlot);
    shakeX = (s16)Rand_CenteredFloat(0x600);
    shakeZ = (s16)Rand_CenteredFloat(0x600);

    OPEN_DISPS(play->state.gfxCtx);
    PotGod_SetCpuSegment(seg);
    gSPSegment(POLY_OPA_DISP++, 0x06, seg);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    Matrix_Push();
    Matrix_Translate(this->guardPos.x, this->guardPos.y - this->guardSink, this->guardPos.z, MTXMODE_NEW);
    Matrix_RotateZYX(shakeX, this->guardYaw, shakeZ, MTXMODE_APPLY);
    Matrix_Scale(0.01f, 0.01f, 0.01f, MTXMODE_APPLY);
    SkelAnime_DrawOpa(play, this->guardSkelAnime.skeleton, this->guardSkelAnime.jointTable, NULL, NULL, this);
    Matrix_Pop();
    seg = PotGod_ObjectSegment(this->actor.objectSlot);
    PotGod_SetCpuSegment(seg);
    gSPSegment(POLY_OPA_DISP++, 0x06, seg);
    CLOSE_DISPS();
}

/* Dead Hand's arms, growing out of the mouth */
static void PotGod_DrawArms(PotGod* this, PlayState* play)
{
    Vec3f mouth;
    f32 s;
    s16 yaw;
    s16 tilt;

    PotGod_GetMouthPos(this, &mouth);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    for (s32 i = 0; i < POT_GOD_ARMS; ++i)
    {
        if (this->armGrow[i] <= 0.01f)
            continue;

        yaw = this->actor.shape.rot.y + kArmYaw[i];
        tilt = POT_GOD_ARM_TILT + (s16)(Math_SinS(kArmPhase[i] + this->shardAngle * 2) * 0x600);
        if (i < POT_GOD_SLAM_ARMS || this->actionFunc == PotGod_Death)
            tilt += this->slamTilt;
        s = POT_GOD_ARM_SCALE * this->armGrow[i];

        this->drawArm = i;
        Matrix_Push();
        Matrix_Translate(mouth.x + Math_SinS(yaw) * POT_GOD_RIM_RADIUS, mouth.y, mouth.z + Math_CosS(yaw) * POT_GOD_RIM_RADIUS, MTXMODE_NEW);
        Matrix_RotateZYX(tilt, yaw, this->wobble, MTXMODE_APPLY);
        Matrix_Scale(s, s, s, MTXMODE_APPLY);
        SkelAnime_DrawFlexOpa(play, this->armSkelAnime.skeleton, this->armSkelAnime.jointTable, this->armSkelAnime.dListCount,
            PotGod_ArmOverrideLimbDraw, PotGod_ArmPostLimbDraw, this);
        Matrix_Pop();
    }
}

static void PotGod_DrawBody(PotGod* this, PlayState* play)
{
    void* seg;
    Vec3f pos;
    s16 angle;

    OPEN_DISPS(play->state.gfxCtx);

    /* The pot parts */
    seg = PotGod_ObjectSegment(this->tsuboSlot);
    gSPSegment(POLY_OPA_DISP++, 0x06, seg);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);

    /* The giant pot */
    pos = this->actor.world.pos;
    pos.y += this->bodyRise + this->hopHeight;
    PotGod_DrawPotDList(play, (Gfx*)object_tsubo_DL_0017C0, &pos, this->wobble, this->actor.shape.rot.y, (s16)(this->wobble / 2), POT_GOD_BODY_SCALE);

    /* Shards circling it */
    for (s32 i = 0; i < POT_GOD_SHARDS; ++i)
    {
        angle = this->shardAngle + i * (0x10000 / POT_GOD_SHARDS);
        pos.x = this->actor.world.pos.x + Math_SinS(angle) * (POT_GOD_BODY_RADIUS + 20.0f);
        pos.y = this->actor.world.pos.y + this->bodyRise + this->hopHeight + 50.0f + Math_SinS(angle * 2) * 12.0f;
        pos.z = this->actor.world.pos.z + Math_CosS(angle) * (POT_GOD_BODY_RADIUS + 20.0f);
        PotGod_DrawPotDList(play, (Gfx*)object_tsubo_DL_001960, &pos, angle * 2, angle, 0, 0.12f);
    }

    /* Back to our own object (Dead Hand's), for the arms */
    seg = PotGod_ObjectSegment(this->actor.objectSlot);
    PotGod_SetCpuSegment(seg);
    gSPSegment(POLY_OPA_DISP++, 0x06, seg);
    CLOSE_DISPS();

    PotGod_DrawArms(this, play);
}

static void PotGod_DrawFlyingPots(PotGod* this, PlayState* play)
{
    void* seg;

    if (!PotGod_PotsActive(this))
        return;

    OPEN_DISPS(play->state.gfxCtx);
    seg = PotGod_ObjectSegment(this->tsuboSlot);
    gSPSegment(POLY_OPA_DISP++, 0x06, seg);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    for (s32 i = 0; i < POT_GOD_FLYING_POTS; ++i)
    {
        Vec3f pos;

        if (!this->pots[i].active)
            continue;
        pos = this->pots[i].pos;
        pos.y -= 12.0f;
        PotGod_DrawPotDList(play, (Gfx*)object_tsubo_DL_0017C0, &pos, this->pots[i].rotX, this->pots[i].rotY, 0, 0.12f);
    }
    seg = PotGod_ObjectSegment(this->actor.objectSlot);
    gSPSegment(POLY_OPA_DISP++, 0x06, seg);
    CLOSE_DISPS();
}

void PotGod_Draw(Actor* thisx, PlayState* play)
{
    PotGod* this = (PotGod*)thisx;

    if (this->drawBody)
        PotGod_DrawBody(this, play);
    if (this->drawGuard)
        PotGod_DrawGuard(this, play);
    PotGod_DrawFlyingPots(this, play);
}

static ActorProfile PotGod_InitVars =
{
    ACTOR_CUSTOM_POT_GOD,
    ACTORCAT_BOSS,
    FLAGS,
    OBJECT_DH,
    sizeof(PotGod),
    (ActorFunc)PotGod_Init,
    (ActorFunc)PotGod_Destroy,
    (ActorFunc)PotGod_Update,
    (ActorFunc)PotGod_Draw,
};

OVL_INFO_ACTOR(ACTOR_CUSTOM_POT_GOD, PotGod_InitVars);
