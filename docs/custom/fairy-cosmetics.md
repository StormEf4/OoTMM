# Fairy cosmetics (Navi and Tatl)

Cosmetics > "Fairy Companion" picks a style for Navi (OoT) and Tatl (MM). It is a cosmetic, so it never changes the seed.

## Where things live

- The drop-down: `packages/core/src/cosmetics/custom.ts` (`fairyStyle`). Fork cosmetics live there and are appended to upstream's `COSMETICS` list, the same way fork settings live in `settings/custom.ts`. `type: 'enum'` cosmetics are drop-downs (`values` + `default`).
- What each style does at ROM build time: `packages/generator/lib/combo/cosmetics/fairy.ts`, called from the cosmetics pass in `cosmetics/index.ts`. It writes the `FAIRY_STYLE` byte (bit 0 Furby sprite, bit 1 RGB glow) for the game code.
- What each style does in game: `packages/generator/src/common/fairy_companion.c`. `EnElf_InitWrapper` (`src/common/actors/En_Elf.c`) calls `FairyCompanion_Init`, which swaps Navi's/Tatl's (fairy type 0 in both games) draw for `FairyCompanion_Draw` when a style is on.
- To add a style: add a value to `fairyStyle` in `custom.ts`, a bit in `FAIRY_STYLE_FLAGS` (`fairy.ts`) and `fairy_companion.c`, and handle it in `FairyCompanion_Draw`.

## What a style can change

| Part | How | Status |
|---|---|---|
| Name in dialogue | Rewrite every message in `oot/nes_message_data_static` / `mm/message_data_static` and update the message table in `code` | Done (`renameInMessages`) |
| Idle glow | Entry 2 (`ACTORCAT_PLAYER`) of `sAttentionColors` in `code`: inner RGBA, then outer RGBA | Done (`patchAttentionColor`) |
| Glow while targeting | The other `sAttentionColors` entries. Changing them also changes the lock-on arrow and reticle colors | Not used, so the targeting hints keep working |
| Any glow color, every frame | `FairyCompanion_Draw` sets the actor's `innerColor`/`outerColor` before drawing; the light the fairy gives off follows them | RGB Glow (`FairyCompanion_RgbColors`) |
| Look | `FairyCompanion_Draw` runs the vanilla draw first (it decides when the fairy is hidden: first person, some cutscenes, Giant's Mask in MM), rewinds the XLU display list to throw that away, then draws two camera-facing quads at the actor's position: a glow (32x32 I8 texture, lerp from outer to inner color, same pulse and fade as vanilla) and the Furby (32x32 RGBA16). Like vanilla, no depth test when `fairyFlags & 4`, so she shows through walls | Talking Furbies. The sprite is drawn by `docs/custom/tools/furby.py` (preview: `docs/custom/furby-sprite.png`); rerun it to tweak the art: `python3 docs/custom/tools/furby.py preview.png preview_big.png packages/generator/src/common/fairy_furby_tex.rgba16.inc packages/generator/src/common/fairy_glow_tex.i8.inc`. Sizes: glow 0.14, Furby 0.07 (of a 200 unit quad) |
| 3D model | A real model would need a zobj built in a 3D tool | Not done |
| Voice | "Hey! Listen!" and Tatl's calls are sound effects (`NA_SE_VO_NAVY_*` in OoT, `NA_SE_VO_NA_*` / Tatl voice in MM). New voices mean new audio samples in the sound banks | Not done |

## Details

- Nothing uses fixed addresses. The message table is found by scanning `code` for a list of 8-byte entries (u16 id, u8 type/pos, pad, u32 address in segment 0x07 for OoT or 0x08 for MM) that starts at the beginning of the message file, only moves forward, stays inside it, ends with id 0xFFFF, and whose last message is at the end of the file. In OoT that picks the English table over the Japanese and credits ones. The colors are found by matching the whole vanilla 13-entry table. Anything not found is skipped with a warning.
- Message formats (control codes with argument bytes are skipped, so an argument byte never matches a name):
  - OoT: end `0x02`; arguments: `05` 1, `06` 1, `07` 2, `0C` 1, `0E` 1, `11` 2, `12` 2, `13` 1, `14` 1, `15` 3, `1E` 1.
  - MM: 11-byte header first, end `0xBF`; arguments: `14` 1, `1B`-`1F` 2.
- The name is replaced as a whole word ("Navi's" yes, "Navigate" no). Changed messages are padded to 4 bytes. The rename makes the message file a little bigger; if it would run into the next file's ROM addresses, the rename is skipped.
- "Furby" is one letter longer than "Navi"/"Tatl". Lines are broken by hand in vanilla text, so a full line can come out one letter wider.
