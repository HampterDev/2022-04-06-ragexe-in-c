"""Pair function definitions of two snapshots of the client tree (before and after the C renaming)
per file, by normalised name first, then by order between the anchors."""
import os, re, sys, json, difflib

old_root, new_root, out = sys.argv[1], sys.argv[2], sys.argv[3]
DEF = re.compile(r'^(?!typedef|#|/\*|\*|//|\s)([A-Za-z_][\w\s\*]*?)\b([A-Za-z_]\w*)\s*\(([^;{]*)\)\s*\{?\s*$')
KW = {'if', 'for', 'while', 'switch', 'return', 'sizeof', 'defined', 'STATIC_ASSERT'}

def defs(path):
    res = []
    for i, line in enumerate(open(path, encoding='utf-8', errors='replace')):
        m = DEF.match(line)
        if m and not line.rstrip().endswith(';'):
            name = m.group(2)
            if name in KW or name.isupper():
                continue
            res.append(name)
    return res

def norm(name):
    s = name
    # drop a leading class-style C prefix: CWorld_RemoveSkill -> World_RemoveSkill
    s = re.sub(r'(^|_)C(?=[A-Z][a-z])', r'\1', s)
    s = s.replace('__', '_')
    s = re.sub(r'(?<=[a-z0-9])([A-Z])', r'_\1', s)
    s = re.sub(r'([A-Z]+)([A-Z][a-z])', r'\1_\2', s)
    s = re.sub(r'_+', '_', s).strip('_').lower()
    return s

def walk(root):
    files = {}
    for dp, dn, fs in os.walk(root):
        for f in fs:
            if f.endswith('.c'):
                p = os.path.join(dp, f)
                files[os.path.relpath(p, root).replace('\\', '/')] = p
    return files

old_files, new_files = walk(old_root), walk(new_root)
pairs = []
stats = {'files_both': 0, 'exact': 0, 'norm': 0, 'order': 0, 'unmatched_old': 0, 'new_only_files': 0}
for rel, op in old_files.items():
    np_ = new_files.get(rel)
    if not np_:
        continue
    stats['files_both'] += 1
    o, n = defs(op), defs(np_)
    nset = set(n)
    nnorm = {}
    for x in n:
        nnorm.setdefault(norm(x), []).append(x)
    used = set()
    matched = {}
    for x in o:
        if x in nset and x not in used:
            matched[x] = (x, 'exact'); used.add(x)
    for x in o:
        if x in matched:
            continue
        cands = [c for c in nnorm.get(norm(x), []) if c not in used]
        if len(cands) == 1:
            matched[x] = (cands[0], 'norm'); used.add(cands[0])
    # order-based fill between anchors
    o_rest = [x for x in o if x not in matched]
    n_rest = [x for x in n if x not in used]
    if o_rest and n_rest:
        # align sequences of normalised names with difflib, pairing replaced blocks of equal length
        sm = difflib.SequenceMatcher(None, [norm(x) for x in o], [norm(x) for x in n], autojunk=False)
        for tag, i1, i2, j1, j2 in sm.get_opcodes():
            if tag == 'replace' and (i2 - i1) == (j2 - j1):
                for a, b in zip(o[i1:i2], n[j1:j2]):
                    if a not in matched and b not in used:
                        matched[a] = (b, 'order'); used.add(b)
    for x in o:
        if x in matched:
            stats[matched[x][1]] += 1
            pairs.append({'file': rel, 'old': x, 'new': matched[x][0], 'how': matched[x][1]})
        else:
            stats['unmatched_old'] += 1
            pairs.append({'file': rel, 'old': x, 'new': None, 'how': 'none'})
stats['new_only_files'] = len(set(new_files) - set(old_files))
with open(out, 'w', encoding='utf-8') as fh:
    for p in pairs:
        fh.write(json.dumps(p) + '\n')
print(stats)
