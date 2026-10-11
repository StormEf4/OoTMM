#include <combo.h>
#include <combo/player.h>
#include <combo/sfx.h>
#include <combo/oot/fierce_deity.h>

/*
 * Fierce Deity's Mask (OoT)
 *
 * Work in progress: the item, its toggles and the item wiring are done, the transformation
 * is not written yet. Until then, using the mask only plays the error sound.
 * See docs/custom/giant-mask.md for the approach this will follow.
 */

s32 FierceDeityOot_IsActive(void)
{
    return 0;
}

s32 FierceDeityOot_IsTransforming(void)
{
    return 0;
}

void FierceDeityOot_UseMask(PlayState* play, Player* player)
{
    PlaySound(NA_SE_SY_ERROR);
}

void FierceDeityOot_Reset(PlayState* play, Player* player)
{
}

void FierceDeityOot_BeforePlayerUpdate(Player* player, PlayState* play)
{
}

void FierceDeityOot_AfterPlayerUpdate(Player* player, PlayState* play)
{
}
