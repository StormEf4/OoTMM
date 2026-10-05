#ifndef COMBO_MM_GIANT_MASK_H
#define COMBO_MM_GIANT_MASK_H

#include <combo/types.h>
#include "play.h"
#include "player.h"

/* Giant's Mask Anywhere */
s32 GiantMask_IsGiant(void);
s32 GiantMask_CanPutOn(PlayState* play);
s32 GiantMask_ShouldBlockUse(PlayState* play, Player* player);
s32 GiantMask_ShouldCheckItemUsabilityWhileSwimming(Player* player, u8 itemAction);
u32 GiantMask_GetWeaponDamageFlags(Player* player, u32 dmgFlags);
void GiantMask_Reset(PlayState* play);
void GiantMask_BeforePlayerUpdate(Player* player, PlayState* play);
void GiantMask_AfterPlayerUpdate(Player* player, PlayState* play);

#endif
