# Syncing with upstream OoTMM

How to bring upstream OoTMM (https://github.com/OoTMM/OoTMM) into this fork without changing the fork's custom features, and the spots that conflict every time.

## Steps

1. `git remote add upstream https://github.com/OoTMM/OoTMM.git` and `git fetch upstream master`.
2. On a `claude/...` branch: `git merge upstream/master` (a merge, not a rebase, so the fork's history stays as it is).
3. Resolve conflicts with the rules below. When upstream rewrote code a custom feature hooks into, rewrite the fork's part on top of upstream's new code so the custom feature behaves the same.
4. Look for upstream features that overlap a custom one (same item, same setting, same hook) even where git found no conflict, and list them for Elijah in the PR. He wants to hear about every one.
5. `pnpm install`, `pnpm --filter @ootmm/data build`, `pnpm typecheck`, `pnpm test`.
6. Build the C patches with the ROMs (this regenerates `include/combo/custom.h` and `custom_animations.h`) and generate seeds with the custom settings on (see "Code space" below).
7. Log the sync in `CUSTOM_CHANGES.md` with the upstream commit it synced to.

## Rules that keep upstream's numbers stable

The fork always appends after upstream, so upstream's ids never move and the next merge stays simple:

- **OoT item ids** (`include/combo/data/items.h`): the fork's items come after upstream's last custom item. When upstream adds one, give upstream's item its upstream number and move the fork's items up. Add a `0` row for the new upstream item to `sCustomItemActions` in `src/oot/actors/Player.c` so the fork's rows still line up.
- **Extended mask drawing** (`comboDrawExtendedMask`, `src/oot/actors/Player.c`): upstream's masks are in `sExtendedMaskDraws`; the fork's masks are in `kForkMaskCallbacks`, drawn for `link->mask` values after upstream's table. The fork's OoT mask values in `include/combo/mask.h` (`MASK_GIANT` 0x0c and up) must follow right after upstream's last mask, in the order of `kForkMaskCallbacks`. If upstream adds a mask, give it upstream's value and move the fork's mask values up.
- **Pause menu item names** (`KaleidoScope_LoadItemName`, `src/oot/ovl_patch/kaleido_scope.c`): the fork's masks are extra `case`s in upstream's switch.
- **Custom objects**: `CustomObjectsBuilder.buildFork()` runs after all of upstream's objects.
- **Link animations** (`data/defs/link-animations.yml`): upstream's new entries go before the fork's block.
- **Config flags**: custom flags start at `CONFVARS.length` (`lib/combo/confvars.ts`), never at the number of names, because upstream's list can repeat a name.
- **GI numbers** (`data/defs/gi.yml`): both sides insert in the middle. That's fine within one build, but patches from different builds don't mix (see `multiplayer.md`).
- **Generated headers** (`include/combo/custom.h`, `custom_animations.h`): take upstream's version in the merge, then rebuild with the ROMs to regenerate them with the fork's entries.

## Code space

The game code (the "payload") has a fixed size per game: 0x80000 bytes for OoT and 0x60000 for MM (`lib/combo/patch-build/index.ts`). Over it, every seed fails with "Payload too large". Since 2026-11 the app build checks this and prints how much room is left (`lib/build/payload-size.ts`, look for `Payload mm:` in the "Build GUI" step of the app build log). Upstream's enemy shuffle (not in upstream `master` yet) raises MM to 0x80000, and the fork's test branch for it raises OoT to 0x90000: see `enemy-shuffle-preview.md` before syncing a version of upstream that contains it.

- The cloud's gcc 12 wrapper makes code about 1.6% bigger than the real toolchain (`ghcr.io/ootmm/toolchain`). Measured on build 48: MM 375,936 bytes with gcc 12, 369,904 in the released app. So a cloud build that is a little over the limit can still fit in the app.
- To generate a test seed in the cloud when the cloud build is over, raise the limit in `buildPatchfiles` locally for the test only and never commit that.
- After the 2026-11 sync, MM has only 3,896 bytes (3.8 KB) left with the real toolchain (app build of PR #20: MM 389,320 / 393,216, OoT 405,824 / 524,288). MM code is the fork's tightest spot: shrink something before adding much more MM (or common) code.
