#ifndef COMBO_OOT_GIANT_MASK_H
#define COMBO_OOT_GIANT_MASK_H

#include <combo/types.h>
#include "play.h"
#include "player.h"

/* Giant's Mask (OoT) */
s32  GiantMaskOot_IsGiant(void);
s32  GiantMaskOot_IsTransforming(void);
void GiantMaskOot_UseMask(PlayState* play, Player* player);
void GiantMaskOot_Reset(PlayState* play, Player* player);
void GiantMaskOot_BeforePlayerUpdate(Player* player, PlayState* play);
void GiantMaskOot_AfterPlayerUpdate(Player* player, PlayState* play);

#endif
