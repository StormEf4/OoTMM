/*
 * Fork: the Pot God's awakening (Custom > Ocarina of Time > "Pot God").
 *
 * Spawned (invisible) in the child Market Pot House by PotGod_SpawnForScene.
 * It watches the pots in the room; once every pot is broken, the guard
 * screams "OH GOD!", collapses and sinks into the floor. Then either:
 * - Hellfire Arena (default): his hand shoots out, grabs Link and drags him
 *   down with him, and the boss kit takes Link to the hellfire arena, where
 *   the Pot God (Custom_Pot_God) bursts out of the ground.
 * - Pot House: the Pot God bursts out of the floor right here.
 * The guard is drawn from his own object (object_sd, in the room), so this
 * actor needs no other object.
 */

#include "Custom_Pot_God_Watcher.h"
#include <combo/config.h>
#include <combo/math.h>
#include <combo/player.h>
#include <combo/sfx_source.h>
#include <assets/oot/objects/object_sd.h>

#define FLAGS (ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED)

#define POT_GOD_MIN_POTS        8

/* OoT voice sfx (not in sfx_id.h's OoT section). Link's are the adult ones; BossKit_PlayChildVoice picks the child's. */
#define SFX_TALON_SURPRISE      0x6851
#define SFX_TALON_CRY           0x6852
#define SFX_INGO_CRY            0x6854
#define SFX_LINK_DAMAGE_S       0x6805
#define SFX_LINK_TAKEN_AWAY     0x680C

/* Link's "grabbed by a Wallmaster" cutscene action */
#define PLAYER_CSACTION_GRABBED 37

void PotGodWatcher_Init(Actor* thisx, PlayState* play);
void PotGodWatcher_Destroy(Actor* thisx, PlayState* play);
void PotGodWatcher_Update(Actor* thisx, PlayState* play);
void PotGodWatcher_Draw(Actor* thisx, PlayState* play);

static void PotGodWatcher_Watch(PotGodWatcher* this, PlayState* play);
static void PotGodWatcher_WaitCalm(PotGodWatcher* this, PlayState* play);
static void PotGodWatcher_Cutscene(PotGodWatcher* this, PlayState* play);

void PotGodWatcher_Init(Actor* thisx, PlayState* play)
{
    PotGodWatcher* this = (PotGodWatcher*)thisx;

    this->guardSlot = BossKit_GetObjectSlot(play, OBJECT_SD);
    this->arena = !Config_Flag(CFG_CUSTOM_POT_GOD_ARENA_ROOM);
    this->cam.subCamId = CAM_ID_MAIN;
    this->actor.draw = NULL;
    this->actionFunc = PotGodWatcher_Watch;
}

void PotGodWatcher_Destroy(Actor* thisx, PlayState* play)
{
}

/* Count the pots left in the room */
static s32 PotGodWatcher_CountPots(PlayState* play)
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

static void PotGodWatcher_Watch(PotGodWatcher* this, PlayState* play)
{
    s32 count;

    count = PotGodWatcher_CountPots(play);
    if (count > this->potsSeen)
        this->potsSeen = (count > 0xff) ? 0xff : count;

    if (this->potsSeen >= POT_GOD_MIN_POTS && count == 0)
    {
        this->calmFrames = 0;
        this->actionFunc = PotGodWatcher_WaitCalm;
    }
}

static Actor* PotGodWatcher_FindGuard(PlayState* play)
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

static void PotGodWatcher_StartCutscene(PotGodWatcher* this, PlayState* play)
{
    Player* player = GET_PLAYER(play);
    Actor* guard;

    guard = PotGodWatcher_FindGuard(play);
    if (guard != NULL && BossKit_ObjectReady(play, this->guardSlot))
    {
        BossKit_UseObjectCpu(this->guardSlot);
        SkelAnime_Init(play, &this->guardSkelAnime, (void*)gEnHeishiSkel, (void*)gEnHeishiIdleAnim, this->guardJointTable, this->guardMorphTable, 17);
        BossKit_UseObjectCpu(this->actor.objectSlot);
        this->guardPos = guard->world.pos;
        this->drawGuard = TRUE;
        Actor_Kill(guard);
    }
    else
    {
        /* No guard: it all happens in front of Link */
        this->guardPos.x = player->actor.world.pos.x + Math_SinS(player->actor.shape.rot.y) * 120.0f;
        this->guardPos.y = player->actor.world.pos.y;
        this->guardPos.z = player->actor.world.pos.z + Math_CosS(player->actor.shape.rot.y) * 120.0f;
        this->drawGuard = FALSE;
    }
    this->guardYaw = Math_Vec3f_Yaw(&this->guardPos, &player->actor.world.pos);
    this->camYaw = this->guardYaw;
    this->actor.world.pos = this->guardPos;

    BossKit_CutsceneStart(play, &this->actor, &this->cam);
    BossKit_StopBgm();

    this->actor.draw = PotGodWatcher_Draw;
    this->csTimer = 0;
    this->actionFunc = PotGodWatcher_Cutscene;
}

static void PotGodWatcher_WaitCalm(PotGodWatcher* this, PlayState* play)
{
    /* Wait for any item from the last pot and its text to be done */
    if (BossKit_PlayerIsCalm(play))
        this->calmFrames++;
    else
        this->calmFrames = 0;

    if (this->calmFrames >= 20)
        PotGodWatcher_StartCutscene(this, play);
}

/* The guard's hand shoots out of the floor and grabs Link */
static void PotGodWatcher_Grab(PotGodWatcher* this, PlayState* play)
{
    Player* player = GET_PLAYER(play);

    this->grabbing = TRUE;
    this->grabPos.x = this->guardPos.x + Math_SinS(this->guardYaw) * 22.0f;
    this->grabPos.y = this->guardPos.y;
    this->grabPos.z = this->guardPos.z + Math_CosS(this->guardYaw) * 22.0f;
    Player_SetCsActionWithHaltedActors(play, &this->actor, PLAYER_CSACTION_GRABBED);
    player->actor.shape.rot.y = player->actor.world.rot.y = this->guardYaw + 0x8000;
    BossKit_PlayChildVoice(play, SFX_LINK_DAMAGE_S);
    Actor_PlaySfx(&this->actor, NA_SE_EN_FALL_CATCH);
    BossKit_Quake(play, 3, 15);
}

/* Drag Link to the guard and down into the floor with him */
static void PotGodWatcher_UpdateGrab(PotGodWatcher* this, PlayState* play)
{
    Player* player = GET_PLAYER(play);

    if (!this->grabbing)
        return;
    Math_ApproachF(&player->actor.world.pos.x, this->grabPos.x, 0.15f, 6.0f);
    Math_ApproachF(&player->actor.world.pos.z, this->grabPos.z, 0.15f, 6.0f);
    player->actor.speed = 0.0f;
    player->actor.shape.yOffset = -this->linkSink / player->actor.scale.y;
}

/* Shout, collapse and sink, then drag Link down (or let the Pot God out, in the Pot House) */
static void PotGodWatcher_Cutscene(PotGodWatcher* this, PlayState* play)
{
    Player* player = GET_PLAYER(play);
    Vec3f center;
    Vec3f mid;
    s16 t;

    t = this->csTimer++;

    center = this->guardPos;
    center.y += 30.0f - this->guardSink;

    if (t == 0)
    {
        BossKit_ShowText(play, TEXT_COLOR_RED "OH GOD!" TEXT_CZ TEXT_FADE("\x28") TEXT_END);
        Actor_PlaySfx(&this->actor, SFX_TALON_SURPRISE);
        BossKit_Quake(play, 2, 20);
    }

    if (t < 60)
    {
        /* Shout: trembling in horror */
        BossKit_CameraOrbit(play, &this->cam, &this->guardPos, this->guardYaw, 110.0f, 50.0f, 45.0f, 20.0f);
    }
    else if (!this->arena)
    {
        /* Pot House: he sinks, and the Pot God bursts out where he went down */
        if (t == 60)
        {
            if (this->drawGuard)
            {
                BossKit_UseObjectCpu(this->guardSlot);
                Animation_Change(&this->guardSkelAnime, (void*)gEnHeishiDyingGuardDieAnim, 1.5f, 0.0f, Animation_GetLastFrame((void*)gEnHeishiDyingGuardDieAnim), ANIMMODE_ONCE, -6.0f);
                BossKit_UseObjectCpu(this->actor.objectSlot);
            }
            Actor_PlaySfx(&this->actor, SFX_INGO_CRY);
            BossKit_Quake(play, 4, 30);
        }
        if (t == 90)
            Actor_PlaySfx(&this->actor, SFX_TALON_CRY);
        Math_ApproachF(&this->guardSink, (t < 100) ? 12.0f : 60.0f, 0.1f, 1.5f);
        if ((t & 3) == 0)
            BossKit_SpawnBlood(play, &center, 30.0f, 60);
        BossKit_CameraOrbit(play, &this->cam, &this->guardPos, this->guardYaw, 90.0f, 25.0f, 20.0f, 20.0f);

        if (t == 120)
        {
            Actor_Spawn(&play->actorCtx, play, ACTOR_CUSTOM_POT_GOD, this->guardPos.x, this->guardPos.y, this->guardPos.z, 0, this->guardYaw, 0, 0);
            BossKit_CutsceneEnd(play, &this->actor, &this->cam);
        }
        if (t >= 128)
        {
            Actor_Kill(&this->actor);
            return;
        }
    }
    else
    {
        /* Hellfire Arena: he grabs Link and takes him down with him */
        if (t == 60)
        {
            if (this->drawGuard)
            {
                BossKit_UseObjectCpu(this->guardSlot);
                Animation_Change(&this->guardSkelAnime, (void*)gEnHeishiDyingGuardDieAnim, 1.5f, 0.0f, Animation_GetLastFrame((void*)gEnHeishiDyingGuardDieAnim), ANIMMODE_ONCE, -6.0f);
                BossKit_UseObjectCpu(this->actor.objectSlot);
            }
            Actor_PlaySfx(&this->actor, SFX_INGO_CRY);
            BossKit_Quake(play, 4, 30);
        }
        if (t == 80)
        {
            this->camYaw = this->guardYaw + 0x4000;
            PotGodWatcher_Grab(this, play);
        }
        if (t == 100)
        {
            BossKit_PlayChildVoice(play, SFX_LINK_TAKEN_AWAY);
            Actor_PlaySfx(&this->actor, SFX_TALON_CRY);
            BossKit_Quake(play, 5, 60);
        }
        if (t == 110)
            Actor_PlaySfx(&this->actor, NA_SE_EN_FALL_UP);

        /* Both of them go down: him first, then Link */
        Math_ApproachF(&this->guardSink, (t < 100) ? 12.0f : 90.0f, 0.1f, (t < 100) ? 1.5f : 1.2f);
        if (t >= 100)
            Math_ApproachF(&this->linkSink, 70.0f, 0.1f, 1.0f);
        if ((t & 3) == 0)
            BossKit_SpawnBlood(play, &center, 30.0f, 60);

        if (t < 80)
        {
            BossKit_CameraOrbit(play, &this->cam, &this->guardPos, this->guardYaw, 90.0f, 25.0f, 20.0f, 20.0f);
        }
        else
        {
            /* From the side: Link being dragged to him, then both going under */
            mid.x = (this->guardPos.x + player->actor.world.pos.x) * 0.5f;
            mid.y = this->guardPos.y;
            mid.z = (this->guardPos.z + player->actor.world.pos.z) * 0.5f;
            BossKit_CameraOrbit(play, &this->cam, &mid, this->camYaw, 140.0f, (t < 100) ? 45.0f : 25.0f, (t < 100) ? 20.0f : 0.0f, 10.0f);
        }

        /* Into the dark: the boss kit takes it from here */
        if (t == 140)
            BossKit_StartFight(play, BOSS_KIT_FIGHT_POT_GOD);
    }

    PotGodWatcher_UpdateGrab(this, play);

    if (this->drawGuard)
    {
        BossKit_UseObjectCpu(this->guardSlot);
        SkelAnime_Update(&this->guardSkelAnime);
        BossKit_UseObjectCpu(this->actor.objectSlot);
    }
}

void PotGodWatcher_Update(Actor* thisx, PlayState* play)
{
    PotGodWatcher* this = (PotGodWatcher*)thisx;

    this->actionFunc(this, play);
}

void PotGodWatcher_Draw(Actor* thisx, PlayState* play)
{
    PotGodWatcher* this = (PotGodWatcher*)thisx;
    s16 shakeX;
    s16 shakeZ;

    if (!this->drawGuard || this->guardSink >= 85.0f || !BossKit_ObjectReady(play, this->guardSlot))
        return;

    shakeX = (s16)Rand_CenteredFloat(0x600);
    shakeZ = (s16)Rand_CenteredFloat(0x600);

    BossKit_UseObject(play, this->guardSlot);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    Matrix_Push();
    Matrix_Translate(this->guardPos.x, this->guardPos.y - this->guardSink, this->guardPos.z, MTXMODE_NEW);
    Matrix_RotateZYX(shakeX, this->guardYaw, shakeZ, MTXMODE_APPLY);
    Matrix_Scale(0.01f, 0.01f, 0.01f, MTXMODE_APPLY);
    SkelAnime_DrawOpa(play, this->guardSkelAnime.skeleton, this->guardSkelAnime.jointTable, NULL, NULL, this);
    Matrix_Pop();
    BossKit_UseObject(play, this->actor.objectSlot);
}

static ActorProfile PotGodWatcher_InitVars =
{
    ACTOR_CUSTOM_POT_GOD_WATCHER,
    ACTORCAT_BOSS,
    FLAGS,
    OBJECT_GAMEPLAY_KEEP,
    sizeof(PotGodWatcher),
    (ActorFunc)PotGodWatcher_Init,
    (ActorFunc)PotGodWatcher_Destroy,
    (ActorFunc)PotGodWatcher_Update,
    (ActorFunc)PotGodWatcher_Draw,
};

OVL_INFO_ACTOR(ACTOR_CUSTOM_POT_GOD_WATCHER, PotGodWatcher_InitVars);
