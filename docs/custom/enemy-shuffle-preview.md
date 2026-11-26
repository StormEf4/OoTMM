# Enemy shuffle preview (test branch)

Upstream OoTMM is building an enemy randomizer (this fork is merged up to upstream preview commit `d5e19be6`, 2026-11-25; first merge was `afc74a6c`) on its `preview/enemy-shuffle` branch (https://github.com/OoTMM/OoTMM/tree/preview/enemy-shuffle). Any enemy can become another one, in OoT and in MM, and there's an option to bring MM enemies into OoT and OoT enemies into MM. It isn't in upstream's `master` or in any release yet.

Elijah asked for a **test build** with it (2026-11-25). It lives on the branch `claude/project-thread-oc38hb`, **not on master**. When upstream merges the enemy shuffle into its `master`, it arrives through a normal upstream sync instead (`upstream-sync.md`), and the fork changes below have to be carried over again.

## Where it shows in the app

A new **Enemies** page (sword icon, between Entrances and Advanced) with upstream's settings:

- Enemy Randomizer (OoT) and (MM), or Mini Boss Only for either game
- Enemy Placement (OoT/MM): Individual (each enemy on its own) or Grouped (every enemy of one kind in an area becomes the same thing)
- Shuffle Laggy Enemy Spawns (MM Leevers and Boes)
- Gold Skulltula Enemy Shuffle, Include Minibosses, Keep Original Minibosses Vanilla, Mini Boss Shuffle Type
- Bari Shuffle and Eeno Shuffle (what they split into)
- Cross-Game Enemies
- Enemy Soul Ghosts (moved here from the souls settings)

These are upstream settings, so they live in upstream's `data.ts`, not in `custom.ts`.

## What the fork had to change

| What | Why | Where |
|---|---|---|
| Pot God actor id `0x003` -> `0x180` | Upstream now uses OoT actor `0x003` for `ACTOR_ENEMY_POE_ENCOUNTER`. `0x180` is unused in vanilla OoT and by upstream (MM's `0x180` is a different table). The watcher stays at `0x006`. | `include/combo/actor_id.h` |
| OoT code area `0x80000` -> `0x90000` | The fork's OoT code plus upstream's enemy shuffle code is 570 KB with the cloud's gcc 12 (a little less with the real toolchain), over the 512 KB upstream reserves. | `include/combo/defs.h`, `lib/build.ts`, `lib/build/payload-size.ts`, `lib/combo/patch-build/index.ts`, `lib/combo/randomizer/enemies.ts` (`verifyNativeActivationSource`) |
| OoT enemy arena moved to `0x80490000` and shrunk by `0x6000` to `0xfa000`; OoT gameplay pool moved to `0x8058a000` and shrunk by `0xa000` to `0x76000` | Upstream puts its enemy memory right after the OoT code (`0x80480000`, then the gameplay pool, then the audio heap at `0x80600000`), so the bigger code area had to come out of them. Both still pass upstream's own checks: arena 8 KB above `selectedEnemyBytes + nativeReserve`, pool 15 KB above two copies of the custom models plus `contiguousReserve`. The per-room enemy budget is unchanged. | `include/combo/oot_gameplay_memory.h`, `src/oot/enemy_arena.c`, `packages/core/src/oot-gameplay-memory-data.ts` (`arenaBytes`, `customReserve`, `customBase`), `lib/combo/enemies/overlay.ts` (`installOotEnemyNativeArena`) |
| MM Giant's Mask Anywhere hooks share one register save/restore routine | MM code was over its limit too (upstream already raised it to `0x80000`). The hooks used to inline about 230 bytes of saves and loads each; now they call `GiantMask_SaveRegs` / `GiantMask_RestoreRegs`. Same registers, same frame layout, so the hooks behave the same. Saves about 4.5 KB. | `src/mm/patches/giant_mask_anywhere.S` |
| Reroll the enemies when a seed keeps failing | Fix for an upstream preview bug, see "Seeds tested in the cloud" below. With the enemy shuffle on, after 20 failed item placements, or 5 failed entrance tries on a world kept from an enemy fight reroll, the generator throws `EnemyRerollError` and the pipeline starts over with a fresh enemy plan and fresh entrances. It also starts over when upstream's enemy planner gives up fitting a scene in memory (`EnemyMemoryAllocationError`, "bounded selected-enemy resource search exhausted"). All of these share upstream's 16-attempt limit for fight rerolls. Nothing changes with the enemy shuffle off. Drop it once upstream fixes the cause. | `packages/logic/src/error.ts`, `pipeline.ts` (`solvedWorldState`), `solver/solve.ts` (`retry`), `solver/entrances.ts` (`run`), `packages/core/src/index.ts` (exports `EnemyMemoryAllocationError`) |
| App workflow can publish a test pre-release | Running "Build My App" by hand on a branch other than master publishes a pre-release `test-<run>`. Pre-releases never become "latest". | `.github/workflows/my-app.yml` |

If upstream changes any of these numbers, every place in the table must move together. Upstream's checks catch most mismatches when a seed is generated (they throw instead of crashing the game).

### Merge conflicts resolved

`actor_id.h` (took upstream's file, added the Pot God ids), `En_Item00.h` and both `En_Ishi.c` (upstream `master` replaced `EnItem00_DropCustomNoInertiaEx` with `EnItem00_DropCustomEx(..., XFDROP_NO_INERTIA)`; the enemy branch's small-rock drop now uses the new call), `custom.h` (regenerated from the ROMs; the fork's Zora fins and Deku bubble objects are now `0x2056`/`0x2057`, after upstream's), `custom-objects-builder.ts` (kept upstream's Redead dances and the fork's `buildFork`), `build.ts` (both the fork's payload check and upstream's enemy memory refresh), `link_mm.in`, MM `Player.c` (upstream's Dark Link wrapper calls `Player_Update`; the Giant's Mask after-update runs right after it), `Generator.tsx` (both the Enemies and Custom pages).

## Building and generating seeds in the cloud

The build runs `refreshEnemyMemory`, which measures every enemy overlay and rewrites `packages/core/src/enemy-memory-data.ts` when it isn't in CI. **Don't commit that file from a cloud build**: gcc 12 makes different (bigger) overlays than the real toolchain, and CI (`CI=true`) fails with "Enemy memory facts are stale" if the committed numbers don't match the real toolchain. If CI ever does fail with that message, its log lists the real numbers (`Enemy memory: <overlay>: old -> new`); copy those in.

For the same reason, generating an enemy shuffle seed from a cloud build needs two local-only tweaks, never committed:

- `lib/combo/enemies/oot-gameplay-memory.ts`: the "Refresh OoT gameplay memory bounds" checks fail on gcc 12 sizes; turn them into warnings for the test.
- Keep the refreshed `enemy-memory-data.ts` while generating, then `git checkout` it.
- Since `d5e19be6`, MM code is over 512 KB with gcc 12 (it fits with the real toolchain). For a cloud seed, raise the MM limit to `0x81000` in `lib/build.ts`, `lib/build/payload-size.ts`, `lib/combo/patch-build/index.ts` and `lib/combo/randomizer/enemies.ts` (`verifyNativeActivationSource`), and the `0x80780000` payload-end checks in `lib/combo/enemies/overlay.ts` (`verifyMmEnemySetupFilter`) and `lib/combo/randomizer/enemies.ts`. Mark each `LOCAL-ONLY` and restore them all before committing.

## Seeds tested in the cloud (2026-11-25)

| Settings | Seed | Result |
|---|---|---|
| Enemy shuffle in both games | `enemytest1` | Generated, under 1 min |
| Grouped mode, Enemy Souls (OoT, MM, lens ghosts), Pot God with its arena | `enemytest3` | Generated, 1 min |
| Fork customs (Giant's Mask, Fierce Deity, transform masks in OoT, Magic Hookshot, progressive infinite magic, Pot God with warp song and arena, boss health bar) plus enemy shuffle in both games | `enemytest3` | Generated |
| Fork customs without enemy shuffle | `enemytest2` | Generated |
| Enemy shuffle in both games, nothing else | `enemytest2` | Failed before the reroll fix; now generates in 2.5 min (1 enemy reroll) |
| Every enemy option (minibosses, Cross-Game, Gold Skulltulas, Bari, Eeno), with and without every fork custom | `enemytest2` | Failed before the reroll fix; now generates in about 17 min (13 fight rerolls, 1 enemy reroll) |
| Every enemy option plus every fork custom | `enemytest3`, `4`, `6` | Generated in 2 to 10 min |
| Every enemy option plus every fork custom | `enemytest5` | Failed before the memory reroll fix ("OoT scene 83: bounded selected-enemy resource search exhausted"); now generates in 25 min (8 fight rerolls, 1 memory reroll) |

Times are from the cloud with up to three seeds generating at once, and the cloud uses gcc 12 overlay sizes (bigger than the real app), so the memory planner has less room here than in the app.

All the failures were upstream preview bugs, not fork ones: the same seeds failed with no fork settings at all.

- With `enemytest2`, MM's regular Big Poe became a Red-Ice ReDead. That swap (a "restore" row in `makeSupplementalEnemyPlan`) has no logic name, so upstream's "Rerolling impossible enemy fights" check never sees it. Its fight is ANDed into the whole Poe soul (`enemySoulSpecializations`), so *MM Ghost Hut HP* and *Beneath The Graveyard Dampe Chest* needed blue fire, which MM doesn't have. The solver retried the same world 100 times and gave up.
- With every enemy option, upstream rerolls impossible fights many times. Each reroll keeps the last world's entrances, which makes the entrances pass run its full check, and for some enemy plans that check can't place the dungeon rewards. The same world was retried up to 1,000 times (20+ minutes) without changing.
- With `enemytest5`, upstream's OoT planner couldn't fit the Graveyard's enemies in memory in 8 tries and stopped the whole seed.

The fork's reroll fix (table above) turns all three into a fresh enemy plan instead. What's still upstream's: seeds with every enemy option take a long time because each fight reroll rebuilds the whole world.

## Re-merging upstream's newer preview commits

Re-merges so far: 2026-11-26, `afc74a6c` -> `d5e19be6` (9 commits: death crash fix, Wizzrobe drawing, Snapper death color, spawn rules rewritten into `data/enemy-logic/spawn-encounters.yml`, animation dedupe). No conflicts; MM code grew about 3.6 KB (gcc 12: 525,448 bytes, so a cloud build needs the MM limit raised locally, see below).

1. `git fetch https://github.com/OoTMM/OoTMM.git preview/enemy-shuffle` and merge it into this branch.
2. Check every row of the table above still holds (grep for `0x80480000`, `0x80580000`, `0x100000`, `base + 0x80000` and `0x003`).
3. Build with the ROMs, generate seeds with the enemy shuffle on, then restore `enemy-memory-data.ts`.
4. Run "Build My App" on the branch to publish a new test pre-release.

## What to test in game

Nothing here has been played yet. Most useful first:

1. Enemy shuffle on, a few overworld areas and one dungeon in each game.
2. The Pot God fight with the enemy shuffle on and off (Pot House and Hellfire Arena), because its actor id moved and the object loading code it uses was rewritten upstream.
3. Giant's Mask Anywhere in MM (walk, jump, swim, climb a ledge, dive), because its hooks were shrunk.
4. Cross-Game Enemies, then the Grouped mode and Enemy Souls.
