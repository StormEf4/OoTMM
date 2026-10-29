#include <combo.h>
#include <combo/boss_kit.h>
#include <combo/config.h>
#include <combo/entrance.h>
#include <combo/player.h>
#include <combo/math.h>
#include <combo/effect.h>
#include <combo/seqcmd.h>
#include <combo/audio.h>
#include <combo/text.h>
#include <combo/collision.h>
#include <combo/play.h>

/*
 * Fork: the boss kit. See include/combo/boss_kit.h and docs/custom/boss-kit.md.
 */

#define EX_OBJECT_SLOTS_NORMAL  19

extern void* gExObjectsAddr[];

typedef struct BossKitArena
{
    u16     sceneId;
    u16     entrance;
    u16     bgm;
    s16     playerYaw;
    s16     bossYaw;
    Vec3f   playerPos;
    Vec3f   bossPos;
    s16     blocked[4];     /* Actors kept out of the arena during a fight, 0 ends the list */
}
BossKitArena;

typedef struct BossKitFight
{
    s16     actorId;
    u16     params;
    u8      arenaId;
    u16     returnEntrance;
}
BossKitFight;

static const BossKitArena kArenas[BOSS_KIT_ARENA_MAX] = {
    [BOSS_KIT_ARENA_HELLFIRE] = {
        SCE_OOT_LAIR_VOLVAGIA,
        ENTR_OOT_BOSS_TEMPLE_FIRE,
        NA_BGM_FIRE_BOSS,
        /* Link on the east side of the arena (where Volvagia's intro puts him), looking north at the boss */
        -0x8000,
        0x0000,
        { 380.0f, 100.0f, 100.0f },
        { 380.0f, 110.0f, -180.0f },
        /* Volvagia brings the falling rocks, the platform, the blue warp and the heart container */
        { ACTOR_BOSS_FD, ACTOR_BOSS_FD2, ACTOR_DOOR_WARP1, ACTOR_ITEM_B_HEART },
    },
};

static const BossKitFight kFights[BOSS_KIT_FIGHT_MAX] = {
    [BOSS_KIT_FIGHT_POT_GOD] = {
        ACTOR_CUSTOM_POT_GOD,
        BOSS_KIT_PARAMS_ARENA,
        BOSS_KIT_ARENA_HELLFIRE,
        ENTR_OOT_HOUSE_POTS,
    },
};

/* The fight Link is headed to or in. Not saved: leaving the arena any other way ends it. */
static s8 sFightId = BOSS_KIT_FIGHT_NONE;

static const BossKitArena* BossKit_Arena(s32 fightId)
{
    return &kArenas[kFights[fightId].arenaId];
}

void BossKit_StartFight(PlayState* play, s32 fightId)
{
    if (fightId < 0 || fightId >= BOSS_KIT_FIGHT_MAX)
        return;
    sFightId = fightId;
    comboTransition(play, BossKit_Arena(fightId)->entrance);
}

void BossKit_FinishFight(PlayState* play)
{
    s32 fightId;

    fightId = BossKit_CurrentFight(play);
    sFightId = BOSS_KIT_FIGHT_NONE;
    if (fightId == BOSS_KIT_FIGHT_NONE)
        return;
    comboTransition(play, kFights[fightId].returnEntrance);
}

s32 BossKit_CurrentFight(PlayState* play)
{
    if (sFightId == BOSS_KIT_FIGHT_NONE)
        return BOSS_KIT_FIGHT_NONE;
    if (play->sceneId != BossKit_Arena(sFightId)->sceneId)
        return BOSS_KIT_FIGHT_NONE;
    return sFightId;
}

u16 BossKit_FightBgm(PlayState* play, u16 defaultBgm)
{
    s32 fightId;

    fightId = BossKit_CurrentFight(play);
    if (fightId == BOSS_KIT_FIGHT_NONE)
        return defaultBgm;
    return BossKit_Arena(fightId)->bgm;
}

/* Called from Play_AfterInit: set up the arena when Link arrives (or comes back after dying) */
void BossKit_OnSceneInit(PlayState* play)
{
    const BossKitFight* fight;
    const BossKitArena* arena;
    Player* player;

    if (sFightId == BOSS_KIT_FIGHT_NONE)
        return;

    fight = &kFights[sFightId];
    arena = BossKit_Arena(sFightId);
    if (play->sceneId != arena->sceneId)
    {
        /* Left the arena some other way (warp, save and quit...): the fight is off */
        sFightId = BOSS_KIT_FIGHT_NONE;
        return;
    }

    /* A child has no tunic for the heat */
    play->roomCtx.curRoom.behaviorType2 = 0;

    /* Into the arena, and come back here after falling in */
    player = GET_PLAYER(play);
    if (player != NULL)
    {
        player->actor.world.pos = arena->playerPos;
        player->actor.home.pos = arena->playerPos;
        player->actor.prevPos = arena->playerPos;
        player->actor.shape.rot.y = arena->playerYaw;
        player->actor.world.rot.y = arena->playerYaw;
        Play_SetupRespawnPoint(play, RESPAWN_MODE_DOWN, 0xdff);
    }

    Actor_Spawn(&play->actorCtx, play, fight->actorId, arena->bossPos.x, arena->bossPos.y, arena->bossPos.z, 0, arena->bossYaw, 0, fight->params);
}

int BossKit_IsActorBlocked(PlayState* play, s16 actorId)
{
    const BossKitArena* arena;

    if (BossKit_CurrentFight(play) == BOSS_KIT_FIGHT_NONE)
        return 0;

    arena = BossKit_Arena(sFightId);
    for (int i = 0; i < ARRAY_COUNT(arena->blocked); ++i)
    {
        if (arena->blocked[i] == 0)
            break;
        if (arena->blocked[i] == actorId)
            return 1;
    }
    return 0;
}

/* Cutscenes */
void BossKit_CutsceneStart(PlayState* play, Actor* actor, BossKitCamera* cam)
{
    Camera* mainCam;

    Cutscene_StartManual(play, &play->csCtx);
    cam->subCamId = Play_CreateSubCamera(play);
    Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STATUS_WAIT);
    Play_ChangeCameraStatus(play, cam->subCamId, CAM_STATUS_ACTIVE);
    Player_SetCsActionWithHaltedActors(play, actor, PLAYER_CSACTION_1);
    mainCam = Play_GetCamera(play, CAM_ID_MAIN);
    cam->eye = mainCam->eye;
    cam->at = mainCam->at;
}

void BossKit_CutsceneEnd(PlayState* play, Actor* actor, BossKitCamera* cam)
{
    if (cam->subCamId != CAM_ID_MAIN)
        Play_ReturnToMainCam(play, cam->subCamId, 0);
    cam->subCamId = CAM_ID_MAIN;
    Cutscene_StopManual(play, &play->csCtx);
    Player_SetCsActionWithHaltedActors(play, actor, PLAYER_CSACTION_END);
}

void BossKit_CameraOrbit(PlayState* play, BossKitCamera* cam, Vec3f* target, s16 yaw, f32 dist, f32 eyeHeight, f32 atHeight, f32 speed)
{
    Vec3f at;
    Vec3f eye;

    if (cam->subCamId == CAM_ID_MAIN)
        return;

    at.x = target->x;
    at.y = target->y + atHeight;
    at.z = target->z;
    eye.x = target->x + Math_SinS(yaw) * dist;
    eye.y = target->y + eyeHeight;
    eye.z = target->z + Math_CosS(yaw) * dist;

    Math_ApproachF(&cam->at.x, at.x, 0.2f, speed);
    Math_ApproachF(&cam->at.y, at.y, 0.2f, speed);
    Math_ApproachF(&cam->at.z, at.z, 0.2f, speed);
    Math_ApproachF(&cam->eye.x, eye.x, 0.2f, speed);
    Math_ApproachF(&cam->eye.y, eye.y, 0.2f, speed);
    Math_ApproachF(&cam->eye.z, eye.z, 0.2f, speed);
    Play_SetCameraAtEye(play, cam->subCamId, &cam->at, &cam->eye);
}

int BossKit_PlayerIsCalm(PlayState* play)
{
    Player* player = GET_PLAYER(play);

    if (play->msgCtx.msgMode != MSGMODE_NONE || player->csAction != PLAYER_CSACTION_NONE)
        return 0;
    if (!(player->actor.bgCheckFlags & BGCHECKFLAG_GROUND))
        return 0;
    if (player->stateFlags1 & (PLAYER_ACTOR_STATE_HOLD_ITEM | PLAYER_ACTOR_STATE_GET_ITEM | PLAYER_ACTOR_STATE_CUTSCENE_FROZEN))
        return 0;
    if (play->transitionTrigger != TRANS_TRIGGER_OFF || play->transitionMode != TRANS_MODE_OFF)
        return 0;
    return 1;
}

/* Text, music, shaking */
void BossKit_ShowText(PlayState* play, const char* text)
{
    char* b;

    Message_StartTextbox(play, 0x88c, NULL);
    b = play->msgCtx.font.msgBuf;
    comboTextAppendHeader(&b);
    comboTextAppendStr(&b, text);
}

void BossKit_ShowTitle(PlayState* play, const char* name, const char* subtitle)
{
    char* b;

    Message_StartTextbox(play, 0x88c, NULL);
    b = play->msgCtx.font.msgBuf;
    comboTextAppendHeader(&b);
    comboTextAppendStr(&b, TEXT_COLOR_RED);
    comboTextAppendStr(&b, name);
    comboTextAppendStr(&b, TEXT_CZ TEXT_NL);
    comboTextAppendStr(&b, subtitle);
    comboTextAppendStr(&b, TEXT_FADE("\x3c") TEXT_END);
}

void BossKit_Quake(PlayState* play, s16 y, s32 duration)
{
    Camera_RequestQuake(GET_ACTIVE_CAM(play), 0, y, duration);
}

void BossKit_PlayBgm(u16 seqId)
{
    SEQCMD_PLAY_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 0, 0, seqId & 0xff);
}

void BossKit_StopBgm(void)
{
    SEQCMD_STOP_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 1);
}

void BossKit_PlayChildVoice(PlayState* play, u16 adultSfx)
{
    /* Each of Link's voice clips has its child version 0x20 further */
    Player_PlaySfx(GET_PLAYER(play), (gSave.age == AGE_CHILD) ? (adultSfx + 0x20) : adultSfx);
}

/* Objects */
s32 BossKit_GetObjectSlot(PlayState* play, s16 objectId)
{
    return Object_GetSlot(&play->objectCtx, objectId);
}

void* BossKit_ObjectSegment(s32 slot)
{
    if (slot < 0)
        return NULL;
    if (slot < EX_OBJECT_SLOTS_NORMAL)
        return gPlay->objectCtx.status[slot].segment;
    return gExObjectsAddr[slot];
}

int BossKit_ObjectReady(PlayState* play, s32 slot)
{
    if (slot < 0 || !Object_IsLoaded(&play->objectCtx, slot))
        return 0;
    return BossKit_ObjectSegment(slot) != NULL;
}

void BossKit_UseObjectCpu(s32 slot)
{
    void* seg;

    seg = BossKit_ObjectSegment(slot);
    if (seg != NULL)
        gSegments[6] = OS_K0_TO_PHYSICAL(seg);
}

void BossKit_UseObject(PlayState* play, s32 slot)
{
    void* seg;

    seg = BossKit_ObjectSegment(slot);
    if (seg == NULL)
        return;
    gSegments[6] = OS_K0_TO_PHYSICAL(seg);

    OPEN_DISPS(play->state.gfxCtx);
    gSPSegment(POLY_OPA_DISP++, 0x06, seg);
    gSPSegment(POLY_XLU_DISP++, 0x06, seg);
    CLOSE_DISPS();
}

/* Effects */
void BossKit_SpawnBlood(PlayState* play, Vec3f* center, f32 spread, s16 scale)
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

void BossKit_DebrisSpawn(BossKitDebris* debris, Vec3f* center, s32 count, f32 speed, f32 scale)
{
    BossKitDebrisPiece* p;
    s16 angle;

    for (s32 i = 0; i < BOSS_KIT_DEBRIS_MAX && count > 0; ++i)
    {
        p = &debris->pieces[i];
        if (p->life > 0)
            continue;
        count--;

        angle = (s16)(Rand_ZeroOne() * 0x10000);
        p->pos.x = center->x + Math_SinS(angle) * 10.0f;
        p->pos.y = center->y + Rand_ZeroOne() * 20.0f;
        p->pos.z = center->z + Math_CosS(angle) * 10.0f;
        p->vel.x = Math_SinS(angle) * speed * (0.5f + Rand_ZeroOne());
        p->vel.y = (Rand_ZeroOne() * speed) + 3.0f;
        p->vel.z = Math_CosS(angle) * speed * (0.5f + Rand_ZeroOne());
        p->rotX = (s16)(Rand_ZeroOne() * 0x10000);
        p->rotY = (s16)(Rand_ZeroOne() * 0x10000);
        p->scale = scale * (0.6f + Rand_ZeroOne() * 0.8f);
        p->life = 40 + (s16)(Rand_ZeroOne() * 20.0f);
    }
}

void BossKit_DebrisUpdate(BossKitDebris* debris, f32 floorY)
{
    BossKitDebrisPiece* p;

    for (s32 i = 0; i < BOSS_KIT_DEBRIS_MAX; ++i)
    {
        p = &debris->pieces[i];
        if (p->life <= 0)
            continue;
        p->life--;
        p->vel.y -= 0.8f;
        p->pos.x += p->vel.x;
        p->pos.y += p->vel.y;
        p->pos.z += p->vel.z;
        if (p->pos.y < floorY)
        {
            /* Bounce once or twice, then settle */
            p->pos.y = floorY;
            p->vel.y *= -0.3f;
            p->vel.x *= 0.5f;
            p->vel.z *= 0.5f;
        }
        else
        {
            p->rotX += 0x900;
            p->rotY += 0x600;
        }
    }
}

void BossKit_DebrisDraw(PlayState* play, BossKitDebris* debris, s32 objectSlot, Gfx* dList)
{
    BossKitDebrisPiece* p;
    void* seg;
    u32 prevSegment;
    int any;

    any = 0;
    for (s32 i = 0; i < BOSS_KIT_DEBRIS_MAX; ++i)
    {
        if (debris->pieces[i].life > 0)
            any = 1;
    }
    seg = BossKit_ObjectSegment(objectSlot);
    if (!any || seg == NULL)
        return;

    prevSegment = gSegments[6];
    OPEN_DISPS(play->state.gfxCtx);
    gSPSegment(POLY_OPA_DISP++, 0x06, seg);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    for (s32 i = 0; i < BOSS_KIT_DEBRIS_MAX; ++i)
    {
        p = &debris->pieces[i];
        if (p->life <= 0)
            continue;
        Matrix_Push();
        Matrix_Translate(p->pos.x, p->pos.y, p->pos.z, MTXMODE_NEW);
        Matrix_RotateZYX(p->rotX, p->rotY, 0, MTXMODE_APPLY);
        Matrix_Scale(p->scale, p->scale, p->scale, MTXMODE_APPLY);
        gSPMatrix(POLY_OPA_DISP++, Matrix_Finalize(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_OPA_DISP++, dList);
        Matrix_Pop();
    }
    gSPSegment(POLY_OPA_DISP++, 0x06, prevSegment + 0x80000000);
    CLOSE_DISPS();
}

void BossKit_KeepOnFloor(PlayState* play, Actor* actor, Vec3f* prevPos, f32 floorY, f32 maxDrop)
{
    CollisionPoly* poly;
    s32 bgId;
    Vec3f probe;
    f32 y;

    probe = actor->world.pos;
    probe.y = floorY + 50.0f;
    y = BgCheck_EntityRaycastDown4(&play->colCtx, &poly, &bgId, actor, &probe);
    if (y < floorY - maxDrop)
    {
        actor->world.pos.x = prevPos->x;
        actor->world.pos.z = prevPos->z;
        actor->speed = 0.0f;
    }
}

void BossKit_DrawHealthBar(PlayState* play, s32 health, s32 maxHealth)
{
    s32 x0;
    s32 x1;
    s32 fill;

    if (!Config_Flag(CFG_CUSTOM_BOSS_KIT_HEALTH_BAR) || maxHealth <= 0)
        return;
    if (health < 0)
        health = 0;
    if (health > maxHealth)
        health = maxHealth;

    x0 = 80;
    x1 = 240;
    fill = ((x1 - x0 - 2) * health) / maxHealth;

    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_39Ptr(&OVERLAY_DISP);
    gDPSetRenderMode(OVERLAY_DISP++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode(OVERLAY_DISP++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 20, 0, 0, 200);
    gDPFillRectangle(OVERLAY_DISP++, x0, 210, x1, 218);
    if (fill > 0)
    {
        gDPPipeSync(OVERLAY_DISP++);
        gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 220, 30, 20, 255);
        gDPFillRectangle(OVERLAY_DISP++, x0 + 1, 211, x0 + 1 + fill, 217);
    }
    gDPPipeSync(OVERLAY_DISP++);
    CLOSE_DISPS();
}
