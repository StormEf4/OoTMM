import { test, expect } from 'vitest';

import { makeTestSeed } from '../helpers';

test("Can make a seed - Pot God", async () => {
  const res = await makeTestSeed("PotGod", {
    potGod: true,
  });

  expect(res).toBeDefined();
})

test("Can make a seed - Pot God with the Pot Room Warp Song", async () => {
  const res = await makeTestSeed("PotGodWarpSong", {
    potGod: true,
    potGodWarpSong: true,
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
    potGodWarpSong: true,
    erIndoors: 'full',
  });

  expect(res).toBeDefined();
})
