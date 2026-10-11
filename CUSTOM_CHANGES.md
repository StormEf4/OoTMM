# Custom Changes

Every change made in this fork, newest first. Each entry says where the change shows up in the app, so this file doubles as a reference for future additions. `pnpm custom:add` and `pnpm custom:log` write entries here automatically, and the app shows this file under Custom > Change Log.

<!-- new entries go below this line -->

## 2026-10-11: Set up the local app and the Custom page

- Where in the app: the new "Custom" page in the left sidebar, with General, Ocarina of Time, Majora's Mask and Change Log tabs
- What it does: runs the ootmm.com generator locally with `pnpm local`, and gives every fork-only setting its own home so it is easy to find and to keep separate from upstream OoTMM
- Files: `packages/core/src/settings/custom.ts` (custom settings), `packages/gui/src/app/components/generator/GeneratorCustom.tsx` (Custom page), `packages/generator/lib/combo/confvars.ts` (automatic `CFG_CUSTOM_...` game flags), `scripts/custom.ts` (`pnpm custom:add` / `pnpm custom:log`), `CUSTOMIZING.md` (guide)
