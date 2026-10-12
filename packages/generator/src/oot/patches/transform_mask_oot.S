#include <combo.h>

/*
 * MM transformation masks (OoT): overlay relocations removed for calls redirected from C (src/oot/transform_mask.c)
 * Addresses are OoT 1.0 US.
 */

/* Player_ActionHandler_5: jal Player_TryEnteringCrawlspace (0x8083d8b4) */
PATCH_START 0x80854678
.int 0
PATCH_END
