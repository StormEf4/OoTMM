import { test, expect } from 'vitest';

import { makeTestSeed } from '../helpers';

/* Fork: Fierce Deity's Mask (OoT), see CUSTOM_CHANGES.md */
test("Can make a seed - Fierce Deity's Mask (OoT)", async () => {
  const res = await makeTestSeed("FIERCE-DEITY-OOT", {
    fierceDeityMaskOot: true,
  });
  expect(res).toBeDefined();
});

test("Can make a seed - Shared Fierce Deity's Mask", async () => {
  const res = await makeTestSeed("FIERCE-DEITY-SHARED", {
    fierceDeityMaskOot: true,
    sharedMaskFierceDeity: true,
  });
  expect(res).toBeDefined();
});
