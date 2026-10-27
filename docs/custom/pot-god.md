# Pot God

Fork feature. Two toggles on the Custom page, under Ocarina of Time:

- **Pot God** (`potGod`): the mini boss and its 20 Hyrule Field pots.
- **Reverse Saria's Song (Pot Room Warp)** (`potGodSariaWarp`): a warp to the Market Pot House.

Not tested in game yet: there are no ROMs in the cloud sessions, so everything below was only compiled.

## In game

1. As child, go into the Market Pot House (the guard's room full of pots by the Market entrance) and break every pot.
2. Once the last pot is gone and Link is standing still (any item from the pot has been collected), a short cutscene starts. The camera turns to the guard, who screams "OH GOD!", collapses in pain bleeding, and twists into the floor while the Pot God bursts up out of it: Dead Hand's body, with a giant pot over its head, pot shards circling it and three small pots for a crown. It roars and its title shows ("POT GOD, Keeper of Every Broken Pot"). The mini boss music starts.
3. The fight is a simpler Dead Hand. It walks at Link and bites when close. From further away it stops and throws three pots at him, which break on hit. Swords, Deku Sticks, bombs and arrows hurt it (the bite and its body both take hits). It has 16 health, like 8 Kokiri Sword hits.
4. When it dies it bleeds, then shatters into pottery, drops two hearts and says "Its pots are scattered all over Hyrule Field!". This sets a save flag.
5. From then on, 20 pots stand in Hyrule Field, spread across the whole field (mostly near trees and the edges). Each pot holds a randomized check ("Hyrule Field Pot God Pot 01" to "20"), whatever the pot shuffle setting is.

If you leave the room before beating it, it comes back the next time and you break the pots again. As adult the room is unchanged.

**Reverse Saria's Song**: once you know Saria's Song, play it backwards in free play (C-Left, C-Right, C-Down, C-Left, C-Right, C-Down). It asks "Warp to the Market Pot House?" and warps you there on Yes. It works wherever warp songs work. These are the same notes as the Song of Healing; with this setting on, playing those notes in free play gives this warp prompt instead, but they still count as the Song of Healing for signs. When an actor or spot asks for a song (for example a hidden Song of Healing spot), the warp is off so the Song of Healing still works there.

## Logic

- `POT_GOD_DEFEATED` is an event in "Market Pot House Child" that needs `setting(potGod) && can_use_sword_or_sticks`.
- "Hyrule Field Pot God Pots" is a region reached from Hyrule Field with that event; its 20 locations need nothing else.
- The pots are always kept in the pool when `potGod` is on (`isPotGodPot` in `packages/logic/src/world/transform.ts`), and always removed when it's off.
- `POT_GOD_WARP` is a global exit (`setting(potGodSariaWarp) && can_play_saria`) to the Market Pot House, for both ages.

## How it works

- **Actor**: `Custom_Pot_God` uses actor id 0x003 (unused in OoT) and `OBJECT_DH`. It also loads `object_tsubo` and the guard's `object_sd`; objects missing from the scene are loaded into the fork's extra object slots, and every draw sets segment 6 to the object it uses (`gExObjectsAddr`). The model is assembled at draw time from the vanilla Dead Hand skeleton and pot display lists, so there's no new asset.
- **Spawning**: `PotGod_SpawnForScene` (`src/oot/play/pot_god.c`), called from `Play_AfterInit`. In the child Market Pot House it spawns the dormant Pot God until the save flag is set. In Hyrule Field, once the flag is set, it spawns the 20 pots (`Obj_Tsubo`, params `0x11f`, so they use `object_tsubo` and drop nothing of their own) at fixed x/z, high up, so they snap to the floor. Each pot is spawned with `g.xflagOverride` set to the xflag of its check (Hyrule Field, setup 0, room 0, actor ids 0xc0 to 0xd3), declared in `data/checks/oot/overworld/hyrule_field.xml`.
- **Trigger**: the Pot God counts `Obj_Tsubo` actors in the room. Once it has seen at least 8 and they are all gone, it waits for 20 calm frames (no text, no cutscene, Link on the ground and not holding or getting an item) and starts the cutscene.
- **Cutscene**: a manual cutscene with a sub camera (like En_Clear_Tag). It is in the `BOSS` category, so it keeps updating while the other actors are halted. The guard (En_Heishi) is removed and redrawn by the Pot God with the vanilla "dying guard" animation.
- **Voice**: there is no recorded "OH GOD!" (the game can't play new voice clips without new audio data). The line is shown as a red text box with vanilla screams (Talon's surprise and cries, Ingo's cry). There is no kneeling animation either, so the guard collapses with the dying guard animation.
- **Song**: `CUSTOM_SONG_SARIA_REVERSE` in `src/oot/ocarina.c`. It is only on in free play, and it is checked after the Song of Healing so it wins when both match. After it is played, the song id is set to the Song of Healing's when you have it (for the signs).

## Tuning

- Pot positions: `kFieldPots` in `src/oot/play/pot_god.c` (x, z; the pots snap to the floor below y 2000). If one lands somewhere bad, move it there. Keep the count at 20, or change the checks file and `hyrule_field.yml` too.
- Fight: the `POT_GOD_*` constants at the top of `Custom_Pot_God.c` (health, scale, number of pots needed, pot gravity and flight time).
