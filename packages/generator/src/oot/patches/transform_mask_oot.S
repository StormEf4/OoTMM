#include <combo.h>

/*
 * MM transformation masks (OoT): overlay relocations removed for calls redirected from C (src/oot/transform_mask.c)
 * Addresses are OoT 1.0 US.
 */

/* Player_ActionHandler_5: jal Player_TryEnteringCrawlspace (0x8083d8b4) */
PATCH_START 0x80854678
.int 0
PATCH_END

/* Attacks (phase 2) */

/* Player_ActionHandler_7: jal func_80837818 (0x8084e090) */
PATCH_START 0x80855e7c
.int 0
PATCH_END

/* jal func_80837948: func_8083BA90 (0x80839b68), Player_Action_80844AF4 (0x80842c6c), func_80844BE4 (0x80842d44),
   Player_ActionHandler_7 (0x8084e0a4) */
PATCH_START 0x8085405c
.int 0
PATCH_END

PATCH_START 0x80854eb4
.int 0
PATCH_END

PATCH_START 0x80854ee4
.int 0
PATCH_END

PATCH_START 0x80855e80
.int 0
PATCH_END

/* Player_ActionHandler_8: jal func_808377DC (0x8083a6cc) */
PATCH_START 0x80854198
.int 0
PATCH_END

/* jal func_8083BA90: func_8083BBA0 (0x80839ca8), Player_ActionHandler_10 (0x80839fcc) */
PATCH_START 0x80854090
.int 0
PATCH_END

PATCH_START 0x808540f0
.int 0
PATCH_END

/* Player_Action_8084411C: jal func_8083BBA0 (0x80842324) */
PATCH_START 0x80854dc8
.int 0
PATCH_END

/* Player_ActionHandler_Roll: jal Player_TryRoll (0x8083a314) */
PATCH_START 0x80854148
.int 0
PATCH_END

/* Player_ActionHandler_10: jal Player_SetupRoll (0x80839fe0) */
PATCH_START 0x808540f4
.int 0
PATCH_END
