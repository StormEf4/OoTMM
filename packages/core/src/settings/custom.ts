/*
 * Custom settings for this fork.
 *
 * Every setting in CUSTOM_SETTINGS shows up in the generator under the
 * "Custom" page, grouped by its category (see CUSTOM_CATEGORIES).
 *
 * Boolean and enum settings are also passed to the game automatically as
 * config flags, so C code can read them without any extra wiring:
 *   boolean `myToggle`              -> Config_Flag(CFG_CUSTOM_MY_TOGGLE)
 *   enum `myChoice` = 'someValue'   -> Config_Flag(CFG_CUSTOM_MY_CHOICE_SOME_VALUE)
 *
 * The easiest way to add one is `pnpm custom:add`, which edits this file and
 * logs the change in CUSTOM_CHANGES.md. You can also edit this file by hand;
 * new entries go above the "@custom-settings-end" marker. Always give each
 * setting a `description` (it can be empty), as the app expects one.
 */

const hasOoT = (x: any) => x.games === 'oot' || x.games === 'ootmm';
const hasMM = (x: any) => x.games === 'mm' || x.games === 'ootmm';
const hasOoTMM = (x: any) => x.games === 'ootmm';

/* Avoid unused warnings when no setting uses a helper yet */
void hasOoT; void hasMM; void hasOoTMM;

export const CUSTOM_CATEGORIES = [
  { key: 'custom.general', name: 'General' },
  { key: 'custom.oot', name: 'Ocarina of Time' },
  { key: 'custom.mm', name: "Majora's Mask" },
] as const;

export const CUSTOM_SETTINGS = [
  {
    key: 'giantMaskAnywhere',
    name: 'Giant\'s Mask Anywhere',
    category: 'custom.mm',
    type: 'boolean',
    description: 'Allows the Giant\'s Mask to be used outside of Twinmold\'s Lair, as long as there is enough room above Link. The mask is removed when leaving the area. No logical applications',
    default: false,
    cond: hasMM,
  },
  {
    key: 'giantMaskOot',
    name: "Giant's Mask (OoT)",
    category: 'items.extensions.crossGame.masks',
    game: 'oot',
    group: "Giant's Mask",
    type: 'boolean',
    description: "Add the Giant's Mask in Ocarina of Time. Wearing it makes Link grow, like Giant's Mask Anywhere does in Majora's Mask, as long as there is enough room above him. It drains magic and comes off when leaving the area.",
    default: false,
    cond: hasOoT,
  },
  {
    key: 'sharedMaskGiant',
    name: "Shared Giant's Mask",
    category: 'items.extensions.crossGame.masks',
    game: 'shared',
    group: "Giant's Mask",
    type: 'boolean',
    description: "Combines the Giant's Masks from OoT and MM into one item for both games",
    default: false,
    cond: (s: any) => hasOoTMM(s) && s.giantMaskOot,
  },
  // @custom-settings-end
] as const;
