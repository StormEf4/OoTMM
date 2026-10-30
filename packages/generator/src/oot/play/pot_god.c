#include <combo.h>

/*
 * Fork: the Pot God (Custom > Ocarina of Time > "Pot God").
 *
 * As child, the Market Pot House gets a watcher (Custom_Pot_God_Watcher)
 * that wakes the Pot God (Custom_Pot_God) once every pot is broken; the fight
 * happens in the boss kit's hellfire arena or right there, depending on the
 * "Pot God Fight" setting. Once it is defeated, 20 pots holding checks
 * appear all over Hyrule Field. They use the xflags declared in
 * data/checks/oot/overworld/hyrule_field.xml (room 0, actors 0xc0 to 0xd3).
 */

#define POT_GOD_FIELD_POTS      20
#define POT_GOD_FIELD_POT_ID    0xc0
#define POT_GOD_FIELD_POT_Y     2000.f

/* Pots use OBJECT_TSUBO (bit 8) and drop nothing of their own (0x1f) */
#define POT_GOD_FIELD_POT_PARAMS    (0x100 | 0x1f)

/* x, z: open spots spread across the field. The pots snap to the floor. */
static const s16 kFieldPots[POT_GOD_FIELD_POTS][2] = {
    { -4600,     0 },
    { -4300,  1050 },
    { -4850,  2950 },
    { -5150,  1650 },
    { -1780,  5409 },
    { -1907,  5260 },
    { -6280,  7250 },
    { -6150,  8579 },
    { -4250, 13600 },
    { -4600, 12900 },
    {   450, 11850 },
    {  -650, 11250 },
    {  1000, 13150 },
    {  3600,  6950 },
    {  3800,  6150 },
    {  2950,  7650 },
    {  3000,  4350 },
    {  3150,  1100 },
    {  2076,    50 },
    {  1100,  3500 },
};

static void PotGod_SpawnFieldPots(PlayState* play)
{
    Xflag xf;

    for (int i = 0; i < POT_GOD_FIELD_POTS; ++i)
    {
        Xflag_Clear(&xf);
        xf.sceneId = SCE_OOT_HYRULE_FIELD;
        xf.setupId = 0;
        xf.roomId = 0;
        xf.sliceId = 0;
        xf.id = POT_GOD_FIELD_POT_ID + i;

        g.xflagOverride = TRUE;
        g.xflagId = Xflag_Lookup(&xf);
        Actor_Spawn(&play->actorCtx, play, ACTOR_OBJ_TSUBO,
            kFieldPots[i][0], POT_GOD_FIELD_POT_Y, kFieldPots[i][1],
            0, 0, 0,
            POT_GOD_FIELD_POT_PARAMS
        );
        g.xflagOverride = FALSE;
    }
}

void PotGod_SpawnForScene(PlayState* play)
{
    if (!Config_Flag(CFG_CUSTOM_POT_GOD))
        return;

    switch (play->sceneId)
    {
    case SCE_OOT_GUARD_HOUSE:
        if (gSave.age == AGE_CHILD && !SaveFlag_Get(SAVE_FLAG_POT_GOD_DEFEATED))
            Actor_Spawn(&play->actorCtx, play, ACTOR_CUSTOM_POT_GOD_WATCHER, 0, 0, 0, 0, 0, 0, 0);
        break;
    case SCE_OOT_HYRULE_FIELD:
        if (SaveFlag_Get(SAVE_FLAG_POT_GOD_DEFEATED))
            PotGod_SpawnFieldPots(play);
        break;
    }
}
