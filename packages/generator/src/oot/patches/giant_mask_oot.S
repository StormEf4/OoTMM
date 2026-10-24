#include <combo.h>

/*
 * Giant's Mask (OoT): movement physics
 *
 * Only applied when the Giant's Mask is in OoT (patch group PG_OOT_GIANT_MASK). The C side is in
 * src/oot/giant_mask.c; every function returns the vanilla value unless the player is giant.
 *
 * How it works: the player code keeps running in normal units (speeds, jump velocities, gravity),
 * and the movement step is multiplied by the giant factor when it is applied to Link's position.
 * Link then runs, jumps and falls exactly like normal Link, only 10x bigger, like in MM.
 * The hooks below convert the few world distances the player code reads back into normal units,
 * and scale the collision distances that are hard-coded.
 *
 * Addresses are OoT 1.0 US. The player overlay starts at 0x808301C0. None of the replaced
 * instructions carry an overlay relocation.
 */

#define GO_FRAME    0xB0
#define GO_V0       0x18
#define GO_V1       0x1C
#define GO_A0       0x20
#define GO_A1       0x24
#define GO_A2       0x28
#define GO_A3       0x2C
#define GO_T7       0x4C
#define GO_RA       0x58
#define GO_AT       0x5C
/* Even float registers are saved as pairs, the single precision value is in the low word */
#define GO_D(n)     (0x60 + (n) * 4)
#define GO_F(n)     (GO_D(n) + 4)

/* Saves every caller-saved register, at included, since some hooks run between a lui at and its use */
.macro GO_SAVE
  addiu sp, sp, -GO_FRAME
  sw    ra, GO_RA (sp)
  sw    at, GO_AT (sp)
  sw    v0, GO_V0 (sp)
  sw    v1, GO_V1 (sp)
  sw    a0, GO_A0 (sp)
  sw    a1, GO_A1 (sp)
  sw    a2, GO_A2 (sp)
  sw    a3, GO_A3 (sp)
  sw    t0, 0x30 (sp)
  sw    t1, 0x34 (sp)
  sw    t2, 0x38 (sp)
  sw    t3, 0x3C (sp)
  sw    t4, 0x40 (sp)
  sw    t5, 0x44 (sp)
  sw    t6, 0x48 (sp)
  sw    t7, 0x4C (sp)
  sw    t8, 0x50 (sp)
  sw    t9, 0x54 (sp)
  sdc1  $f0, GO_D(0) (sp)
  sdc1  $f2, GO_D(2) (sp)
  sdc1  $f4, GO_D(4) (sp)
  sdc1  $f6, GO_D(6) (sp)
  sdc1  $f8, GO_D(8) (sp)
  sdc1  $f10, GO_D(10) (sp)
  sdc1  $f12, GO_D(12) (sp)
  sdc1  $f14, GO_D(14) (sp)
  sdc1  $f16, GO_D(16) (sp)
  sdc1  $f18, GO_D(18) (sp)
.endm

.macro GO_RESTORE
  lw    ra, GO_RA (sp)
  lw    at, GO_AT (sp)
  lw    v0, GO_V0 (sp)
  lw    v1, GO_V1 (sp)
  lw    a0, GO_A0 (sp)
  lw    a1, GO_A1 (sp)
  lw    a2, GO_A2 (sp)
  lw    a3, GO_A3 (sp)
  lw    t0, 0x30 (sp)
  lw    t1, 0x34 (sp)
  lw    t2, 0x38 (sp)
  lw    t3, 0x3C (sp)
  lw    t4, 0x40 (sp)
  lw    t5, 0x44 (sp)
  lw    t6, 0x48 (sp)
  lw    t7, 0x4C (sp)
  lw    t8, 0x50 (sp)
  lw    t9, 0x54 (sp)
  ldc1  $f0, GO_D(0) (sp)
  ldc1  $f2, GO_D(2) (sp)
  ldc1  $f4, GO_D(4) (sp)
  ldc1  $f6, GO_D(6) (sp)
  ldc1  $f8, GO_D(8) (sp)
  ldc1  $f10, GO_D(10) (sp)
  ldc1  $f12, GO_D(12) (sp)
  ldc1  $f14, GO_D(14) (sp)
  ldc1  $f16, GO_D(16) (sp)
  ldc1  $f18, GO_D(18) (sp)
  addiu sp, sp, GO_FRAME
.endm

/*
 * Actor floor check (func_8002E2AC): how far above the actor the floor is looked up (50)
 * Runs for every actor, the C function only changes the value for the giant player
 * s0 is the actor
 */
GiantMaskOot_FloorCheckOffset_Hook:
  GO_SAVE
  jal   GiantMaskOot_GetFloorCheckOffset
   or   a0, s0, zero
  swc1  $f0, GO_F(6) (sp)
  GO_RESTORE
  jr    ra
   nop

/*
 * Player_ProcessSceneCollision: sYDistToFloor = pos.y - floorHeight, in normal units
 */
GiantMaskOot_YDistToFloor_Hook:
  GO_SAVE
  jal   GiantMaskOot_ScaleDownDistance
   sub.s $f12, $f10, $f18
  swc1  $f0, GO_F(4) (sp)
  GO_RESTORE
  jr    ra
   nop

/*
 * func_8083AA10: fallDistance = fallStartHeight (t7) - (s32)pos.y (v0), in normal units
 * t7 is only used by that subtraction
 */
GiantMaskOot_FallDistance_Hook:
  mfc1  v0, $f6
  nop
  GO_SAVE
  or    a0, t7, zero
  jal   GiantMaskOot_ScaleFallStart
   or   a1, v0, zero
  sw    v0, GO_T7 (sp)
  GO_RESTORE
  jr    ra
   nop

/*
 * Player_ActionHandler_12: climbing a ledge
 * f0 is the height the climb animation doesn't cover, f18 the model scale for shape.yOffset (100)
 */
GiantMaskOot_LedgeClimb_Hook:
  GO_SAVE
  or    a0, s0, zero
  jal   GiantMaskOot_GetLedgeClimbOffset
   mfc1 a1, $f0
  swc1  $f0, GO_F(0) (sp)
  lui   t0, 0x42c8
  sw    t0, GO_F(18) (sp)
  GO_RESTORE
  jr    ra
   lui  at, 0x42c8

/*
 * Player_ActionHandler_12: small ledge hop, velocity = yDistToLedge * 0.08 + 5.5
 * Loads yDistToLedge in normal units into f6, and at = 5.5f like the replaced code
 */
GiantMaskOot_LedgeHop_Hook:
  GO_SAVE
  jal   GiantMaskOot_GetJumpYDistToLedge
   or   a0, s0, zero
  swc1  $f0, GO_F(6) (sp)
  GO_RESTORE
  jr    ra
   lui  at, 0x40b0

/*
 * Player_Action_80845668: jump up a tall ledge, velocity = yDistToLedge * 0.085 or 0.072
 * Loads yDistToLedge in normal units into f0. at was set by the delay slot and is kept.
 */
GiantMaskOot_LedgeJump_Hook:
  GO_SAVE
  jal   GiantMaskOot_GetJumpYDistToLedge
   or   a0, s0, zero
  swc1  $f0, GO_F(0) (sp)
  GO_RESTORE
  jr    ra
   nop

PATCH_GROUP PG_OOT_GIANT_MASK

/*
Replaces (func_8002E2AC):
  lui     at, 0x4248
  mtc1    at, $f6
*/
PATCH_START 0x80021C7C
  jal   GiantMaskOot_FloorCheckOffset_Hook
   nop
PATCH_END

/*
Replaces func_8002E234 (ledge walk-off check, -11) with GiantMaskOot_FloorLeaveCheck
*/
PATCH_START 0x80021BE8
  j     GiantMaskOot_FloorLeaveCheck
   nop
PATCH_END

/*
Player_UpdateCommon: jal Actor_UpdatePos
*/
PATCH_START 0x80847708
  jal   GiantMaskOot_PlayerUpdatePos
PATCH_END

/*
Player_ProcessSceneCollision and climbing: jal Actor_UpdateBgCheckInfo (wall height 26)
*/
PATCH_START 0x80845E28
  jal   GiantMaskOot_PlayerUpdateBgCheckInfo
PATCH_END

PATCH_START 0x8084A17C
  jal   GiantMaskOot_PlayerUpdateBgCheckInfo
PATCH_END

/*
Replaces (Player_ProcessSceneCollision):
  sub.s   $f4, $f10, $f18
The delay slot (addiu a0, a0, 0x7C0) is kept
*/
PATCH_START 0x80845E5C
  jal   GiantMaskOot_YDistToFloor_Hook
PATCH_END

/*
Replaces (func_8083AA10):
  mfc1    v0, $f6
  nop
*/
PATCH_START 0x80838B68
  jal   GiantMaskOot_FallDistance_Hook
   nop
PATCH_END

/*
Replaces (Player_ActionHandler_12):
  lui     at, 0x42c8
  mtc1    at, $f18
*/
PATCH_START 0x80836E28
  jal   GiantMaskOot_LedgeClimb_Hook
   nop
PATCH_END

/*
Replaces (Player_ActionHandler_12):
  lwc1    $f6, 0x874(s0)
  lui     at, 0x40b0
*/
PATCH_START 0x80836F08
  jal   GiantMaskOot_LedgeHop_Hook
   nop
PATCH_END

/*
Replaces (Player_Action_80845668):
  lwc1    $f0, 0x874(s0)
The delay slot (lui at, %hi(.rodata)) carries a relocation and is kept
*/
PATCH_START 0x80843790
  jal   GiantMaskOot_LedgeJump_Hook
PATCH_END

PATCH_GROUP_END
