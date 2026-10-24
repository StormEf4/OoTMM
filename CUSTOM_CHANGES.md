# Custom Changes

Every change made in this fork, newest first. Each entry says where the change shows up in the app, so this file doubles as a reference for future additions. `pnpm custom:add` and `pnpm custom:log` write entries here automatically, and the app shows this file under Custom > Change Log.

<!-- new entries go below this line -->

## 2026-10-24: Magic Hookshot costs 2 magic

- Where in the app: Custom > Ocarina of Time > "Magic Hookshot (OoT)" and Custom > Majora's Mask > "Magic Hookshot (MM)" (no new control; changes that item)
- What it does: in magic mode, each latch onto a surface the normal hookshot can't grab now uses 2 magic. Latching onto normal hookshot targets is still free. With less than 2 magic (or no magic meter) the hook bounces off like a normal hookshot. In OoT the Chateau Romani makes it free, the same as in MM
- Files: `packages/generator/src/common/hookshot_magic.c`, `packages/core/src/settings/custom.ts` (description)

## 2026-10-24: Magic Hookshot (hookshot anywhere as its own item)

- Where in the app: Custom > Ocarina of Time > "Magic Hookshot (OoT)" and Custom > Majora's Mask > "Magic Hookshot (MM)" (Off / On / Logical)
- What it does: adds a new "Magic Hookshot" item to the pool. It is the same as your Hookshot or Longshot, but it can latch onto any surface, while the normal hookshots keep working as usual. Once you have it, highlight the Hookshot in the pause menu and press L or C-Up: the slot now cycles Hookshot, Longshot, Magic Hookshot, Magic Longshot. The pause cursor turns purple (OoT) or yellow (MM) on the Hookshot while magic mode is on, and a chime plays when you switch it on. It starts in magic mode when you find it. You still need a Hookshot to use it. "Logical" lets the logic expect hookshot-anywhere tricks once you have both. In the world it looks like the other game's hookshot so you can tell it apart
- How it works: the hookshot's surface check (`SurfaceType_IsHookshotSurface`) is swapped for `ArmsHook_IsHookshotSurface`, which says yes to every surface while magic mode is on. This is the same spot upstream's global "Hookshot Anywhere" setting patches, so both can be used together. Patch addresses were checked by compiling the OoT (NTSC 1.0) and MM (US) decomps
- Files: `packages/core/src/settings/custom.ts` (settings), `data/defs/gi.yml` (items), `data/macros/macros_oot.yml`, `data/macros/macros_mm.yml` (logic), `packages/logic/src/items/defs.ts`, `packages/logic/src/items/groups.ts`, `packages/logic/src/world/transform.ts` (item pool), `packages/generator/lib/combo/names.ts`, `packages/gui/src/app/components/mmAgeRequirements.tsx`, `packages/generator/include/combo/save_flags.h`, `packages/generator/include/combo/inventory.h`, `packages/generator/include/combo/common/api.h`, `packages/generator/src/common/hookshot_magic.c` (new), `packages/generator/src/common/trade.c` (pause menu cycling), `packages/generator/src/common/text/text.c`, `packages/generator/src/oot/ovl_patch/kaleido_scope.c`, `packages/generator/src/mm/ovl_patch/kaleido_scope.c` (cursor colour), `packages/generator/src/oot/actors/Misc/Arms_Hook.S`, `packages/generator/src/mm/actors/Misc/Arms_Hook.S` (hook patch), `packages/generator/src/link_oot.in`, `packages/generator/src/link_mm.in`, `packages/logic/tests/seeds/magic-hookshot.test.ts` (seed test)

## 2026-10-12: Fix: Fierce Deity's sword beam drained all magic (OoT)

- Where in the app: Items > Extended Items > Cross-Game Masks > "Fierce Deity's Mask" row, OoT column (no new control; fixes that item).
- What it does: each sword beam now costs exactly 1 magic, like in MM. Before, one beam emptied the meter because OoT's magic drain takes 2 points per frame and only stops when it lands exactly on the target, which an odd cost never does. The beam now takes its 1 point directly; no beam fires with an empty meter, and the Chateau Romani still makes it free.
- Files: `packages/generator/src/oot/fierce_deity.c`, `docs/custom/fierce-deity.md`

## 2026-10-12: Fix: Fierce Deity's sword beam crashed the game (OoT)

- Where in the app: Items > Extended Items > Cross-Game Masks > "Fierce Deity's Mask" row, OoT column (no new control; fixes that item).
- What it does: Z-targeting and slashing as Fierce Deity no longer crashes. The beam graphics come from MM's gameplay_keep, which is compressed in the ROM, and the game's file loader unpacks compressed files whole: loading just the beam part wrote the whole 600 KB file into a 6 KB buffer. A new loader unpacks only the needed part of a compressed file (`comboDmaLoadFileRange`, usable for any future MM keep graphics). The beam graphics now load when the mask is put on, so the first beam doesn't stutter.
- Files: `packages/generator/src/common/dma_range.c` (new), `packages/generator/include/combo/dma_range.h` (new), `packages/generator/src/oot/fierce_deity.c`, `docs/custom/fierce-deity.md`

## 2026-10-12: Goron and Zora punches, Zora jump kick and Deku spin in OoT

- Where in the app: Items > Extended Items > Cross-Game Masks, OoT column of the "Deku Mask", "MM Goron Transformation Mask" and "MM Zora Transformation Mask" rows (no new control; this is what those items do).
- What it does: the forms now attack like in MM, with MM's own animations. B as a Goron or Zora throws MM's three-hit punch combo (left, right, then the finisher); pressing B again during a punch queues the next one, like in MM. Goron punches hit like the hammer, Zora's like the Master Sword. Goron and Zora can't charge a spin attack. A Zora jump attack (B in the air, or Z-target + A) is MM's jump kick; a Goron has no jump attack (Z-target + A rolls). As a Deku Scrub, A spins (three turns, slowing down, steerable) and the spin hits like the Kokiri Sword. The 22 animations are streamed from MM's ROM like the existing Kamaro dance. How it hooks into OoT's sword code, and what is still not done: `docs/custom/transform-masks.md`.
- Files: `packages/generator/src/oot/transform_mask.c` (attack table swap, combo order, jump kick, Deku spin action), `packages/generator/src/oot/patches/transform_mask_oot.S` (relocations of the redirected calls), `data/defs/link-animations.yml` and `packages/generator/include/combo/custom_animations.h` (MM animations), `docs/custom/transform-masks.md`

## 2026-10-12: Deku, Goron and Zora forms in OoT

- Where in the app: Items > Extended Items > Cross-Game Masks, OoT column of the "Deku Mask", "MM Goron Transformation Mask" and "MM Zora Transformation Mask" rows (no new control; this is what those items do).
- What it does: wearing one of these masks in OoT turns Link (child or adult) into MM's Deku Scrub, Goron or Zora, drawn with MM's own models (face, hands and waist included) and MM's size, ledge, water-depth and movement values for each form. Using another form's mask switches straight to it, and the Fierce Deity's Mask works from a form too. The form stays on through loading zones, like in MM. Like MM, a form can only use the ocarina, lens, bottles, trade items and the transformation masks (Deku also Deku Nuts, Goron the Powder Keg). Goron and Zora always punch with B (Goron hits like the hammer, Zora like the Master Sword); Deku has no B attack yet. Goron is heat proof and Zora never drowns; Goron and Deku sink and void out in deep water; Deku and Zora void out when set on fire; Goron and Zora can't crawl. The forms' special moves (rolls, fins, bubbles, spins, fast swimming) come in later updates. Details, data tables and what is still unverified: `docs/custom/transform-masks.md`.
- Files: `packages/generator/src/oot/transform_mask.c` (new), `packages/generator/include/combo/oot/transform_mask.h` (new), `packages/generator/src/oot/patches/transform_mask_oot.S` (new), `packages/generator/src/oot/actors/Player.c` (item use, draw, mask callbacks, update and init hooks), `packages/generator/src/oot/giant_mask.c` (heights), `packages/generator/src/common/dpad.c`, `packages/generator/src/common/interface/interface.c` (tunic perks for the timers), `docs/custom/transform-masks.md` (new), `CUSTOMIZING.md`

## 2026-10-12: Deku Mask and MM Goron and Zora Transformation Masks in Ocarina of Time + shared options

- Where in the app: Items > Extended Items > Cross-Game Masks > "Deku Mask", "MM Goron Transformation Mask" and "MM Zora Transformation Mask" rows. Tick OoT to add the mask to Ocarina of Time; tick Shared (needs OoT ticked) to make the OoT and MM masks one item. The Shared MM Goron/Zora toggles are hidden while Items > Extended Items > Shared "Shared Goron Mask" / "Shared Zora Mask" is on, since that option already pairs the MM mask with OoT's own mask.
- What it does: adds MM's three transformation masks to OoT as child trade items that cycle with the other masks. OoT's own Goron Mask and Zora Mask are untouched; the MM ones are separate items named "MM Goron Transformation Mask" and "MM Zora Transformation Mask" everywhere so the two can't be confused. They count for mask special conditions. Child trade bits 0x11-0x13 (stored in `ootTradeChildExt`).
- Files: `packages/core/src/settings/custom.ts`, `packages/core/src/settings/special-conds.ts`, `packages/logic/src/items/defs.ts`, `packages/logic/src/items/groups.ts`, `packages/logic/src/world/transform.ts`, `packages/logic/tests/seeds/transform-masks-oot.test.ts`, `data/defs/gi.yml`, `data/macros/macros_mm.yml`, `packages/generator/lib/combo/names.ts`, `packages/generator/lib/combo/randomizer/checks.ts`, `packages/generator/include/combo/data/items.h`, `packages/generator/include/combo/mask.h`, `packages/generator/src/common/trade.c`, `packages/generator/src/common/item/item_add.c`, `packages/generator/src/common/text/text.c`, `packages/generator/src/common/special_conds.c`, `packages/generator/src/common/debug.c`, `packages/generator/src/oot/interface/interface.c`, `packages/generator/src/oot/ovl_patch/kaleido_scope.c`, `packages/generator/src/oot/ovl/gamemodes/ovl_file_choose/file_choose_custom_file_info.c`, `docs/custom/cross-game-items.md`

## 2026-10-12: Fierce Deity has a sword even without one in the inventory (OoT)

- Where in the app: Items > Extended Items > Cross-Game Masks > "Fierce Deity's Mask" row, OoT column (no new control; part of that item).
- What it does: like in MM, Fierce Deity can always attack with his sword (and fire beams), even if Link has no sword or has something else on B. While transformed, a sword (Master Sword for adult Link, Kokiri Sword for child Link) is put on B only during the player update, where the B button is read, and the real B item is put back right after, so the HUD, pause menu and save are unchanged.
- Files: `packages/generator/src/oot/fierce_deity.c` (`FierceDeityOot_LendSword`, `FierceDeityOot_ReturnSword`), `docs/custom/fierce-deity.md`

## 2026-10-12: Sword works while giant or Fierce Deity in OoT

- Where in the app: Items > Extended Items > Cross-Game Masks, OoT column of the "Giant's Mask" and "Fierce Deity's Mask" rows (no new control; fixes those items).
- What it does: the B-button sword now swings while wearing the OoT Giant's or Fierce Deity's Mask. The item block also caught the sword, because OoT sends the B button through the same `Player_UseItem` call as the C buttons, so Link couldn't attack at all, and Fierce Deity's sword beams (which fire on Z-targeted slashes) never had a slash to fire from. Other items stay blocked.
- Files: `packages/generator/src/oot/actors/Player.c` (`Player_IsAllowedWhileTransformed`), `docs/custom/giant-mask.md`, `docs/custom/fierce-deity.md`

## 2026-10-12: Fierce Deity always holds his sword in OoT

- Where in the app: Items > Extended Items > Cross-Game Masks > "Fierce Deity's Mask" row, OoT column (no new control; fixes that item).
- What it does: Fierce Deity now always has his sword in his left hand, like in MM. Before, the sword only showed while OoT Link had his sword drawn, so Fierce Deity walked around empty-handed.
- Files: `packages/generator/src/oot/fierce_deity.c` (`FierceDeityOot_ShowsSword`), `docs/custom/fierce-deity.md`

## 2026-10-11: Giant's Mask in Ocarina of Time moves like in MM

- Where in the app: Items > Extended Items > Cross-Game Masks > "Giant's Mask" row, OoT column (same toggle as before; the new behavior is always on with it).
- What it does: a giant Link in OoT now jumps, falls, rolls and backflips 10x bigger with normal timing, steps over walls up to 260 units, sticks to slopes and stairs, climbs ledges at giant scale, and takes fall damage like normal Link would at his size. Speeds, jump velocities and gravity stay in normal units in the player code, and the step applied to Link's position is multiplied by 10; asm hooks convert the floor, fall and ledge distances the player code reads back. The hooks are in patch group `OOT_GIANT_MASK`, only in the ROM when the OoT Giant's Mask is on. The movement REGs are no longer scaled. Addresses were found by rebuilding the OoT code from the decomp (`docs/custom/oot-addresses.md`, outputs in the shared folder `notes/oot-player-map/`).
- Files: `packages/generator/src/oot/patches/giant_mask_oot.S` (hooks), `packages/generator/src/oot/giant_mask.c` (physics functions, REG scaling removed, `unk_34` no longer scaled), `packages/generator/lib/combo/patch-build/group.ts` and `packages/generator/lib/combo/patch-build/index.ts` (patch group), `docs/custom/giant-mask.md` (hook table), `docs/custom/oot-addresses.md` (new), `CUSTOMIZING.md`

## 2026-10-11: Fierce Deity's Mask in Ocarina of Time + shared option

- Where in the app: Items > Extended Items > Cross-Game Masks > "Fierce Deity's Mask" row. Tick OoT to add it to Ocarina of Time; tick Shared (needs OoT ticked) to make the OoT and MM masks one item.
- What it does: adds the Fierce Deity's Mask to OoT as a child trade item that cycles with the other masks (pause menu, L or C-Up). Wearing it turns Link (child or adult) into MM's Fierce Deity: MM's model, 1.5x size and speed, a sword that hits like Biggoron's Sword with longer reach, and a sword beam (1 magic) on slashes while Z-targeting. Other items are blocked, and the mask comes off when used again or when leaving the area. It counts for mask special conditions. The OoT child trade slot was full (16 items), so items past the 16th are stored in `gSharedCustomSave.ootTradeChildExt` through the `OotTradeChild_*` helpers in `include/combo/save.h`. Details and unverified parts: `docs/custom/fierce-deity.md`.
- Files: `packages/core/src/settings/custom.ts`, `packages/core/src/settings/special-conds.ts`, `packages/logic/src/items/defs.ts`, `packages/logic/src/items/groups.ts`, `packages/logic/src/world/transform.ts`, `packages/logic/tests/seeds/fierce-deity-oot.test.ts`, `data/defs/gi.yml`, `data/macros/macros_mm.yml`, `packages/generator/lib/combo/names.ts`, `packages/generator/lib/combo/randomizer/checks.ts`, `packages/generator/include/combo/save.h`, `packages/generator/include/combo/data/items.h`, `packages/generator/include/combo/mask.h`, `packages/generator/include/combo/oot/fierce_deity.h`, `packages/generator/src/oot/fierce_deity.c` (transformation, model, beams), `packages/generator/src/oot/giant_mask.c` (height), `packages/generator/src/common/trade.c`, `packages/generator/src/oot/trade.c`, `packages/generator/src/common/item/item_add.c`, `packages/generator/src/common/text/text.c`, `packages/generator/src/common/special_conds.c`, `packages/generator/src/common/dpad.c`, `packages/generator/src/common/debug.c`, `packages/generator/src/oot/actors/Player.c`, `packages/generator/src/oot/interface/interface.c`, `packages/generator/src/oot/ovl_patch/kaleido_scope.c`, `packages/generator/src/oot/ovl/gamemodes/ovl_file_choose/file_choose_custom_file_info.c`, `docs/custom/fierce-deity.md`

## 2026-10-11: Giant's Mask in Ocarina of Time + shared option

- Where in the app: Items > Extended Items > Cross-Game Masks > "Giant's Mask" row. Tick OoT to add it to Ocarina of Time; tick Shared (needs OoT ticked) to make the OoT and MM masks one item.
- What it does: adds a Giant's Mask to OoT as a child trade item (C button or D-pad up). Wearing it makes Link grow 10x if there is room above him, with a short grow animation. He runs like a giant, his sword hits like a hammer and an explosion, other items are blocked, magic drains 1 point every 20 frames, and the mask comes off when magic runs out or he leaves the area. At first jumps, gravity, wall and ledge heights stayed normal in OoT; the entry "Giant's Mask in Ocarina of Time moves like in MM" fixed that. The shared option works like the other shared masks. The logic counts the OoT mask for mask special conditions; it isn't required anywhere in OoT.
- Files: `packages/core/src/settings/custom.ts` (settings), `packages/core/src/settings/special-conds.ts`, `packages/logic/src/items/defs.ts`, `packages/logic/src/items/groups.ts`, `packages/logic/src/world/transform.ts` (item pool), `packages/logic/tests/seeds/giant-mask-oot.test.ts`, `data/defs/gi.yml`, `data/macros/macros_mm.yml`, `packages/generator/lib/combo/names.ts`, `packages/generator/lib/combo/randomizer/checks.ts`, `packages/generator/include/combo/data/items.h`, `packages/generator/include/combo/mask.h`, `packages/generator/src/common/trade.c`, `packages/generator/src/common/item/item_add.c`, `packages/generator/src/common/text/text.c`, `packages/generator/src/common/special_conds.c`, `packages/generator/src/common/dpad.c`, `packages/generator/src/oot/giant_mask.c` and `packages/generator/include/combo/oot/giant_mask.h` (the growing), `packages/generator/src/oot/actors/Player.c`, `packages/generator/src/oot/interface/interface.c`, `packages/generator/src/oot/ovl_patch/kaleido_scope.c`, `packages/generator/src/oot/ovl/gamemodes/ovl_file_choose/file_choose_custom_file_info.c`, `docs/custom/cross-game-items.md` and `docs/custom/giant-mask.md` (notes for future additions), `CUSTOMIZING.md`

## 2026-10-11: Giant's Mask Anywhere

- Where in the app: Custom > Majora's Mask > "Giant's Mask Anywhere" (toggle, off by default)
- What it does: lets Link put on the Giant's Mask anywhere in MM where there is room above him, not just in Twinmold's Lair. He really grows 10x, his movement and physics scale with him, his sword hits like a giant's, and the mask comes off when he leaves the area. Twinmold's Lair keeps the vanilla behavior. No logic changes.
- Files: `packages/core/src/settings/custom.ts` (setting), `packages/generator/src/mm/giant_mask.c` and `packages/generator/include/combo/mm/giant_mask.h` (transformation, reads `CFG_CUSTOM_GIANT_MASK_ANYWHERE`), `packages/generator/src/mm/patches/giant_mask_anywhere.S` (physics hooks, patch group `MM_GIANT_MASK_ANYWHERE` in `patch-build/group.ts` and `patch-build/index.ts`), `packages/generator/src/mm/actors/Player.c`, `packages/generator/src/mm/z_parameter.c`, `packages/generator/src/common/dpad.c` (when the mask can be used), `packages/generator/src/link_mm.in` (game function addresses)

## 2026-10-11: One-click desktop app

- Where in the app: the whole app. Download it from https://github.com/StormEf4/OoTMM/releases/latest, and the General page shows its version as `custom-<commit>`
- What it does: every push to master builds the desktop app for Windows, Mac and Linux and publishes it as a release, so using the generator is just opening a file and dragging in the ROMs
- Files: `.github/workflows/my-app.yml` (build and release), `packages/desktop/neutralino.config.json` (window title "OoTMM Custom"), `CUSTOMIZING.md`

## 2026-10-11: Set up the local app and the Custom page

- Where in the app: the new "Custom" page in the left sidebar, with General, Ocarina of Time, Majora's Mask and Change Log tabs
- What it does: runs the ootmm.com generator locally with `pnpm local`, and gives every fork-only setting its own home so it is easy to find and to keep separate from upstream OoTMM
- Files: `packages/core/src/settings/custom.ts` (custom settings), `packages/gui/src/app/components/generator/GeneratorCustom.tsx` (Custom page), `packages/generator/lib/combo/confvars.ts` (automatic `CFG_CUSTOM_...` game flags), `scripts/custom.ts` (`pnpm custom:add` / `pnpm custom:log`), `CUSTOMIZING.md` (guide)
