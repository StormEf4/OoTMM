---
name: fork-pot-god
description: Pot God mini boss + reverse Saria warp + OoT boss kit (PRs #9-#11); custom actors, arenas, runtime xflag pots, crash history, emulator notes
metadata:
  type: project
  modified: 2026-11-03T00:00:00.000Z
---

Pot God history: PRs #9, #10 and #11 (merged 2026-10-27/28/30) all crashed the moment the Pot God started to rise. ROOT CAUSE (found 2026-11-03): OoTMM's CSMC patches (`src/oot/objects/tsubo.S`, `box.S`, `kusa.S`) replace texture loads in vanilla pot/chest/grass DLs with `gsSPDisplayList(0x0A000000)`/`0x0B`; the vanilla actor sets those segments via `csmc*PreDraw` before drawing. Drawing object_tsubo DL_0017C0 without them = RSP crash. The earlier suspects (ex-object slots, Dead Hand skeleton, Kakera) were wrong. Fix (PR #12): `BossKit_UseObject` calls `BossKit_PrepareTextures(play, objectId)` (empty DL default + plain look for patched objects); the kit tracks slot→object in `BossKit_GetObjectSlot`. Lesson: before drawing any vanilla DL from a custom actor, grep `src/*/objects/*.S` for patches to that object. Settings: potGod, potGodArena (arena/room), potGodWarpSong, bossKitHealthBar, all custom.oot. Warp song (2026-11-03, replaced Reverse Saria's Song which overlapped Song of Healing): A, C-Up, A, C-Up, C-Left, C-Right, CUSTOM_SONG_POT_WARP in src/oot/ocarina.c; songs match on buffer suffix (comboCheckSong), so check new songs against suffixes of every OoT + MM song.

Emulator lesson (2026-10-29): a crash after leaving Link's house in Kokiri was the emulator (Elijah's RetroArch ParaLLEl install broke). He now tests on Rosalie's Mupen GUI. Rule out the emulator before chasing a crash that has nothing to do with the change.

Boss kit facts:
- Arena fight state is a non-saved static (`sFightId`). It is cleared on any non-arena scene init. Dying → Continue reloads the arena (save.entranceIndex), so the fight retries.
- Arena scenes must skip the CUSTOM_RESPAWN_MODE_DUNGEON_ENTRANCE copy in Play_AfterInit. Otherwise save/reload would drop Link into the real boss room.
- Arena actors are blocked in canSpawnActor (src/oot/actors.c). Room actors spawn after Play_AfterInit (first Actor_UpdateAll), so they can't be killed there.
- Heat off: `play->roomCtx.curRoom.behaviorType2 = 0` (= environmentType).
- Link's sink is visual only, via `shape.yOffset`. OoT csAction 37 is the Wallmaster "grabbed" pose. Link's voice clips: child = adult + 0x20.

Config flags (`config[0x40]`, 512 bits) are at 0x1d6 after PR #11, so about 40 left.

Reusable techniques:
- Custom OoT actors go in unused actor ids (0x003, 0x006 used) as `src/oot/ovl/actors/<Name>/` with OVL_INFO_ACTOR. No TS wiring is needed; a new dir is picked up by the CMake glob on build.
- Objects not in the scene load into extra slots via the patched Object_GetSlot. Draw from them with BossKit_UseObject (CPU + RSP segment 6).
- Runtime-spawned check actors: `g.xflagOverride = TRUE; g.xflagId = Xflag_Lookup(&xf)`. Hyrule Field pots use room 0, actor ids 0xc0-0xd3.
- The NA_SE_VO_* ids are missing from the OoT part of sfx_id.h; define them locally. Verify them against /tmp/claude-0/oot include/tables/sfx/*.h when that clone exists.
- Custom songs: add to the CustomOcarinaSong enum and all parallel arrays in src/oot/ocarina.c.

See [[c-patch-verification]], [[fork-custom-items]].
