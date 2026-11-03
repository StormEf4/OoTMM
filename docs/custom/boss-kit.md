# Boss kit (OoT)

Fork feature: shared pieces for the fork's custom boss fights, so a new boss only has to write its own moves. The Pot God is the first boss built on it. Code: `packages/generator/include/combo/boss_kit.h` and `packages/generator/src/oot/boss_kit.c`.

App controls: Custom > Ocarina of Time > "Boss Kit: Health Bars" (`bossKitHealthBar`). Each boss adds its own toggles (for the Pot God, "Pot God" and "Pot God Fight").

Not tested in game yet: there are no ROMs in the cloud sessions, so this was only compiled.

## Fights and arenas

A **fight** is a boss actor plus an **arena** and where to send Link after winning. An arena is a vanilla scene that the kit empties of its own boss.

| Arena | Scene | Notes |
| --- | --- | --- |
| `BOSS_KIT_ARENA_HELLFIRE` | Volvagia's lair (`SCE_OOT_LAIR_VOLVAGIA`, entrance `ENTR_OOT_BOSS_TEMPLE_FIRE`) | Lava all around. Volvagia, its falling rocks, its platform, the blue warp and the heart container never spawn. The room's heat is turned off, so child Link needs no tunic. Music: Volvagia's theme. |

How a fight goes:

1. The boss's trigger calls `BossKit_StartFight(play, BOSS_KIT_FIGHT_...)`. The screen fades to black and Link is taken to the arena.
2. When the arena loads (`BossKit_OnSceneInit`, called from `Play_AfterInit`), the kit turns off the heat, puts Link at the arena's spot (and makes that his respawn point if he falls in), and spawns the boss with params `BOSS_KIT_PARAMS_ARENA`. Actors on the arena's blocked list are refused by `canSpawnActor` (`src/oot/actors.c`).
3. The boss runs its own intro and fight. For the music it can call `BossKit_FightBgm(play, defaultBgm)`, which gives the arena's music in an arena.
4. When it is beaten, the boss calls `BossKit_FinishFight(play)`: fade out, back to the fight's return entrance.

Dying in the arena restarts the fight there (Continue brings Link back to the arena and the kit sets it up again). The fight isn't saved: leaving the arena any other way (save and quit, warp song, reset) ends it. An arena isn't treated as a dungeon, so saving there loads at Link's house like any other overworld save.

## Adding a fight

1. Write the boss actor (see `Custom_Pot_God` for a full example). Give it an unused actor id in `include/combo/actor_id.h`.
2. Add a `BOSS_KIT_FIGHT_*` id in `boss_kit.h` and its entry in `kFights` (`boss_kit.c`): actor id, params, arena, return entrance.
3. For a new arena, add a `BOSS_KIT_ARENA_*` id and its entry in `kArenas`: scene, entrance, music, Link's and the boss's spots and facing, and up to 4 actors to keep out. Pick spots on solid floor (boss intros in the decomp are a good source of coordinates).
4. Add a toggle for the boss in `packages/core/src/settings/custom.ts`, as for any fork feature.

## Helpers

- **Cutscenes**: `BossKit_CutsceneStart` and `BossKit_CutsceneEnd` (manual cutscene, sub camera, Link frozen, the other actors halted; bosses are in the `BOSS` category so they keep updating). `BossKit_CameraOrbit` eases the camera toward a shot of a point from a given direction, distance and heights. `BossKit_PlayerIsCalm` says when Link can be put in a cutscene (on the ground, no text, no item held up, no fade going on).
- **Text and sound**: `BossKit_ShowText`, `BossKit_ShowTitle` (red name plus a subtitle, closes itself), `BossKit_Quake`, `BossKit_PlayBgm`, `BossKit_StopBgm`, `BossKit_PlayChildVoice` (plays one of Link's voice clips, the child version when he is a child: child clips are 0x20 after the adult ones).
- **Objects**: `BossKit_GetObjectSlot` loads an object that isn't in the scene into the fork's extra object slots (always get extra objects through it, so the kit knows which object a slot holds). `BossKit_ObjectReady` checks it's there. `BossKit_UseObject` points segment 6 at an object for both the CPU (skeletons, animations) and the RSP (display lists), and sets the texture segments that OoTMM's patched lists need (see below); call it again with the actor's own slot afterwards. `BossKit_UseObjectCpu` is the CPU half, for `SkelAnime_Init` and `SkelAnime_Update` on another object's skeleton.
- **Effects**: `BossKit_SpawnBlood` (Dead Hand's blood). `BossKitDebris` is a small debris system the boss keeps in its own struct and draws itself from any display list (`BossKit_DebrisSpawn`, `BossKit_DebrisUpdate`, `BossKit_DebrisDraw`). It replaces the vanilla shard effect, which needs its object to be in the scene's normal slots.
- **Arena safety**: `BossKit_KeepOnFloor` undoes a move that would take the boss off the arena floor (into a pit or the lava).
- **Health bar**: `BossKit_DrawHealthBar(play, health, maxHealth)` draws a red bar at the bottom of the screen when "Boss Kit: Health Bars" is on. Call it from the boss's draw while it is fighting.

## Patched vanilla display lists (read this before drawing a vanilla object)

OoTMM rewrites some vanilla display lists so pots, grass and chests can show what they hold (`packages/generator/src/oot/objects/*.S`): their texture loads become calls to segments 0x0A and 0x0B, and the vanilla actor sets those segments (the `csmc*PreDraw` functions in `src/common/csmc/`) right before drawing. Drawing one of those lists without setting them makes the RSP jump to garbage, and the game crashes on the first frame it's drawn. That was the Pot God's crash for three versions.

The kit handles it: `BossKit_UseObject` and `BossKit_DebrisDraw` call `BossKit_PrepareTextures`, which points 0x0A and 0x0B at the plain look for the patched objects and at an empty list for every other object.

| Object | Patched lists | Set to |
| --- | --- | --- |
| `object_tsubo` | the pot (`DL_0017C0`) | plain pot (`csmcPotPreDraw`) |
| `gameplay_dangeon_keep` | dungeon pots | plain dungeon pot |
| `object_kusa` | grass | plain grass (`csmcGrassPreDraw`, alt) |
| `gameplay_field_keep` | grass | plain grass |
| `object_box` | chests | plain chest (`csmcChestPreDraw`) |

Rules for a new boss:

1. Draw other objects only after `BossKit_UseObject` (never set segment 6 by hand), and get them with `BossKit_GetObjectSlot`.
2. Keep objects (`gameplay_keep`, `gameplay_field_keep`, `gameplay_dangeon_keep`) aren't in slots: call `BossKit_PrepareTextures(play, OBJECT_...)` yourself before drawing their lists.
3. If you set segments 0x0A or 0x0B for your own textures, do it after `BossKit_UseObject`.
4. When OoTMM patches another object this way, add it to the switch in `BossKit_PrepareTextures` (`src/oot/boss_kit.c`) and to this table. Until then it draws untextured, which is ugly but doesn't crash.
