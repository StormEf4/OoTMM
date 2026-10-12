# MM transformation masks in OoT: how they work

Reference for future changes. Settings (Items > Extended Items > Cross-Game Masks):

| item | OoT setting | shared setting | item id | trade bit | mask id |
|---|---|---|---|---|---|
| Deku Mask | `dekuMaskOot` | `sharedMaskDeku` | `OOT_MASK_DEKU` (0xAB) | `XITEM_OOT_CHILD_MASK_DEKU` 0x11 | `MASK_DEKU` 0x0E |
| MM Goron Transformation Mask | `goronMmTransformMaskOot` | `sharedMaskGoronMmTransform` | `OOT_MASK_GORON_MM_TRANSFORM` (0xAC) | 0x12 | `MASK_GORON_MM_TRANSFORM` 0x0F |
| MM Zora Transformation Mask | `zoraMmTransformMaskOot` | `sharedMaskZoraMmTransform` | `OOT_MASK_ZORA_MM_TRANSFORM` (0xAD) | 0x13 | `MASK_ZORA_MM_TRANSFORM` 0x10 |

- OoT's own Goron Mask and Zora Mask (Happy Mask Shop, cosmetic) are unchanged. The MM ones are separate items with "MM ... Transformation Mask" names everywhere (app, spoiler log, in game, code), so the two are never confused.
- The fork's Shared MM Goron/Zora option is hidden while upstream's Shared Goron/Zora Mask is on: both replace `MM_MASK_GORON` / `MM_MASK_ZORA` in the pool. `transform.ts` checks both settings too.
- The item wiring follows [cross-game-items.md](cross-game-items.md). The transformation reuses the Fierce Deity approach ([fierce-deity.md](fierce-deity.md)).
- Code: `src/oot/transform_mask.c`, `include/combo/oot/transform_mask.h`, `src/oot/patches/transform_mask_oot.S`.
- Full research notes (MM decomp line references, every DL offset, all hook sites, the phased plan): shared folder `notes/transform-masks-research.md`.

## Behavior

- Put on from the child trade slot (C button or D-pad up). Link must be on the ground, not in water, not crawling, not riding Epona, and not in a cutscene. Putting on and taking off freeze Link for 20 frames; he swaps at frame 10.
- Using another form's mask while transformed switches straight to that form, like in MM. Using the Fierce Deity's Mask turns Link back first, then into Fierce Deity, and the other way round.
- The form stays on through loading zones, like in MM (if the mask is still owned). Fierce Deity and the Giant's Mask still come off when leaving the area.
- Items, like MM's `gPlayerFormItemRestrictions`: ocarina, lens, bottles, trade items (not the regular masks) and the transformation masks. Deku also has Deku Nuts, Goron the Powder Keg.
- B button:
  - Goron and Zora always punch, even with no sword: a sword is lent on B during the player update, like Fierce Deity, and drawn as a fist. Reach is 2000 (Goron) and 2500 (Zora) instead of 3000 / 4000.
  - Goron hits as `DMG_HAMMER_SWING` 2 (jump attack `DMG_HAMMER_JUMP` 4), so Goron punches break what the hammer breaks. Zora hits as `DMG_SLASH_MASTER` 1 (spin `DMG_SPIN_MASTER` 2, jump `DMG_JUMP_MASTER` 2). MM's form damage bits mean something else in OoT and must never be copied (table in the research notes).
  - Goron and Zora punch with MM's three-hit combo and MM's animations (see Attacks below). They can't charge a spin attack.
  - Deku has no sword: B does nothing yet (no error sound). The Deku attack is the spin on A.
- Size and movement: MM's age properties and boot data for each form (below). Actor scale stays 0.01, the size comes from the model, like in MM. Heights: Deku 36, Goron 80, Zora 68.
- Rules:
  - Goron counts as wearing the Goron Tunic (hot rooms, hot floors), Zora as wearing the Zora Tunic (no drowning timer), and Zora's underwater timer stays at 0.
  - Goron and Deku can't swim: in deep water they sink, and after 20 frames void out to the last entrance. Within 60 frames of entering an area they take the mask off instead, so an entrance in water can't loop void outs.
  - Deku and Zora void out when set on fire.
  - Goron and Zora can't enter crawlspaces (child Link wearing them would otherwise crawl). Deku can.
  - Forms don't wear Link's tunic, boots or shield: they are swapped for the form's during the player update and put back after.

## Attacks (phase 2)

| form | input | what happens | hits as |
|---|---|---|---|
| Goron | B | MM punch combo: left (`pg_punchA`), right (`pg_punchB`), finisher (`pg_punchC`) | `DMG_HAMMER_SWING` 2 |
| Goron | Z-target + A forward, B in the air | roll / nothing (MM has no Goron jump attack) | |
| Zora | B | MM punch combo: `pz_attackA`, `pz_attackB`, kick `pz_attackC` | `DMG_SLASH_MASTER` 1 |
| Zora | B in the air, Z-target + A forward (sword drawn) | MM jump kick `pz_jumpAT`, landing `pz_jumpATend`; 1.1x forward and 0.9x up speed like MM's `func_808395F0` | `DMG_JUMP_MASTER` 2 |
| Deku | A (when A has nothing else to do), Z-target + A forward | MM spin (`pn_attack`): 3 turns, 20000 binary angle per frame slowing by 800, steerable, about 15 frames | `DMG_SLASH_KOKIRI` 1, cylinder r30 h40 |

How it works:
- **Animations:** the 22 MM animations (`pg_punch*`, `pz_attack*`, `pz_jumpAT*`, `pz_wait`, `pn_attack`) are in `data/defs/link-animations.yml` and streamed from MM's `link_animetion` like the Kamaro dance (`src/common/animations.c`). `include/combo/custom_animations.h` is generated by `lib/combo/custom/custom-animation-builder.ts` when the ROMs are present, and is committed; new entries were added to it by hand in the same format (frame counts from MM's `link_animetion.xml`). The builder checks each `frame_data_offset` against the MM header when it runs with ROMs.
- **Punches and jump kick reuse OoT's sword attack.** OoT's `func_80837948` plays `D_80854190[meleeWeaponAnimation]` (attack table at 1.0 `0x80852050`, entries `{start, end, end when locked on, active start frame, active end frame}`), and `Player_Action_808502D0` reads the active frames from it every frame. While a Goron or Zora updates, `TransformOot_SwapAttackTable` writes MM's entries (`sMeleeAttackAnimInfo`, MM z_player.c) into slots 0, 4, 8 (punches) and 17, 19 (Zora jump kick start and landing), then `TransformOot_RestoreAttackTable` puts OoT's back after the update. The active frames are MM's, since the punch animations are longer than OoT's slashes.
- **Combo order:** MM picks left, right, finisher (`func_808335F4`); OoT picks the slash from the stick and does its third-slash variant (+2) on the third same slash. `TransformOot_GetMeleeWeaponAnim` returns slot 0, 4 or 8 in MM's order while the combo is open (`unk_845 != 0`), and `TransformOot_StartMeleeAttack` clears `unk_845` so the +2 never happens and keeps the combo open for the punch's length + 4 frames (`unk_844`, MM's `unk_ADC`; OoT uses 8).
- **Queued punches:** MM remembers a B press during a punch and chains the next punch when it ends (`Player_Action_84`). OoT only reads B on the frame the animation ends, so `TransformOot_QueuePunch` adds B to the controller's pressed buttons on that frame (only during the player update, removed right after). The finisher can't be chained, like in MM.
- **Deku spin** is its own player action, `TransformOot_DekuSpinAction` (MM `Player_Action_95`), set with `Player_SetupAction` like the Kamaro dance. It owns a cylinder collider set as AT every frame, like the Fierce Deity beams. It ends in OoT's idle (`func_80839F90`).

Redirected calls (all OoT 1.0, relocations zeroed in `transform_mask_oot.S`):

| call site (jal) | original | reloc entry | wrapper |
|---|---|---|---|
| 0x8084E090 (`Player_ActionHandler_7`) | `func_80837818` (pick the slash) | 0x80855E7C | `TransformOot_GetMeleeWeaponAnim` |
| 0x80839B68, 0x80842C6C, 0x80842D44, 0x8084E0A4 | `func_80837948` (start an attack) | 0x8085405C, 0x80854EB4, 0x80854EE4, 0x80855E80 | `TransformOot_StartMeleeAttack` |
| 0x8083A6CC (`Player_ActionHandler_8`) | `func_808377DC` (start a spin charge) | 0x80854198 | `TransformOot_StartSpinCharge` |
| 0x80839CA8 (`func_8083BBA0`), 0x80839FCC (`Player_ActionHandler_10`) | `func_8083BA90` (jump attack) | 0x80854090, 0x808540F0 | `TransformOot_StartJumpAttack` |
| 0x80842324 (`Player_Action_8084411C`, in the air) | `func_8083BBA0` (B in the air) | 0x80854DC8 | `TransformOot_TryAirAttack` |
| 0x8083A314 (`Player_ActionHandler_Roll`) | `Player_TryRoll` | 0x80854148 | `TransformOot_TryRoll` |
| 0x80839FE0 (`Player_ActionHandler_10`, no sword) | `Player_SetupRoll` | 0x808540F4 | `TransformOot_SetupLockOnRoll` |

## Per-form data (`kForms` in `transform_mask.c`)

| | Deku | Goron | Zora |
|---|---|---|---|
| MM body object | object_link_nuts 0x154 | object_link_goron 0x14C | object_link_zora 0x14D |
| skeleton header | 0x0600BA24 | 0x06017A84 | 0x06012C34 |
| face mask object / DL | object_mask_nuts 0x1E3 / 0x0A001D90 | object_mask_goron 0x1E1 / 0x0A0014A0 | object_mask_zora 0x1E2 / 0x0A000DB0 |
| OoT age properties base | child | adult | adult |
| root scale (`unk_08`) | 0.3 | 0.74 | 1.0 |
| waist / sheath | 0x2C20 / 0x35F0 | 0x8C00 / none | 0xAB40 / none |
| left hand open / closed / bottle | 0x3BC0 (all) | 0xA500 / 0xDE68 / 0x11468 | 0xFDF0 / 0xC270 / 0x10D08 |
| right hand open / closed | 0x38C0 (both) | 0x9EF8 / 0xE7E8 | 0xFBB8 / 0xBBE0 |
| eyes (seg 08) / mouth (seg 09) | none | 4 at 0x0, 0x800, 0x1000, 0x1800 / none | 8 at 0x0..0x3800 / 4 at 0x4000..0x4C00 |
| boot REGs that differ from OoT adult Kokiri | REG(38) 1000 | REG(37) 700, REG(38) 200, REG(68) -140, REG(48) 200 | REG(37) 700, REG(38) 300 |

Age property floats (OoT `PlayerAgeProperties` order, values from MM's `sPlayerAgeProperties`) are in the table in the code.

## How it's drawn

Same as Fierce Deity: `Player_DrawFlexLod` (`Player.c`) calls `TransformOot_DrawSkeleton` after `FierceDeityOot_DrawSkeleton`. It swaps segment 6 to the form object, forces `gSave.age = AGE_ADULT` while drawing, sets the eyes and mouth (segments 08/09) from the form object using OoT's face index (`jointTable[22]`, or the blink table), and draws with `TransformOot_OverrideLimbDraw`:
- the root position is scaled by the form's `unk_08` before the vanilla override (MM `Player_OverrideLimbDrawGameplayCommon`);
- after the vanilla override, hands come from `leftHandType` / `rightHandType`, waist and sheath from the table, other limbs keep the form skeleton's own DL; nothing in first person.

The mask draw callbacks (`kMaskCallbacks` in `Player.c`, entries for mask ids 0x0E-0x10) call `TransformOot_DrawMask`: while transformed it rewinds `POLY_OPA_DISP` to drop what `Player_DrawImpl` drew after the skeleton (gauntlets, boots); while putting the mask on it draws the MM mask on Link's face.

## Hooks

| hook | where | why |
|---|---|---|
| `TransformOot_Before/AfterPlayerUpdate` | `Player.c` around `Player_Update` | state machine, age properties, boot REGs, lent sword/tunic/boots/shield, damage flags, water and fire rules |
| `TransformOot_AfterPlayerInit` | `Player_AfterInit` | keeps the form through loading zones |
| `TransformOot_DrawSkeleton` / `DrawMask` | `Player_DrawFlexLod`, `kMaskCallbacks` | model |
| `TransformOot_Before/AfterInterfaceUpdate` | `src/common/interface/interface.c` | lends the Goron / Zora Tunic in the save while `Interface_Update` checks the hot room and underwater timers |
| `TransformOot_TryEnteringCrawlspace` | `PATCH_CALL(0x8083d8b4)`, reloc 0x80854678 zeroed in `transform_mask_oot.S` | Goron/Zora can't crawl |
| `GiantMaskOot_PlayerGetHeight` | `giant_mask.c` (owns `Player_GetHeight`) | form heights |
| `Player_UseItem`, `dpad.c` | item rules | |
| `Player_IsStrengthGoronBracelet` | `Player.c` | no OoT bracelet drawn on a form |

## Not done yet / unverified

- Phases 3-5 from the research plan: Zora fins and Deku bubble (B), Goron roll / spikes / pound, Zora fast swim and barrier, Deku water hops, guards, Deku flower, instruments. More MM animations can be imported the same way as the phase 2 ones (offsets in the research notes).
- Attack details not ported: the punch hitbox follows OoT's sword in the left hand, so right-hand punches hit on Link's left side; sounds and voices are OoT's sword swing and Link's voice (MM has Goron and Zora punch sounds and form voices that OoT doesn't have); no Goron fist growth (MM `unk_3D0`), no spin dust or sparkles; Goron and Zora still use OoT's own animations for everything else (idle, walking).
- Not checked in game (no ROM in the build container): whether any form DL references MM gameplay_keep (segment 04), the eye texture format, how OoT's animations look on each form, and the attack timing (MM's active frames played at OoT's attack speed).
- Leg IK still uses OoT adult values (feet may float or clip on slopes). A C port with MM's per-form constants can replace the calls at 0x8007aa20 / 0x8007aa64.
- `Player_ApplyAnimMovementScaledByAge` still moves Link by OoT's adult/child amount during animation-driven moves (ledge climbs), not by the form's root scale.
- Logic doesn't use any form ability (Goron as hammer or heat proof, Zora no drowning).
