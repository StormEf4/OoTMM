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
  - Goron and Zora punch with MM's three-hit combo and MM's animations (see Attacks below). They can't charge a spin attack: holding B after a punch readies the Zora's boomerang fins instead (see Zora boomerang fins below).
  - Deku blows a bubble with B (2 magic, see Deku bubble below). The Deku melee attack is the spin on A.
- Size and movement: MM's age properties and boot data for each form (below). Actor scale stays 0.01, the size comes from the model, like in MM. Heights: Deku 36, Goron 80, Zora 68.
- Rules:
  - Goron counts as wearing the Goron Tunic (hot rooms, hot floors), Zora as wearing the Zora Tunic (no drowning timer), and Zora's underwater timer stays at 0.
  - Goron and Deku can't swim: in deep water they sink, and after 20 frames void out to the last entrance. Within 60 frames of entering an area they take the mask off instead, so an entrance in water can't loop void outs. A Deku first hops off the water up to 5 times (see Deku water hops below).
  - Deku and Zora void out when set on fire.
  - Goron and Zora can't enter crawlspaces (child Link wearing them would otherwise crawl). Deku can.
  - Forms don't wear Link's tunic, boots or shield: they are swapped for the form's during the player update and put back after.

## Attacks (phase 2)

| form | input | what happens | hits as |
|---|---|---|---|
| Goron | B | MM punch combo: left (`pg_punchA`), right (`pg_punchB`), finisher (`pg_punchC`) | `DMG_HAMMER_SWING` 2 |
| Goron | A, Z-target + A forward / B in the air | curl up and roll (see Goron roll below) / nothing (MM has no Goron jump attack) | |
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

## Goron roll and ground pound (phase 3)

A Goron curls up with A (from `TransformOot_TryRoll`, `TransformOot_SetupLockOnRoll` and the Z-target jump attack) into its own player action, `TransformOot_GoronRollAction`, a simplified MM `Player_Action_96`:

| input | what happens | hits as |
|---|---|---|
| A | curl up: `pg_maru_change` frames 0-7 (imported from MM), then the ball replaces the skeleton | |
| hold A + stick | roll: speed target = stick (linear) x 2.6, spin steps up by 0x7D0 to speed x 900 and acceleration follows the spin (spin x 0.0003), turning rate = speed x 20 + 300; walls bounce the ball back (MM's reflection rounding) at speed 6 or more | `DMG_SLASH_KOKIRI` 1, r25, at speed 6 or more |
| keep rolling at full speed | spikes: 54 frames with the spin at 0x36B0 or more and 2 magic, then the spikes pop out (2 magic, then 1 every 10 frames), speed 18; a head-on wall hit, letting go of A or running out of magic takes them away | `DMG_HAMMER_SWING` 2, r25 |
| B while curled on the ground (no spikes) | hop (14 up), hang at the top (gravity -0.2), drop (gravity -10), and pound on landing like the Megaton Hammer (`func_80842A28`: quake, `actorCtx.unk_02 = 4`, hammer sound) with MM's white shockwave; 10 frames to settle | `DMG_HAMMER_JUMP` 4, r60, 3 frames |
| let go of A | spikes go first, then Goron uncurls (`pg_maru_change` backwards from frame 7) and goes to OoT's idle | |

Details:
- `player->state3 |= PLAYER_STATE3_1` every frame keeps OoT's ledge code (`func_8083AA10`) from starting a fall, so the ball rolls off ledges and keeps rolling, like MM. No fall damage while curled.
- **Draw:** `TransformOot_DrawSkeleton` draws `TransformOot_DrawGoronBall` instead of the skeleton while the action is in its ball phase: `gLinkGoronCurledDL` (0x0600BDD8) and the spikes `lg_spike_model` (0x0600C540, scaled in over 3 frames) from `object_link_goron`, with MM's transform (12 units up, scale 1.15, rotated on X by the spin). Both lists call `0x0C000010`, the second entry of MM's four-entry render mode list (`Scene_SetRenderModeXlu(play, 0, 1)`), which `TransformOot_DrawSkeleton` sets for every form (see How it's drawn). The env color goes from white to MM's pound blue (80, 80, 200) while dropping into a pound and while the spikes charge. Both feet shadows are put under the ball.
- **Sounds** are OoT's, since OoT has none of MM's Goron ball sounds: curling `NA_SE_PL_ROLL`, rolling `NA_SE_EV_BIGBALL_ROLL` (the Goron City Goron), bounce `NA_SE_EV_STONE_BOUND`, charge `NA_SE_IT_SWORD_CHARGE`, spikes `NA_SE_IT_ROLLING_CUT_LV1`, pound `NA_SE_IT_HAMMER_HIT`.
- Not ported: MM's momentum vector (`unk_B08`/`unk_B0C`, the ball keeps its direction for a moment after turning), slope rolling, the bounces after a pound, the ball squash (`unk_ABC`), the spike trail and dust effects, MM's pound actor (`EnTest`), breaking MM-only objects.

## Zora fast swimming (phase 4)

Holding A in water as a Zora swims like MM's Zora (`func_80850734`, `Player_Action_56`, simplified): a dash at speed 16 that settles at 9, the stick's up and down pitch the Zora (up dives, like MM), and left and right turn it (`func_80850BF8`: up to 1100 binary angle per frame) and roll it over and over. Letting go of A hands back to OoT's swimming.

It runs on top of OoT's swim actions instead of being an action of its own (OoT's diving, surfacing and breath code all stay as they are):
- `TransformOot_UpdateZoraSwim`, after the player update, replaces the movement OoT's swim action set: `speedXZ = cos(pitch) x speed`, `velocity.y = -sin(pitch) x speed`, gravity 0, yaw turned by the stick (MM `func_80850BA8`). It runs while the Zora is in water (`PLAYER_ACTOR_STATE_WATER`), A is held, and nothing else holds Link (talking, getting an item, cutscene, dying).
- Near the surface (depth under the form's `unk_24` + 10) the pitch stays at least 0x7D0 down, like MM, so the Zora skims just under the surface instead of leaving the water. Touching the floor reflects the pitch up.
- `TransformOot_RedirectAnimation` loads MM's `pz_fishswim` (10 frames, looping) for every frame OoT's swim actions load.
- The model is pitched and rolled at the root limb like MM's `Player_OverrideLimbDrawGameplayCommon` (`TransformOot_ZoraSwimRoot`, after OoT's override). OoT's own swim pitch, Player `unk_6C2`, is cleared while swimming fast so OoT doesn't pitch the model a second time, and set to MM's pitch when it ends so OoT's dive carries on from it. **That field is at 0x6B2 in OoT 1.0, not 0x6C2 as in the decomp's debug layout** (checked in the ROM: `func_8083D330` stores 16000 at 0x6B2).
- Sounds are OoT's: `NA_SE_PL_DIVE_BUBBLE` for the dash and `NA_SE_PL_SWIM` once per stroke (MM's Zora swim sounds aren't in OoT).
- Not ported: the `pz_waterroll` start animation, the dolphin jump out of the water, the Zora barrier (R), MM's stroke trails.

## Zora boomerang fins (phase 4)

Like MM's Zora (`func_80830E30`, `Player_UpperAction_12` to `16`, En_Boom `ZORA_BOOMERANG_LEFT` / `RIGHT`, simplified): still holding B when a punch ends readies the fins, letting go throws both, and they fly out and come back to be caught.

- **Ready.** OoT would start charging a spin attack there (`Player_ActionHandler_8`, the `TransformOot_StartSpinCharge` hook), so for the Zora that hook calls `TransformOot_TryZoraFins` instead. It sets a custom upper body action (OoT's `Player_SetUpperActionFunc`, 1.0 `0x8083164C`), so the Zora can still walk and turn: `pz_cutterwaitA`, `B` or `C` depending on the punch it came from, then `pz_cutterwaitanim` looping. OoT's `Player_UpdateUpperBody` copies the upper body animation to the arms while moving and to the whole body while standing, like MM.
- **Throw.** Letting go of B plays `pz_cutterattack`; on its frame 6, `TransformOot_ThrowZoraFins` spawns two OoT boomerangs (`ACTOR_EN_BOOM`) from the hands (`bodyPartsPos`, 50 above the feet). Not locked on, they leave at the facing angle -/+ 0x190 (MM's numbers: the left one leaves slightly right, and they cross); locked on, at +/- 0x36B0 so they close in on the target from both sides. `PLAYER_STATE1_BOOMERANG_THROWN` and `boomerangActor` are set so the camera follows them like OoT's boomerang, and Z-targeting waits, like MM.
- **The fins.** Each fin is OoT's En_Boom with its update and draw replaced after the spawn (`params` 0 = left, 1 = right). The update (`TransformOot_ZoraFinUpdate`) runs En_Boom's own flight (`actionFunc`), but first forgets an AT hit on anything that isn't an item or a token, so the fins cut through enemies like MM's and only walls, items and tokens send them back. They fly out for 20 frames (36 locked on), MM's numbers. Damage is MM's 2 as `DMG_BOOMERANG | DMG_SLASH_MASTER`: OoT picks the damage table entry from the highest bit, so enemies take Master Sword damage like the Zora punches, while items, tokens and boomerang switches (which only accept the boomerang bit) still work. The draw (`TransformOot_ZoraFinDraw`, from MM's `EnBoom_Draw`) updates the hitbox and trail like En_Boom's (`Player_UpdateWeaponInfo`, 1.0 `0x8007AF14`), tilts each fin out by 0x1F40 to its side and spins them in opposite directions, with MM's fin models from MM's gameplay_keep (`0x0406FE20`, `0x0406FF68`, custom object `EQ_ZORA_FINS`).
- **Catch.** En_Boom kills itself when it comes back to Link. `TransformOot_TrackZoraFins` looks for the fins in the actor list before each player update; when one is gone the upper body plays `pz_cuttercatch` (`NA_SE_PL_CATCH_BOOMERANG`), then returns to the held item's upper action (OoT's `func_80834644`, 1.0 `0x808326A0`).
- While a fin is out, B presses are hidden from the player update (no punching, like MM) and the arm fin it came from isn't drawn.
- Not ported: MM's over-the-shoulder aiming camera while holding the fins (`CAM_MODE_ZORAFIN`), the fin trail and splash effects in water, the Zora voice.

## Deku bubble (phase 5)

Like MM's Deku (`func_808306F8`, `Player_UpperAction_7` to `9`, En_Arrow `ARROW_TYPE_DEKU_BUBBLE`, simplified): B blows a bubble for 2 magic, it grows while B is held, and letting go shoots it.

- **B.** The Deku can't use a sword, so a B press reaches the item use hook (`Player.c`) and stops there. `TransformOot_TryDekuBubble` is called first in that hook: on a B press (`heldItemButton` 0) as a Deku it blows the bubble. When B is empty, the Deku gets a sword on B during the player update like Goron and Zora (`TransformOot_LendSword`), so the press always gets to the hook. No magic meter or not enough magic: the error sound, no bubble (`Magic_RequestChangeCustom`, so the Chateau Romani's free magic works).
- **Blowing.** A custom upper body action (`TransformOot_DekuBubbleReadyUpper`) plays `pn_tamahakidf` (imported from MM), so the Deku can still walk and turn. The bubble grows to MM's full size (16) with MM's step; after 20 frames at full size it shrinks and pops in front of the mouth, like MM. It sits in front of the mouth (MM's offset from the head, `1300, -400, 0`), set from `TransformOot_PostLimbDraw` on the head limb.
- **Shooting.** Letting go of B (once the pose is reached, like MM) plays `pn_tamahaki` and sends the bubble at the Z-target if there is one, otherwise straight ahead. Like MM, the smaller the bubble the faster it goes (16 - size, at least 1); it shrinks by 0.4 a frame as it flies, wobbles, and pops at size 1, so a full bubble goes furthest. It bounces off shields and other hard things (`AT_BOUNCED`, not torches), and pops on walls (`BgCheck_EntityLineTest1`) and on anything it hits.
- **The bubble** is OoT's En_Boom with its update and draw replaced, like the Zora fins, with its state in En_Boom's own fields. The draw is MM's `EnArrow_Draw`: see-through and facing the camera while held, solid and stretched in flight, with MM's three display lists from MM's gameplay_keep (`0x0406F380` render setup, `0x0406F9F0` held, `0x0406FAE0` flying, custom object `EQ_DEKU_BUBBLE`). They use the frame buffer (segment 0x0F, set by OoT as in MM) as the second texture. The hitbox is MM's: two crossed 40 unit quads every other frame, through `Player_UpdateWeaponInfo`. It hits as `DMG_SLINGSHOT`, damage 2, so enemies react like to a Deku Seed (MM's `DMG_DEKU_BUBBLE` means something else in OoT).
- Sounds are OoT's: `NA_SE_EN_NUTS_THROW` (a Deku Scrub spitting) to shoot, `NA_SE_EN_AWA_BREAK` (the Shabom bubble) to pop.
- Not ported: first person aiming (MM aims the bubble in first person unless Z-targeting; here it goes straight ahead), the head squash and lean back while blowing, MM's bubble sounds, shooting again straight from `pn_tamahaki`, the water splash when the bubble enters water.

## Deku water hops (phase 5)

Like MM (`func_808373F8`, `remainingHopsCounter`): a Deku that lands in, or walks into, deep water hops off it instead of sinking, up to 5 times before touching ground again, each hop longer than the last. The sixth time it sinks and voids out as before.

- OoT starts swimming from `func_8083D53C` (1.0 `0x8083B5F0`), with `jal func_8083D36C` (1.0 `0x8083B41C`). That call is redirected to `TransformOot_EnterDeepWater` (`PATCH_CALL(0x8083b720)`, reloc 0x808543A8 zeroed in `transform_mask_oot.S`), which hops instead while hops are left.
- The hop is OoT's automatic jump (1.0 `0x808385CC`): `func_80838940` (1.0 `0x80836A40`) with the jump or running jump animation and the speed from the form's boot REGs (IREG 66-69), at least 8 for the Deku, then scaled by MM's `0.3 + (5 - hops left) x 0.18` (at least 4). Link is put on the water surface first, with a splash, ripples and `NA_SE_PL_JUMP_WATER0`.
- Hops come back to 5 on solid ground (`BGCHECKFLAG_GROUND`, checked before the player update).
- Not ported: the spin on the last hop (`pn_attack`), MM's pitched hop sounds.
- Deku flowers: OoT has no Deku flowers in any scene, so there is nothing to launch from. They would need flower actors added to OoT maps first.

## Arm fins

MM draws the Zora's arm fins separately from the limbs, on the forearms (`func_80126BD0`), so they were missing before. `TransformOot_DrawZoraArmFin` draws them from `TransformOot_PostLimbDraw`: `0x0600CC38` / `0x0600CDA0` folded to 40%, 60%, 70% while the B weapon is put away and full size once it's out (OoT's decomp `heldItemAction`, `itemAction` in OoTMM's header, is a sword), opening and closing with the jump kick (`D_801C0820`, `D_801C0838`) and put away for the guitar (`D_801C0580`). The right one is the big guard fin (`0x060110A8`) while guarding (`PLAYER_STATE1_SHIELDING`), and the swim fins (`0x06010868` / `0x06010978`, wider while rolling) take their place while swimming fast.

## Idle

MM gives the Goron and Zora their own idle (`Player_GetIdleAnim`: `pg_wait`, `pz_wait`). The other movement animations (walk, run, jump, roll, ...) are Link's in MM too, so they stay OoT's. `TransformOot_RedirectAnimation` loads the form idle's pose whenever OoT loads one of its three idles (`link_normal_wait_free` 0x04003240, `link_normal_wait` 0x04003238, `link_fighter_wait_long` 0x04002BE0, 89 frames each), with the frame scaled to the form idle's length. `skelAnime.animation` stays OoT's, so OoT's idle and fidget logic is untouched. Still missing from MM: the form versions of doors, chests, ladders, drinking and taking the mask off (they have OoT timing tied to them, so they need more than a swap).

## Instruments (phase 5)

Like MM, a Deku, Goron or Zora plays the ocarina on its own instrument: Deku pipes, Goron drums, Zora guitar.

- **Animations:** MM's `pn_/pg_/pz_gakkistart` (bring the instrument out) and `gakkiplay` (playing, loops) are imported like the attack animations. OoT's ocarina code is unchanged: when it sets `gPlayerAnim_link_normal_okarina_start` (0x040030A0), `_swing` (0x040030A8) or `_end` (0x04003098) on a form, `TransformOot_UpdateInstrument` (after the player update) swaps in the form's animation with the frame range and length fixed up: start becomes gakkistart, swing becomes the gakkiplay loop, end becomes gakkistart played backwards (MM's `Player_Anim_PlayOnceAdjustedReverse`). The frame loaded in that same update is redirected by `TransformOot_RedirectAnimation`, called from the player frame loader in `src/common/animations.c`, so OoT's pose never shows on the form.
- **Models** (MM `Player_PostLimbDrawGameplay`): Goron drums (`0x0600FC18` + five drums) on the torso and Deku pipes (`0x06007390` + five pipes) on the head, from `TransformOot_PostLimbDraw` (wraps the vanilla post-limb draw). They grow in during gakkistart with MM's scale keys (`D_801C0428`, `D_801C0340`, `D_801C0368`). The Zora holds the guitar as its left hand (`0x0600E2A0`) from frame 6 of pz_gakkistart and while playing (no grow-in scale, MM's `D_801C0538`).
- Not ported: the instrument sounds (OoT's sound bank has no drums, guitar or pipes, so it still sounds like the ocarina), the Goron per-note drum hits (`pg_gakkiplayA/L/D/U/R`) and `pg_gakkiwait`, and the torso squash while the Goron brings the drums out.

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

Same as Fierce Deity: `Player_DrawFlexLod` (`Player.c`) calls `TransformOot_DrawSkeleton` after `FierceDeityOot_DrawSkeleton`. It swaps segment 6 to the form object, points segment 0x0C at four end commands (MM's `sRenderModeSetNoneDL`: every Deku limb calls `0x0C000000` and the Goron ball calls `0x0C000010`, and OoT sets nothing there for the player), forces `gSave.age = AGE_ADULT` while drawing, sets the eyes and mouth (segments 08/09) from the form object using OoT's face index (`jointTable[22]`, or the blink table), and draws with `TransformOot_OverrideLimbDraw`:
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
| `TransformOot_EnterDeepWater` | `PATCH_CALL(0x8083b720)`, reloc 0x808543a8 zeroed in `transform_mask_oot.S` | Deku water hops |
| `TransformOot_TryDekuBubble` | `Player_UseItem` (`Player.c`) | Deku bubble on B |
| `GiantMaskOot_PlayerGetHeight` | `giant_mask.c` (owns `Player_GetHeight`) | form heights |
| `Player_UseItem`, `dpad.c` | item rules | |
| `Player_IsStrengthGoronBracelet` | `Player.c` | no OoT bracelet drawn on a form |

## Not done yet / unverified

- Phases 3-5 from the research plan: Zora barrier, MM's guard animations, Deku flower (OoT has no Deku flowers, see Deku water hops). Instruments, the Goron roll / spikes / pound, the Zora fast swim, the Zora boomerang fins, the Deku bubble and the Deku water hops are done (above), simplified. More MM animations can be imported the same way as the phase 2 ones (offsets in the research notes).
- Attack details not ported: the punch hitbox follows OoT's sword in the left hand, so right-hand punches hit on Link's left side; sounds and voices are OoT's sword swing and Link's voice (MM has Goron and Zora punch sounds and form voices that OoT doesn't have); no Goron fist growth (MM `unk_3D0`), no spin dust or sparkles; Goron and Zora still use OoT's own animations for everything else (idle, walking).
- Not checked in game (no ROM in the build container): whether any form DL references MM gameplay_keep (segment 04), the eye texture format, how OoT's animations look on each form, and the attack timing (MM's active frames played at OoT's attack speed).
- Leg IK still uses OoT adult values (feet may float or clip on slopes). A C port with MM's per-form constants can replace the calls at 0x8007aa20 / 0x8007aa64.
- `Player_ApplyAnimMovementScaledByAge` still moves Link by OoT's adult/child amount during animation-driven moves (ledge climbs), not by the form's root scale.
- Logic doesn't use any form ability (Goron as hammer or heat proof, Zora no drowning).
