#ifndef CUSTOM_POT_GOD_H
#define CUSTOM_POT_GOD_H

#include <combo.h>

/*
 * Fork: the Pot God, a mini boss in the Market Pot House (child).
 * See docs/custom/pot-god.md.
 */

#define POT_GOD_FLYING_POTS 3

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
    SkelAnime               skelAnime;          /* Dead Hand body */
    Vec3s                   jointTable[16];
    Vec3s                   morphTable[16];
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
    u16                     prevBgm;
    u8                      potsSeen;
    u8                      calmFrames;
    u8                      actionState;
    u8                      drawBody;
    u8                      drawGuard;
    u8                      isHurt;
    s8                      tsuboSlot;
    s8                      guardSlot;
    f32                     guardScale;
    f32                     guardSink;
    Vec3f                   guardPos;
    Vec3f                   headPos;
    Vec3f                   subCamAt;
    Vec3f                   subCamEye;
    PotGodFlyingPot         pots[POT_GOD_FLYING_POTS];
    ColliderCylinder        collider;
    ColliderJntSph          colliderBite;
    ColliderJntSphElement   colliderBiteElements[1];
    ColliderJntSph          colliderPots;
    ColliderJntSphElement   colliderPotsElements[POT_GOD_FLYING_POTS];
}
PotGod;

#endif
