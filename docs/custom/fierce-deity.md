# Fierce Deity's Mask in OoT: how it works

Reference for future changes. Settings: `fierceDeityMaskOot` and `sharedMaskFierceDeity` (Items > Extended Items > Cross-Game Masks). The item wiring follows [cross-game-items.md](cross-game-items.md); the transformation reuses the approach of the OoT Giant's Mask ([giant-mask.md](giant-mask.md)). Code: `src/oot/fierce_deity.c`, `include/combo/oot/fierce_deity.h`.

## Behavior

- Put on from the child trade slot (C button or D-pad up). Link must be on the ground, not in water, not crawling, not riding Epona, and not in a cutscene.
- Putting on and taking off freeze Link for 20 frames. He swaps to Fierce Deity at frame 10.
- As Fierce Deity:
  - Link is drawn with MM's Fierce Deity model at scale 0.015, and runs 1.5× faster.
  - The adult age properties are scaled by 1.5, for a child Link too; MM's Fierce Deity values are exactly OoT adult × 1.5.
  - The sword hits like Biggoron's Sword (4, or 8 for jump attacks, same as MM) and reaches 5500 units.
  - A slash while Z-targeting fires a sword beam for 1 magic: 80 units per frame, about 20 frames, damage 3 as `DMG_SPIN_GIANT`.
  - Every other item is blocked.
- The mask comes off when the item is used again, when something else removes the mask, or when Link leaves the area.

## How the model is drawn (C only)

- MM's `object_link_boy` (Fierce Deity) is MM object id **0x010**, loaded in OoT with `comboGetObject(0x0010 | MASK_FOREIGN_OBJECT)`. It is 0xD890 bytes.
- `gLinkFierceDeitySkel` is a Flex skeleton with LOD limbs. Its header is at 0x0600D878 and the limb table at 0x0600D824. The `dListCount` is read from header byte +8.
- OoT and MM use the same 22-entry player limb enum, so the FD skeleton is drawn with Link's own joint table.
- **Hook:** `Player_DrawFlexLod` (`src/oot/actors/Player.c`, `PATCH_CALL(0x8007a0d0)`), which replaces the skeleton draw inside `Player_DrawImpl`. For the gameplay player (`arg == GET_PLAYER(play)`) it calls `FierceDeityOot_DrawSkeleton`. That function:
  - points `gSegments[6]` and RSP segment 06 at the FD object;
  - temporarily sets `gSave.age = AGE_ADULT`, which removes the child root ×0.64 and uses adult leg IK;
  - draws the FD limb table;
  - restores segment 6 and the age.
- **Limb override:** `FierceDeityOot_OverrideLimbDraw` calls the vanilla override (head look, IK, body part positions, OoTMM's wrappers) and then puts back the FD display list.
  - The left hand always uses `0x060080F0` (`gLinkFierceDeityLeftHandHoldingSwordDL`, hand plus sword), like MM where Fierce Deity never sheathes his sword. Only a bottle, boomerang or hammer in that hand falls back to the empty hand. (The first version only showed the sword while OoT Link had his sword out, so Fierce Deity walked around empty-handed.)
  - In first person it draws nothing.
  - It resets the env color to white, because OoTMM tints some limbs with the tunic color.
  - The vanilla post-limb draw is kept, because sword collision depends on it.
- **Things Player_DrawImpl draws after the skeleton:**
  - The child Goron bracelet is turned off in `Player_IsStrengthGoronBracelet`.
  - The adult gauntlets and boots are dropped by rewinding `POLY_OPA_DISP` in the FD entry of `kMaskCallbacks`, which runs right after `Player_DrawImpl` (hook 0x808483c4).
- **Height:** `GiantMaskOot_PlayerGetHeight` returns 124 (MM's Fierce Deity height) while transformed.
- **Mask on the face during the transformation:** MM `object_mask_boy` (0x1E4), display list `0x0A000900`.

## Sword beam

- OoT has no beam actor, damage flag or graphics, so the beams live inside `fierce_deity.c` as up to 3 projectiles.
- Each beam has a `ColliderCylinder` owned by the player, registered with `CollisionCheck_SetAT` every frame. The timing and size follow MM's `EnMThunder_SwordBeam_Attack`.
- **Trigger:** the first active frame of a slash (`meleeWeaponState` going from 0 to nonzero) with `meleeWeaponAnimation < 22`, while Z-targeting and holding a sword. It aims at `focusActor` when locked on.
- **Graphics:**
  - `LoadFile` brings in only MM gameplay_keep 0x269E0–0x281DC: the two beam textures, vertices and `gSwordBeamDL` at 0x04027CA0.
  - The beam is drawn on XLU with segment 04 temporarily pointed at that buffer.

## Not verified without the game

These come from the decomps and could not be checked against the ROM data:
- whether the FD display lists reference MM's gameplay_keep (segment 04), which is OoT's in OoT;
- whether `gSwordBeamDL` references anything outside the loaded range;
- whether the FD display lists read the env color.

If the model or beam shows garbage, these are the first suspects.

## Ideas for later

- Transformation flash and sound like MM (the MM effect functions are MM overlay code and can't be reused).
- Sparkles on the sword while Z-targeting (`EffectSsKirakira_SpawnDispersed`).
- Keeping the form across scene changes. Today it resets with the area, like the Giant's Mask.
