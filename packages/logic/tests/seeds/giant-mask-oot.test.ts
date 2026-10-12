import { test, expect } from 'vitest';

import { makeTestSeed } from '../helpers';

/* Fork: Giant's Mask (OoT), see CUSTOM_CHANGES.md */
test("Can make a seed - Giant's Mask (OoT)", async () => {
  const res = await makeTestSeed("GIANT-MASK-OOT", {
    giantMaskOot: true,
  });
  expect(res).toBeDefined();
});

test("Can make a seed - Shared Giant's Mask", async () => {
  const res = await makeTestSeed("GIANT-MASK-SHARED", {
    giantMaskOot: true,
    sharedMaskGiant: true,
  });
  expect(res).toBeDefined();
});
