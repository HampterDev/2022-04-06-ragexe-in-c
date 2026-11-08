"""Rank the cuts that shrink the knot most, greedily, and measure after each.

Usage: knot_cuts.py <checkout> <symbols> <out json> [steps]

The graph is depmap.load's with communal globals counted (as the link check counts them) and
the registry and startup files cut (modules.registry_of), so the knot is the one modules.py and
the library table describe. A cut drops edges:

- `lower X`: X's calls into the rest of the knot go away (they become hooks, or what X needs
  moves below the knot), so X can sit below it;
- `upper X`: the rest of the knot no longer reaches X (it goes through a table or hook that app
  installs, or the definition it uses moves out of X), so X can sit above it;
- `edge A -> B`: A no longer uses B.

X is a file, a directory, a stem group (`dir/stem*`) or a top directory. Each step takes the
cut with the most knot lines freed per symbol to reroute (freeing at least MIN_LINES), applies it
and measures again; it repeats for the second largest SCC. The printed symbols are the work:
each is a definition to move, a call to hook or a table row to add. The tool does not say which,
docs/custom-review.md ("Ranking the knot's cuts") does for the published run.
"""
import json
import sys
from collections import defaultdict, Counter

import depmap
import modules

sys.stdout.reconfigure(encoding='utf-8', errors='replace')
MIN_LINES = 800


def main():
    root, symbols, outp = sys.argv[1:4]
    steps = int(sys.argv[4]) if len(sys.argv) > 4 else 40
    files = depmap.load(root, symbols, communal=True)
    edges = depmap.edges_of(files)
    registry = modules.registry_of(set(files))
    nodes = sorted(f for f in files if f not in registry)
    size = {f: files[f]['lines'] for f in files}
    succ_syms = {a: {b: set(s) for b, s in edges[a].items() if b not in registry} for a in nodes}

    def lines(fs):
        return sum(size[f] for f in fs)

    def sccs(sub, removed):
        def succ(v):
            for w in succ_syms[v]:
                if w in sub and (v, w) not in removed:
                    yield w
        return sorted(depmap.scc(sorted(sub), succ), key=len, reverse=True)

    def candidates(knot):
        groups = defaultdict(set)
        for f in sorted(knot):
            d, base = f.rsplit('/', 1)
            stem = base.split('_')[0].split('.')[0]
            groups['file ' + f].add(f)
            groups['dir ' + d].add(f)
            groups[f'grp {d}/{stem}*'].add(f)
            groups['top ' + d.split('/')[0]].add(f)
        seen = set()
        for g, gs in groups.items():
            key = frozenset(gs)
            if key in seen:
                continue
            seen.add(key)
            outs = {(a, b) for a in gs for b in succ_syms[a] if b in knot and b not in gs}
            ins = {(a, b) for a in knot if a not in gs for b in succ_syms[a] if b in gs}
            for kind, rem in (('lower', outs), ('upper', ins)):
                if rem:
                    yield f'{kind} {g}', frozenset(rem)
        for a in sorted(knot):
            for b in sorted(succ_syms[a]):
                if b in knot:
                    yield f'edge {a} -> {b}', frozenset([(a, b)])

    def cost(rem):
        return set().union(*[succ_syms[a][b] for a, b in rem])

    def run(start, label):
        removed = set()
        knot = set(start)
        log = []
        print(f'\n== {label}: {len(knot)} files / {lines(knot)} lines')
        for step in range(steps):
            best = None
            for name, rem in candidates(knot):
                c = cost(rem)
                comps = sccs(knot, removed | rem)
                k2 = set(comps[0]) if comps and len(comps[0]) > 1 else set()
                fr = lines(knot) - lines(k2)
                if fr < MIN_LINES:
                    continue
                score = fr / len(c)
                if best is None or score > best[0]:
                    best = (score, name, rem, c, k2, fr, comps)
            if best is None:
                print(f'  no single cut frees {MIN_LINES} lines')
                break
            _, name, rem, c, k2, fr, comps = best
            removed |= rem
            freed = sorted(knot - k2)
            callers = sorted({a for a, _ in rem})
            print(f'{step + 1:2d}. {name}: reroute {len(c)} symbols from {len(callers)} files, frees'
                  f' {len(freed)} files / {fr} lines -> {len(k2)} files / {lines(k2)} lines;'
                  f' next SCCs {[len(x) for x in comps[1:5]]}')
            print('     symbols:', ', '.join(sorted(c)[:14]) + (' ...' if len(c) > 14 else ''))
            print('     freed by directory:', Counter(f.rsplit('/', 1)[0] for f in freed).most_common(6))
            log.append({'cut': name, 'symbols': sorted(c), 'edges': sorted(rem), 'callers': callers,
                        'freed': freed, 'knot_files': len(k2), 'knot_lines': lines(k2),
                        'next': [len(x) for x in comps[1:5]]})
            knot = k2
            if len(knot) < 2:
                break
        print('  left:', Counter(f.rsplit('/', 1)[0] for f in knot).most_common(12))
        return {'start': sorted(start), 'steps': log, 'left': sorted(knot)}

    comps = sccs(set(nodes), set())
    out = {'knot': run(comps[0], 'knot'), 'second': run(comps[1], 'second SCC')}
    with open(outp, 'w') as f:
        json.dump(out, f, indent=1, default=list)


if __name__ == '__main__':
    main()
