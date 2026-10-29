/*
 * Fork: the Pot God (Custom > Ocarina of Time > "Pot God").
 *
 * Woken in the child Market Pot House (Custom_Pot_God_Watcher), then spawned
 * by the boss kit in the hellfire arena (params BOSS_KIT_PARAMS_ARENA), or
 * by the watcher in the Pot House itself. It bursts out of the ground: a
 * giant pot, bleeding from its rim, with four of Dead Hand's grasping arms
 * writhing out of its mouth and pot shards circling it. It hops at Link,
 * slams him with its two front arms and throws pots. Defeating it sets
 * SAVE_FLAG_POT_GOD_DEFEATED, which makes 20 pots holding checks appear in
 * Hyrule Field, and the boss kit sends Link back to the Pot House.
 * Everything is drawn from vanilla objects (Dead Hand's hand from object_dh
 * and object_tsubo), so no new asset is needed.
 */

#include "Custom_Pot_God.h"
#include <combo/rand.h>
#include <combo/math.h>
#include <combo/player.h>
#include <combo/effect.h>
#include <combo/sfx_source.h>
#include <combo/audio.h>
#include <assets/oot/objects/object_dh.h>
#include <assets/oot/objects/object_tsubo.h>

#define FLAGS (ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED)

#define POT_GOD_HEALTH          16
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

/* Intro */
#define POT_GOD_CALM_FRAMES     12
#define POT_GOD_INTRO_RISE      10
#define POT_GOD_INTRO_ARMS      45
#define POT_GOD_INTRO_ROAR      85
#define POT_GOD_INTRO_END       145

#define ANIM_DH_HAND            object_dh_Anim_0015B0
#define SKEL_DH_HAND            object_dh_Skel_000BD8

void PotGod_Init(Actor* thisx, PlayState* play);
void PotGod_Destroy(Actor* thisx, PlayState* play);
void PotGod_Update(Actor* thisx, PlayState* play);
void PotGod_Draw(Actor* thisx, PlayState* play);

static void PotGod_WaitIntro(PotGod* this, PlayState* play);
static void PotGod_Intro(PotGod* this, PlayState* play);
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

static int PotGod_IsFacingPlayer(PotGod* this, s16 maxAngle)
{
    s16 diff;

    diff = this->actor.yawTowardsPlayer - this->actor.shape.rot.y;
    return ABS(diff) < maxAngle;
}

static void PotGod_SpawnShards(PotGod* this, Vec3f* center, s32 count, f32 speed)
{
    BossKit_DebrisSpawn(&this->debris, center, count, speed, 0.1f);
}

static void PotGod_BreakPot(PotGod* this, PlayState* play, PotGodFlyingPot* pot)
{
    pot->active = FALSE;
    PotGod_SpawnShards(this, &pot->pos, 6, 4.0f);
    SfxSource_PlaySfxAtFixedWorldPos(play, &pot->pos, 20, NA_SE_EV_POT_BROKEN);
}

/* The middle of the pot's mouth, where the arms come out */
static void PotGod_GetMouthPos(PotGod* this, Vec3f* dst)
{
    *dst = this->actor.world.pos;
    dst->y += POT_GOD_BODY_HEIGHT - 4.0f + this->bodyRise + this->hopHeight;
}

void PotGod_Init(Actor* thisx, PlayState* play)
{
    PotGod* this = (PotGod*)thisx;

    this->tsuboSlot = BossKit_GetObjectSlot(play, OBJECT_TSUBO);
    if (this->tsuboSlot < 0)
    {
        Actor_Kill(&this->actor);
        return;
    }

    this->inArena = (this->actor.params & BOSS_KIT_PARAMS_ARENA) != 0;
    Actor_ProcessInitChain(&this->actor, sInitChain);
    Actor_SetScale(&this->actor, 0.01f);
    this->actor.colChkInfo.damageTable = &sDamageTable;
    this->actor.colChkInfo.mass = MASS_HEAVY;
    this->actor.colChkInfo.health = POT_GOD_HEALTH;
    ActorShape_Init(&this->actor.shape, 0.0f, NULL, 0.0f);

    /* Dead Hand's hand: our own object, already in segment 6 */
    SkelAnime_InitFlex(play, &this->armSkelAnime, (void*)SKEL_DH_HAND, (void*)ANIM_DH_HAND, this->armJointTable, this->armMorphTable, 4);

    Collider_InitCylinder(play, &this->collider);
    Collider_SetCylinder(play, &this->collider, &this->actor, &sCylinderInit);
    Collider_InitJntSph(play, &this->colliderHands);
    Collider_SetJntSph(play, &this->colliderHands, &this->actor, &sHandsInit, this->colliderHandsElements);
    Collider_InitJntSph(play, &this->colliderPots);
    Collider_SetJntSph(play, &this->colliderPots, &this->actor, &sPotsInit, this->colliderPotsElements);

    this->cam.subCamId = CAM_ID_MAIN;
    this->floorY = this->actor.world.pos.y;
    this->bodyRise = -(POT_GOD_BODY_HEIGHT + 20.0f);
    this->actionFunc = PotGod_WaitIntro;
}

void PotGod_Destroy(Actor* thisx, PlayState* play)
{
    PotGod* this = (PotGod*)thisx;

    Collider_DestroyCylinder(play, &this->collider);
    Collider_DestroyJntSph(play, &this->colliderHands);
    Collider_DestroyJntSph(play, &this->colliderPots);
}

/* Wait for Link to be free (after the fade into the arena) and for our objects */
static void PotGod_WaitIntro(PotGod* this, PlayState* play)
{
    Player* player = GET_PLAYER(play);

    if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)
    {
        if (!this->grounded)
            this->floorY = this->actor.floorHeight;
        this->grounded = TRUE;
    }

    if (BossKit_PlayerIsCalm(play))
        this->calmFrames++;
    else
        this->calmFrames = 0;

    if (this->calmFrames < POT_GOD_CALM_FRAMES || !this->grounded)
        return;
    if (!BossKit_ObjectReady(play, this->actor.objectSlot) || !BossKit_ObjectReady(play, this->tsuboSlot))
        return;

    this->actor.shape.rot.y = this->actor.world.rot.y = this->actor.yawTowardsPlayer;
    this->camYaw = Math_Vec3f_Yaw(&this->actor.world.pos, &player->actor.world.pos) + 0x1800;
    BossKit_CutsceneStart(play, &this->actor, &this->cam);
    this->prevBgm = Audio_GetActiveSeqId(SEQ_PLAYER_BGM_MAIN);
    BossKit_StopBgm();
    this->csTimer = 0;
    this->actionFunc = PotGod_Intro;
}

static void PotGod_EndIntro(PotGod* this, PlayState* play)
{
    BossKit_CutsceneEnd(play, &this->actor, &this->cam);
    BossKit_PlayBgm(BossKit_FightBgm(play, NA_BGM_MINI_BOSS));
    this->actor.flags |= ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_HOSTILE;
    this->summonTimer = 60;
    PotGod_SetupHop(this);
}

/* The ground splits, the pot bursts out bleeding, the arms tear out of it, roar */
static void PotGod_Intro(PotGod* this, PlayState* play)
{
    s16 t;
    Vec3f center;
    Vec3f mouth;

    t = this->csTimer++;

    center = this->actor.world.pos;
    center.y += 10.0f;

    if (t < POT_GOD_INTRO_RISE)
    {
        /* Rumbling, blood welling up from the ground */
        if (t == 0)
        {
            Actor_PlaySfx(&this->actor, NA_SE_EV_EARTHQUAKE);
            BossKit_Quake(play, 3, 30);
        }
        BossKit_SpawnBlood(play, &center, 30.0f, 70);
        BossKit_CameraOrbit(play, &this->cam, &this->actor.world.pos, this->camYaw, 260.0f, 120.0f, 40.0f, 30.0f);
    }
    else if (t < POT_GOD_INTRO_ARMS)
    {
        /* The pot bursts out of the ground */
        if (t == POT_GOD_INTRO_RISE)
        {
            this->drawBody = TRUE;
            Actor_PlaySfx(&this->actor, NA_SE_EN_DEADHAND_HIDE);
            BossKit_Quake(play, 6, 40);
        }
        Math_ApproachF(&this->bodyRise, 0.0f, 0.25f, 8.0f);
        if ((t & 1) == 0)
            BossKit_SpawnBlood(play, &center, 50.0f, 90);
        if ((t % 8) == 2)
        {
            PotGod_SpawnShards(this, &center, 4, 6.0f);
            SfxSource_PlaySfxAtFixedWorldPos(play, &center, 20, NA_SE_EV_POT_BROKEN);
        }
        BossKit_CameraOrbit(play, &this->cam, &this->actor.world.pos, this->camYaw, 220.0f, 90.0f, 70.0f, 20.0f);
    }
    else if (t < POT_GOD_INTRO_ROAR)
    {
        /* One by one, the arms tear out of its mouth */
        this->bodyRise = 0.0f;
        for (s32 i = 0; i < POT_GOD_ARMS; ++i)
        {
            if (t == POT_GOD_INTRO_ARMS + i * 8)
            {
                PotGod_GetMouthPos(this, &mouth);
                Actor_PlaySfx(&this->actor, NA_SE_EN_DEADHAND_HAND_AT);
                for (s32 j = 0; j < 4; ++j)
                    BossKit_SpawnBlood(play, &mouth, 20.0f, 80);
                BossKit_Quake(play, 3, 8);
            }
            if (t >= POT_GOD_INTRO_ARMS + i * 8)
                Math_ApproachF(&this->armGrow[i], 1.0f, 0.3f, 0.12f);
        }
        BossKit_CameraOrbit(play, &this->cam, &this->actor.world.pos, this->camYaw, 210.0f, 110.0f, 90.0f, 20.0f);
    }
    else if (t < POT_GOD_INTRO_END)
    {
        /* Roar */
        if (t == POT_GOD_INTRO_ROAR)
        {
            Actor_PlaySfx(&this->actor, NA_SE_EN_DEADHAND_LAUGH);
            BossKit_Quake(play, 5, 30);
            BossKit_ShowTitle(play, "POT GOD", "Keeper of Every Broken Pot");
        }
        for (s32 i = 0; i < POT_GOD_ARMS; ++i)
            this->armGrow[i] = 1.0f;
        BossKit_CameraOrbit(play, &this->cam, &this->actor.world.pos, this->camYaw, 200.0f, 90.0f, 90.0f, 20.0f);
    }
    else if (play->msgCtx.msgMode == MSGMODE_NONE)
    {
        PotGod_EndIntro(this, play);
        return;
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
            BossKit_Quake(play, 2, 6);
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
            BossKit_Quake(play, 4, 10);
            for (s32 i = 0; i < POT_GOD_SLAM_ARMS; ++i)
                BossKit_SpawnBlood(play, &this->handPos[i], 10.0f, 70);
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
            BossKit_SpawnBlood(play, &mouth, 30.0f, 80);
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
            BossKit_SpawnBlood(play, &mouth, 40.0f, 80);
        if (this->timer == 30)
            Actor_PlaySfx(&this->actor, NA_SE_EN_DEADHAND_HAND_DEAD);
        return;
    }

    if (this->timer == 60)
    {
        /* Shatter into a hail of pottery */
        PotGod_SpawnShards(this, &mouth, 12, 8.0f);
        PotGod_SpawnShards(this, &center, 12, 6.0f);
        SfxSource_PlaySfxAtFixedWorldPos(play, &mouth, 40, NA_SE_EV_POT_BROKEN);
        SfxSource_PlaySfxAtFixedWorldPos(play, &center, 40, NA_SE_EV_POT_BROKEN);
        BossKit_Quake(play, 5, 20);
        this->drawBody = FALSE;
        Item_DropCollectible(play, &this->actor.world.pos, ITEM00_RECOVERY_HEART);
        Item_DropCollectible(play, &this->actor.world.pos, ITEM00_RECOVERY_HEART);
        if (this->inArena || this->prevBgm == NA_BGM_DISABLED || (this->prevBgm & 0xff) == 0 || (this->prevBgm & 0xff) == 0xff)
            BossKit_StopBgm();
        else
            BossKit_PlayBgm(this->prevBgm);
        Audio_PlaySfx_AtPosWithReverb(&gSfxDefaultPos, NA_SE_SY_CORRECT_CHIME, 0);
        return;
    }

    if (this->timer == 75)
    {
        BossKit_ShowText(play, "The Pot God shattered..." TEXT_NL "Its pots are scattered all over" TEXT_NL TEXT_COLOR_RED "Hyrule Field" TEXT_CZ "!" TEXT_END);
        if (!this->inArena)
            Actor_Kill(&this->actor);
        return;
    }

    /* Arena: once the text is read (and the hearts picked up), back to the Pot House */
    if (this->timer >= 100 && play->msgCtx.msgMode == MSGMODE_NONE)
    {
        BossKit_FinishFight(play);
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
    if (this->actionFunc == PotGod_Death || this->actionFunc == PotGod_Intro || this->actionFunc == PotGod_WaitIntro)
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

    this->shardAngle += 0x300;
    Actor_MoveXZGravity(&this->actor);
    if (this->grounded)
        BossKit_KeepOnFloor(play, &this->actor, &this->actor.prevPos, this->floorY, 30.0f);
    BossKit_DebrisUpdate(&this->debris, this->floorY);
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

/* Dead Hand's arms, growing out of the mouth (our own object must be in segment 6) */
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
    Vec3f pos;
    s16 angle;

    /* The pot parts */
    BossKit_UseObject(play, this->tsuboSlot);
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
    BossKit_UseObject(play, this->actor.objectSlot);
    PotGod_DrawArms(this, play);
}

static void PotGod_DrawFlyingPots(PotGod* this, PlayState* play)
{
    Vec3f pos;

    if (!PotGod_PotsActive(this))
        return;

    BossKit_UseObject(play, this->tsuboSlot);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    for (s32 i = 0; i < POT_GOD_FLYING_POTS; ++i)
    {
        if (!this->pots[i].active)
            continue;
        pos = this->pots[i].pos;
        pos.y -= 12.0f;
        PotGod_DrawPotDList(play, (Gfx*)object_tsubo_DL_0017C0, &pos, this->pots[i].rotX, this->pots[i].rotY, 0, 0.12f);
    }
    BossKit_UseObject(play, this->actor.objectSlot);
}

void PotGod_Draw(Actor* thisx, PlayState* play)
{
    PotGod* this = (PotGod*)thisx;

    /* Never draw from an object that is not there */
    if (!BossKit_ObjectReady(play, this->tsuboSlot) || !BossKit_ObjectReady(play, this->actor.objectSlot))
        return;

    if (this->drawBody)
        PotGod_DrawBody(this, play);
    PotGod_DrawFlyingPots(this, play);
    BossKit_DebrisDraw(play, &this->debris, this->tsuboSlot, (Gfx*)object_tsubo_DL_001960);
    if (PotGod_IsFighting(this))
        BossKit_DrawHealthBar(play, this->actor.colChkInfo.health, POT_GOD_HEALTH);
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
