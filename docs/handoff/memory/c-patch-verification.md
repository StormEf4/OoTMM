---
name: c-patch-verification
description: How to find/verify OoT/MM patch addresses and build the C patches in a cloud session with no ROMs
metadata:
  type: reference
  modified: 2026-10-24T03:47:02.427Z
---

No ROMs are available in cloud sessions, so in-game testing isn't possible. What works instead:

- **Addresses:** `git clone --depth 1` zeldaret/oot and zeldaret/mm (github.com clone works; codeload tarballs are blocked). Compile one file with IDO 7.1 (decompals/ido-static-recomp release `ido-7.1-recomp-linux.tar.gz`) using the decomp's own `make -n` flags (OoT `VERSION=ntsc-1.0`, MM `n64-us`). Disassemble with `mips-linux-gnu-objdump` and align offsets to addresses already in `link_oot.in` / existing `PATCH_START`s. MM also has `tools/disasm/n64-us/functions.txt` with every function's vram.
- **Building the C patches:** `apt-get install gcc-mips-linux-gnu binutils-mips-linux-gnu ninja-build`, `./scripts/deps.sh` (ultralib), `pnpm --filter @ootmm/tools build` (zovl). gcc 12 lacks the C23 `bool` keyword, so put a wrapper named `mips64-ultra-elf-gcc` first on PATH that adds `-std=gnu2x -Dbool=_Bool -Dtrue=1 -Dfalse=0` (symlink the binutils too). Then run `CI=1 pnpm build` in `packages/generator`.

The Magic Hookshot (PR #5, 2026-10-24) was verified this way. See [[fork-custom-items]].
