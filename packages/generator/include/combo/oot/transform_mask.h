#ifndef COMBO_OOT_TRANSFORM_MASK_H
#define COMBO_OOT_TRANSFORM_MASK_H

#include <combo/types.h>
#include "play.h"
#include "player.h"

/* MM transformation masks in OoT: Deku Mask, MM Goron and MM Zora Transformation Masks */
#define TRANSFORM_OOT_FORM_NONE     0
#define TRANSFORM_OOT_FORM_DEKU     1
#define TRANSFORM_OOT_FORM_GORON    2
#define TRANSFORM_OOT_FORM_ZORA     3

s32  TransformOot_IsActive(void);
s32  TransformOot_IsTransforming(void);
s32  TransformOot_GetForm(void);
s32  TransformOot_IsFormMaskItem(s16 itemId);
s32  TransformOot_CanUseItem(s16 itemId);
void TransformOot_UseMask(PlayState* play, Player* player, s16 itemId);
void TransformOot_ForceLeave(PlayState* play, Player* player);
void TransformOot_AfterPlayerInit(PlayState* play, Player* player);
void TransformOot_BeforePlayerUpdate(Player* player, PlayState* play);
void TransformOot_AfterPlayerUpdate(Player* player, PlayState* play);
f32  TransformOot_GetHeight(Player* player);
s32  TransformOot_DrawSkeleton(PlayState* play, Vec3s* jointTable, s32 dListCount, void* overrideLimbDraw, void* postLimbDraw, void* arg, s32 lod);
s32  TransformOot_DrawMask(PlayState* play, Player* player, s32 form);
void TransformOot_BeforeInterfaceUpdate(void);
void TransformOot_AfterInterfaceUpdate(void);

#endif
