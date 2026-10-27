#ifndef CUSTOM_POT_GOD_H
#define CUSTOM_POT_GOD_H

#include <combo.h>

/*
 * Fork: the Pot God, a mini boss in the Market Pot House (child).
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
    SkelAnime               guardSkelAnime;     /* The guard, during the cutscene */
    Vec3s                   guardJointTable[17];
    Vec3s                   guardMorphTable[17];
    PotGodActionFunc        actionFunc;
    s16                     timer;
    s16                     csTimer;
    s16                     subCamId;
    s16                     summonTimer;
    s16                     shardAngle;
    s16                     guardYaw;
    s16                     slamTilt;           /* Extra forward tilt of the two front arms */
    s16                     wobble;             /* Body shake when hurt */
    u16                     prevBgm;
    u8                      potsSeen;
    u8                      calmFrames;
    u8                      actionState;
    u8                      drawBody;
    u8                      drawGuard;
    u8                      isHurt;
    u8                      drawArm;            /* Arm being drawn, for the limb callbacks */
    u8                      slamActive;
    s8                      tsuboSlot;
    s8                      guardSlot;
    f32                     guardSink;
    f32                     bodyRise;           /* 0 when fully out of the floor */
    f32                     hopHeight;
    f32                     armGrow[POT_GOD_ARMS];
    Vec3f                   guardPos;
    Vec3f                   handPos[POT_GOD_ARMS];
    Vec3f                   subCamAt;
    Vec3f                   subCamEye;
    PotGodFlyingPot         pots[POT_GOD_FLYING_POTS];
    ColliderCylinder        collider;
    ColliderJntSph          colliderHands;
    ColliderJntSphElement   colliderHandsElements[POT_GOD_SLAM_ARMS];
    ColliderJntSph          colliderPots;
    ColliderJntSphElement   colliderPotsElements[POT_GOD_FLYING_POTS];
}
PotGod;

#endif
