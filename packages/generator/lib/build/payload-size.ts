import { promises as fs } from 'fs';
import path from 'path';

/*
 * Fork: checks the size of the C payloads right after they are built.
 *
 * The limits are the ones `buildPatchfiles` (lib/combo/patch-build/index.ts) enforces when a
 * seed is made: a payload over them makes every seed fail with "Payload too large". Checking at
 * build time makes the app build fail instead, and the log shows how much room is left, so the
 * fork's custom code never ships an app that can't make seeds.
 *
 * Only production builds (the app) fail. Debug builds (pnpm local) are bigger and only warn.
 */
const PAYLOAD_LIMITS = {
  oot: 0x90000, /* Fork: grown for the enemy shuffle merge, see include/combo/defs.h */
  mm: 0x80000, /* Upstream enemy shuffle raised MM PAYLOAD_SIZE to 0x80000 (include/combo/defs.h) */
} as const;

export async function checkPayloadSizes(binDir: string, isProd: boolean) {
  const errors: string[] = [];

  for (const [game, limit] of Object.entries(PAYLOAD_LIMITS)) {
    const stat = await fs.stat(path.join(binDir, `${game}_payload.bin`));
    const free = limit - stat.size;
    console.log(`Payload ${game}: ${stat.size} / ${limit} bytes (${free} free)`);
    if (free < 0) {
      errors.push(`Payload too large ${game}: ${stat.size} bytes, limit ${limit} (${-free} over)`);
    }
  }

  if (errors.length) {
    if (isProd) {
      throw new Error(errors.join('\n'));
    }
    for (const e of errors) {
      console.warn(`warning: ${e} (not a production build, so only a warning)`);
    }
  }
}
