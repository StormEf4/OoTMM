# Multiplayer with this fork

The fork works with the official OoTMM multiplayer client (https://github.com/OoTMM/multiplayer) and its server (`multi.ootmm.com`), for both Multiworld and Co-op. The fork never changed the multiplayer code or the patch file format, so no fork build of the client or server is needed.

This was checked on 2026-11-24 by reading the code and generating 12 multiworld and co-op seeds with every fork setting on. It has not been played online yet.

## How to play

1. **Everyone uses Project64-EM** (https://github.com/OoTMM/Project64-EM/releases/latest) on Windows, plus the OoTMM client ("client" package from https://github.com/OoTMM/multiplayer/releases/latest). The game only talks to the client through Project64-EM's IPC registers (`src/common/ipc/project64.c`), so other emulators such as Rosalie's Mupen GUI can't connect.
2. **One person generates the seed in this fork's app**, with General > Mode set to Multiworld or Co-op. Multiworld (and any seed with more than one team) gives one `.ootmm` patch file per world.
3. **Everyone builds their ROM in this fork's app**: General > Patch mode, load their `.ootmm` file and their ROMs. Don't use ootmm.com for this. It accepts the patch today (same patch version), but it drops the fork's Fairy Companion cosmetic, and a later upstream change could make it build a bad ROM.
4. Start the client by opening the `.ootmm` file with it, then open the ROM in Project64-EM.

Don't mix patch files from different app versions in one session. The fork adds item ids in the middle of `data/defs/gi.yml`, so item numbers differ between builds (every MM item is 7 higher than upstream). Inside one generated session they always match.

## Why it works

- The game sends items to the server as numbers with a 32-bit check key (`src/common/multi/multi.c`). The client and the server only relay them. They name items and checks for auto-trackers from `manifests/items.json` and `manifests/locations.json`, which the generating app packs into every patch file, so the fork's items and checks are included.
- The 9 fork items (OoT Giant's, Fierce Deity's, Deku and MM Goron/Zora masks, Magic Hookshot and Infinite Magic in both games) are added through the normal item handlers in both games. Progressive Infinite Magic is resolved by whoever receives it.
- The 20 Hyrule Field Pot God pots are normal xflag checks, so items in them are sent to their owner in multiworld, and in co-op a partner's pickup marks the pot collected.
- The Pot God and the boss kit never set boss or dungeon flags and never send boss events, so co-op partners are never told that Volvagia was beaten. Beating the Pot God is per save: in co-op, each player fights it for their own field pots, and the pot items are shared as usual.
- While Link is transforming, in a Pot God cutscene or warping with the Pot Room Warp Song, incoming items wait until he is free, like upstream's cutscenes.

## Fork changes made for multiplayer (2026-11-24)

- `Custom_Pot_God.c`: if the arena is reloaded after the Pot God was beaten (Link voided out during its death, for example from another player's trap), it doesn't fight again and sends Link back to the Pot House. Its death text and the arena exit wait for a received item's or boss event's text box.
- `multi_pos.c`: players in a boss kit arena get their own wisp room key (`0x1f0 + fight id`), so they don't show up as wisps in the real Volvagia fight.
- Upstream's 2026-11-12 auto-tracker fixes 70f41036 and 9ba17638 are cherry-picked. Upstream's e24c3176 (entrance info) needs upstream's entrance refactor and comes with the next full sync.

## Known limits

- The Pot God and its 20 field pots are not tested in game. In multiworld another player's item can be in one of those pots, so until the fight has been played, turn Pot God off for multiworld or add the 20 "Hyrule Field Pot God Pot NN" locations to Junk Locations.
- External auto-trackers don't know the fork's item names (`OOT_MASK_GIANT`, `OOT_MAGIC_INFINITE`, ...), so they skip or show them as unknown.
- The auto-tracker reports the Pot God arena as Volvagia's boss room (`OOT_BOSS_TEMPLE_FIRE`).
