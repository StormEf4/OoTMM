#ifndef CUSTOM_POT_GOD_H
#define CUSTOM_POT_GOD_H

#include <combo.h>
#include <combo/boss_kit.h>

/*
 * Fork: the Pot God, a mini boss woken in the Market Pot House (child) and
 * fought in the boss kit's hellfire arena (or the Pot House itself).
 * See docs/custom/pot-god.md.
 */

#define POT_GOD_FLYING_POTS 3
#define POT_GOD_ARMS        4
#define POT_GOD_SLAM_ARMS   2

struct PotGod;

typedef void (*PotGodActionFunc)(struct PotGod*, PlayState*);

typedef struct PotGodFlyingPot
{
    Vec3f   pos;
    Vec3f   vel;
    s16     rotX;
    s16     rotY;
    s16     timer;
    u8      active;
}
PotGodFlyingPot;

typedef struct PotGod
{
    Actor                   actor;
    SkelAnime               armSkelAnime;       /* One Dead Hand arm, drawn once per arm */
    Vec3s                   armJointTable[4];
    Vec3s                   armMorphTable[4];
    PotGodActionFunc        actionFunc;
    BossKitCamera           cam;
    BossKitDebris           debris;             /* Pot shards */
    s16                     timer;
    s16                     csTimer;
    s16                     summonTimer;
    s16                     shardAngle;
    s16                     slamTilt;           /* Extra forward tilt of the two front arms */
    s16                     wobble;             /* Body shake when hurt */
    s16                     camYaw;
    u16                     prevBgm;
    u8                      calmFrames;
    u8                      drawBody;
    u8                      isHurt;
    u8                      drawArm;            /* Arm being drawn, for the limb callbacks */
    u8                      slamActive;
    u8                      inArena;
    u8                      grounded;
    s8                      tsuboSlot;
    f32                     floorY;             /* The arena floor, to stay out of pits */
    f32                     bodyRise;           /* 0 when fully out of the floor */
    f32                     hopHeight;
    f32                     armGrow[POT_GOD_ARMS];
    Vec3f                   handPos[POT_GOD_ARMS];
    PotGodFlyingPot         pots[POT_GOD_FLYING_POTS];
    ColliderCylinder        collider;
    ColliderJntSph          colliderHands;
    ColliderJntSphElement   colliderHandsElements[POT_GOD_SLAM_ARMS];
    ColliderJntSph          colliderPots;
    ColliderJntSphElement   colliderPotsElements[POT_GOD_FLYING_POTS];
}
PotGod;

#endif
