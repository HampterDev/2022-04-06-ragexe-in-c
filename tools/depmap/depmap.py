"""Symbol-level dependency map of the client.

Usage: depmap.py <checkout> <symbols> <out json> [--communal]

<symbols> is the `dumpbin /symbols` listing that symbols.bat writes for a build folder (one file
for all objects), a folder of per-object `.sym` files (the earlier layout), or an `nm -P -A`
listing. Writes the file graph with sizes and prints a summary.

Edges are link-time references between `.c` files: file A uses an external function or data that
file B defines. Communal globals (`int g_x;` without an initializer) have no defining object in
MSVC's listing and draw no edge, as in every map recorded in docs/custom-review.md; with
--communal they count as defined by the files that declare them that way (as the link check
counts them).
"""
import json
import sys
from collections import defaultdict
from pathlib import Path

import objsyms

sys.setrecursionlimit(100000)
sys.stdout.reconfigure(encoding='utf-8', errors='replace')


def load(root, symbols, communal=False):
    """Return {src-relative .c path: {'lines', 'defs': {sym: kind}, 'refs': set}}."""
    src = Path(root) / 'src'
    sources = sorted(c.relative_to(src).as_posix() for c in src.rglob('*.c'))
    by_stem = {Path(s).stem: s for s in sources}
    objects = {}
    sp = Path(symbols)
    if sp.is_dir():
        for f in sorted(sp.glob('*.sym')):
            objsyms.read_dumpbin(f, objects)
    else:
        objsyms.read(sp, objects)
    files = {}
    for rel in sources:
        text = (src / rel).read_text(encoding='utf-8', errors='replace')
        files[rel] = {'lines': text.count('\n') + 1, 'defs': {}, 'refs': set()}
    for name, obj in objects.items():
        rel = objsyms.source_of_object(name, by_stem, files)
        if rel is None:
            continue
        d = files[rel]
        for s, kind in obj.defs.items():
            if kind == 'communal' and not communal:
                continue
            d['defs'][objsyms.plain(s)] = kind
        d['refs'] |= {objsyms.plain(s) for s in obj.refs}
    return files


def edges_of(files):
    owner = {}
    for f, d in files.items():
        for s, kind in d['defs'].items():
            owner.setdefault(s, f)
    edges = defaultdict(lambda: defaultdict(list))   # a -> b -> symbols
    for f, d in files.items():
        for s in d['refs']:
            o = owner.get(s)
            if o is not None and o != f:
                edges[f][o].append(s)
    return edges


def scc(nodes, succ):
    """Strongly connected components (Tarjan, iterative)."""
    index, low, on, stack, out = {}, {}, set(), [], []
    counter = [0]
    for root in nodes:
        if root in index:
            continue
        work = [(root, iter(succ(root)))]
        index[root] = low[root] = counter[0]; counter[0] += 1
        stack.append(root); on.add(root)
        while work:
            v, it = work[-1]
            advanced = False
            for w in it:
                if w not in index:
                    index[w] = low[w] = counter[0]; counter[0] += 1
                    stack.append(w); on.add(w)
                    work.append((w, iter(succ(w))))
                    advanced = True
                    break
                elif w in on:
                    low[v] = min(low[v], index[w])
            if advanced:
                continue
            work.pop()
            if work:
                low[work[-1][0]] = min(low[work[-1][0]], low[v])
            if low[v] == index[v]:
                comp = []
                while True:
                    w = stack.pop(); on.discard(w); comp.append(w)
                    if w == v:
                        break
                out.append(comp)
    return out


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    root, symbols, out = Path(args[0]), args[1], Path(args[2])
    files = load(root, symbols, communal='--communal' in sys.argv)
    edges = edges_of(files)
    src = root / 'src'
    header_lines = defaultdict(int)
    for h in src.rglob('*.h'):
        header_lines[h.parent.relative_to(src).as_posix()] += \
            h.read_text(encoding='utf-8', errors='replace').count('\n') + 1

    nodes = sorted(files)
    fan_out = {f: len(edges[f]) for f in nodes}
    fan_in = defaultdict(int)
    for a in edges:
        for b in edges[a]:
            fan_in[b] += 1
    hub_fanout = 40
    hubs = {f for f in nodes if fan_out[f] >= hub_fanout}

    def lines_of(fs):
        return sum(files[f]['lines'] for f in fs)

    raw = sorted(scc(nodes, lambda v: edges[v].keys()), key=len, reverse=True)
    cut = sorted(scc(nodes, lambda v: [] if v in hubs else edges[v].keys()), key=len, reverse=True)
    result = {
        'files': {f: {'lines': files[f]['lines'], 'fan_in': fan_in[f], 'fan_out': fan_out[f],
                      'out': {b: len(s) for b, s in edges[f].items()},
                      'syms': {b: sorted(s)[:6] for b, s in edges[f].items()}} for f in nodes},
        'hubs': sorted(hubs),
        'raw_scc': [sorted(c) for c in raw if len(c) > 1],
        'cut_scc': [sorted(c) for c in cut if len(c) > 1],
        'header_lines': dict(header_lines),
    }
    out.write_text(json.dumps(result, indent=1))

    print(f'files {len(nodes)}, lines {lines_of(nodes)} (+ headers {sum(header_lines.values())})')
    print(f'hubs (fan-out >= {hub_fanout}): {len(hubs)} files, {lines_of(hubs)} lines')
    for f in sorted(hubs, key=lambda f: -fan_out[f]):
        print(f'  {fan_out[f]:4d} {f} ({files[f]["lines"]})')
    print(f'raw: biggest SCC {len(raw[0])} files / {lines_of(raw[0])} lines; next sizes {[len(c) for c in raw[1:6]]}')
    print(f'hub edges cut: biggest SCC {len(cut[0])} files / {lines_of(cut[0])} lines; next {[len(c) for c in cut[1:8]]}')
    print('most depended-on files:')
    for f in sorted(nodes, key=lambda f: -fan_in[f])[:25]:
        print(f'  {fan_in[f]:4d} {f}')


if __name__ == '__main__':
    main()
