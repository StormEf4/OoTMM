# Pot God

Fork feature. Controls on the Custom page, under Ocarina of Time:

- **Pot God** (`potGod`): the mini boss and its 20 Hyrule Field pots.
- **Pot God Fight** (`potGodArena`): where it is fought. "Hellfire Arena" (default): the guard drags Link down into Volvagia's lair, emptied out. "Pot House": right in the room.
- **Reverse Saria's Song (Pot Room Warp)** (`potGodSariaWarp`): a warp to the Market Pot House.
- **Boss Kit: Health Bars** (`bossKitHealthBar`): a health bar while fighting it.

The fight is built on the [boss kit](boss-kit.md).

Not tested in game yet: there are no ROMs in the cloud sessions, so everything below was only compiled.

## In game

1. As child, go into the Market Pot House (the guard's room full of pots by the Market entrance) and break every pot.
2. Once the last pot is gone and Link is standing still (any item from the pot has been collected), a short cutscene starts. The camera turns to the guard, who screams "OH GOD!", collapses in pain bleeding, and sinks into the floor.
   - **Hellfire Arena**: his hand shoots out and grabs Link. Link is dragged to him screaming and they both sink into the floor, and the screen goes black. Link lands in a hellfire arena (Volvagia's lair, with no Volvagia and no heat).
   - **Pot House**: the fight happens right there.
3. The ground rumbles and wells up with blood, and a giant pot bursts out of it, bleeding from its rim. Four of Dead Hand's pale grasping arms tear out of its mouth one by one, with pot shards circling it. It roars and its title shows ("POT GOD, Keeper of Every Broken Pot"). Volvagia's music starts in the arena, the mini boss music in the Pot House.
4. It has no legs, so it hops at Link, landing with a thud. Up close, its two front arms rear back and slam down in front of it. From further away its arms fling three pots at him, which break on hit. Swords, Deku Sticks, bombs and arrows hurt it (the pot and its two front hands both take hits). It has 16 health, like 8 Kokiri Sword hits. It doesn't follow Link into the lava pits.
5. When it dies it bleeds, then shatters into pottery, drops two hearts and says "Its pots are scattered all over Hyrule Field!". This sets a save flag. In the arena, once the text is closed, the screen fades and Link is back in the Pot House.
6. From then on, 20 pots stand in Hyrule Field, spread across the whole field (mostly near trees and the edges). Each pot holds a randomized check ("Hyrule Field Pot God Pot 01" to "20"), whatever the pot shuffle setting is.

If you leave the room before beating it, it comes back the next time and you break the pots again. Dying in the arena restarts the fight in the arena; saving and quitting there loads at Link's house, and the pots are back in the Pot House. As adult the room is unchanged.

**Reverse Saria's Song**: once you know Saria's Song, play it backwards in free play (C-Left, C-Right, C-Down, C-Left, C-Right, C-Down). It asks "Warp to the Market Pot House?" and warps you there on Yes. It works wherever warp songs work. These are the same notes as the Song of Healing; with this setting on, playing those notes in free play gives this warp prompt instead, but they still count as the Song of Healing for signs. When an actor or spot asks for a song (for example a hidden Song of Healing spot), the warp is off so the Song of Healing still works there.

## Logic

- `POT_GOD_DEFEATED` is an event in "Market Pot House Child" that needs `setting(potGod) && can_use_sword_or_sticks`.
- "Hyrule Field Pot God Pots" is a region reached from Hyrule Field with that event; its 20 locations need nothing else.
- The pots are always kept in the pool when `potGod` is on (`isPotGodPot` in `packages/logic/src/world/transform.ts`), and always removed when it's off.
- `POT_GOD_WARP` is a global exit (`setting(potGodSariaWarp) && can_play_saria`) to the Market Pot House, for both ages.

## How it works

- **Actors**: two, both in the `BOSS` category so they keep updating while the other actors are halted.
  - `Custom_Pot_God_Watcher` (actor id 0x006, unused in OoT, object `gameplay_keep`) lives in the Pot House. It counts `Obj_Tsubo` actors in the room; once it has seen at least 8 and they are all gone, it waits for 20 calm frames (`BossKit_PlayerIsCalm`) and starts the cutscene. The guard (En_Heishi) is removed and redrawn from his own `object_sd` with the vanilla "dying guard" animation. In arena mode it then puts Link in the "grabbed by a Wallmaster" cutscene action, pulls him to the guard, sinks him with `shape.yOffset` (only how he is drawn, so he can't fall out of the room) and calls `BossKit_StartFight`. In Pot House mode it spawns the Pot God where the guard went down.
  - `Custom_Pot_God` (actor id 0x003, `OBJECT_DH`) is the boss. In the arena the boss kit spawns it (params `BOSS_KIT_PARAMS_ARENA`). It also loads `object_tsubo`. The model is assembled at draw time: the room pot's display list scaled up 4x as the body, and Dead Hand's hand skeleton (the arms that grab from the floor, as En_Dha draws them) drawn four times from the pot's mouth, each with its own writhing. There's no new asset. Pot shards use the boss kit's debris, drawn by the boss itself.
- **Crash history**: the first two versions crashed the moment the Pot God appeared in the Pot House (the exact cause wasn't found without a ROM or a crash screen). What changed at that moment both times: the pot's display list and Dead Hand's skeleton drawn from extra object slots, and the vanilla shard effect (`Effect_Ss_Kakera`) with `object_tsubo`. Now the Pot House actor only uses objects the room already has, the shards no longer use the vanilla effect, and nothing is drawn from an object that isn't loaded. If it still crashes, a photo of the crash screen says where.
- **Spawning**: `PotGod_SpawnForScene` (`src/oot/play/pot_god.c`), called from `Play_AfterInit`. In the child Market Pot House it spawns the watcher until the save flag is set. In Hyrule Field, once the flag is set, it spawns the 20 pots (`Obj_Tsubo`, params `0x11f`, so they use `object_tsubo` and drop nothing of their own) at fixed x/z, high up, so they snap to the floor. Each pot is spawned with `g.xflagOverride` set to the xflag of its check (Hyrule Field, setup 0, room 0, actor ids 0xc0 to 0xd3), declared in `data/checks/oot/overworld/hyrule_field.xml`.
- **Cutscenes**: manual cutscenes with a sub camera (like En_Clear_Tag), through the boss kit.
- **Voice**: there is no recorded "OH GOD!" (the game can't play new voice clips without new audio data). The line is shown as a red text box with vanilla screams (Talon's surprise and cries, Ingo's cry). There is no kneeling animation either, so the guard collapses with the dying guard animation.
- **Song**: `CUSTOM_SONG_SARIA_REVERSE` in `src/oot/ocarina.c`. It is only on in free play, and it is checked after the Song of Healing so it wins when both match. After it is played, the song id is set to the Song of Healing's when you have it (for the signs).

## Tuning

- Pot positions: `kFieldPots` in `src/oot/play/pot_god.c` (x, z; the pots snap to the floor below y 2000). If one lands somewhere bad, move it there. Keep the count at 20, or change the checks file and `hyrule_field.yml` too.
- Fight: the `POT_GOD_*` constants at the top of `Custom_Pot_God.c` (health, scale, pot gravity and flight time, intro timings). Pots needed: `POT_GOD_MIN_POTS` in `Custom_Pot_God_Watcher.c`.
- Arena spots (Link and the Pot God): `kArenas` in `src/oot/boss_kit.c`. They were taken from Volvagia's intro (Link at x 380, y 100) without seeing the room, so they may need moving.
