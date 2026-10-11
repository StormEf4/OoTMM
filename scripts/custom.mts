/*
 * Helpers for customizing this fork.
 *
 *   pnpm custom:add   Add a new setting (toggle, dropdown, number or multi-select)
 *                     to the Custom page, and log it in CUSTOM_CHANGES.md.
 *   pnpm custom:log   Log any other change in CUSTOM_CHANGES.md.
 */
import { promises as fs } from 'fs';
import path from 'path';
import { createInterface } from 'readline';
import { stdin, stdout } from 'process';

const ROOT = path.resolve(import.meta.dirname, '..');
const CUSTOM_FILE = path.join(ROOT, 'packages/core/src/settings/custom.ts');
const DATA_FILE = path.join(ROOT, 'packages/core/src/settings/data.ts');
const LOG_FILE = path.join(ROOT, 'CUSTOM_CHANGES.md');
const LOG_MARKER = '<!-- new entries go below this line -->';
const SETTINGS_MARKER = '  // @custom-settings-end';

const rl = createInterface({ input: stdin });
const lines = rl[Symbol.asyncIterator]();

async function ask(question: string, def?: string): Promise<string> {
  const suffix = def !== undefined && def !== '' ? ` [${def}]` : '';
  stdout.write(`${question}${suffix}: `);
  const line = await lines.next();
  if (line.done) throw new Error('No more input');
  if (!stdin.isTTY) stdout.write(`${line.value}\n`);
  const answer = line.value.trim();
  return answer || def || '';
}

async function choose<T extends string>(question: string, options: { value: T, label: string }[], def: T): Promise<T> {
  console.log(question);
  options.forEach((o, i) => console.log(`  ${i + 1}) ${o.label}`));
  const defIndex = options.findIndex(o => o.value === def) + 1;
  for (;;) {
    const answer = await ask('Pick a number', String(defIndex));
    const n = Number(answer);
    if (n >= 1 && n <= options.length) return options[n - 1].value;
    console.log('Please type one of the numbers above.');
  }
}

function camelCase(s: string) {
  const words = s.replace(/'/g, '').split(/[^A-Za-z0-9]+/).filter(Boolean);
  return words.map((w, i) => i === 0 ? w.toLowerCase() : w[0].toUpperCase() + w.slice(1).toLowerCase()).join('');
}

function flagName(...parts: string[]) {
  return 'CFG_' + ['CUSTOM', ...parts].map(p => p.replace(/([a-z0-9])([A-Z])/g, '$1_$2').replace(/[^A-Za-z0-9]+/g, '_').toUpperCase()).join('_');
}

function q(s: string) {
  return s.replace(/\\/g, '\\\\').replace(/'/g, "\\'");
}

function today() {
  return new Date().toISOString().slice(0, 10);
}

async function appendLog(title: string, lines: string[]) {
  const log = await fs.readFile(LOG_FILE, 'utf8');
  if (!log.includes(LOG_MARKER)) {
    throw new Error(`Could not find "${LOG_MARKER}" in CUSTOM_CHANGES.md`);
  }
  const entry = [`## ${today()}: ${title}`, '', ...lines.map(l => `- ${l}`)].join('\n');
  await fs.writeFile(LOG_FILE, log.replace(LOG_MARKER, `${LOG_MARKER}\n\n${entry}`));
}

async function addSetting() {
  const game = await choose('Which game does it affect?', [
    { value: 'general', label: 'Both / general' },
    { value: 'oot', label: 'Ocarina of Time' },
    { value: 'mm', label: "Majora's Mask" },
  ], 'general');
  const name = await ask('Name shown in the app (e.g. "Faster Epona")');
  if (!name) throw new Error('A name is required');

  const existing = await fs.readFile(DATA_FILE, 'utf8') + await fs.readFile(CUSTOM_FILE, 'utf8');
  let key = await ask('Internal key', camelCase(name));
  while (!/^[a-z][A-Za-z0-9]*$/.test(key) || existing.includes(`key: '${key}'`)) {
    key = await ask(`"${key}" is invalid or already used, pick another key`);
  }

  const description = await ask('Description shown when hovering the setting (optional)');
  const type = await choose('What kind of control?', [
    { value: 'boolean', label: 'Toggle (on/off checkbox)' },
    { value: 'enum', label: 'Drop-down (pick one option)' },
    { value: 'number', label: 'Number' },
    { value: 'set', label: 'Multi-select (pick several options)' },
  ], 'boolean');

  const body: string[] = [];
  const flags: string[] = [];
  let summary = '';

  if (type === 'boolean') {
    const def = await choose('Default value?', [{ value: 'false', label: 'Off' }, { value: 'true', label: 'On' }], 'false');
    body.push(`default: ${def},`);
    flags.push(flagName(key));
    summary = `Toggle, default ${def === 'true' ? 'on' : 'off'}`;
  } else if (type === 'enum' || type === 'set') {
    console.log('Enter the options one per line. Leave a line empty when done.');
    const values: { value: string, name: string }[] = [];
    for (;;) {
      const optName = await ask(`Option ${values.length + 1} name`);
      if (!optName) {
        if (values.length >= 2) break;
        console.log('Please enter at least two options.');
        continue;
      }
      values.push({ value: camelCase(optName) || `option${values.length + 1}`, name: optName });
    }
    body.push('values: [');
    for (const v of values) body.push(`  { value: '${v.value}', name: '${q(v.name)}' },`);
    body.push('],');
    if (type === 'enum') {
      const def = await choose('Default option?', values.map(v => ({ value: v.value, label: v.name })), values[0].value);
      body.push(`default: '${def}',`);
      flags.push(...values.map(v => flagName(key, v.value)));
      summary = `Drop-down (${values.map(v => v.name).join(', ')}), default ${values.find(v => v.value === def)!.name}`;
    } else {
      const def = await choose('Default selection?', [{ value: 'none', label: 'None' }, { value: 'all', label: 'All' }], 'none');
      body.push(`default: '${def}',`);
      summary = `Multi-select (${values.map(v => v.name).join(', ')}), default ${def}`;
    }
  } else {
    const def = Number(await ask('Default value', '0'));
    const min = await ask('Minimum (optional)');
    const max = await ask('Maximum (optional)');
    body.push(`default: ${def},`);
    if (min !== '') body.push(`min: ${Number(min)},`);
    if (max !== '') body.push(`max: ${Number(max)},`);
    summary = `Number, default ${def}${min !== '' ? `, min ${min}` : ''}${max !== '' ? `, max ${max}` : ''}`;
  }

  const lines = [
    '{',
    `  key: '${key}',`,
    `  name: '${q(name)}',`,
    `  category: 'custom.${game}',`,
    `  type: '${type}',`,
    `  description: '${q(description)}',`,
    ...body.map(l => `  ${l}`),
    ...(game === 'oot' ? ['  cond: hasOoT,'] : game === 'mm' ? ['  cond: hasMM,'] : []),
    '},',
  ].map(l => `  ${l}`);

  const custom = await fs.readFile(CUSTOM_FILE, 'utf8');
  if (!custom.includes(SETTINGS_MARKER)) {
    throw new Error(`Could not find "${SETTINGS_MARKER.trim()}" in ${CUSTOM_FILE}`);
  }
  await fs.writeFile(CUSTOM_FILE, custom.replace(SETTINGS_MARKER, `${lines.join('\n')}\n${SETTINGS_MARKER}`));

  const page = { general: 'General', oot: 'Ocarina of Time', mm: "Majora's Mask" }[game];
  await appendLog(`Added setting "${name}"`, [
    `Where in the app: Custom > ${page}`,
    `Control: ${summary}`,
    `Key: \`${key}\` in \`packages/core/src/settings/custom.ts\``,
    ...(description ? [`What it does: ${description}`] : []),
    flags.length
      ? `Game flags: ${flags.map(f => `\`${f}\``).join(', ')} (read in C with \`Config_Flag(...)\`)`
      : `Game flags: none (read \`settings.${key}\` in the TypeScript generator or logic)`,
    'Status: setting added, in-game effect still to be written',
  ]);

  console.log(`\nAdded "${name}" to Custom > ${page}, and logged it in CUSTOM_CHANGES.md.`);
  if (flags.length) console.log(`In C, check it with: ${flags.map(f => `Config_Flag(${f})`).join(', ')}`);
  console.log('Restart `pnpm local` (or refresh the page) to see it.');
}

async function logChange() {
  const title = await ask('Short title of the change');
  if (!title) throw new Error('A title is required');
  const where = await ask('Where it shows in the app (e.g. "Custom > Majora\'s Mask > Faster Epona")');
  const details = await ask('What changed and why');
  const files = await ask('Files changed (optional)');
  await appendLog(title, [
    ...(where ? [`Where in the app: ${where}`] : []),
    ...(details ? [`What it does: ${details}`] : []),
    ...(files ? [`Files: ${files}`] : []),
  ]);
  console.log('Logged in CUSTOM_CHANGES.md.');
}

const command = process.argv[2];
try {
  if (command === 'add') await addSetting();
  else if (command === 'log') await logChange();
  else console.log('Usage: tsx scripts/custom.mts <add|log>');
} catch (e) {
  console.error(`Error: ${(e as Error).message}`);
  process.exitCode = 1;
} finally {
  rl.close();
}
