# Finding OoT 1.0 addresses without the ROM

OoTMM patches OoT 1.0 US. Hooking the middle of a function needs the exact address of an instruction, which normally means disassembling the ROM. Without a ROM, the code can be rebuilt from the decomp with the original compiler, and the result matches the ROM byte for byte. This is how the OoT Giant's Mask movement hooks ([giant-mask.md](giant-mask.md)) and the Fierce Deity work ([fierce-deity.md](fierce-deity.md)) were done.

## Ready-made outputs

The project's shared folder has the results and the scripts: `/mnt/project-files/notes/oot-player-map/` (see its README).

- `functions.txt`: every function of the player overlay (`ovl_player_actor`) with its 1.0 address and decomp name.
- `z_player.dis`: the whole player overlay disassembled at its real addresses, with the C source line above each instruction, relocations marked, and float constants decoded.
- `key_sites.txt`: gravity, jump, ledge, wall and floor code, already cut out.
- `data_symbols.txt`: static variables (for example `sAgeProperties` at 0x808512E8, `sYDistToFloor` at 0x808514C0).
- `z_actor/z_actor.dis`: `src/code/z_actor.c` (code segment) at its real addresses.

## How it was built

1. Sparse clone of https://github.com/zeldaret/oot (commit `ceeb6f3`) with `include`, `src/code`, `src/overlays` and the asset headers the file includes.
2. IDO 7.1 (static recomp v1.2, the one the decomp's `tools/Makefile` downloads) and the decomp's flags for `VERSION=ntsc-1.0`: `-O2 -g3 -mips2 -G 0`, `-DOOT_VERSION=NTSC_1_0 -DPLATFORM_N64=1 -DDEBUG_FEATURES=0 -DNDEBUG -DF3DEX_GBI_2`, plus the decomp's `preprocess_pragmas` step. **`-g3` matters**: without it IDO generates different code.
3. Player overlay: link at VRAM 0x808301C0 (`.text`). All 85 addresses OoTMM already patches in that overlay matched.
4. Code segment file: compile it alone and shift by one anchor function whose address is in `packages/generator/src/link_oot.in`. For `z_actor.c` the base is 0x8001EC20, and all 63 functions also in `link_oot.in` agree.

## Rules for hooking the player overlay

- The overlay is relocated when it is loaded. An instruction that carries a relocation (a `jal` to a function inside the overlay, a `lui`/`lw` with `%hi`/`%lo` of overlay data) is rewritten by the loader, so never replace it with a `jal` to our code. `z_player.dis` marks these (`; %hi(...)`, `; %lo -> ...`, `call X` without `[real]`). Calls to the code segment (`[real]`) have no relocation and can be swapped for a C function with the same signature (`jal` in a `PATCH_START`, or `PATCH_CALL` in C).
- The instruction after a `jal` (the delay slot) runs before the hook. Either keep it as it was, or replace both instructions with `jal hook; nop` and redo the second one in the hook.
- IDO keeps values in `at` between a `lui at` and the instruction that uses it; a hook placed in between must save and restore `at`.
- Put the hooks in a patch group so they are only in the ROM when the setting is on (`PATCH_GROUP PG_...` in the `.S`, the group in `patch-build/group.ts` and `patch-build/index.ts`).
- `packages/generator/src/oot/patches/giant_mask_oot.S` is a worked example of all of the above.
