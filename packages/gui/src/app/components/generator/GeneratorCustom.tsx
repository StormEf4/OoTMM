import type { TabViewRoute } from '../nav';

import { LuWrench, LuSword, LuDrama, LuScrollText } from 'react-icons/lu';
import { CUSTOM_CATEGORIES, CUSTOM_SETTINGS } from '@ootmm/core';

import { TabView } from '../nav';
import { SettingsPanel } from '../settings';
import changeLog from '../../../../../../CUSTOM_CHANGES.md?raw';

const ICONS: { [k: string]: TabViewRoute['icon'] } = {
  'custom.general': LuWrench,
  'custom.oot': LuSword,
  'custom.mm': LuDrama,
};

function CustomCategory({ category }: { category: string }) {
  const hasSettings = (CUSTOM_SETTINGS as ReadonlyArray<{ category: string }>).some(s => s.category === category);

  return (
    <main className="p-8">
      {hasSettings
        ? <SettingsPanel category={category}/>
        : <p className="opacity-70">No custom settings here yet. Run <code>pnpm custom:add</code> to add one.</p>}
    </main>
  );
}

/* Render `code` spans from the markdown */
function inline(text: string) {
  return text.split('`').map((part, i) => i % 2 ? <code key={i} className="text-sm px-1 rounded bg-neutral-300 dark:bg-gray-800">{part}</code> : part);
}

function ChangeLog() {
  const lines = changeLog.split('\n');

  return (
    <main className="p-8 flex flex-col gap-1 max-w-4xl">
      {lines.map((line, i) => {
        if (line.startsWith('# ')) return <h1 key={i} className="font-bold text-2xl mb-2">{inline(line.slice(2))}</h1>;
        if (line.startsWith('## ')) return <h2 key={i} className="font-bold text-xl mt-6">{inline(line.slice(3))}</h2>;
        if (line.startsWith('### ')) return <h3 key={i} className="font-semibold text-lg mt-4">{inline(line.slice(4))}</h3>;
        if (line.startsWith('- ')) return <p key={i} className="pl-4">• {inline(line.slice(2))}</p>;
        if (line.trim() === '' || line.startsWith('<!--')) return null;
        return <p key={i}>{inline(line)}</p>;
      })}
    </main>
  );
}

const ROUTES: TabViewRoute[] = [
  ...CUSTOM_CATEGORIES.map(c => ({
    name: c.name,
    icon: ICONS[c.key],
    component: () => <CustomCategory category={c.key}/>,
  })),
  { name: 'Change Log', icon: LuScrollText, component: ChangeLog },
];

export function GeneratorCustom() {
  return <TabView routes={ROUTES}/>;
}
