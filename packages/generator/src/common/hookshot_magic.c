#include <combo.h>
#include <combo/inventory.h>
#include <combo/sfx.h>
#include <combo/magic.h>

/*
 * Magic Hookshot: a separate item that lets the hookshot/longshot latch onto
 * any surface. Once owned, the hookshot slot gains a "magic" mode that is
 * toggled from the pause menu, so the vanilla hookshots keep working as usual.
 */

#if defined(GAME_OOT)
# define SF_HOOKSHOT_MAGIC      SAVE_FLAG_HOOKSHOT_MAGIC_OOT
# define SF_HOOKSHOT_MAGIC_OFF  SAVE_FLAG_HOOKSHOT_MAGIC_OFF_OOT
#else
# define SF_HOOKSHOT_MAGIC      SAVE_FLAG_HOOKSHOT_MAGIC_MM
# define SF_HOOKSHOT_MAGIC_OFF  SAVE_FLAG_HOOKSHOT_MAGIC_OFF_MM
#endif

int HookshotMagic_IsOwned(void)
{
    return SaveFlag_Get(SF_HOOKSHOT_MAGIC);
}

int HookshotMagic_IsActive(void)
{
    return HookshotMagic_IsOwned() && !SaveFlag_Get(SF_HOOKSHOT_MAGIC_OFF);
}

void HookshotMagic_Toggle(void)
{
    if (!HookshotMagic_IsOwned())
        return;

    if (HookshotMagic_IsActive())
    {
        SaveFlag_Set(SF_HOOKSHOT_MAGIC_OFF);
    }
    else
    {
        BITMAP8_CLEAR(gSharedCustomSave.saveFlags, SF_HOOKSHOT_MAGIC_OFF);
        PlaySound(NA_SE_SY_TRE_BOX_APPEAR);
    }
}

/* Magic spent each time the magic mode latches onto a surface the normal hookshot can't */
#define HOOKSHOT_MAGIC_COST 2

static int HookshotMagic_Consume(void)
{
#if defined(GAME_OOT)
    /* Handles the Chateau Romani (free magic) */
    return Magic_RequestChangeCustom(gPlay, HOOKSHOT_MAGIC_COST, MAGIC_CONSUME_NOW);
#else
    return Magic_RequestChange(gPlay, HOOKSHOT_MAGIC_COST, MAGIC_CONSUME_NOW);
#endif
}

/* Called by Arms_Hook in place of SurfaceType_IsHookshotSurface */
s32 ArmsHook_IsHookshotSurface(CollisionContext* colCtx, CollisionPoly* poly, s32 bgId)
{
    if (SurfaceType_IsHookshotSurface(colCtx, poly, bgId))
        return 1;
    /* Without enough magic the hook bounces off like a normal hookshot */
    if (HookshotMagic_IsActive() && HookshotMagic_Consume())
        return 1;
    return 0;
}
