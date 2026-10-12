# Adding a cross-game item (and its shared version)

How to bring an item from one game into the other, with an optional "shared" version that is one item for both games. The Giant's Mask in OoT followed exactly the pattern upstream used for the cross-game masks (Kamaro's Mask is the cleanest template), so search for `KAMARO` / `kamaro` to see every spot.

Worked example: Giant's Mask in OoT (`giantMaskOot`, `sharedMaskGiant`), see `CUSTOM_CHANGES.md` and [giant-mask.md](giant-mask.md).

## Settings (where it shows in the app)

`packages/core/src/settings/custom.ts`. A custom setting may use an upstream category so it shows next to its siblings instead of on the Custom page:

```ts
{ key: 'giantMaskOot', category: 'items.extensions.crossGame.masks', game: 'oot', group: "Giant's Mask", type: 'boolean', ... cond: hasOoT },
{ key: 'sharedMaskGiant', category: 'items.extensions.crossGame.masks', game: 'shared', group: "Giant's Mask", type: 'boolean', ... cond: (s) => hasOoTMM(s) && s.giantMaskOot },
```

- `game` picks the column (OoT / MM / Shared) and `group` the row in Items > Extended Items > Cross-Game Masks. The MM column reads "Native" because the item already exists there.
- They are still `custom.ts` settings, so they still get `CFG_CUSTOM_<KEY>` C flags automatically and stay out of upstream's `data.ts`.
- Special-condition counts: `packages/core/src/settings/special-conds.ts` (`masksOot` max goes up by one when the item is on).

## Logic (TypeScript)

| File | What to add |
|---|---|
| `packages/logic/src/items/defs.ts` | `OOT_<ITEM>` and `SHARED_<ITEM>` names |
| `packages/logic/src/items/groups.ts` | the groups the item belongs to (masks: `MASKS_OOT`, `MASKS_REGULAR_VANILLA_MM` for the shared one, `REQUIRED`) |
| `packages/logic/src/world/transform.ts` | add to the plentiful list; `if (shared) replaceItem(MM_X, SHARED_X) else if (oot) addItem(OOT_X)` |
| `data/macros/macros_mm.yml` (and `_oot`) | macros that check the item also accept `SHARED_X` |
| `packages/generator/lib/combo/randomizer/checks.ts` | `['SHARED_X','OOT_X']` and `['SHARED_X','MM_X']` pairs so the shared item becomes the right per-game GI |
| `packages/generator/lib/combo/names.ts` | spoiler names (rename the native one to "... (MM)") |
| `packages/logic/tests/seeds/` | a seed test with each setting on |

## Data

`data/defs/gi.yml`: append the new GI **at the end of that game's block** so existing GI IDs don't shift. `add:` names the save handler and its argument, `draw`/`object` can point at the other game's model (`[mm, 0x0226]`).

## Game code (C)

| File | What to add |
|---|---|
| `include/combo/data/items.h` | the new `ITEM_OOT_*` id (bump `ITEM_OOT_CUSTOM_MAX`), the per-game alias, and any save bit (`XITEM_OOT_CHILD_*`) |
| `src/common/trade.c` | for an OoT child trade item, add it to `kOotTradeChild` at the index of its bit in `gOotExtraTrade.child` |
| `src/common/item/item_add.c` | shared table row `{ CFG_CUSTOM_SHARED_X, GI_OOT_X, GI_MM_X }` |
| `src/common/text/text.c` | when to print the "(OoT)" / "(MM)" suffix |
| `src/common/special_conds.c` | count it for mask/item special conditions |
| `src/oot/interface/interface.c` | `comboItemIcon` (icon from the other game) and C-button enable rules |
| `src/oot/ovl_patch/kaleido_scope.c` | pause-menu icon |
| `src/oot/ovl/gamemodes/ovl_file_choose/file_choose_custom_file_info.c` | file-select icon |
| `src/oot/actors/Player.c` | `Player_UseItem` case, `kMaskCallbacks` draw entry, `sCustomItemActions` entry |

## Limits to watch

- OoT child trade bits: `gOotExtraTrade.child` is a u16 and is full (bit `0xf` is the Giant's Mask). Items from `0x10` up (Fierce Deity's Mask `0x10`, Deku Mask `0x11`, MM Goron Transformation Mask `0x12`, MM Zora Transformation Mask `0x13`) are stored in `gSharedCustomSave.ootTradeChildExt` / `ootTradeChildExtSave`. Always read and write child trade bits through the `OotTradeChild_*` helpers in `include/combo/save.h`, which combine both into one 32-bit mask. `kOotTradeChild` order must match the bit numbers.
- Game flags: about 65 `CFG_CUSTOM_*` flags are left (see `CUSTOMIZING.md`).

## Checking the C without the N64 toolchain

The real build needs the toolchain and ROMs (CI's app build compiles the C for you). For a fast local syntax check:

1. `./scripts/deps.sh` (fetches `third_party/ultralib`).
2. Run the generator codegen once so `packages/generator/build/include/combo/generated_config.h` exists (it holds the `CFG_CUSTOM_*` values).
3. From `packages/generator`, for each changed file:

```sh
clang --target=mips-unknown-elf -mabi=32 -march=mips3 -EB -std=gnu2x -fsyntax-only \
  -DGAME_OOT \   # or -DGAME_MM
  -Iinclude -Ibuild/include -I../data/dist/include \
  -Isrc/oot/ovl -Isrc/common/ovl \
  -I../../third_party/ultralib/include -I../../third_party/ultralib/include/PR \
  src/oot/giant_mask.c
```

Files that include ROM-extracted asset headers (some overlays, MM `Player.c`) can't be checked this way.
