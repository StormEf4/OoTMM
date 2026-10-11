# Giant's Mask: how it works (MM and OoT)

Reference for future changes to the Giant's Mask, or for anything else that needs to scale Link. Settings: `giantMaskAnywhere` (Custom > Majora's Mask), `giantMaskOot` and `sharedMaskGiant` (Items > Extended Items > Cross-Game Masks). How the item itself was wired into OoT is in [cross-game-items.md](cross-game-items.md).

## Majora's Mask (`src/mm/giant_mask.c`, `src/mm/patches/giant_mask_anywhere.S`)

- Based on the MM Randomizer's Giant's Mask Anywhere. Twinmold's Lair keeps the vanilla behavior (there the arena shrinks around Link instead).
- Elsewhere we run our own grow/shrink cutscene (camera, white flash, skippable with A/B/C) and set Link's scale from 0.01 to 0.1 (10x).
- Movement REGs and `PlayerAgeProperties` are scaled ×10.
- Physics constants that are hard-coded in the player overlay are scaled through asm hooks in `giant_mask_anywhere.S`: player height, floor-check delta, ledge walk-off height, form speed, wall collision height, dive depth, water-surface climb height, ledge climb factors, ledge-jump wall height, mid-air jump-slash height.
- The hooks live in patch group `PG_MM_GIANT_MASK_ANYWHERE` (`patch-build/group.ts`, `patch-build/index.ts`), so they are only applied when the setting is on. Function addresses are in `src/link_mm.in`.
- `dpad.c` and `Player.c` block every item except the Giant's Mask while giant.

## Ocarina of Time (`src/oot/giant_mask.c`, `include/combo/oot/giant_mask.h`)

OoT has no Giant's Mask code at all, so this is new. It is written in C only, with no mid-function asm hooks, because finding hook sites in the OoT player overlay needs a ROM and a disassembly. That is why it is a bit simpler than MM.

**States:** NORMAL, GROWING (40 frames), GIANT, SHRINKING (30 frames). The scale moves with `Math_ApproachF(&scale, target, 0.15f, 0.006f)`. Link is frozen while growing and shrinking with `Player_SetCsActionWithHaltedActors(play, NULL, PLAYER_CSACTION_1)` and released with `PLAYER_CSACTION_7`.

**When it can be put on:** magic above 0, standing on the ground, not riding Epona, not in water, not hanging or climbing (`STATE1_23`, `STATE1_29`), not crawling, no cutscene, and a line test from Link up to 10× his ceiling-check height hits nothing. Otherwise it plays the error sound.

**While giant:**
- Magic drains 1 point every 20 frames (none while the Chateau Romani effect is active). At 0 magic the mask comes off.
- Every item except the Giant's Mask is blocked (`Player_UseItem` in `src/oot/actors/Player.c`, and `canUseDpadItem` in `src/common/dpad.c`).
- The sword gets `DMG_EXPLOSIVE | DMG_HAMMER_SWING` so it breaks what a hammer or bomb would.
- Leaving the area (`Player_AfterInit` calls `GiantMaskOot_Reset`) takes the mask off, restores everything and caps the speed at 6 so a giant's run doesn't carry over.

**What is scaled ×10:**
- the actor scale (0.01 to 0.1);
- the first 0x44 bytes of `PlayerAgeProperties` (`GiantMaskAgeProperties` in the C file): `ceilingCheckHeight`, `unk_0C` to `unk_1C`, `unk_24` to `unk_34`, `wallCheckRadius`, `unk_3C`, `unk_40`. Not `unk_04`/`unk_08`/`unk_20`; `unk_08` is the animation root-motion factor and breaks walking if scaled;
- the movement REGs (run speed limit, acceleration, deceleration, walk/backwalk thresholds and animation speeds), but not gravity (`REG(68)`);
- `Player_GetHeight`, replaced wholesale with `PATCH_FUNC(0x80021600, GiantMaskOot_PlayerGetHeight)` (used by the camera and many actors);
- the body cylinder radius.

**Gotchas found while writing it:**
- The OoT player overlay (and its age-properties data) is reloaded on unpause, so the code re-checks `ceilingCheckHeight < 200` before scaling again, and only restores through the pointer it grew.
- `Player_SetBootData` resets the REGs on boot and room changes, so they are re-applied whenever `R_RUN_SPEED_LIMIT < 2000`. When shrinking ends, `UpdateEquipment(play, player)` restores them to normal.
- The scale is only forced while not NORMAL, so vanilla code that changes Link's scale isn't fought.
- Defining a C function named `Player_GetHeight` clashes with the symbol in `link_oot.in`, hence the different name plus `PATCH_FUNC`.

**Known limits in OoT (would need asm hooks):** jump height, gravity and fall speed stay normal; wall-collision height (26), floor-check distances (50 / -11) and ledge-climb heights (60 / 59 / 41) are not scaled. In practice Link walks and runs like a giant but jumps and climbs like normal Link.

**Drawing:** the worn mask uses MM's `object_mask_kyojin` (object 0x214) with display list `0x0a000380`, loaded through `prepareMask(play, 0x214 | MASK_FOREIGN_OBJECT, 1)` (`DrawExtendedMaskGiant` in OoT `Player.c`). The item model is `[mm, 0x0226]` drawn as `MM_MASK_GIANT`. Icons come from MM (`comboLoadMmIcon(dst, 0xa27660, ITEM_MM_MASK_GIANT)`).

**OoT 1.0 addresses used** (from `src/link_oot.in`): `Player_GetHeight` 0x80021600, `gRegEditor` 0x8011ba00, `BgCheck_EntityLineTest1` 0x800308b4, `UpdateEquipment` 0x80079764, `Player_SetCsActionWithHaltedActors` 0x800218ec, `Magic_RequestChange` 0x80072588.

## Adding the missing OoT physics later

To match MM fully, port the MM hooks one by one: find the matching spot in the OoT player overlay (the decomp names below help), write an asm hook like those in `giant_mask_anywhere.S`, and put it in an OoT patch group gated on `giantMaskOot`. That needs a ROM (`roms/oot.z64`) and the N64 toolchain locally, or a session running on Elijah's computer.

## Decomp sources

- OoT: https://github.com/zeldaret/oot. Its GitHub API is often rate-limited; a sparse clone works: `git clone --depth 1 --filter=blob:none --sparse https://github.com/zeldaret/oot` then `git sparse-checkout set src/overlays/actors/ovl_player_actor include`. It has no per-version address lists; use `src/link_oot.in` here for addresses.
- MM: https://github.com/zeldaret/mm, single files via `https://raw.githubusercontent.com/zeldaret/mm/main/<path>` (e.g. `assets/xml/objects/object_mask_kyojin.xml` for the mask display list).
