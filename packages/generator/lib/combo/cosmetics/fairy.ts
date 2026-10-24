import type { Game } from '@ootmm/data';
import type { Monitor } from '@ootmm/core';

import { RomBuilder } from '../rom-builder';
import { bufReadU16BE, bufReadU32BE, bufWriteU32BE } from '../util/buffer';

/*
 * Fairy Companion cosmetic (fork).
 *
 * Talking Furbies renames Navi and Tatl to "Furby" in every vanilla message
 * and recolors their idle glow. Everything is found by scanning the vanilla
 * files, so no addresses are hardcoded. If something isn't found the change
 * is skipped with a warning instead of breaking the seed.
 *
 * The Furby sprite and the RGB glow are drawn by the game
 * (src/common/fairy_companion.c), turned on through the FAIRY_STYLE byte.
 */

export type FairyStyle = 'default' | 'furby' | 'rgb' | 'furbyRgb';

/* Bits of FAIRY_STYLE, see src/common/fairy_companion.c */
const FAIRY_STYLE_FURBY = 1 << 0;
const FAIRY_STYLE_RGB = 1 << 1;

const FAIRY_STYLE_FLAGS: Record<FairyStyle, number> = {
  default: 0,
  furby: FAIRY_STYLE_FURBY,
  rgb: FAIRY_STYLE_RGB,
  furbyRgb: FAIRY_STYLE_FURBY | FAIRY_STYLE_RGB,
};

export type MessageFormat = {
  /* Segment used by the message table entries (0x07 in OoT, 0x08 in MM) */
  segment: number;
  /* Bytes before the text (MM messages start with an 11 byte header) */
  headerSize: number;
  /* End of message control code */
  end: number;
  /* Control codes followed by argument bytes, and how many */
  args: { [code: number]: number };
};

export const MESSAGE_FORMATS: Record<Game, MessageFormat> = {
  oot: {
    segment: 0x07,
    headerSize: 0,
    end: 0x02,
    args: { 0x05: 1, 0x06: 1, 0x07: 2, 0x0c: 1, 0x0e: 1, 0x11: 2, 0x12: 2, 0x13: 1, 0x14: 1, 0x15: 3, 0x1e: 1 },
  },
  mm: {
    segment: 0x08,
    headerSize: 11,
    end: 0xbf,
    args: { 0x14: 1, 0x1b: 2, 0x1c: 2, 0x1d: 2, 0x1e: 2, 0x1f: 2 },
  },
};

type FairyGame = {
  name: string;
  messageFile: string;
  /* Vanilla sAttentionColors in code: 13 entries of inner RGBA + outer RGBA */
  attentionColors: number[][];
};

const ATTENTION_GREEN = [0, 255, 0, 255, 0, 255, 0, 0];
const ATTENTION_NPC = [150, 150, 255, 255, 150, 150, 255, 0];
const ATTENTION_ENEMY = [255, 255, 0, 255, 200, 155, 0, 0];

function attentionColors(player: number[]) {
  return [
    ATTENTION_GREEN, ATTENTION_GREEN, player, ATTENTION_GREEN, ATTENTION_NPC, ATTENTION_ENEMY, ATTENTION_GREEN,
    ATTENTION_GREEN, ATTENTION_GREEN, ATTENTION_ENEMY, ATTENTION_GREEN, ATTENTION_GREEN, ATTENTION_GREEN,
  ];
}

const FAIRY_GAMES: Record<Game, FairyGame> = {
  oot: {
    name: 'Navi',
    messageFile: 'oot/nes_message_data_static',
    attentionColors: attentionColors([255, 255, 255, 255, 0, 0, 255, 0]),
  },
  mm: {
    name: 'Tatl',
    messageFile: 'mm/message_data_static',
    attentionColors: attentionColors([255, 255, 230, 255, 220, 160, 80, 0]),
  },
};

const ATTENTION_CATEGORY_PLAYER = 2;

const FURBY_NAME = 'Furby';
const FURBY_INNER = [255, 140, 230];
const FURBY_OUTER = [150, 60, 220];

function isLetter(b: number) {
  return (b >= 0x41 && b <= 0x5a) || (b >= 0x61 && b <= 0x7a);
}

function isLowercase(b: number) {
  return b >= 0x61 && b <= 0x7a;
}

function isText(b: number) {
  return b >= 0x20 && b <= 0x7e;
}

function encode(str: string) {
  return Array.from(str).map(c => c.charCodeAt(0));
}

/*
 * Replace whole-word `from` with `to` in one message. Control codes and their
 * arguments are skipped, so an argument byte never matches. Returns null when
 * nothing changed or the message has no end code.
 */
export function replaceNameInMessage(msg: Uint8Array, fmt: MessageFormat, from: string, to: string): Uint8Array | null {
  const fromBytes = encode(from);
  const toBytes = encode(to);
  const out: number[] = Array.from(msg.subarray(0, fmt.headerSize));
  let prevLetter = false;
  let count = 0;
  let i = fmt.headerSize;

  for (;;) {
    if (i >= msg.length)
      return null;
    const b = msg[i];

    if (b === fmt.end) {
      out.push(b);
      break;
    }

    if (isText(b)) {
      if (!prevLetter && i + fromBytes.length <= msg.length && fromBytes.every((x, j) => msg[i + j] === x)) {
        const next = i + fromBytes.length < msg.length ? msg[i + fromBytes.length] : 0;
        if (!isLowercase(next)) {
          out.push(...toBytes);
          i += fromBytes.length;
          prevLetter = true;
          count++;
          continue;
        }
      }
      out.push(b);
      prevLetter = isLetter(b);
      i++;
      continue;
    }

    /* Control code, or a special character */
    const argCount = fmt.args[b] || 0;
    if (i + 1 + argCount > msg.length)
      return null;
    out.push(...msg.subarray(i, i + 1 + argCount));
    prevLetter = false;
    i += 1 + argCount;
  }

  if (count === 0)
    return null;

  /* Keep messages 4-byte aligned */
  while (out.length % 4)
    out.push(0);

  return new Uint8Array(out);
}

/*
 * Find the message table in code: 8 byte entries (u16 id, u8 type/pos, pad,
 * u32 segment address) ending with id 0xffff. The right table starts at the
 * beginning of the file, only goes forward, stays inside it, and its last
 * message sits right at the end of the file. This tells the OoT English table
 * apart from the Japanese and credits ones.
 */
export function findMessageTables(code: Uint8Array, dataSize: number, segment: number): number[] {
  const base = (segment << 24) >>> 0;
  const found: number[] = [];

  for (let o = 0; o + 8 <= code.length; o += 4) {
    if (bufReadU32BE(code, o + 4) !== base)
      continue;

    let prev = 0;
    let count = 0;
    let ok = false;
    for (let e = o; e + 8 <= code.length; e += 8) {
      if (bufReadU16BE(code, e) === 0xffff) {
        ok = true;
        break;
      }
      const addr = bufReadU32BE(code, e + 4);
      const off = addr - base;
      if ((addr >>> 24) !== segment || off < prev || off >= dataSize)
        break;
      prev = off;
      count++;
    }

    if (ok && count >= 500 && dataSize - prev <= 0x1000)
      found.push(o);
  }

  return found;
}

export type RenameResult = {
  data: Uint8Array;
  renamed: number;
};

/*
 * Rename the fairy in every message and repack the message file. The table in
 * `code` is updated in place; the new message file is returned.
 */
export function renameInMessages(code: Uint8Array, tableOffset: number, data: Uint8Array, fmt: MessageFormat, from: string, to: string): RenameResult {
  const base = (fmt.segment << 24) >>> 0;

  const entries: number[] = [];
  for (let e = tableOffset; bufReadU16BE(code, e) !== 0xffff; e += 8)
    entries.push(e);

  const offsets = [...new Set(entries.map(e => bufReadU32BE(code, e + 4) - base))].sort((a, b) => a - b);
  const chunks: Uint8Array[] = [];
  const newOffsets = new Map<number, number>();
  let renamed = 0;
  let size = offsets[0];

  chunks.push(data.subarray(0, offsets[0]));
  for (let i = 0; i < offsets.length; ++i) {
    const start = offsets[i];
    const end = i + 1 < offsets.length ? offsets[i + 1] : data.length;
    const chunk = data.subarray(start, end);
    const newChunk = replaceNameInMessage(chunk, fmt, from, to);

    newOffsets.set(start, size);
    if (newChunk) {
      chunks.push(newChunk);
      size += newChunk.length;
      renamed++;
    } else {
      chunks.push(chunk);
      size += chunk.length;
    }
  }

  const newData = new Uint8Array(size);
  let pos = 0;
  for (const c of chunks) {
    newData.set(c, pos);
    pos += c.length;
  }

  for (const e of entries) {
    const off = bufReadU32BE(code, e + 4) - base;
    bufWriteU32BE(code, e + 4, base + newOffsets.get(off)!);
  }

  return { data: newData, renamed };
}

function findAll(haystack: Uint8Array, needle: number[]): number[] {
  const found: number[] = [];
  for (let i = 0; i + needle.length <= haystack.length; ++i) {
    let j = 0;
    while (j < needle.length && haystack[i + j] === needle[j])
      j++;
    if (j === needle.length)
      found.push(i);
  }
  return found;
}

export function patchAttentionColor(code: Uint8Array, vanilla: number[][], inner: number[], outer: number[]): boolean {
  const matches = findAll(code, vanilla.flat());
  if (matches.length !== 1)
    return false;
  const entry = matches[0] + ATTENTION_CATEGORY_PLAYER * 8;
  code.set([...inner, 0xff, ...outer, 0x00], entry);
  return true;
}

function renameFairy(monitor: Monitor, builder: RomBuilder, game: Game, code: Uint8Array) {
  const g = FAIRY_GAMES[game];
  const fmt = MESSAGE_FORMATS[game];
  const file = builder.fileByNameRequired(g.messageFile);

  if (file.injected || file.type !== 'uncompressed' || file.index === undefined || file.vaddr === undefined) {
    monitor.warn(`Fairy Companion: can't edit ${g.messageFile}, ${g.name} keeps her name`);
    return;
  }

  const tables = findMessageTables(code, file.data.length, fmt.segment);
  if (tables.length !== 1) {
    monitor.warn(`Fairy Companion: found ${tables.length} ${game} message tables instead of 1, ${g.name} keeps her name`);
    return;
  }

  /* The bigger file must not run into the next file's addresses */
  const next = builder.fileByIndex(game, file.index + 1);
  const room = next?.vaddr !== undefined ? next.vaddr - file.vaddr : Infinity;

  const backup = code.slice();
  const result = renameInMessages(code, tables[0], file.data, fmt, g.name, FURBY_NAME);
  if (result.data.length > room) {
    code.set(backup);
    monitor.warn(`Fairy Companion: not enough room in ${g.messageFile}, ${g.name} keeps her name`);
    return;
  }

  file.data = result.data;
  monitor.debug(`Fairy Companion: renamed ${g.name} in ${result.renamed} ${game} messages`);
}

export function applyFairyStyle(monitor: Monitor, builder: RomBuilder, style: FairyStyle, patchSymbol: (name: string, data: Uint8Array) => void) {
  const flags = FAIRY_STYLE_FLAGS[style] ?? 0;
  if (!flags)
    return;

  patchSymbol('FAIRY_STYLE', new Uint8Array([flags]));
  if (!(flags & FAIRY_STYLE_FURBY))
    return;

  for (const game of ['oot', 'mm'] as const) {
    const code = builder.fileByNameRequired(`${game}/code`).data;
    renameFairy(monitor, builder, game, code);
    if (!patchAttentionColor(code, FAIRY_GAMES[game].attentionColors, FURBY_INNER, FURBY_OUTER)) {
      monitor.warn(`Fairy Companion: ${FAIRY_GAMES[game].name}'s colors not found, glow unchanged`);
    }
  }
}
