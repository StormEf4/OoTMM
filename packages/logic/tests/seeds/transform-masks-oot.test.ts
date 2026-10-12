import { test, expect } from 'vitest';

import { makeTestSeed } from '../helpers';

/* Fork: MM transformation masks in OoT, see CUSTOM_CHANGES.md */
test("Can make a seed - Deku Mask (OoT)", async () => {
  const res = await makeTestSeed("DEKU-OOT", {
    dekuMaskOot: true,
  });
  expect(res).toBeDefined();
});

test("Can make a seed - Shared Deku Mask", async () => {
  const res = await makeTestSeed("DEKU-SHARED", {
    dekuMaskOot: true,
    sharedMaskDeku: true,
  });
  expect(res).toBeDefined();
});

test("Can make a seed - Goron and Zora Masks transform in OoT", async () => {
  const res = await makeTestSeed("GORON-ZORA-OOT", {
    goronMaskTransformOot: true,
    zoraMaskTransformOot: true,
    sharedMaskGoron: true,
    sharedMaskZora: true,
  });
  expect(res).toBeDefined();
});
