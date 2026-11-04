"""Cut the registry and startup files, find the remaining knot, and cluster the rest.

Usage: modules.py <depmap json> <out json> [--keep FILE ...]

The files in REGISTRY list whole feature sets or start the client; their edges are cut, so the
knot that remains is what the features themselves tie together. `--keep FILE` leaves FILE in the
graph (what modules_drop.py did), to see whether it still ties features into the knot.

Prints the knot, the library layer below it (files that never reach it) and the files above it,
grouped into clusters (connected components among themselves).
"""
import json
import re
import sys
from collections import defaultdict, Counter

from depmap import scc

sys.setrecursionlimit(100000)
sys.stdout.reconfigure(encoding='utf-8', errors='replace')

# The 23 files of the first measurements (docs/purist-review.md, "Restructuring: definitions and
# hooks"), the window and object-type row files and their registries (window and object-type
# tables), and the manager list (manager lifecycle table).
REGISTRY = {
    'core/object_type.c', 'effects/effect_registry.c',
    'game/packet_dispatch.c', 'game/packet_dispatch2.c', 'login/login_dispatch.c',
    'ui/wndmgr/make_window.c', 'ui/wndmgr/make_window_hud.c', 'ui/wndmgr/make_window_trade.c',
    'ui/wndmgr/make_window_social.c', 'ui/wndmgr/make_window_character.c',
    'ui/wndmgr/window_slots.c', 'ui/wndmgr/forget_window.c', 'ui/wndmgr/teardown.c',
    'core/app.c', 'core/startup.c', 'core/client_init.c', 'core/winmain.c', 'core/wndproc.c',
    'session/session_lifetime.c', 'game/mode_msg.c', 'game/mode_run.c', 'login/login_states.c',
    'login/mode.c',
    'ui/wndmgr/make_window_login.c', 'ui/wndmgr/window_registry.c',
    'core/object_type_registry.c', 'core/client_managers.c',
}


def registry_of(nodes):
    reg = {f for f in REGISTRY if f in nodes}
    reg |= {f for f in nodes if f.endswith('_object_types.c')}
    return reg


def analyse(files, registry):
    nodes = sorted(files)
    out = {f: dict(files[f]['out']) for f in nodes}

    def lines(fs):
        return sum(files[f]['lines'] for f in fs)

    succ_cut = lambda v: [] if v in registry else [w for w in out[v] if w not in registry]
    comps = sorted(scc(nodes, succ_cut), key=len, reverse=True)
    knot = set(comps[0])
    rev = defaultdict(set)
    for a in nodes:
        for b in out[a]:
            rev[b].add(a)
    stack = list(knot)
    can_reach = set(knot)
    while stack:
        v = stack.pop()
        for u in rev[v]:
            if u not in can_reach and u not in registry:
                can_reach.add(u); stack.append(u)
    below = [f for f in nodes if f not in can_reach and f not in registry]
    above = sorted(f for f in can_reach if f not in knot)
    adj = defaultdict(set)
    for a in above:
        for b in out[a]:
            if b in above:
                adj[a].add(b); adj[b].add(a)
    seen, clusters = set(), []
    for f in above:
        if f in seen:
            continue
        comp, st = [], [f]
        seen.add(f)
        while st:
            v = st.pop(); comp.append(v)
            for w in adj[v]:
                if w not in seen:
                    seen.add(w); st.append(w)
        clusters.append(sorted(comp))
    clusters.sort(key=lambda c: (-lines(c), c))
    return {'comps': comps, 'knot': knot, 'below': below, 'above': above, 'clusters': clusters,
            'rev': rev, 'out': out, 'lines': lines}


def name(c):
    dirs = Counter(f.rsplit('/', 1)[0] for f in c)
    stems = Counter(re.split(r'[_.]', f.rsplit('/', 1)[1])[0] for f in c)
    return f'{dirs.most_common(1)[0][0]}:{stems.most_common(1)[0][0]}'


def main():
    args = sys.argv[1:]
    keep = set()
    while '--keep' in args:
        i = args.index('--keep')
        keep.add(args[i + 1])
        del args[i:i + 2]
    data = json.load(open(args[0]))
    files = data['files']
    registry = registry_of(set(files)) - keep
    a = analyse(files, registry)
    lines, knot, out, rev = a['lines'], a['knot'], a['out'], a['rev']
    print(f'registry/startup files cut: {len(registry)} files, {lines(registry)} lines')
    print(f'remaining knot: {len(knot)} files, {lines(knot)} lines; next SCCs: {[len(c) for c in a["comps"][1:8]]}')
    kin, kout = Counter(), Counter()
    for f in knot:
        for b in out[f]:
            if b in knot:
                kout[f] += 1
                kin[b] += 1
    print('knot files with the most callers inside the knot:')
    for f, n in kin.most_common(25):
        print(f'  in {n:4d} out {kout[f]:3d} {f} ({files[f]["lines"]})')
    print(f'library layer (never reaches the knot): {len(a["below"])} files, {lines(a["below"])} lines')
    print(f'above the knot (use it, not used by it): {len(a["above"])} files, {lines(a["above"])} lines')
    print(f'clusters above the knot: {len(a["clusters"])}')
    for c in a['clusters'][:60]:
        users = set()
        for f in c:
            users |= {u for u in rev[f] if u not in c}
        print(f'  {lines(c):6d} lines {len(c):3d} files  {name(c):40s} used by {len(users)} outside files'
              f' ({len([u for u in users if u in registry])} registry)')
    json.dump({'registry': sorted(registry), 'knot': sorted(knot), 'below': a['below'],
               'clusters': a['clusters'], 'knot_in': kin.most_common(60)}, open(args[1], 'w'), indent=1)


if __name__ == '__main__':
    main()
