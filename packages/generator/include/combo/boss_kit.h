#ifndef COMBO_BOSS_KIT_H
#define COMBO_BOSS_KIT_H

/*
 * Fork: the boss kit (OoT). Shared pieces for custom boss fights, so a new
 * fight only has to write its own moves. See docs/custom/boss-kit.md.
 *
 * - Fights and arenas: a fight is a boss actor plus an arena (a vanilla scene
 *   emptied of its own boss) and where to return after winning. Calling
 *   BossKit_StartFight warps Link there; the kit clears the arena, places Link,
 *   spawns the boss, keeps the arena safe for a child, and sends Link back on
 *   BossKit_FinishFight. Dying retries the fight.
 * - Cutscenes: a sub camera that eases between shots, with Link frozen.
 * - Objects: draw from objects that are not the actor's own.
 * - Effects: blood, quakes, title cards, and debris drawn by the boss itself.
 * - A health bar (Custom > Ocarina of Time > "Boss Kit: Health Bars").
 */

#if defined(GAME_OOT)

#include <combo.h>

/* Arenas */
typedef enum BossKitArenaId
{
    BOSS_KIT_ARENA_HELLFIRE,        /* Volvagia's lair: lava all around, no heat, no Volvagia */
    BOSS_KIT_ARENA_MAX,
}
BossKitArenaId;

/* Fights: add one per new boss */
typedef enum BossKitFightId
{
    BOSS_KIT_FIGHT_POT_GOD,
    BOSS_KIT_FIGHT_MAX,
}
BossKitFightId;

#define BOSS_KIT_FIGHT_NONE     (-1)

/* Fight params given to the boss actor when the kit spawns it in its arena */
#define BOSS_KIT_PARAMS_ARENA   0x8000

/* Start a fight: fade out and warp Link to the fight's arena */
void    BossKit_StartFight(PlayState* play, s32 fightId);
/* The boss is dead: fade out and send Link back to where the fight started */
void    BossKit_FinishFight(PlayState* play);
/* The fight being held in this scene, or BOSS_KIT_FIGHT_NONE */
s32     BossKit_CurrentFight(PlayState* play);
/* The music for this fight: the arena's in an arena, else defaultBgm */
u16     BossKit_FightBgm(PlayState* play, u16 defaultBgm);

/* Hooks */
void    BossKit_OnSceneInit(PlayState* play);
int     BossKit_IsActorBlocked(PlayState* play, s16 actorId);

/* Cutscenes */
typedef struct BossKitCamera
{
    s16     subCamId;
    Vec3f   at;
    Vec3f   eye;
}
BossKitCamera;

void    BossKit_CutsceneStart(PlayState* play, Actor* actor, BossKitCamera* cam);
void    BossKit_CutsceneEnd(PlayState* play, Actor* actor, BossKitCamera* cam);
/* Ease the camera toward a shot of `target`: `dist` away along `yaw`, eye and look-at heights above it */
void    BossKit_CameraOrbit(PlayState* play, BossKitCamera* cam, Vec3f* target, s16 yaw, f32 dist, f32 eyeHeight, f32 atHeight, f32 speed);
/* Is Link free to be put in a cutscene (on the ground, no text, no item held up) */
int     BossKit_PlayerIsCalm(PlayState* play);

/* Text, music, shaking */
void    BossKit_ShowText(PlayState* play, const char* text);
void    BossKit_ShowTitle(PlayState* play, const char* name, const char* subtitle);
void    BossKit_Quake(PlayState* play, s16 y, s32 duration);
void    BossKit_PlayBgm(u16 seqId);
void    BossKit_StopBgm(void);
void    BossKit_PlayChildVoice(PlayState* play, u16 adultSfx);

/* Objects */
s32     BossKit_GetObjectSlot(PlayState* play, s16 objectId);
int     BossKit_ObjectReady(PlayState* play, s32 slot);
void*   BossKit_ObjectSegment(s32 slot);
/* Point segment 6 at an object, on the CPU (skeletons, animations) and the RSP (display lists) */
void    BossKit_UseObject(PlayState* play, s32 slot);
/* Same, CPU only (for SkelAnime updates) */
void    BossKit_UseObjectCpu(s32 slot);

/* Effects */
void    BossKit_SpawnBlood(PlayState* play, Vec3f* center, f32 spread, s16 scale);

/* Debris drawn by the boss from one of its objects (no effect overlay needed) */
#define BOSS_KIT_DEBRIS_MAX     24

typedef struct BossKitDebrisPiece
{
    Vec3f   pos;
    Vec3f   vel;
    s16     rotX;
    s16     rotY;
    s16     life;
    f32     scale;
}
BossKitDebrisPiece;

typedef struct BossKitDebris
{
    BossKitDebrisPiece pieces[BOSS_KIT_DEBRIS_MAX];
}
BossKitDebris;

void    BossKit_DebrisSpawn(BossKitDebris* debris, Vec3f* center, s32 count, f32 speed, f32 scale);
void    BossKit_DebrisUpdate(BossKitDebris* debris, f32 floorY);
void    BossKit_DebrisDraw(PlayState* play, BossKitDebris* debris, s32 objectSlot, Gfx* dList);

/* Keep a boss out of pits and lava: undo a move that would take it off the arena floor */
void    BossKit_KeepOnFloor(PlayState* play, Actor* actor, Vec3f* prevPos, f32 floorY, f32 maxDrop);

/* Health bar at the bottom of the screen, when enabled in the settings */
void    BossKit_DrawHealthBar(PlayState* play, s32 health, s32 maxHealth);

#endif

#endif
