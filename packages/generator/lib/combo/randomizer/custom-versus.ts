/*
 * Fork: Custom > Versus (1v1) settings.
 *
 * The Rupee Sabotage Shop (src/common/sabotage.c) reads its effects, prices
 * and cooldown from SABOTAGE_CONFIG. See docs/custom/sabotage-shop.md.
 */
import type { Settings } from '@ootmm/core';

import { Patchfile } from '../patch-build/patchfile';
import { toU16Buffer } from '../util';

/* Same order as SABOTAGE_* in include/combo/sabotage.h */
export const SABOTAGE_EFFECTS = ['ice', 'fire', 'shock', 'knockback', 'antiMagic', 'rupeeThief', 'bombs', 'reverse', 'ambush', 'drain'] as const;

type SetValue = { type: string, values?: readonly string[], set?: readonly string[] };

export function sabotageEffects(settings: Settings): string[] {
  const value = (settings as any).sabotageEffects as SetValue | undefined;
  switch (value?.type) {
  case 'none':
    return [];
  case 'specific':
    return SABOTAGE_EFFECTS.filter(e => value.values?.includes(e));
  case 'random-mixed':
    return SABOTAGE_EFFECTS.filter(e => value.set?.includes(e));
  default:
    return [...SABOTAGE_EFFECTS];
  }
}

export function patchCustomVersus(settings: Settings, patchfile: Patchfile) {
  if (!settings.sabotageShop)
    return;

  const enabled = sabotageEffects(settings);
  let mask = 0;
  SABOTAGE_EFFECTS.forEach((e, i) => { if (enabled.includes(e)) mask |= (1 << i); });
  patchfile.addSymbolPatch('SABOTAGE_CONFIG', toU16Buffer([mask, settings.sabotagePrices, settings.sabotageCooldown, 0]));
}

/* The multiplayer client only talks to the server for Co-op and Multiworld
 * patches, so a Singleplayer seed with the shop is marked Co-op for it. The
 * game itself stays Singleplayer (no item sharing). */
export function versusPatchMode(settings: Settings): string {
  if (settings.mode === 'single' && settings.sabotageShop)
    return 'coop';
  return settings.mode;
}
