#include <combo.h>
#include <combo/magic.h>

/*
 * Infinite Magic: a fork custom item. While it is owned, the game behaves as if
 * Chateau Romani was always active (magic costs nothing and the meter is blue),
 * and the meter is kept full so nothing can run out of magic.
 */

#if defined(GAME_OOT)
# define SF_MAGIC_INFINITE  SAVE_FLAG_MAGIC_INFINITE_OOT
#else
# define SF_MAGIC_INFINITE  SAVE_FLAG_MAGIC_INFINITE_MM
#endif

int comboHasInfiniteMagic(void)
{
    return SaveFlag_Get(SF_MAGIC_INFINITE);
}

/* Called every frame from Play_Update */
void comboInfiniteMagicUpdate(void)
{
    if (gSaveContext.gameMode)
        return;
    if (!comboHasInfiniteMagic())
        return;

    /* Reuse the Chateau Romani effect, which every magic cost already checks */
#if defined(GAME_OOT)
    gCustomSave.chateauActive = 1;
#else
    MM_SET_EVENT_WEEK(EV_MM_WEEK_DRANK_CHATEAU_ROMANI);
#endif

    /* Keep the meter full, e.g. after an Anti-Magic Trap or if it was not full when found */
    if (gSave.info.playerData.isMagicAcquired && gSaveContext.magicState == MAGIC_STATE_IDLE && gSave.info.playerData.magic < gSaveContext.magicCapacity)
        gSave.info.playerData.magic = gSaveContext.magicCapacity;
}
