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
  {
    key: 'fierceDeityMaskOot',
    name: "Fierce Deity's Mask (OoT)",
    category: 'items.extensions.crossGame.masks',
    game: 'oot',
    group: "Fierce Deity's Mask",
    type: 'boolean',
    description: "Add the Fierce Deity's Mask in Ocarina of Time. Wearing it turns Link into the Fierce Deity, like in Majora's Mask: full size, a stronger sword that shoots beams with magic, and no other items. It comes off when leaving the area.",
    default: false,
    cond: hasOoT,
  },
  {
    key: 'sharedMaskFierceDeity',
    name: "Shared Fierce Deity's Mask",
    category: 'items.extensions.crossGame.masks',
    game: 'shared',
    group: "Fierce Deity's Mask",
    type: 'boolean',
    description: "Combines the Fierce Deity's Masks from OoT and MM into one item for both games",
    default: false,
    cond: (s: any) => hasOoTMM(s) && s.fierceDeityMaskOot,
  },
  {
    key: 'dekuMaskOot',
    name: "Deku Mask (OoT)",
    category: 'items.extensions.crossGame.masks',
    game: 'oot',
    group: "Deku Mask",
    type: 'boolean',
    description: "Add the Deku Mask in Ocarina of Time. Wearing it turns Link into a Deku Scrub, like in Majora's Mask: small and light, no sword or other items, and he sinks in deep water. The mask comes off when used again or when leaving the area.",
    default: false,
    cond: hasOoT,
  },
  {
    key: 'sharedMaskDeku',
    name: "Shared Deku Mask",
    category: 'items.extensions.crossGame.masks',
    game: 'shared',
    group: "Deku Mask",
    type: 'boolean',
    description: "Combines the Deku Masks from OoT and MM into one item for both games",
    default: false,
    cond: (s: any) => hasOoTMM(s) && s.dekuMaskOot,
  },
  {
    key: 'goronMaskTransformOot',
    name: "Goron Mask transforms (OoT)",
    category: 'items.extensions.crossGame.masks',
    game: 'oot',
    group: "Goron Mask",
    type: 'boolean',
    description: "The OoT Goron Mask turns Link into a Goron, like the Majora's Mask one, instead of being a cosmetic mask. Use the Shared Goron Mask option (Shared items) to make it the same item as the MM one.",
    default: false,
    cond: hasOoT,
  },
  {
    key: 'zoraMaskTransformOot',
    name: "Zora Mask transforms (OoT)",
    category: 'items.extensions.crossGame.masks',
    game: 'oot',
    group: "Zora Mask",
    type: 'boolean',
    description: "The OoT Zora Mask turns Link into a Zora, like the Majora's Mask one, instead of being a cosmetic mask. Use the Shared Zora Mask option (Shared items) to make it the same item as the MM one.",
    default: false,
    cond: hasOoT,
  },
  // @custom-settings-end
] as const;
