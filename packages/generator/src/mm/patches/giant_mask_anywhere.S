#include <combo.h>

/*
 * Giant's Mask Anywhere
 *
 * Scales the player's physics while wearing the Giant's Mask outside of Twinmold's Lair.
 * See giant_mask.c for the transformation itself.
 *
 * Hook sites are the same as the MM Randomizer's Giant's Mask Anywhere.
 *
 * Most hooks are inserted in the middle of functions, so the wrappers below preserve
 * every caller-saved register, and only write the values the replaced code produced.
 */

#define GM_FRAME    0xB0
#define GM_V0       0x18
#define GM_V1       0x1C
#define GM_A0       0x20
#define GM_A1       0x24
#define GM_A2       0x28
#define GM_A3       0x2C
#define GM_RA       0x58
#define GM_TMP      0x5C
/* Even float registers are saved as pairs, the single precision value is in the low word */
#define GM_D(n)     (0x60 + (n) * 4)
#define GM_F(n)     (GM_D(n) + 4)

.macro GM_SAVE
  addiu sp, sp, -GM_FRAME
  sw    ra, GM_RA (sp)
  sw    v0, GM_V0 (sp)
  sw    v1, GM_V1 (sp)
  sw    a0, GM_A0 (sp)
  sw    a1, GM_A1 (sp)
  sw    a2, GM_A2 (sp)
  sw    a3, GM_A3 (sp)
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
  sdc1  $f0, GM_D(0) (sp)
  sdc1  $f2, GM_D(2) (sp)
  sdc1  $f4, GM_D(4) (sp)
  sdc1  $f6, GM_D(6) (sp)
  sdc1  $f8, GM_D(8) (sp)
  sdc1  $f10, GM_D(10) (sp)
  sdc1  $f12, GM_D(12) (sp)
  sdc1  $f14, GM_D(14) (sp)
  sdc1  $f16, GM_D(16) (sp)
  sdc1  $f18, GM_D(18) (sp)
.endm

/* Restores everything but sp */
.macro GM_RESTORE_REGS
  lw    ra, GM_RA (sp)
  lw    v0, GM_V0 (sp)
  lw    v1, GM_V1 (sp)
  lw    a0, GM_A0 (sp)
  lw    a1, GM_A1 (sp)
  lw    a2, GM_A2 (sp)
  lw    a3, GM_A3 (sp)
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
  ldc1  $f0, GM_D(0) (sp)
  ldc1  $f2, GM_D(2) (sp)
  ldc1  $f4, GM_D(4) (sp)
  ldc1  $f6, GM_D(6) (sp)
  ldc1  $f8, GM_D(8) (sp)
  ldc1  $f10, GM_D(10) (sp)
  ldc1  $f12, GM_D(12) (sp)
  ldc1  $f14, GM_D(14) (sp)
  ldc1  $f16, GM_D(16) (sp)
  ldc1  $f18, GM_D(18) (sp)
.endm

.macro GM_RESTORE
  GM_RESTORE_REGS
  addiu sp, sp, GM_FRAME
.endm

/*
 * Calls a C function returning a float, and stores the result in the given float register
 * If passPlayer is set, the player in s0 is passed as the first argument
 */
.macro GM_HOOK_F name, func, freg, passPlayer=0
\name:
  GM_SAVE
.if \passPlayer
  jal   \func
   or   a0, s0, zero
.else
  jal   \func
   nop
.endif
  swc1  $f0, GM_F(\freg) (sp)
  GM_RESTORE
  jr    ra
   nop
.endm

/* Same as above, also restoring at to the value the replaced code left in it */
.macro GM_HOOK_F_AT name, func, freg, atHi
\name:
  GM_SAVE
  jal   \func
   nop
  swc1  $f0, GM_F(\freg) (sp)
  GM_RESTORE
  jr    ra
   lui  at, \atHi
.endm

/*
 * Player_GetHeight
 * Used by the camera and many actors
 */
GiantMask_PlayerGetHeight_Hook:
  addiu sp, sp, -0x20
  sw    ra, 0x1C (sp)
  jal   GiantMask_PlayerGetHeight_Original
   sw   a0, 0x20 (sp)
  mfc1  a1, $f0
  jal   GiantMask_ScalePlayerHeight
   lw   a0, 0x20 (sp)
  lw    ra, 0x1C (sp)
  jr    ra
   addiu sp, sp, 0x20

GiantMask_PlayerGetHeight_Original:
  /* Displaced code */
  lw    t6, 0x0A6C (a0)
  j     0x800B6FD0
   lui  at, 0x4200

/*
Replaces:
  LW      T6, 0x0A6C (A0)
  LUI     AT, 0x4200
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x800B6FC8
  j     GiantMask_PlayerGetHeight_Hook
   nop
PATCH_END
PATCH_GROUP_END

/*
 * Actor floor check: how far above the actor the floor is looked up
 * Called for every actor, the C function only modifies the result for the player
 */
GiantMask_GetFloorHeightCheckDelta_Hook:
  GM_SAVE
  /* Read pos and flags from the caller's stack, like the replaced code */
  lw    a2, (GM_FRAME + 0x48) (sp)
  jal   GiantMask_GetFloorHeightCheckDelta
   lw   a3, (GM_FRAME + 0x4C) (sp)
  swc1  $f0, GM_F(0) (sp)
  GM_RESTORE
  jr    ra
   nop

/*
Replaces:
  LW      T6, 0x004C (SP)
  LW      T8, 0x0048 (SP)
  LUI     AT, 0x4248
  ANDI    T7, T6, 0x0800
  BEQZL   T7, .+0x2C
   MTC1   AT, F0
  LUI     AT, 0x4120
  MTC1    AT, F0
  B       .+0x14
   LWC1   F4, 0x0004 (T8)
  MTC1    AT, F0
  NOP
  LWC1    F4, 0x0004 (T8)
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x800B7690
  jal   GiantMask_GetFloorHeightCheckDelta_Hook
   nop
  lw    t8, 0x0048 (sp)
  lwc1  $f4, 0x0004 (t8)
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
PATCH_END
PATCH_GROUP_END

/*
 * Actor floor check: how far below the actor the floor can be while still snapping to it
 */
GM_HOOK_F GiantMask_GetLedgeWalkOffHeight_Hook, GiantMask_GetLedgeWalkOffHeight, 16, 1

/*
Replaces:
  LUI     AT, 0xC130
  MTC1    AT, F16
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x800B7764
  nop
  jal   GiantMask_GetLedgeWalkOffHeight_Hook
PATCH_END
PATCH_GROUP_END

/*
 * Movement speed
 */
GiantMask_HandleFormSpeed_Hook:
  GM_SAVE
  jal   GiantMask_HandleFormSpeed
   nop
  GM_RESTORE
  jr    ra
   nop

/*
Replaces:
  LBU     T2, 0x014B (A1)
  LUI     AT, 0x3FC0
  BNEZ    T2, .+0x24
   NOP
  LWC1    F6, 0x0000 (A2)
  MTC1    AT, F8
  NOP
  MUL.S   F10, F6, F8
  SWC1    F10, 0x0000 (A2)
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x80832DE4
  jal   GiantMask_HandleFormSpeed_Hook
   nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
PATCH_END
PATCH_GROUP_END

/*
 * Wall collision height
 */
GiantMask_GetWallCollisionHeight_Hook:
  GM_SAVE
  jal   GiantMask_GetWallCollisionHeight
   or   a0, s0, zero
  swc1  $f0, GM_A2 (sp)
  GM_RESTORE
  jr    ra
   nop

/*
Replaces:
  LUI     A2, 0x41D6
  MFC1    A3, F0
  ORI     A2, A2, 0x6667
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x80843360
  jal   GiantMask_GetWallCollisionHeight_Hook
   mfc1 a3, $f0
  nop
PATCH_END
PATCH_GROUP_END

/*
 * Depth at which the player starts swimming
 * The function is a leaf, so its stack frame is extended to save ra
 */
GiantMask_GetDiveDepth_Hook:
  GM_SAVE
  jal   GiantMask_GetDiveDepth
   nop
  sdc1  $f0, GM_D(0) (sp)
  GM_RESTORE
  jr    ra
   c.lt.s $f6, $f14

/*
Replaces:
  ADDIU   SP, SP, -0x08
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x808475B8
  addiu sp, sp, -0x18
PATCH_END

/*
Replaces:
  SWC1    F8, 0x0004 (SP)
*/
PATCH_START 0x80847674
  swc1  $f8, 0x0010 (sp)
PATCH_END

/*
Replaces:
  SWC1    F4, 0x0004 (SP)
*/
PATCH_START 0x808476B8
  swc1  $f4, 0x0010 (sp)
PATCH_END

/*
Replaces:
  SWC1    F6, 0x0004 (SP)
*/
PATCH_START 0x808476E8
  swc1  $f6, 0x0010 (sp)
PATCH_END

/*
Replaces:
  SWC1    F4, 0x0004 (SP)
  LUI     AT, 0x42C8
  MTC1    AT, F6
  NOP
  C.LT.S  F6, F14
*/
PATCH_START 0x80847764
  swc1  $f4, 0x0010 (sp)
  sw    ra, 0x0014 (sp)
  jal   GiantMask_GetDiveDepth_Hook
   nop
  lw    ra, 0x0014 (sp)
PATCH_END

/*
Replaces:
  LWC1    F8, 0x0004 (SP)
*/
PATCH_START 0x80847780
  lwc1  $f8, 0x0010 (sp)
PATCH_END

PATCH_START 0x80847794
  lwc1  $f8, 0x0010 (sp)
PATCH_END

PATCH_START 0x808477A4
  lwc1  $f8, 0x0010 (sp)
PATCH_END

/*
Replaces:
  ADDIU   SP, SP, 0x08
*/
PATCH_START 0x808477CC
  addiu sp, sp, 0x18
PATCH_END
PATCH_GROUP_END

/*
 * Ledge climbing
 */
GM_HOOK_F GiantMask_GetWaterSurfaceClimbHeight_Hook, GiantMask_GetWaterSurfaceClimbHeight, 0, 1

/*
Replaces:
  LBU     T1, 0x014B (S0)
  LUI     AT, 0x4248
  BNEZL   T1, .+0x1C
   MTC1   AT, F0
  LUI     AT, 0x42A0
  MTC1    AT, F0
  B       .+0x14
   LWC1   F8, 0x008C (S0)
  MTC1    AT, F0
  NOP
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x80834EA0
  jal   GiantMask_GetWaterSurfaceClimbHeight_Hook
   nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
PATCH_END
PATCH_GROUP_END

GM_HOOK_F GiantMask_GetLedgeClimbFactorFromSwim_Hook, GiantMask_GetLedgeClimbFactorFromSwim, 18

/*
Replaces:
  LW      T0, 0x0A68 (S0)
  LUI     AT, 0x4270
  MTC1    AT, F18
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x808350D8
  jal   GiantMask_GetLedgeClimbFactorFromSwim_Hook
   nop
  lw    t0, 0x0A68 (s0)
PATCH_END
PATCH_GROUP_END

GiantMask_GetLedgeClimbFactor_Hook:
  GM_SAVE
  jal   GiantMask_GetLedgeClimbFactor
   nop
  swc1  $f0, GM_TMP (sp)
  GM_RESTORE_REGS
  lw    at, GM_TMP (sp)
  /* Displaced code */
  lwc1  $f8, 0x0018 (v0)
  jr    ra
   addiu sp, sp, GM_FRAME

/*
Replaces:
  LUI     AT, 0x4224
  LWC1    F8, 0x0018 (V0)
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x80835118
  jal   GiantMask_GetLedgeClimbFactor_Hook
   nop
PATCH_END
PATCH_GROUP_END

GM_HOOK_F_AT GiantMask_GetLedgeClimbFactor2_Hook, GiantMask_GetLedgeClimbFactor2, 10, 0x426C

/*
Replaces:
  LUI     AT, 0x426C
  MTC1    AT, F10
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x80835130
  jal   GiantMask_GetLedgeClimbFactor2_Hook
   nop
PATCH_END
PATCH_GROUP_END

GM_HOOK_F_AT GiantMask_GetInvertedLedgeClimbFactor_Hook, GiantMask_GetInvertedLedgeClimbFactor, 16, 0x42C8

/*
Replaces:
  LUI     AT, 0x42C8
  MTC1    AT, F16
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x80835170
  jal   GiantMask_GetInvertedLedgeClimbFactor_Hook
   nop
PATCH_END
PATCH_GROUP_END

GiantMask_ModifyLedgeJumpWallHeight_Hook:
  GM_SAVE
  jal   GiantMask_ModifyLedgeJumpWallHeight
   addiu a0, sp, GM_F(4)
  GM_RESTORE
  /* Displaced code */
  lui   at, 0x40B0
  jr    ra
   mtc1 at, $f8

/*
Replaces:
  LUI     AT, 0x40B0
  MTC1    AT, F8
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x808352CC
  jal   GiantMask_ModifyLedgeJumpWallHeight_Hook
   nop
PATCH_END
PATCH_GROUP_END

/*
 * Jumping
 */
GiantMask_GetMidAirJumpSlashHeight_Hook:
  GM_SAVE
  jal   GiantMask_GetMidAirJumpSlashHeight
   addiu a0, sp, GM_F(4)
  swc1  $f0, GM_A3 (sp)
  GM_RESTORE
  jr    ra
   lui  at, 0x4090

/*
Replaces:
  LUI     AT, 0x4090
  MTC1    AT, F4
  LUI     A3, 0x4040
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x808397D0
  jal   GiantMask_GetMidAirJumpSlashHeight_Hook
   nop
  nop
PATCH_END
PATCH_GROUP_END

GiantMask_SetVelocityY_Hook:
  GM_SAVE
  jal   GiantMask_SetVelocityY
   mfc1 a2, $f4
  /* The replaced code left the new velocity in f6 */
  lw    a0, GM_A0 (sp)
  lwc1  $f0, 0x0068 (a0)
  swc1  $f0, GM_F(6) (sp)
  GM_RESTORE
  jr    ra
   nop

/*
Replaces:
  MTC1    A1, F12
  ANDI    A1, A2, 0xFFFF
  SW      RA, 0x0014 (SP)
  SW      A2, 0x0020 (SP)
  LUI     AT, %hi(sWaterSpeedFactor)    (kept, relocated)
  LWC1    F4, %lo(sWaterSpeedFactor)    (kept, relocated)
  LHU     T6, 0x0090 (A0)
  MUL.S   F6, F12, F4
  ANDI    T7, T6, 0xFFFE
  SH      T7, 0x0090 (A0)
  BEQZ    A1, .+0x24
   SWC1   F6, 0x0068 (A0)
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x80834CD4
  sw    ra, 0x0014 (sp)
  lhu   t6, 0x0090 (a0)
  andi  t7, t6, 0xFFFE
  sh    t7, 0x0090 (a0)
PATCH_END

PATCH_START 0x80834CEC
  sw    a2, 0x0020 (sp)
  jal   GiantMask_SetVelocityY_Hook
   mtc1 a1, $f12
  andi  a1, a2, 0xFFFF
  beqz  a1, .+0x24
   nop
PATCH_END
PATCH_GROUP_END

GiantMask_ModifyJumpVelocity_Hook:
  GM_SAVE
  jal   GiantMask_ModifyJumpVelocity
   nop
  GM_RESTORE
  /* Displaced code */
  jr    ra
   lw   t2, 0x0A70 (s0)

/*
Replaces:
  LW      T2, 0x0A70 (S0)
  LUI     AT, 0x0008
  OR      A0, S0, R0
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x80839934
  jal   GiantMask_ModifyJumpVelocity_Hook
   or   a0, s0, zero
  lui   at, 0x0008
PATCH_END
PATCH_GROUP_END

GM_HOOK_F_AT GiantMask_GetJumpSlashHeight_Hook, GiantMask_GetJumpSlashHeight, 0, 0x40A0

/*
Replaces:
  LW      A0, 0x0034 (SP)
  BEQ     T9, AT, .+0x2C
   LUI    AT, 0x40A0
  MTC1    AT, F0
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x80839C5C
  beq   t9, at, .+0x2C
   lw   a0, 0x0034 (sp)
  jal   GiantMask_GetJumpSlashHeight_Hook
   nop
PATCH_END
PATCH_GROUP_END

/*
 * Terminal velocity
 */
GM_HOOK_F_AT GiantMask_GetNewMinVelocityY_Hook, GiantMask_GetNewMinVelocityY, 4, 0xC1A0

/*
Replaces:
  LUI     AT, 0xC1A0
  MTC1    AT, F4
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x8083BF6C
  jal   GiantMask_GetNewMinVelocityY_Hook
   nop
PATCH_END
PATCH_GROUP_END

/*
 * Deceleration
 */
GM_HOOK_F_AT GiantMask_GetRunDeceleration_Hook, GiantMask_GetRunDeceleration, 16, 0x3FC0

/*
Replaces:
  LUI     AT, 0x3FC0
  MTC1    AT, F16
  MTC1    T6, F4
  MTC1    A1, F12
  SWC1    F16, 0x0010 (SP)
  CVT.S.W F6, F4
  LH      T7, 0x004A (V0)
  SW      T7, 0x0014 (SP)
  DIV.S   F10, F6, F8
  MFC1    A3, F10
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x8083CB84
  mtc1  t6, $f4
  mtc1  a1, $f12
  cvt.s.w $f6, $f4
  lh    t7, 0x004A (v0)
  sw    t7, 0x0014 (sp)
  div.s $f10, $f6, $f8
  mfc1  a3, $f10
  jal   GiantMask_GetRunDeceleration_Hook
   nop
  swc1  $f16, 0x0010 (sp)
PATCH_END
PATCH_GROUP_END

/*
Backwalk and sidewalk
Replaces:
  JAL     Math_AsymStepToF
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x8084A708
  jal   GiantMask_Math_AsymStepToF
PATCH_END

PATCH_START 0x8084AAF8
  jal   GiantMask_Math_AsymStepToF
PATCH_END
PATCH_GROUP_END

/*
 * Bonking
 */
GiantMask_IsAboveBonkThreshold_Hook:
  GM_SAVE
  jal   GiantMask_IsAboveBonkThreshold
   mfc1 a3, $f12
  sw    v0, GM_V0 (sp)
  GM_RESTORE
  /* Displaced code */
  jr    ra
   lwc1 $f4, 0x0000 (a2)

/*
Replaces:
  LWC1    F4, 0x0000 (A2)
  OR      A3, R0, R0
  C.LE.S  F12, F4
  NOP
  BC1FL   0x80840CC0
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x80840A48
  jal   GiantMask_IsAboveBonkThreshold_Hook
   nop
  nop
  or    a3, zero, zero
  beqzl v0, .+0x268
PATCH_END

/*
Speed after bonking
Replaces:
  JAL     Math_StepToF
*/
PATCH_START 0x8084C7D0
  jal   GiantMask_Math_StepToF
PATCH_END
PATCH_GROUP_END

/*
 * Mid-air movement
 */
GiantMask_GetMidAirAcceleration_Hook:
  GM_SAVE
  addiu a0, sp, GM_F(8)
  jal   GiantMask_GetMidAirAcceleration
   addiu a1, sp, GM_F(10)
  GM_RESTORE
  /* Displaced code */
  addiu t9, zero, 0x00C8
  jr    ra
   lui  a3, 0x3F80

/*
Replaces:
  ADDIU   T9, R0, 0x00C8
  SW      T9, 0x0018 (SP)
  LH      A2, 0x0042 (SP)
  LUI     A3, 0x3F80
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x8084C374
  jal   GiantMask_GetMidAirAcceleration_Hook
   nop
  sw    t9, 0x0018 (sp)
  lh    a2, 0x0042 (sp)
PATCH_END
PATCH_GROUP_END

GM_HOOK_F GiantMask_GetLedgeGrabDistance_Hook, GiantMask_GetLedgeGrabDistance, 12

/*
Replaces:
  MTC1    AT, F12
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x8084C564
  jal   GiantMask_GetLedgeGrabDistance_Hook
PATCH_END
PATCH_GROUP_END

/*
 * Jump slash gravity
 */
GiantMask_AfterJumpSlashGravity_Hook:
  GM_SAVE
  jal   GiantMask_AfterJumpSlashGravity
   nop
  GM_RESTORE
  /* Displaced code */
  j     PlayerAnimation_Update
   addiu a1, a1, 0x0240

/*
Replaces:
  ADDIU   A1, S0, 0x0240
  JAL     PlayerAnimation_Update
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x8084CBBC
  or    a1, s0, zero
  jal   GiantMask_AfterJumpSlashGravity_Hook
PATCH_END
PATCH_GROUP_END

/*
 * Spin attack charge walking animation speed
 */
GM_HOOK_F_AT GiantMask_GetSpinChargeWalkSpeedFactor_Hook, GiantMask_GetSpinChargeWalkSpeedFactor, 14, 0x3FC0
GM_HOOK_F GiantMask_GetSpinChargeWalkSpeedFactor2_Hook, GiantMask_GetSpinChargeWalkSpeedFactor, 14

/*
Replaces:
  LUI     AT, 0x3FC0
  MTC1    AT, F14
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x8084CEC0
  jal   GiantMask_GetSpinChargeWalkSpeedFactor_Hook
   nop
PATCH_END

/*
Replaces:
  MTC1    AT, F14
  NOP
*/
PATCH_START 0x8084D248
  jal   GiantMask_GetSpinChargeWalkSpeedFactor2_Hook
   nop
PATCH_END
PATCH_GROUP_END

/*
 * Swimming down speed
 */
GM_HOOK_F_AT GiantMask_GetDiveSpeed_Hook, GiantMask_GetDiveSpeed, 10, 0xC000

/*
Replaces:
  LUI     AT, 0xC000
  MTC1    AT, F10
  NOP
  SWC1    F10, 0x0068 (S0)
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x808517CC
  nop
  jal   GiantMask_GetDiveSpeed_Hook
   nop
  swc1  $f10, 0x0068 (s0)
PATCH_END
PATCH_GROUP_END

/*
 * Ladder and vine climbing
 */
GiantMask_GetClimbDelta_Hook:
  GM_SAVE
  jal   GiantMask_GetClimbXZDelta
   nop
  sdc1  $f0, GM_D(0) (sp)
  jal   GiantMask_GetClimbYDelta
   nop
  sdc1  $f0, GM_D(0) (sp)
  GM_RESTORE
  jr    ra
   lui  at, 0x4248

/*
Replaces:
  LUI     AT, 0x4248
  MTC1    AT, F8
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x8084F6D8
  jal   GiantMask_GetClimbDelta_Hook
   nop
PATCH_END

/*
Replaces:
  LUI     A2, 0x41D6
*/
PATCH_START 0x8084F6EC
  mfc1  a2, $f0
PATCH_END

/*
Replaces:
  ORI     A2, A2, 0x6667
*/
PATCH_START 0x8084F6F4
  nop
PATCH_END
PATCH_GROUP_END

/*
 * Collision cylinder radius
Replaces:
  JR      RA
*/
PATCH_GROUP PG_MM_GIANT_MASK_ANYWHERE
PATCH_START 0x8082F0DC
  j     GiantMask_AfterUpdateCylinder
PATCH_END

PATCH_START 0x8082F128
  j     GiantMask_AfterUpdateCylinder
PATCH_END

PATCH_START 0x8082F14C
  j     GiantMask_AfterUpdateCylinder
PATCH_END

PATCH_START 0x8082F15C
  j     GiantMask_AfterUpdateCylinder
PATCH_END
PATCH_GROUP_END
