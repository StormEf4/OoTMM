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
  { key: 'custom.versus', name: 'Versus (1v1)' },
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
  {
    key: 'potGod',
    name: 'Pot God',
    category: 'custom.oot',
    type: 'boolean',
    description: 'Adds the Pot God, a mini boss hiding in the Market Pot House. As child, break every pot in the room: the guard screams "OH GOD!", collapses and sinks into the floor, and the Pot God bursts out of the ground: a giant bleeding pot with Dead Hand\'s arms writhing out of it. With the Hellfire Arena (see "Pot God Fight"), the guard grabs Link and drags him down with him first. Defeat it (sword or Deku Sticks) and 20 pots appear all over Hyrule Field, each holding a randomized check. The 20 pots are added to the pool whatever the pot shuffle setting is.',
    default: false,
    cond: hasOoT,
  },
  {
    key: 'potGodWarpSong',
    name: 'Pot Room Warp Song',
    category: 'custom.oot',
    type: 'boolean',
    description: "A new song, A, C-Up, A, C-Up, C-Left, C-Right, that warps you to the Market Pot House from anywhere in Ocarina of Time where warp songs work. It asks before warping. It only needs the Ocarina (and those buttons, with ocarina buttons shuffled), and no other song in either game ends with these notes, so it doesn't touch the Song of Healing or any other song. It works when you play the ocarina freely, not when something asks you for a song. Logic can use the warp.",
    default: false,
    cond: hasOoT,
  },
  {
    key: 'potGodArena',
    name: 'Pot God Fight',
    category: 'custom.oot',
    type: 'enum',
    description: 'Where the Pot God is fought once it wakes up.',
    values: [
      { value: 'arena', name: 'Hellfire Arena', description: 'The guard drags Link down with him into a hellfire arena (Volvagia\'s lair, without Volvagia or the heat), where the Pot God bursts out of the ground. Dying there restarts the fight; winning sends Link back to the Pot House.' },
      { value: 'room', name: 'Pot House', description: 'The Pot God bursts out of the floor of the Pot House and is fought right there.' },
    ],
    default: 'arena',
    cond: (s: any) => hasOoT(s) && s.potGod,
  },
  {
    key: 'bossKitHealthBar',
    name: 'Boss Kit: Health Bars',
    category: 'custom.oot',
    type: 'boolean',
    description: 'Shows a health bar at the bottom of the screen while fighting the fork\'s custom bosses (the Pot God, and any future boss made with the boss kit).',
    default: false,
    cond: hasOoT,
  },
  {
    key: 'sabotageShop',
    name: 'Rupee Sabotage Shop',
    category: 'custom.versus',
    type: 'boolean',
    description: 'For two (or more) players connected through the OoTMM multiplayer client: spend your rupees to mess with your rival. Pause, go to the Map page and press L (or C-Up) to open the "Sabotage Shop". Pick an effect with the stick and press A to buy it: the rupees are taken and the effect hits your rival a moment later, with a message saying who sent it. Works on a Singleplayer seed that both players race (one person generates it and the other builds their ROM from the same .ootmm file in Patch mode) and on Multiworld seeds (it hits every other player). Both players need Project64-EM and the OoTMM client running, like for Multiworld. Purchases only go through while the client is connected.',
    default: false,
  },
  {
    key: 'sabotageEffects',
    name: 'Sabotage Effects',
    category: 'custom.versus',
    type: 'set',
    description: 'Which effects the Sabotage Shop sells. Prices are for "Sabotage Prices" at 100%.',
    values: [
      { value: 'ice', name: 'Ice Trap (30)', description: 'Freezes the rival in ice.' },
      { value: 'fire', name: 'Fire Trap (40)', description: 'Sets the rival on fire (the Goron Tunic protects).' },
      { value: 'shock', name: 'Shock Trap (40)', description: 'Electrocutes the rival.' },
      { value: 'knockback', name: 'Knockback Trap (20)', description: 'Knocks the rival over in a random direction.' },
      { value: 'antiMagic', name: 'Anti-Magic Trap (30)', description: 'Empties the rival\'s magic meter.' },
      { value: 'rupeeThief', name: 'Rupee Thief (30)', description: 'The rival loses 50 rupees (five Rupoors).' },
      { value: 'bombs', name: 'Bomb Drop (50)', description: 'Three lit bombs land around the rival.' },
      { value: 'reverse', name: 'Reverse Controls (60)', description: 'The rival\'s control stick is reversed for 20 seconds.' },
      { value: 'ambush', name: 'Enemy Ambush (80)', description: 'A Wallmaster drops on the rival or a swarm of Keese attacks them. Where those enemies can\'t appear (cleared rooms, missing enemy souls, or in Majora\'s Mask a place without them), the rival gets a Bomb Drop instead.' },
      { value: 'drain', name: 'Drain Trap (99)', description: 'Drops the rival to almost no health.' },
    ],
    default: 'all',
    cond: (s: any) => s.sabotageShop,
  },
  {
    key: 'sabotagePrices',
    name: 'Sabotage Prices (%)',
    category: 'custom.versus',
    type: 'number',
    description: 'Scales every Sabotage Shop price. 100 uses the prices listed in "Sabotage Effects", 50 halves them, 200 doubles them. Prices are capped at 999 rupees.',
    default: 100,
    min: 0,
    max: 1000,
    cond: (s: any) => s.sabotageShop,
  },
  {
    key: 'sabotageCooldown',
    name: 'Sabotage Cooldown (seconds)',
    category: 'custom.versus',
    type: 'number',
    description: 'How long a player has to wait after buying something in the Sabotage Shop before they can buy again.',
    default: 30,
    min: 0,
    max: 600,
    cond: (s: any) => s.sabotageShop,
  },
  // @custom-settings-end
] as const;
