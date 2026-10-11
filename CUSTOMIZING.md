# Customizing this fork

This fork runs the same generator as ootmm.com on your own computer, with a **Custom** page for your own settings.

## Use the app (easiest)

Download the zip for your computer from https://github.com/StormEf4/OoTMM/releases/latest, unzip it, and open the `ootmm` app inside. It opens straight to your generator, with the Custom page. Drag your ROMs in just like on ootmm.com. Keep `resources.neu` next to the app.

- Windows: `ootmm-win_x64.zip`. If Windows warns you, click "More info" then "Run anyway".
- Mac: `ootmm-mac_arm64.zip` (Apple Silicon) or `ootmm-mac_x64.zip` (Intel). The first time, right-click the app and choose Open.
- Linux: `ootmm-linux_x64.zip`.

Every change merged into `master` builds a new version automatically (`.github/workflows/my-app.yml`, in the repo's Actions tab). Download it again to get your latest changes. The General page shows the version as `custom-<commit>`.

## Run it from the code (for developing)

One-time setup (Linux, WSL2 on Windows, or the repo's DevContainer in VS Code):

1. Put your ROMs in `roms/`: `oot.z64` (OoT U or J 1.0) and `mm.z64` (MM U), both big-endian.
2. Run `./scripts/setup.sh` to install the toolchain and dependencies.

Then, any time:

| Command | What it does |
|---|---|
| `pnpm local` | Builds the game patches and opens the full app at the URL it prints (usually http://localhost:5173). You can generate seeds here exactly like on ootmm.com. |
| `pnpm local:ui` | Starts only the website, without building the patches. Fast, and needs no ROMs or toolchain, so it is handy for checking how a new setting looks. Generating a seed needs `pnpm local`. |

## Add a setting

Run `pnpm custom:add` and answer the questions: which game, the name, the description, and the kind of control (toggle, drop-down, number or multi-select). It then:

- adds the setting to `packages/core/src/settings/custom.ts`, so it appears on the Custom page under General, Ocarina of Time or Majora's Mask;
- adds an entry to `CUSTOM_CHANGES.md`;
- prints the game flag(s) the C code can check.

Restart `pnpm local` (or refresh `pnpm local:ui`) to see it. You can also edit `custom.ts` by hand; its comment at the top explains the format.

## Make the setting do something in the game

A new setting is only a control until code reads it. There are three places code can read it:

- **C patches** (`packages/generator/src/oot/`, `src/mm/`, `src/common/`): toggles and drop-downs on the Custom page are sent to the game automatically.
  - A toggle `fasterEpona` is `Config_Flag(CFG_CUSTOM_FASTER_EPONA)`.
  - A drop-down `eponaSpeed` set to `double` is `Config_Flag(CFG_CUSTOM_EPONA_SPEED_DOUBLE)`.
  - The game has room for about 65 more flags in total, and each drop-down option uses one. Numbers and multi-selects don't use any; read those from TypeScript.
- **Logic YAML** (`data/world/`, `data/macros/`): `setting(fasterEpona)` or `setting(eponaSpeed, double)`.
- **TypeScript generator or logic** (`packages/generator/lib/`, `packages/logic/src/`): `settings.fasterEpona`.

Hooking new C code into the game is the hard part; "Giant's Mask Anywhere" (`packages/generator/src/mm/giant_mask.c` and `src/mm/patches/giant_mask_anywhere.S`, see its `CUSTOM_CHANGES.md` entry) is a full worked example of a setting that changes the game. Assembly hooks that should only apply when a setting is on go in their own patch group (`patch-build/group.ts` and `patch-build/index.ts`).

## Log every change

Every change goes in `CUSTOM_CHANGES.md`, newest first, including where it shows up in the app. `pnpm custom:add` does this for new settings; for anything else run `pnpm custom:log`. The app shows the log under Custom > Change Log.

## Check your work

- `pnpm typecheck` checks the TypeScript.
- `pnpm test` runs the logic tests.
