/*
 * Custom cosmetics for this fork.
 *
 * These are appended to the upstream COSMETICS list in data.ts, so they show
 * in the Cosmetics tab next to the upstream ones. Keeping them here keeps
 * upstream merges of data.ts clean.
 *
 * An 'enum' cosmetic is a drop-down: `values` lists the choices and
 * `default` is the value used when nothing is picked.
 */

export const CUSTOM_COSMETICS = [{
  key: 'fairyStyle',
  name: 'Fairy Companion',
  description: 'Changes Navi (OoT) and Tatl (MM).<br>Talking Furbies: Navi and Tatl become a little Furby that always faces the camera, inside the usual fairy glow. They are called "Furby" in all dialogue and glow Furby pink and purple when idle. The voice is unchanged for now.<br>RGB Glow: Navi and Tatl keep their look, but their glow cycles through every color.<br>Talking Furbies + RGB Glow: both.',
  type: 'enum',
  values: [
    { value: 'default', name: 'Default' },
    { value: 'furby', name: 'Talking Furbies' },
    { value: 'rgb', name: 'RGB Glow' },
    { value: 'furbyRgb', name: 'Talking Furbies + RGB Glow' },
  ],
  default: 'default',
}] as const;
