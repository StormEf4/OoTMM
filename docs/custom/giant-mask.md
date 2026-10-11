# Giant's Mask: how it works (MM and OoT)

Reference for future changes to the Giant's Mask, or for anything else that needs to scale Link. Settings: `giantMaskAnywhere` (Custom > Majora's Mask), `giantMaskOot` and `sharedMaskGiant` (Items > Extended Items > Cross-Game Masks). How the item itself was wired into OoT is in [cross-game-items.md](cross-game-items.md).

## Majora's Mask (`src/mm/giant_mask.c`, `src/mm/patches/giant_mask_anywhere.S`)

- Based on the MM Randomizer's Giant's Mask Anywhere. Twinmold's Lair keeps the vanilla behavior (there the arena shrinks around Link instead).
- Elsewhere we run our own grow/shrink cutscene (camera, white flash, skippable with A/B/C) and set Link's scale from 0.01 to 0.1 (10x).
- Movement REGs and `PlayerAgeProperties` are scaled ×10.
- Physics constants that are hard-coded in the player overlay are scaled through asm hooks in `giant_mask_anywhere.S`: player height, floor-check delta, ledge walk-off height, form speed, wall collision height, dive depth, water-surface climb height, ledge climb factors, ledge-jump wall height, mid-air jump-slash height.
- The hooks live in patch group `PG_MM_GIANT_MASK_ANYWHERE` (`patch-build/group.ts`, `patch-build/index.ts`), so they are only applied when the setting is on. Function addresses are in `src/link_mm.in`.
- `dpad.c` and `Player.c` block every item except the Giant's Mask while giant.

## Ocarina of Time (`src/oot/giant_mask.c`, `src/oot/patches/giant_mask_oot.S`, `include/combo/oot/giant_mask.h`)

OoT has no Giant's Mask code at all, so this is new. The transformation is in C; the movement physics are asm hooks into the OoT player overlay and two code-segment functions, found without the ROM (see [oot-addresses.md](oot-addresses.md)).

**States:** NORMAL, GROWING (40 frames), GIANT, SHRINKING (30 frames). The scale moves with `Math_ApproachF(&scale, target, 0.15f, 0.006f)`. Link is frozen while growing and shrinking with `Player_SetCsActionWithHaltedActors(play, NULL, PLAYER_CSACTION_1)` and released with `PLAYER_CSACTION_7`.

**When it can be put on:** magic above 0, standing on the ground, not riding Epona, not in water, not hanging or climbing (`STATE1_23`, `STATE1_29`), not crawling, no cutscene, and a line test from Link up to 10× his ceiling-check height hits nothing. Otherwise it plays the error sound.

**While giant:**
- Magic drains 1 point every 20 frames (none while the Chateau Romani effect is active). At 0 magic the mask comes off.
- Every item except the Giant's Mask is blocked (`Player_UseItem` in `src/oot/actors/Player.c`, and `canUseDpadItem` in `src/common/dpad.c`).
- The sword gets `DMG_EXPLOSIVE | DMG_HAMMER_SWING` so it breaks what a hammer or bomb would.
- Leaving the area (`Player_AfterInit` calls `GiantMaskOot_Reset`) takes the mask off and restores everything.

**What is scaled ×10 in C:**
- the actor scale (0.01 to 0.1);
- the first 0x44 bytes of `PlayerAgeProperties` (`GiantMaskAgeProperties` in the C file): `ceilingCheckHeight`, `unk_0C` to `unk_1C`, `unk_24` to `unk_30`, `wallCheckRadius`, `unk_3C`, `unk_40`. Not `unk_04`/`unk_08`/`unk_20` (`unk_08` is the animation root-motion factor and breaks walking if scaled), and not `unk_34`, which is compared to the floor distance the hooks keep in normal units;
- `Player_GetHeight`, replaced wholesale with `PATCH_FUNC(0x80021600, GiantMaskOot_PlayerGetHeight)` (used by the camera and many actors);
- the body cylinder radius.

**Movement (asm hooks, patch group `PG_OOT_GIANT_MASK`, on when `giantMaskOot` is on):**

The idea differs from MM: the player code keeps running in normal units (speed, jump velocity, gravity, the movement REGs are not touched), and only the step applied to Link's position is multiplied by 10. Link then runs, jumps, rolls, backflips and falls exactly like normal Link at 10× the size, with the same timing. The other hooks convert the world distances the player code reads back into normal units, and scale the collision distances that are hard-coded.

| Address | Where (decomp name) | What the hook does | C function |
|---|---|---|---|
| 0x80847708 | `Player_UpdateCommon`, `jal Actor_UpdatePos` | velocity ×10 while moving Link, then restored | `GiantMaskOot_PlayerUpdatePos` |
| 0x80845E28, 0x8084A17C | `Player_ProcessSceneCollision` and climbing, `jal Actor_UpdateBgCheckInfo` | wall height 26 → 260 | `GiantMaskOot_PlayerUpdateBgCheckInfo` |
| 0x80021C7C | `func_8002E2AC` (code segment, every actor) | floor raycast starts 500 above instead of 50 | `GiantMaskOot_GetFloorCheckOffset` |
| 0x80021BE8 | `func_8002E234` (code segment), replaced | ledge walk-off distance -11 → -110 | `GiantMaskOot_FloorLeaveCheck` |
| 0x80845E5C | `sYDistToFloor = pos.y - floorHeight` | divided by 10 (auto-jump off ledges, fall animations, voids) | `GiantMaskOot_ScaleDownDistance` |
| 0x80838B68 | `func_8083AA10`, `fallDistance = fallStartHeight - pos.y` | divided by 10 (fall damage) | `GiantMaskOot_ScaleFallStart` |
| 0x80836E28 | `Player_ActionHandler_12`, ledge climb `shape.yOffset` | climb animation covers 10× more, offset in giant model units | `GiantMaskOot_GetLedgeClimbOffset` |
| 0x80836F08 | `Player_ActionHandler_12`, small hop `yDistToLedge * 0.08 + 5.5` | ledge height in normal units | `GiantMaskOot_GetJumpYDistToLedge` |
| 0x80843790 | `Player_Action_80845668`, tall ledge jump `yDistToLedge * 0.085` | ledge height in normal units | `GiantMaskOot_GetJumpYDistToLedge` |

None of the replaced instructions carries an overlay relocation (an instruction with one would be rewritten by the loader). Where a hook sits between a `lui at` and its use, the wrapper saves and restores `at` too.

**Not hooked (giant edge cases):** the mid-air ledge grab (`yDistToLedge` 150 / `70 * unk_08`), vine climbing (79) and crawlspaces (39) keep their normal thresholds; the interact-wall probe stays at 18 units high; `Actor_MoveXZGravity` in the `PLAYER_STATE2_15` branch is not scaled.

**Gotchas found while writing it:**
- The OoT player overlay (and its age-properties data) is reloaded on unpause, so the code re-checks `ceilingCheckHeight < 200` before scaling again, and only restores through the pointer it grew.
- The scale is only forced while not NORMAL, so vanilla code that changes Link's scale isn't fought.
- Defining a C function named `Player_GetHeight` clashes with the symbol in `link_oot.in`, hence the different name plus `PATCH_FUNC`.
- An earlier version scaled the movement REGs (run speed etc.) instead. That made actions with fixed speeds (backflips, jump slashes, knockback) feel tiny next to a giant run, which is why the position step is scaled now.

**Drawing:** the worn mask uses MM's `object_mask_kyojin` (object 0x214) with display list `0x0a000380`, loaded through `prepareMask(play, 0x214 | MASK_FOREIGN_OBJECT, 1)` (`DrawExtendedMaskGiant` in OoT `Player.c`). The item model is `[mm, 0x0226]` drawn as `MM_MASK_GIANT`. Icons come from MM (`comboLoadMmIcon(dst, 0xa27660, ITEM_MM_MASK_GIANT)`).

**OoT 1.0 addresses used** (from `src/link_oot.in`): `Player_GetHeight` 0x80021600, `gRegEditor` 0x8011ba00, `BgCheck_EntityLineTest1` 0x800308b4, `Actor_UpdatePos` 0x80021124, `Actor_UpdateBgCheckInfo` 0x80021e6c, `Player_SetCsActionWithHaltedActors` 0x800218ec, `Magic_RequestChange` 0x80072588.

## Decomp sources

- OoT: https://github.com/zeldaret/oot. Its GitHub API is often rate-limited; a sparse clone works: `git clone --depth 1 --filter=blob:none --sparse https://github.com/zeldaret/oot` then `git sparse-checkout set src/overlays/actors/ovl_player_actor include`. It has no per-version address lists; see [oot-addresses.md](oot-addresses.md) for how addresses were found without the ROM.
- MM: https://github.com/zeldaret/mm, single files via `https://raw.githubusercontent.com/zeldaret/mm/main/<path>` (e.g. `assets/xml/objects/object_mask_kyojin.xml` for the mask display list).
