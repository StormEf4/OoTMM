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
    key: 'goronMmTransformMaskOot',
    name: "MM Goron Transformation Mask (OoT)",
    category: 'items.extensions.crossGame.masks',
    game: 'oot',
    group: "MM Goron Transformation Mask",
    type: 'boolean',
    description: "Add Majora's Mask's Goron Mask in Ocarina of Time, as a new item next to OoT's own Goron Mask (which is unchanged). Wearing it turns Link into a Goron, like in Majora's Mask. The mask comes off when used again or when leaving the area.",
    default: false,
    cond: hasOoT,
  },
  {
    key: 'sharedMaskGoronMmTransform',
    name: "Shared MM Goron Transformation Mask",
    category: 'items.extensions.crossGame.masks',
    game: 'shared',
    group: "MM Goron Transformation Mask",
    type: 'boolean',
    description: "Combines the MM Goron Mask and the OoT MM Goron Transformation Mask into one item for both games. Not available with the Shared Goron Mask option, which already pairs the MM Goron Mask with OoT's own Goron Mask.",
    default: false,
    cond: (s: any) => hasOoTMM(s) && s.goronMmTransformMaskOot && !s.sharedMaskGoron,
  },
  {
    key: 'zoraMmTransformMaskOot',
    name: "MM Zora Transformation Mask (OoT)",
    category: 'items.extensions.crossGame.masks',
    game: 'oot',
    group: "MM Zora Transformation Mask",
    type: 'boolean',
    description: "Add Majora's Mask's Zora Mask in Ocarina of Time, as a new item next to OoT's own Zora Mask (which is unchanged). Wearing it turns Link into a Zora, like in Majora's Mask. The mask comes off when used again or when leaving the area.",
    default: false,
    cond: hasOoT,
  },
  {
    key: 'sharedMaskZoraMmTransform',
    name: "Shared MM Zora Transformation Mask",
    category: 'items.extensions.crossGame.masks',
    game: 'shared',
    group: "MM Zora Transformation Mask",
    type: 'boolean',
    description: "Combines the MM Zora Mask and the OoT MM Zora Transformation Mask into one item for both games. Not available with the Shared Zora Mask option, which already pairs the MM Zora Mask with OoT's own Zora Mask.",
    default: false,
    cond: (s: any) => hasOoTMM(s) && s.zoraMmTransformMaskOot && !s.sharedMaskZora,
  },
  {
    key: 'magicHookshotOot',
    name: 'Magic Hookshot (OoT)',
    category: 'custom.oot',
    type: 'enum',
    description: 'Adds a Magic Hookshot to the item pool. It works like your Hookshot or Longshot but can latch onto any surface, for 2 magic each time it latches onto a surface the normal hookshot cannot (latching onto normal hookshot targets stays free). Once found, press L or C-Up on the Hookshot in the pause menu to switch between the normal and magic modes (the cursor changes colour in magic mode). It needs a Hookshot to use. "Logical" lets the logic expect hookshotting anywhere once you have it.',
    values: [
      { value: 'off', name: 'Off' },
      { value: 'on', name: 'On' },
      { value: 'logical', name: 'Logical' },
    ],
    default: 'off',
    cond: hasOoT,
  },
  {
    key: 'magicHookshotMm',
    name: 'Magic Hookshot (MM)',
    category: 'custom.mm',
    type: 'enum',
    description: 'Adds a Magic Hookshot to the item pool. It works like your Hookshot or Longshot but can latch onto any surface, for 2 magic each time it latches onto a surface the normal hookshot cannot (latching onto normal hookshot targets stays free). Once found, press L or C-Up on the Hookshot in the pause menu to switch between the normal and magic modes (the cursor changes colour in magic mode). It needs a Hookshot to use. "Logical" lets the logic expect hookshotting anywhere once you have it.',
    values: [
      { value: 'off', name: 'Off' },
      { value: 'on', name: 'On' },
      { value: 'logical', name: 'Logical' },
    ],
    default: 'off',
    cond: hasMM,
  },
  {
    key: 'infiniteMagic',
    name: 'Infinite Magic',
    category: 'custom.general',
    type: 'enum',
    description: 'Adds Infinite Magic to the item pool. Once you have it, using magic never empties your meter (it turns blue, like after drinking Chateau Romani), so spells, magic arrows, the Lens of Truth and the Magic Hookshot cost nothing. It only works once you have a magic meter. "Progressive" makes it a third Magic Upgrade after the Larger Magic Upgrade. "Separate" makes it its own item that can be found at any time. With Shared Magic it works in both games.',
    values: [
      { value: 'off', name: 'Off' },
      { value: 'progressive', name: 'Progressive', description: 'Infinite Magic is the third Magic Upgrade: Magic Meter -> Larger Magic Meter -> Infinite Magic.' },
      { value: 'separate', name: 'Separate', description: 'Infinite Magic is its own item, independent of the Magic Upgrades.' },
    ],
    default: 'off',
  },
  // @custom-settings-end
] as const;
