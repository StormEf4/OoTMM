import { test, expect } from 'vitest';

import { makeTestSeed } from '../helpers';

test("Can make a seed - Infinite Magic (progressive)", async () => {
  const res = await makeTestSeed("InfiniteMagicProgressive", {
    infiniteMagic: 'progressive',
  });

  expect(res).toBeDefined();
})

test("Can make a seed - Infinite Magic (separate)", async () => {
  const res = await makeTestSeed("InfiniteMagicSeparate", {
    infiniteMagic: 'separate',
  });

  expect(res).toBeDefined();
})

test("Can make a seed - Infinite Magic (progressive, shared magic)", async () => {
  const res = await makeTestSeed("InfiniteMagicProgressiveShared", {
    infiniteMagic: 'progressive',
    sharedMagic: true,
  });

  expect(res).toBeDefined();
})

test("Can make a seed - Infinite Magic (separate, shared magic)", async () => {
  const res = await makeTestSeed("InfiniteMagicSeparateShared", {
    infiniteMagic: 'separate',
    sharedMagic: true,
  });

  expect(res).toBeDefined();
})
