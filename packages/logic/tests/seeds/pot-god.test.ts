import { test, expect } from 'vitest';

import { makeTestSeed } from '../helpers';

test("Can make a seed - Pot God", async () => {
  const res = await makeTestSeed("PotGod", {
    potGod: true,
  });

  expect(res).toBeDefined();
})

test("Can make a seed - Pot God with reverse Saria's Song warp", async () => {
  const res = await makeTestSeed("PotGodSariaWarp", {
    potGod: true,
    potGodSariaWarp: true,
  });

  expect(res).toBeDefined();
})

test("Can make a seed - Pot God with pot shuffle", async () => {
  const res = await makeTestSeed("PotGodPots", {
    potGod: true,
    shufflePotsOot: 'all',
  });

  expect(res).toBeDefined();
})

test("Can make a seed - Pot God with indoors ER", async () => {
  const res = await makeTestSeed("PotGodEr", {
    potGod: true,
    potGodSariaWarp: true,
    erIndoors: 'full',
  });

  expect(res).toBeDefined();
})
