"""Build the original-function -> lifted-function map.

Inputs (all paths as arguments):
  refmap.jsonl     per annotated C function: cited sub_ addresses (refmap.py)
  rename.jsonl     annotated name -> this tree's name (rename_map.py)
  function_map.jsonl   the assembly listing's inventory: name, addr, chunk, line0, line1
  class_map.tsv    the listing's class map: function  addr  class  vtable
  main_src         our main's src/ (to locate each final name today)
  out.tsv
"""
import os, re, sys, json, collections

refmap, rename, fmap, cmap, main_src, out = sys.argv[1:7]

DEF = re.compile(r'^(?!typedef|#|/\*|\*|//|\s)([A-Za-z_][\w\s\*]*?)\b([A-Za-z_]\w*)\s*\(([^;{]*)\)\s*\{?\s*$')
KW = {'if', 'for', 'while', 'switch', 'return', 'sizeof', 'defined'}

def norm_label(s):
    return re.sub(r'[^a-z0-9]', '', s.lower())

# --- asm inventory
asm_by_addr, asm_by_norm = {}, {}
for l in open(fmap, encoding='utf-8'):
    d = json.loads(l)
    if d.get('addr') is None or 'thunk' in d['name']:
        continue
    a = '%06x' % d['addr']
    asm_by_addr[a] = d
    if not d['name'].startswith(('sub_', 'label_', 'xt_')):
        asm_by_norm.setdefault(norm_label(d['name']), []).append(d)
classes = {}
for i, l in enumerate(open(cmap, encoding='utf-8')):
    if i == 0: continue
    p = l.rstrip('\n').split('\t')
    if len(p) >= 3: classes[p[1]] = p[2]

# --- rename map: (basename, old) -> new, old -> [new]
ren_by_file, ren_by_name = {}, collections.defaultdict(set)
for l in open(rename, encoding='utf-8'):
    d = json.loads(l)
    if d['new']:
        ren_by_file[(os.path.basename(d['file']), d['old'])] = d['new']
        ren_by_name[d['old']].add(d['new'])

# --- our main's definitions
main_defs = collections.defaultdict(list)
for dp, dn, fs in os.walk(main_src):
    for f in fs:
        if not f.endswith('.c'): continue
        p = os.path.join(dp, f)
        rel = os.path.relpath(p, main_src).replace('\\', '/')
        for i, line in enumerate(open(p, encoding='utf-8', errors='replace')):
            m = DEF.match(line)
            if m and not line.rstrip().endswith(';') and m.group(2) not in KW:
                main_defs[m.group(2)].append(f'{rel}:{i+1}')

rows = []
stats = collections.Counter()
for l in open(refmap, encoding='utf-8'):
    r = json.loads(l)
    name = r['name']
    addrs, method = [], ''
    if name.startswith('sub_') and re.fullmatch(r'sub_[0-9a-f]{6}', name):
        addrs, method = [name[4:]], 'name'
    elif len(r['above']) == 1:
        addrs, method = r['above'], 'cited'
    elif len(r['above']) > 1:
        addrs, method = r['above'], 'cited_multi'
    elif len(r['above_hex']) == 1 and re.search(r'lifted from|lifts|original|\(0x[0-9a-f]{6}\)', r['above_text']):
        addrs, method = r['above_hex'], 'cited_hex'
    if not addrs:
        hit = asm_by_norm.get(norm_label(name))
        if not hit:
            # CGameMode_Zc_ParChange -> try without a leading C on the class
            hit = asm_by_norm.get(norm_label(re.sub(r'^C(?=[A-Z])', '', name)))
        if hit and len(hit) == 1:
            addrs, method = ['%06x' % hit[0]['addr']], 'label'
    if not addrs:
        stats['no_address'] += 1
        continue
    base = os.path.basename(r['file'])
    final = ren_by_file.get((base, name))
    if not final:
        cands = ren_by_name.get(name, set())
        final = next(iter(cands)) if len(cands) == 1 else (name if name in main_defs else None)
    where = main_defs.get(final or '', [])
    stats[method] += 1
    for a in addrs:
        asm = asm_by_addr.get(a)
        rows.append({
            'addr': a,
            'asm_label': asm['name'] if asm else '',
            'asm_chunk': f"{asm['chunk']}:{asm['line0']}-{asm['line1']}" if asm else '',
            'class': classes.get(a, ''),
            'ref_name': name, 'ref_file': r['file'],
            'final_name': final or '', 'main_where': ';'.join(where[:3]),
            'method': method, 'n_addrs': len(addrs),
        })

rows.sort(key=lambda x: x['addr'])
with open(out, 'w', encoding='utf-8', newline='') as fh:
    fh.write('\t'.join(['addr', 'asm_label', 'asm_chunk', 'class', 'ref_name', 'ref_file', 'final_name', 'main_where', 'method', 'n_addrs']) + '\n')
    for x in rows:
        fh.write('\t'.join(str(x[k]) for k in ['addr', 'asm_label', 'asm_chunk', 'class', 'ref_name', 'ref_file', 'final_name', 'main_where', 'method', 'n_addrs']) + '\n')

print('reference functions by method:', dict(stats))
print('rows:', len(rows), 'distinct addresses:', len({x['addr'] for x in rows}))
print('rows with asm inventory hit:', sum(1 for x in rows if x['asm_label']))
print('rows with a final name:', sum(1 for x in rows if x['final_name']), 'located on main:', sum(1 for x in rows if x['main_where']))
# coverage of the asm side: functions in chunks that look like game code
game_chunks = ('text/actor', 'text/contents', 'text/core', 'text/effects', 'text/gamemode', 'text/loginmode', 'text/session', 'text/world', 'text/render', 'text/ui', 'text/skills', 'text/fileio', 'text/navi', 'text/iteminfo', 'text/replay', 'text/security', 'text/platform', 'text/asyncwork')
mapped = {x['addr'] for x in rows}
per_chunk = collections.Counter(); per_chunk_mapped = collections.Counter()
for a, d in asm_by_addr.items():
    if d['chunk'].startswith(game_chunks) and not d['name'].startswith(('label_', 'xt_')):
        key = d['chunk'].split('/')[1].split('_')[0]
        per_chunk[key] += 1
        if a in mapped: per_chunk_mapped[key] += 1
print('asm coverage by chunk family (mapped/total):')
for k in sorted(per_chunk):
    print(f'  {k:10} {per_chunk_mapped[k]:5}/{per_chunk[k]:5}')
