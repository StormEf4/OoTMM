# Fork rules

This is a personal fork of OoTMM. Read `CUSTOMIZING.md` first. For every change:

1. **Log it** in `CUSTOM_CHANGES.md`: add an entry at the top (below the marker comment) with the date, a title, where it shows in the app, what it does, and the files changed. `pnpm custom:log` and `pnpm custom:add` write entries in the right format.
2. **Expose it in the app.** Every new behavior gets a control (toggle, drop-down, number or multi-select) on the Custom page, defined in `packages/core/src/settings/custom.ts` with category `custom.general`, `custom.oot` or `custom.mm`. Do not add fork settings to `data.ts`; keeping them in `custom.ts` keeps upstream merges clean.
3. Toggles and drop-downs in `custom.ts` become C flags automatically (`CFG_CUSTOM_<KEY>` and `CFG_CUSTOM_<KEY>_<VALUE>`), see `packages/generator/lib/combo/confvars.ts`.
4. Run `pnpm typecheck` and `pnpm test` before pushing.
