# Handoff: Elijah's OoTMM fork

Written 2026-11-23 to hand this project to a new Claude account. Read this first, then `CLAUDE.md`, `CUSTOMIZING.md` and the top of `CUSTOM_CHANGES.md`. The old account's project memory is copied, unchanged, into `docs/handoff/memory/` so nothing is lost.

## 1. What this project is

- A personal fork of the OoTMM randomizer (Ocarina of Time + Majora's Mask combo). Repo: https://github.com/StormEf4/OoTMM-personal-fork, default branch `master`. Upstream is https://github.com/OoTMM/OoTMM (PR #1 was the last sync, 2026-10-11).
- It ships its own **desktop app** that is the same generator as ootmm.com plus a **Custom** page (fork settings) and a **Change Log** view (renders `CUSTOM_CHANGES.md`).
- Every merge to `master` builds the app for Windows, Mac and Linux via `.github/workflows/my-app.yml` and publishes a GitHub release (`app-<run>`, latest at https://github.com/StormEf4/OoTMM-personal-fork/releases/latest). The app is a Neutralino build (`packages/desktop`). The General page shows the version as `custom-<commit>`.
- Owner: Elijah (GitHub `StormEf4`). He tests seeds himself on an emulator (Rosalie's Mupen GUI; his RetroArch ParaLLEl install was broken and caused a false crash once, so rule out the emulator first).

## 2. Working rules (from CLAUDE.md and Elijah's preferences)

1. Log every change at the top of `CUSTOM_CHANGES.md` (below the marker comment): date, title, where it shows in the app, what it does, files changed. `pnpm custom:log` / `pnpm custom:add` write entries in the right format.
2. Every new behavior gets a control on the Custom page, defined in `packages/core/src/settings/custom.ts` with category `custom.general`, `custom.oot` or `custom.mm` (or an upstream category if it belongs next to upstream options, still in `custom.ts`). Never add fork settings to `data.ts`, to keep upstream merges clean.
3. Toggles and drop-downs become C flags automatically: `CFG_CUSTOM_<KEY>` and `CFG_CUSTOM_<KEY>_<VALUE>` (`packages/generator/lib/combo/confvars.ts`).
4. Run `pnpm typecheck` and `pnpm test` before pushing.
5. **Exception, cosmetics:** Elijah wants cosmetic options in the **Cosmetics** tab, not the Custom page. They live in `packages/core/src/cosmetics/custom.ts`.
6. New gameplay features are added as **separate items or options next to vanilla, never replacing vanilla**.
7. Work goes on a `claude/...` branch and lands by PR into `master`; Elijah merges.

## 3. Build and run

| Command | What it does |
|---|---|
| `./scripts/setup.sh` | One-time: toolchain deps (`scripts/deps.sh`) + `pnpm install`. |
| `pnpm local` | Builds the C patches and opens the full app (usually http://localhost:5173). Needs ROMs + toolchain. |
| `pnpm local:ui` | Website only, no patch build, no ROMs needed. Good for checking how a setting looks. |
| `pnpm typecheck` / `pnpm test` | TS checks / logic tests (includes `packages/logic/tests/seeds/pot-god.test.ts`). |
| `pnpm custom:add` / `pnpm custom:log` | Add a Custom setting / log a change. |

ROMs go in `roms/` as `oot.z64` (OoT U or J 1.0) and `mm.z64` (MM U), big-endian. `*.z64` is git-ignored. **Never commit ROMs.**

### Building and checking C patches in a cloud session (no ROMs)

Cloud sessions have no ROMs, so nothing can be tested in game; everything is compile-checked only.

- **Build the C patches:** `apt-get install gcc-mips-linux-gnu binutils-mips-linux-gnu ninja-build`, `./scripts/deps.sh` (ultralib), `pnpm --filter @ootmm/tools build` (zovl). gcc 12 lacks the C23 `bool` keyword, so put a wrapper named `mips64-ultra-elf-gcc` first on PATH that runs `mips-linux-gnu-gcc` with `-std=gnu2x -Dbool=_Bool -Dtrue=1 -Dfalse=0` (symlink the matching binutils names too). Then `CI=1 pnpm build` in `packages/generator`. gDP*/gSP* macros need braces inside if/else.
- **Find/verify addresses:** `git clone --depth 1` zeldaret/oot and zeldaret/mm (github.com clones work; codeload tarballs are blocked). Compile one file with IDO 7.1 (decompals/ido-static-recomp release `ido-7.1-recomp-linux.tar.gz`) using the decomp's own `make -n` flags (OoT `VERSION=ntsc-1.0`, MM `n64-us`). Disassemble with `mips-linux-gnu-objdump` and align offsets to addresses already in `link_oot.in` / existing `PATCH_START`s. MM also has `tools/disasm/n64-us/functions.txt` with every function's vram. More in `docs/custom/oot-addresses.md` and `docs/custom/cross-game-items.md`.

## 4. Architecture of the fork additions

| Area | Where |
|---|---|
| Fork settings (Custom page) | `packages/core/src/settings/custom.ts` |
| Fork cosmetics (Cosmetics tab) | `packages/core/src/cosmetics/custom.ts` (appended to upstream `COSMETICS`; `type: 'enum'` = drop-down). Game reads them through an `EXPORT_SYMBOL` byte set with `patchSymbol` in `packages/generator/lib/combo/cosmetics/index.ts`, no CFG flags. |
| Custom page + Change Log UI | `packages/gui` |
| Desktop app | `packages/desktop`, workflow `.github/workflows/my-app.yml` |
| C patches | `packages/generator/src/oot/`, `src/mm/`, `src/common/`; asm hooks in `src/*/patches/*.S`; setting-gated hook groups in `patch-build/group.ts` and `patch-build/index.ts` |
| Logic | YAML in `data/world/`, `data/macros/` (`setting(x)`), TS in `packages/logic/src/` (`settings.x`) |
| Reference notes | `docs/custom/*.md` (one per feature; listed in `CUSTOMIZING.md`) |

**Config flag budget:** `config[0x40]` (512 bits). After the 2026-11-25 upstream sync, upstream uses bits 0-337 and the fork's 25 custom flags 338-362, so 149 are left (upstream dropped its MM age requirement flags). Each drop-down option costs one flag; numbers and multi-selects cost none.

### Adding a new item (pattern, modeled on upstream's Shovel)

`gi.yml` entry with `add: [SAVE_FLAG, SAVE_FLAG_X]`, a new enum value in `save_flags.h` (shared saveFlags byte), the item id in logic `defs.ts`, a name in `names.ts`, `ItemGroups.REQUIRED` if major, and `transform.ts` `addItem` gated on the setting. Use Off/On/Logical enums for "can logic use it". Mode toggles reuse pause-menu slot cycling in `common/trade.c` (`comboToggleSlot`, `comboSlotCanToggle`). Shared (both-games) items: a `kSimpleSharedItems` row in `item_add.c` plus `SHARED_ITEMS_OOT`/`MM` rows in `randomizer/checks.ts`. Cross-game items: `docs/custom/cross-game-items.md`.

### Custom actors and the OoT boss kit

- Custom OoT actors go in unused actor ids (0x003 and 0x006 are taken by the Pot God; upstream's enemy shuffle preview also uses 0x003, so the enemy shuffle test branch moves the Pot God to 0x180, see `docs/custom/enemy-shuffle-preview.md`) as `src/oot/ovl/actors/<Name>/` with `OVL_INFO_ACTOR`; the CMake glob picks up a new dir, no TS wiring. Ids live in `include/combo/actor_id.h`.
- Objects not in the scene load into extra slots via the patched `Object_GetSlot`; draw from them with `BossKit_UseObject` (CPU + RSP segment 6).
- Runtime-spawned check actors: `g.xflagOverride = TRUE; g.xflagId = Xflag_Lookup(&xf)`. Hyrule Field Pot God pots use room 0, actor ids 0xc0-0xd3.
- Boss kit (`include/combo/boss_kit.h`, `src/oot/boss_kit.c`, doc `docs/custom/boss-kit.md`): arenas, cutscene camera, title cards, quakes, music, debris, health bar. Fight state is a non-saved static (`sFightId`), cleared on any non-arena scene init; dying then Continue reloads the arena so the fight retries. Arena scenes must skip the `CUSTOM_RESPAWN_MODE_DUNGEON_ENTRANCE` copy in `Play_AfterInit`. Arena actors are blocked in `canSpawnActor` (`src/oot/actors.c`); room actors spawn after `Play_AfterInit` so they can't be killed there. Heat off: `play->roomCtx.curRoom.behaviorType2 = 0`. Link's sink is visual (`shape.yOffset`); OoT csAction 37 is the Wallmaster "grabbed" pose; child voice clips = adult + 0x20. `NA_SE_VO_*` ids are missing from the OoT part of `sfx_id.h`: define locally, verify against zeldaret/oot `include/tables/sfx/*.h`.
- Custom songs: add to the `CustomOcarinaSong` enum and all parallel arrays in `src/oot/ocarina.c`. Songs match on buffer suffix (`comboCheckSong`), so check a new song against suffixes of every OoT and MM song.

## 5. Feature history (all merged unless noted)

| PR | Date | Feature | Setting(s) / doc |
|---|---|---|---|
| #1 | 10-11 | Sync with upstream (38 commits) | |
| #2 | 10-11/12 | Giant's Mask Anywhere (MM); OoT Giant's, Fierce Deity's, Deku/Goron/Zora transformation masks + shared options; FD sword beam fixes | `giantMaskAnywhere`, `giantMaskOot`, `fierceDeityMaskOot`, `dekuMaskOot`, `goron/zoraMmTransformMaskOot`, `sharedMask*`. Docs: giant-mask, fierce-deity, transform-masks |
| #3, #4 | 10-11 | Local app with Custom page and Change Log; one-click desktop app | `CUSTOMIZING.md` |
| #5, #6 | 10-24 | **Magic Hookshot**: separate hookshot-anywhere item, 2 magic per magic latch | `magicHookshotOot`, `magicHookshotMm` |
| #7 | 10-24 | **Infinite Magic**: Off / Progressive (3rd Magic Upgrade, via `progressive.c`) / Separate. Effect reuses Chateau Romani (OoT `chateauActive`, MM week event) set every frame from play.c, so every magic cost (incl. Magic Hookshot) is free | `infiniteMagic` (custom.general) |
| #8 | 10-24 | **Fairy Companion** cosmetic: Default / Talking Furbies / RGB Glow / both. Furby = flat 32x32 camera-facing sprite, "Navi"/"Tatl" text rewritten to "Furby", pink/purple idle glow. RGB = 3 s color cycle. `FAIRY_STYLE` byte, `src/common/fairy_companion.c`, `lib/combo/cosmetics/fairy.ts`. Sprite drawn by `docs/custom/tools/furby.py` (PIL); preview `docs/custom/furby-sprite.png`. Elijah plans more fairy styles | Cosmetics > Fairy Companion. Doc: fairy-cosmetics |
| #9 | 10-26 | **Pot God** mini boss in the child Market Pot House (break every pot) + 20 Hyrule Field pot checks; Reverse Saria's Song warp | `potGod`, `potGodSariaWarp`. Doc: pot-god |
| #10 | 10-27 | Pot God redesign (giant pot + four Dead Hand arms) | |
| #11 | 10-29 | Drag-down cutscene, **hellfire arena** (emptied Volvagia lair) and the **OoT boss kit** | `potGodArena`, `bossKitHealthBar`. Doc: boss-kit |
| #12 | 11-03 (merged 11-23) | **Fixes the Pot God crash** and replaces Reverse Saria's Song with a **Pot Room Warp Song** (A, C-Up, A, C-Up, C-Left, C-Right; `CUSTOM_SONG_POT_WARP`), because the reversed Saria notes were exactly the Song of Healing | `potGodWarpSong` |
| #13 | 11-23 | This handoff document | |

Nothing above has been tested in game by Claude (no ROMs). Elijah tests on his machine.

## 6. Known pitfalls

- **CSMC-patched display lists need segments 0x0A/0x0B.** PRs #9, #10 and #11 all crashed the moment the Pot God rose. Root cause: OoTMM's CSMC patches (`src/oot/objects/tsubo.S`, `box.S`, `kusa.S`) replace texture loads in vanilla pot/chest/grass DLs with `gsSPDisplayList(0x0A000000)` / `0x0B000000`; the vanilla actor sets those segments in `csmc*PreDraw`. Drawing object_tsubo `DL_0017C0` from a custom actor without them = RSP crash. Earlier suspects (extra object slots, Dead Hand skeleton, Kakera) were wrong. Fixed in PR #12: `BossKit_UseObject` calls `BossKit_PrepareTextures(play, objectId)` (empty DL default + plain look for patched objects) and the kit tracks slot to object in `BossKit_GetObjectSlot`. **Rule: before drawing any vanilla DL from a custom actor, grep `src/*/objects/*.S` for patches to that object.**
- **No ROMs in the cloud.** Verify addresses against the decomps (section 3); say clearly in PRs and the change log that a change is compile-checked only.
- **Emulator first.** A crash unrelated to the change may be the emulator (see section 1).
- **Song collisions.** Songs match on suffix; a new song must not equal the tail of any OoT or MM song.
- **Flag budget.** About 149 config flags left (section 4).
- **MM code space.** The MM payload is limited to 0x60000 bytes and only 3,896 bytes (3.8 KB) are left after the 2026-11-25 sync. See `docs/custom/upstream-sync.md`.
- **CRLF.** Some files keep CRLF endings (e.g. `mmAgeRequirements.tsx`, see commit f736a8c). Don't normalize them.
- **gcc 12 vs C23** `bool` (section 3 wrapper).

## 7. Open and unfinished work

1. **Pot God crash fix (PR #12) is merged but not yet confirmed in game.** Next step: Elijah plays the latest app release through the Pot God fight (Pot House and Hellfire Arena) and the Pot Room Warp Song, and reports anything broken.
2. **Nothing since PR #5 has been tested in game**: Magic Hookshot, Infinite Magic, Fairy Companion, Pot God, boss kit. Expect follow-up fixes after Elijah tests.
3. **More fairy styles** planned (Fairy Companion drop-down). "Hey! Listen!" voice is still vanilla for the Furby.
4. **Pot God extras:** no recorded "OH GOD!" voice (text box + vanilla screams), no kneeling animation. Only one arena exists (`BOSS_KIT_ARENA_HELLFIRE`); the kit is ready for more bosses.
5. **Transformation masks (OoT)**, from `docs/custom/transform-masks.md`: phases 3-5 not done (Zora fins / Deku bubble, Goron roll / pound, Zora swim / barrier, Deku hops, instruments); punches hit on Link's left; OoT sounds; leg IK uses OoT adult values; logic uses no form abilities.
6. **Fierce Deity (OoT)** unverified items and ideas: `docs/custom/fierce-deity.md` (segment 04 references, transformation flash, keeping the form across scenes).
7. Upstream was last synced on 2026-11-25 (upstream `b127286`). How to sync: `docs/custom/upstream-sync.md`.

## 8. What the new account needs besides the GitHub repo

From the project's shared files (copied to the `handoff/` folder Elijah received):

| File | Why | Required? |
|---|---|---|
| `HANDOFF.md` | This document (also in the repo root) | Yes, read first |
| `memory/c-patch-verification.md`, `memory/fork-custom-items.md`, `memory/fork-cosmetics.md`, `memory/fork-pot-god.md` | The old account's project memory, verbatim. Also in the repo at `docs/handoff/memory/`. Load them into the new project's memory. | Yes |
| `fairy-companion/furby-sprite.png` | Furby sprite preview. A copy is already in the repo as `docs/custom/furby-sprite.png` and it can be regenerated with `docs/custom/tools/furby.py`. | Optional |

Elijah must supply himself (never commit these):

- **ROMs**: `oot.z64` (OoT U or J 1.0) and `mm.z64` (MM U), big-endian, in `roms/`. Only needed to build/play locally; cloud sessions work without them.
- **GitHub access** for the new Claude account: connect GitHub on the new account and install the Claude GitHub App on `StormEf4/OoTMM-personal-fork`, so it can push branches and open PRs.
- **Repo secrets**: none are needed for the fork's app build (`my-app.yml` only uses the built-in `GITHUB_TOKEN`). The upstream `ci.yml`/`cleanup.yml` reference Netlify and AWS secrets for upstream's website deploy; the fork doesn't need them and they should not be added to code.
- **Project instructions** for the new Claude project: point it at this file, e.g. "Read HANDOFF.md, CLAUDE.md and CUSTOMIZING.md first."
