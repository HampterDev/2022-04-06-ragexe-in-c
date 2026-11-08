"""Add the confidence column, sort, and print the statistics for the README."""
import sys, csv, collections, json
inp, fmap, out = sys.argv[1:4]
rows = list(csv.DictReader(open(inp, encoding='utf-8'), delimiter='\t'))
HIGH = {'cited', 'name', 'label', 'ustr', 'cited_hex'}
for r in rows:
    r['confidence'] = 'high' if r['method'] in HIGH else 'low'
rows.sort(key=lambda r: (r['addr'], r['method']))
fields = ['addr', 'asm_label', 'asm_chunk', 'class', 'ref_name', 'final_name', 'main_where', 'method', 'confidence', 'ref_file']
with open(out, 'w', encoding='utf-8', newline='') as fh:
    fh.write('\t'.join(fields) + '\n')
    for r in rows:
        fh.write('\t'.join(r.get(k, '') or '' for k in fields) + '\n')
print('rows', len(rows), 'distinct addresses', len({r['addr'] for r in rows}))
print('by method', dict(collections.Counter(r['method'] for r in rows)))
print('high confidence rows', sum(1 for r in rows if r['confidence'] == 'high'), 'with a function on main', sum(1 for r in rows if r['confidence'] == 'high' and r['main_where']))
asm = {}
for l in open(fmap, encoding='utf-8'):
    d = json.loads(l)
    if d['addr'] is None or 'thunk' in d['name'] or d['name'].startswith(('label_', 'xt_')): continue
    asm['%06x' % d['addr']] = d
GAME = ('text/actor', 'text/contents', 'text/core', 'text/effects', 'text/gamemode', 'text/loginmode', 'text/session', 'text/world', 'text/render', 'text/skills', 'text/fileio', 'text/navi', 'text/iteminfo', 'text/platform', 'text/asyncwork', 'text/replay')
mapped = {r['addr'] for r in rows if r['confidence'] == 'high'}
tot = collections.Counter(); got = collections.Counter(); big = collections.Counter(); bigot = collections.Counter()
for a, d in asm.items():
    if not d['chunk'].startswith(GAME): continue
    k = d['chunk'].split('/')[1].split('_')[0]
    tot[k] += 1; got[k] += a in mapped
    if d['ninsn'] >= 20:
        big[k] += 1; bigot[k] += a in mapped
print('coverage of game chunks, high confidence (all / >=20 instructions):')
for k in sorted(tot):
    print(f'  {k:10} {got[k]:5}/{tot[k]:5}   {bigot[k]:5}/{big[k]:5}')
print(f'  {"total":10} {sum(got.values()):5}/{sum(tot.values()):5}   {sum(bigot.values()):5}/{sum(big.values()):5}')
