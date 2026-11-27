# Rupee Sabotage Shop (1v1)

A pause menu shop where a player spends rupees to send traps, enemies and other bad effects to the other player. Made for 1v1 races, and it works with any number of players in one session.

Settings: Custom > Versus (1v1) (`packages/core/src/settings/custom.ts`, category `custom.versus`):

| Setting | Key | What it does |
| --- | --- | --- |
| Rupee Sabotage Shop | `sabotageShop` | Turns the shop on (C flag `CFG_CUSTOM_SABOTAGE_SHOP`) |
| Sabotage Effects | `sabotageEffects` | Which effects the shop sells (all by default) |
| Sabotage Prices (%) | `sabotagePrices` | Scales every price, 100 = the prices below |
| Sabotage Cooldown (seconds) | `sabotageCooldown` | Wait after a purchase before the next one, 30 by default |

Not tested in game or online yet (2026-11-27). Compile-checked, and seeds generated with the ROMs for Singleplayer, Multiworld and Patch mode.

## How to play

1. Everyone uses Project64-EM and the official OoTMM client, like any multiplayer seed (see `multiplayer.md`). Rosalie's Mupen GUI can't connect, so the shop says "Client not connected" there.
2. One person generates the seed in this fork's app with the shop on.
   - **Singleplayer race (1v1 on the same seed):** General > Mode stays Singleplayer. Send the `.ootmm` patch file to the other player, and they build their own ROM from it in this app (General > Patch mode). Both open the same `.ootmm` file with the client, so both land in the same session. With the shop on, the patch is marked `coop` (`versusPatchMode` in `lib/combo/randomizer/custom-versus.ts`) because the client only connects for multiplayer patches. The seed itself stays a normal Singleplayer seed: items are not shared, only sabotages are.
   - **Multiworld:** set Mode to Multiworld as usual. Sabotages go to everyone in the session except the buyer.
3. In game: pause, go to the Map page, and press L or C-Up. The first info page is now the "Sabotage Shop" (press again for upstream's pages). Move with the stick and press A to buy. The price comes out of the wallet and the effect is sent.
4. The other player gets a text box ("<name> sabotaged you: Ice Trap!") as soon as they can receive an item, then the effect hits.

Each player should build their own ROM. Two players on one ROM file with the same file name look like the same sender, and each would ignore the other's sabotages.

## Effects

Prices are at 100%; the Prices setting scales them (capped at 999, and a wallet holds at most 500).

| Effect | Price | What happens |
| --- | --- | --- |
| Ice Trap | 30 | Upstream's ice trap |
| Fire Trap | 40 | Upstream's fire trap |
| Shock Trap | 40 | Upstream's shock trap |
| Knockback Trap | 20 | Upstream's knockback trap |
| Anti-Magic Trap | 30 | Upstream's anti-magic trap |
| Rupee Thief | 30 | Five Rupoor traps |
| Bomb Drop | 50 | Three lit bombs around Link |
| Reverse Controls | 60 | The control stick is flipped for 20 seconds (stacks) |
| Enemy Ambush | 80 | A Wallmaster or three flying Keese next to Link, picked at random. If neither can spawn (a cleared room, the enemy's soul not found, or in MM a scene that doesn't have their models), it drops bombs instead |
| Drain Trap | 99 | Upstream's drain trap |

Trap effects add to `gSharedCustomSave.traps[]`, so upstream's `Play_UpdateTraps` (`src/common/traps.c`) applies them when Link is free, like a trap item. The other three are run by `Sabotage_Update` once Link is free and not climbing, dying or talking, one per frame.

## How it works

Code: `src/common/sabotage.c` (shop page, receive, effects), `include/combo/sabotage.h`, hooks in `src/common/menu.c` and `src/common/multi/multi.c`, generator side in `lib/combo/randomizer/custom-versus.ts`.

- **Menu:** the shop is `MENU_SABOTAGE` (0x40), outside upstream's L/C-Up page numbers. `comboMenuNext` asks `Sabotage_MenuNext` first, so the cycle is shop, then upstream's pages, then back to the map. `comboMenuUpdate` and `comboMenuDraw` hand that page to the shop. Eight rows show at a time and the list scrolls.
- **Settings in the ROM:** `sabotageShop` is a C flag. The effect set, price scale and cooldown are four u16s written to the `SABOTAGE_CONFIG` symbol (`gSabotageConfig`) by `patchCustomVersus`.
- **Sending:** `Multi_SendSabotage` sends a WAL item entry through `Multi_ReliableSendWAL`, the same path as a multiworld item. It goes out even when `CFG_MULTIPLAYER` is off, and the shop flag also turns on `Multi_Resend` so a lost packet is sent again. It refuses (and the shop says "Client not connected") when the client isn't connected or another packet is still waiting.
- **Packet:** `to = SABOTAGE_TO` (0xFE), `gi` = the effect number, `flags = OVF_RENEW`, `key = 0x5A << 24 | Multi_SabotageId()`. 0xFE is not a real world (worlds are 1 to 127, 0 is "self", 0xFF is "everyone"), so a build without the shop never gives an item for it. `OVF_RENEW` makes the client add a random nonce, so the server keeps every purchase instead of merging identical ones. The server broadcasts every entry to the whole session.
- **Who sent it:** `Multi_SabotageId` mixes the ROM's random player id with the save file's name into 24 bits. The receiver drops entries whose world and id are its own (its own purchase coming back) and shows the sender's player name from the client, or "Your rival".
- **Receiving:** `MultiProcessMessageItemWAL` hands every `to == 0xFE` entry to `Sabotage_Receive`. It waits (returns 0 so the client sends it again later) while the game is paused, a text box or cutscene is up (`Item_SafeToReceive`), or decoys are active.

## Known limits

- Not tested in game or online.
- On a Singleplayer seed with the shop on, the players see each other as wisps (the client connects them as a co-op session).
- MM can only spawn enemies whose models the scene already has, so in MM the Enemy Ambush mostly turns into a Bomb Drop. OoT loads the models when needed.
- The cooldown and Reverse Controls timers are not saved: they reset when the game is reset.
- Code space: the shop adds about 3.6 KB to both games (gcc 12 measure). MM had 5,832 bytes free in the app build before it (CI run 63), so roughly 2.2 KB is left. See `upstream-sync.md`.
