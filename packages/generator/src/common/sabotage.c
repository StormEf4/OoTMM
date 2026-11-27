/*
 * Fork: Rupee Sabotage Shop (Custom > Versus (1v1) > "Rupee Sabotage Shop").
 *
 * A page of the pause menu's info screens (Map page, L or C-Up) sells effects
 * for rupees. Buying one sends it through the multiplayer client as a WAL item
 * entry for SABOTAGE_TO (multi.c, Multi_SendSabotage). Every other player in
 * the session gets it in Sabotage_Receive: a text box says who sent what, then
 * the effect hits. Trap effects reuse upstream's traps (traps.c), the others
 * are run here from Sabotage_Update, every frame before Play_Main.
 *
 * Notes: docs/custom/sabotage-shop.md.
 */
#include <combo.h>
#include <combo/actor.h>
#include <combo/config.h>
#include <combo/context.h>
#include <combo/global.h>
#include <combo/item.h>
#include <combo/math.h>
#include <combo/menu.h>
#include <combo/message.h>
#include <combo/multi.h>
#include <combo/player.h>
#include <combo/rand.h>
#include <combo/sabotage.h>
#include <combo/text.h>
#include <combo/traps.h>

#define ROWS            8       /* Effect rows on the page, a status line follows */
#define REVERSE_FRAMES  (20 * 20)
#define STATUS_FRAMES   40

enum
{
    STATUS_READY,
    STATUS_SENT,
    STATUS_POOR,
    STATUS_OFFLINE,
    STATUS_WAIT,
};

static const char* const kStatusText[] = {
    "A: Buy",
    "Sent!",
    "Not enough rupees",
    "Client not connected",
    "Ready in",
};

/* Set by the generator (lib/combo/randomizer/custom-versus.ts) */
typedef struct
{
    u16 effects;    /* Bit per SABOTAGE_* effect */
    u16 priceScale; /* Percent */
    u16 cooldown;   /* Seconds */
    u16 unused;
}
SabotageConfig;

SabotageConfig gSabotageConfig;
EXPORT_SYMBOL(SABOTAGE_CONFIG, gSabotageConfig);

typedef struct
{
    const char* name;
    u8          price;
    u8          trap;   /* TRAP_* for trap effects, 0xff otherwise */
    u8          count;  /* Traps to queue */
}
SabotageEffect;

static const SabotageEffect kSabotageEffects[SABOTAGE_MAX] = {
    [SABOTAGE_ICE]          = { "Ice Trap",         30, TRAP_ICE,        1 },
    [SABOTAGE_FIRE]         = { "Fire Trap",        40, TRAP_FIRE,       1 },
    [SABOTAGE_SHOCK]        = { "Shock Trap",       40, TRAP_SHOCK,      1 },
    [SABOTAGE_KNOCKBACK]    = { "Knockback Trap",   20, TRAP_KNOCKBACK,  1 },
    [SABOTAGE_ANTI_MAGIC]   = { "Anti-Magic Trap",  30, TRAP_ANTI_MAGIC, 1 },
    [SABOTAGE_RUPEES]       = { "Rupee Thief",      30, TRAP_RUPOOR,     5 },
    [SABOTAGE_BOMBS]        = { "Bomb Drop",        50, 0xff,            0 },
    [SABOTAGE_REVERSE]      = { "Reverse Controls", 60, 0xff,            0 },
    [SABOTAGE_AMBUSH]       = { "Enemy Ambush",     80, 0xff,            0 },
    [SABOTAGE_DRAIN]        = { "Drain Trap",       99, TRAP_DRAIN,      1 },
};

static struct
{
    u8  pending[SABOTAGE_MAX]; /* Effects waiting to hit us (non-trap ones) */
    u16 reverseTimer;
    u16 cooldown;
    u8  status;
    u8  statusTimer;
    u8  sel;
    u8  delay;
}
sState;

static int __attribute__((noinline)) Sabotage_List(u8* out)
{
    int count;

    count = 0;
    for (int i = 0; i < SABOTAGE_MAX; ++i)
    {
        if (gSabotageConfig.effects & (1 << i))
            out[count++] = i;
    }
    return count;
}

static int __attribute__((noinline)) Sabotage_Price(int effect)
{
    int price;

    price = kSabotageEffects[effect].price * gSabotageConfig.priceScale / 100;
    return price > 999 ? 999 : price;
}

static void Sabotage_Buy(int effect)
{
    int price;
    u8 status;

    price = Sabotage_Price(effect);
    if (sState.cooldown)
        status = STATUS_WAIT;
    else if (gSave.info.playerData.rupees < price)
        status = STATUS_POOR;
    else if (!Multi_SendSabotage(effect))
        status = STATUS_OFFLINE;
    else
    {
        gSave.info.playerData.rupees -= price;
        sState.cooldown = gSabotageConfig.cooldown * 20;
        status = STATUS_SENT;
    }

    sState.status = status;
    sState.statusTimer = STATUS_FRAMES;
    PlaySound(status == STATUS_SENT ? 0x4808 : 0x4806);
}

int Sabotage_MenuNext(void)
{
    if (!Config_Flag(CFG_CUSTOM_SABOTAGE_SHOP))
        return 0;

    /* The shop comes first, then upstream's pages */
    if (g.menuScreen == MENU_NONE)
    {
        PlaySound(0x4809);
        g.menuScreen = MENU_SABOTAGE;
        return 1;
    }
    if (g.menuScreen == MENU_SABOTAGE)
        g.menuScreen = MENU_NONE;
    return 0;
}

void Sabotage_MenuUpdate(PlayState* play)
{
    u8 list[SABOTAGE_MAX];
    int count;
    int stickY;
    int sel;

    count = Sabotage_List(list);
    if (!count)
        return;
    sel = sState.sel;
    if (sel >= count)
        sel = count - 1;

    if (sState.delay)
        sState.delay--;
    else
    {
        stickY = play->state.input[0].cur.stick_y;
        if ((stickY > 64 && sel > 0) || (stickY < -64 && sel < count - 1))
        {
            sel += (stickY > 0) ? -1 : 1;
            sState.delay = 3;
            PlaySound(0x4809);
        }
    }
    sState.sel = sel;

    if (play->state.input[0].press.button & A_BUTTON)
        Sabotage_Buy(list[sel]);
}

static void __attribute__((noinline)) Sabotage_Color(PlayState* play, u32 rgb)
{
    OPEN_DISPS(play->state.gfxCtx);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff, 255);
    CLOSE_DISPS();
}

void Sabotage_MenuDraw(PlayState* play)
{
    u8 list[SABOTAGE_MAX];
    int count;
    int top;
    int index;
    int price;
    int y;
    int status;

    count = Sabotage_List(list);
    top = (sState.sel >= ROWS) ? sState.sel - (ROWS - 1) : 0;

    Sabotage_Color(play, 0xffff00);
    Menu_PrintStr(play, "Sabotage Shop", -110, 54);

    y = 42;
    for (int i = 0; i < ROWS; ++i)
    {
        index = top + i;
        if (index >= count)
            break;
        price = Sabotage_Price(list[index]);
        Sabotage_Color(play, index == sState.sel ? 0xffff00 : 0xffffff);
        if (index == sState.sel)
            Menu_PrintStr(play, ">", -118, y);
        Menu_PrintStr(play, kSabotageEffects[list[index]].name, -110, y);
        Menu_PrintNum(play, price, 70, y);
        y -= 12;
    }

    /* Status line */
    y = 42 - 12 * ROWS;
    if (sState.statusTimer && sState.status != STATUS_WAIT)
        status = sState.status;
    else if (sState.cooldown)
    {
        status = STATUS_WAIT;
        Menu_PrintNum(play, (sState.cooldown + 19) / 20, -30, y);
    }
    else
        status = gComboCtx.isMultiConnected ? STATUS_READY : STATUS_OFFLINE;
    Sabotage_Color(play, 0xffffff);
    Menu_PrintStr(play, kStatusText[status], -110, y);
}

int Sabotage_Receive(PlayState* play, u16 effect, u32 key, u8 from, const u8* name)
{
    const SabotageEffect* e;
    char* b;
    char* start;
    char sender[9];

    if (!Config_Flag(CFG_CUSTOM_SABOTAGE_SHOP) || (key >> 24) != SABOTAGE_KEY_MAGIC || effect >= SABOTAGE_MAX)
        return 1;

    /* Our own purchase coming back */
    if (from == Multi_WorldID() && (key & 0xffffff) == Multi_SabotageId())
        return 1;

    if (!play || play->pauseCtx.state || !Item_SafeToReceive(play) || g.decoysCount)
        return 0;

    /* Queue the effect: traps run from traps.c once Link is free, the rest from Sabotage_Update */
    e = &kSabotageEffects[effect];
    if (e->trap != 0xff)
    {
        for (int i = 0; i < e->count; ++i)
        {
            if (gSharedCustomSave.traps[e->trap] < 255)
                gSharedCustomSave.traps[e->trap]++;
        }
    }
    else if (sState.pending[effect] < 255)
        sState.pending[effect]++;

    /* Tell who did it */
    memcpy(sender, name, 8);
    sender[8] = 0;
    Message_StartBlocking(play);
#if defined(GAME_OOT)
    b = play->msgCtx.font.msgBuf;
#else
    b = play->msgCtx.font.textBuffer.schar;
#endif
    comboTextAppendHeader(&b);
    start = b;
    comboTextAppendStr(&b, TEXT_COLOR_YELLOW);
    comboTextAppendStr(&b, sender[0] ? sender : "Your rival");
    comboTextAppendClearColor(&b);
    comboTextAppendStr(&b, " sabotaged you: " TEXT_COLOR_RED);
    comboTextAppendStr(&b, e->name);
    comboTextAppendClearColor(&b);
    comboTextAppendStr(&b, "!" TEXT_END);
    comboTextAutoLineBreaks(start);

    return 1;
}

static int __attribute__((noinline)) Sabotage_SpawnAround(PlayState* play, Player* link, s16 actorId, u16 params, int count, float radius, float height)
{
    int spawned;
    s16 angle;

    spawned = 0;
    angle = link->actor.shape.rot.y;
    for (int i = 0; i < count; ++i)
    {
        if (Actor_Spawn(&play->actorCtx, play, actorId,
            link->actor.world.pos.x + radius * Math_SinS(angle),
            link->actor.world.pos.y + height,
            link->actor.world.pos.z + radius * Math_CosS(angle),
            0, angle + 0x8000, 0, params))
        {
            spawned++;
        }
        angle += 0x5555;
    }
    return spawned;
}

static void Sabotage_Hit(PlayState* play, Player* link, int effect)
{
    switch (effect)
    {
    case SABOTAGE_REVERSE:
        sState.reverseTimer += REVERSE_FRAMES;
        return;
    case SABOTAGE_AMBUSH:
        /* A Wallmaster (its shadow follows Link, then it drops) or three
         * flying Keese, picked at random. Enemies can't spawn in cleared rooms
         * or without their soul, and MM only has their models where they
         * normally appear: try the other one, then drop bombs */
        for (int i = 0; i < 2; ++i)
        {
            if (((play->gameplayFrames ^ i) & 1)
                ? Sabotage_SpawnAround(play, link, ACTOR_EN_FIREFLY, 2, 3, 80.f, 60.f)
                : Sabotage_SpawnAround(play, link, ACTOR_EN_WALLMAS, 0, 1, 0.f, 0.f))
                return;
        }
        /* fallthrough */
    case SABOTAGE_BOMBS:
        Sabotage_SpawnAround(play, link, ACTOR_EN_BOM, 0, 3, 45.f, 30.f);
        return;
    }
}

static void __attribute__((noinline)) Sabotage_Flip(s8* v)
{
    *v = (*v == -128) ? 127 : -*v;
}

void Sabotage_Update(PlayState* play)
{
    Player* link;
    Input* input;

    if (!Config_Flag(CFG_CUSTOM_SABOTAGE_SHOP))
        return;

    if (sState.cooldown)
        sState.cooldown--;
    if (sState.statusTimer)
        sState.statusTimer--;

    if (play->pauseCtx.state)
        return;

    if (sState.reverseTimer && !Player_InCsMode(play))
    {
        sState.reverseTimer--;
        input = &play->state.input[0];
        for (int i = 0; i < 2; ++i)
        {
            Sabotage_Flip(&input->cur.stick_x + i);
            Sabotage_Flip(&input->rel.stick_x + i);
        }
    }

    if (!Item_SafeToReceive(play))
        return;
    link = GET_PLAYER(play);
    if (!link || (link->stateFlags1 & (PLAYER_ACTOR_STATE_CLIMB | PLAYER_ACTOR_STATE_DEATH | PLAYER_ACTOR_STATE_TALKING)))
        return;

    for (int i = SABOTAGE_BOMBS; i <= SABOTAGE_AMBUSH; ++i)
    {
        if (sState.pending[i])
        {
            sState.pending[i]--;
            Sabotage_Hit(play, link, i);
            return;
        }
    }
}
