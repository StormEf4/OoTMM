---
name: fork-custom-items
description: Pattern used in Elijah's OoTMM fork for adding a brand-new randomizer item (Magic Hookshot as the example)
metadata:
  type: project
  modified: 2026-10-24T08:56:20.959Z
---

Elijah wants new gameplay features added as separate items or options next to vanilla, never replacing it. Each one gets a Custom-page setting and a CUSTOM_CHANGES.md entry (see CLAUDE.md).

Template for a new item, modeled on upstream's Shovel: a `gi.yml` entry with `add: [SAVE_FLAG, SAVE_FLAG_X]`, a new enum value in `save_flags.h` (the shared saveFlags byte), the item id in logic `defs.ts`, a name in `names.ts`, `ItemGroups.REQUIRED` if it's major, and `transform.ts` `addItem` gated on the custom setting. Use Off/On/Logical enums for "can logic use it". Mode toggles reuse the pause-menu slot cycling in `common/trade.c` (`comboToggleSlot`, `comboSlotCanToggle`).

Magic Hookshot: PR #5 (2026-10-24), not yet tested in-game. See [[c-patch-verification]].

Infinite Magic (2026-10-24): one `infiniteMagic` drop-down (Off/Progressive/Separate) in custom.general. Upstream magic upgrades are always progressive (no setting), so "progressive" here means a 3rd Magic Upgrade in the pool that `progressive.c` turns into the infinite item. The effect reuses Chateau Romani (OoT `chateauActive`, MM week event) set every frame from play.c, since all magic costs (incl. Magic Hookshot) already check it. Shared items: add a `kSimpleSharedItems` row in item_add.c plus SHARED_ITEMS_OOT/MM rows in randomizer/checks.ts.
