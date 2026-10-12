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

test("Can make a seed - MM Goron and Zora Transformation Masks (OoT)", async () => {
  const res = await makeTestSeed("GORON-ZORA-MM-OOT", {
    goronMmTransformMaskOot: true,
    zoraMmTransformMaskOot: true,
  });
  expect(res).toBeDefined();
});

test("Can make a seed - Shared MM Goron and Zora Transformation Masks", async () => {
  const res = await makeTestSeed("GORON-ZORA-MM-SHARED", {
    goronMmTransformMaskOot: true,
    zoraMmTransformMaskOot: true,
    sharedMaskGoronMmTransform: true,
    sharedMaskZoraMmTransform: true,
  });
  expect(res).toBeDefined();
});

test("Can make a seed - MM Transformation Masks next to the upstream Shared Goron and Zora Masks", async () => {
  const res = await makeTestSeed("GORON-ZORA-MM-UPSTREAM-SHARED", {
    goronMmTransformMaskOot: true,
    zoraMmTransformMaskOot: true,
    sharedMaskGoron: true,
    sharedMaskZora: true,
  });
  expect(res).toBeDefined();
});
