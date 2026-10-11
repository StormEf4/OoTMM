#ifndef COMBO_OOT_FIERCE_DEITY_H
#define COMBO_OOT_FIERCE_DEITY_H

#include <combo/types.h>
#include "play.h"
#include "player.h"

/* Fierce Deity's Mask (OoT) */
s32  FierceDeityOot_IsActive(void);
s32  FierceDeityOot_IsTransforming(void);
void FierceDeityOot_UseMask(PlayState* play, Player* player);
void FierceDeityOot_Reset(PlayState* play, Player* player);
void FierceDeityOot_BeforePlayerUpdate(Player* player, PlayState* play);
void FierceDeityOot_AfterPlayerUpdate(Player* player, PlayState* play);
s32  FierceDeityOot_IsFdForm(void);
f32  FierceDeityOot_GetHeight(void);
s32  FierceDeityOot_DrawSkeleton(PlayState* play, Vec3s* jointTable, s32 dListCount, void* overrideLimbDraw, void* postLimbDraw, void* arg, s32 lod);
s32  FierceDeityOot_DrawMask(PlayState* play, Player* player);

#endif
