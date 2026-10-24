import { test, expect } from 'vitest';

import { makeTestSeed } from '../helpers';

test("Can make a seed - Magic Hookshot", async () => {
  const res = await makeTestSeed("MagicHookshot", {
    magicHookshotOot: 'logical',
    magicHookshotMm: 'logical',
  });

  expect(res).toBeDefined();
})

test("Can make a seed - Magic Hookshot (not logical)", async () => {
  const res = await makeTestSeed("MagicHookshotOn", {
    magicHookshotOot: 'on',
    magicHookshotMm: 'on',
  });

  expect(res).toBeDefined();
})
