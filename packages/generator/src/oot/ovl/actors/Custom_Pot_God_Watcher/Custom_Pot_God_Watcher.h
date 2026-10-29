#ifndef CUSTOM_POT_GOD_WATCHER_H
#define CUSTOM_POT_GOD_WATCHER_H

#include <combo.h>
#include <combo/boss_kit.h>

/*
 * Fork: the Pot God's awakening, in the child Market Pot House.
 * See docs/custom/pot-god.md.
 */

struct PotGodWatcher;

typedef void (*PotGodWatcherActionFunc)(struct PotGodWatcher*, PlayState*);

typedef struct PotGodWatcher
{
    Actor                   actor;
    SkelAnime               guardSkelAnime;
    Vec3s                   guardJointTable[17];
    Vec3s                   guardMorphTable[17];
    PotGodWatcherActionFunc actionFunc;
    BossKitCamera           cam;
    Vec3f                   guardPos;
    Vec3f                   grabPos;            /* Where Link is dragged to */
    f32                     guardSink;
    f32                     linkSink;
    s16                     csTimer;
    s16                     guardYaw;
    s16                     camYaw;
    s8                      guardSlot;
    u8                      potsSeen;
    u8                      calmFrames;
    u8                      drawGuard;
    u8                      grabbing;
    u8                      arena;              /* Drag Link to the arena, instead of fighting here */
}
PotGodWatcher;

#endif
