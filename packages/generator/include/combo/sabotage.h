#ifndef COMBO_SABOTAGE_H
#define COMBO_SABOTAGE_H

/*
 * Fork: Rupee Sabotage Shop (Custom > Versus (1v1) > "Rupee Sabotage Shop").
 * See src/common/sabotage.c and docs/custom/sabotage-shop.md.
 */

#include <ultra64.h>

/* Effects, in shop order. Bit N of SABOTAGE_CONFIG.effects enables effect N
 * (lib/combo/randomizer/custom-versus.ts uses the same order). */
#define SABOTAGE_ICE        0
#define SABOTAGE_FIRE       1
#define SABOTAGE_SHOCK      2
#define SABOTAGE_KNOCKBACK  3
#define SABOTAGE_ANTI_MAGIC 4
#define SABOTAGE_RUPEES     5
#define SABOTAGE_BOMBS      6
#define SABOTAGE_REVERSE    7
#define SABOTAGE_AMBUSH     8
#define SABOTAGE_DRAIN      9
#define SABOTAGE_MAX        10

/* A sabotage travels as a WAL item entry addressed to this "player", which no
 * real world uses (worlds are 1-127, 0 is self and 0xff is everyone), so
 * builds without the shop ignore it. Its key is SABOTAGE_KEY_MAGIC << 24 plus
 * the sender's 24-bit id (Multi_SabotageId). */
#define SABOTAGE_TO         0xfe
#define SABOTAGE_KEY_MAGIC  0x5a

typedef struct PlayState PlayState;

/* multi.c */
u32  Multi_SabotageId(void);
int  Multi_SendSabotage(u8 effect);

/* sabotage.c */
int  Sabotage_Receive(PlayState* play, u16 effect, u32 key, u8 from, const u8* name);
void Sabotage_Update(PlayState* play);
int  Sabotage_MenuNext(void);
void Sabotage_MenuUpdate(PlayState* play);
void Sabotage_MenuDraw(PlayState* play);

/* menu.c */
void Menu_PrintStr(PlayState* play, const char* str, int x, int y);
void Menu_PrintNum(PlayState* play, int num, int x, int y); /* 3 digits */

#endif
