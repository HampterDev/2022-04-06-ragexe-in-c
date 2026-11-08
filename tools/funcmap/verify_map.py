"""Checks function_map.tsv against the disassembly's function starts and the pairs the oracle tests
proved, and rewrites it.

    python -I tools/funcmap/verify_map.py [--dump PATH] [--write]

Every row's address is looked up among the dump's function-start lines. A row whose address is
not a function start is marked: its method gets the suffix `_inside` and its confidence drops to
`low`, since the label it came from sits inside another routine. When the label names a C++
method (`Class__Method`) and the dump knows that method by name, the row's address becomes the
method's real start, method `dump_symbol`, confidence high.

The pairs in `oracle_pairs.tsv` (address, final name; each proven by a test under tests/oracle
driving the two sides over their input space) are written with method `oracle`, confidence
high; a row that paired that address or that function differently is dropped, with a note.
The pairs in `dispatch_pairs.tsv` (written by dispatch_check.py from the packet dispatcher's
jump table) are written the same way with method `dispatch`.
"""
import argparse
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, '..', '..'))
MAP = os.path.join(HERE, 'function_map.tsv')
PAIRS = ((os.path.join(HERE, 'oracle_pairs.tsv'), 'oracle', 'the oracle'),
         (os.path.join(HERE, 'dispatch_pairs.tsv'), 'dispatch', 'the packet dispatcher'))
DEFAULT_DUMP = os.path.normpath(os.path.join(REPO, '..', 'client-disassembly',
                                             '2022-04-06_Ragexe_1648707856.exe.bndb_disassembly.txt'))
START = re.compile(r'^00([0-9a-f]{6})  ([A-Za-z_].*?)\(')
NAME = re.compile(r'([A-Za-z_][A-Za-z0-9_]*::~?[A-Za-z_][A-Za-z0-9_]*)\s*$')


def read_dump(path):
    starts = set()
    by_name = {}
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            m = START.match(line)
            if not m:
                continue
            addr = int(m.group(1), 16)
            starts.add(addr)
            head = m.group(2)
            n = NAME.search(head)
            if n:
                by_name.setdefault(n.group(1), addr)
    return starts, by_name


def read_tsv(path):
    with open(path, encoding='utf-8') as f:
        head = f.readline().rstrip('\n').split('\t')
        rows = [dict(zip(head, l.rstrip('\n').split('\t'))) for l in f if l.strip()]
    return head, rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--dump', default=DEFAULT_DUMP)
    ap.add_argument('--write', action='store_true', help='rewrite function_map.tsv (else report only)')
    args = ap.parse_args()

    starts, by_name = read_dump(args.dump)
    head, rows = read_tsv(MAP)
    pairs = []
    for path, method, source in PAIRS:
        if os.path.exists(path):
            _, prows = read_tsv(path)
            pairs += [(int(r['addr'], 16), r['final_name'], r.get('note', ''), method, source) for r in prows]

    inside = moved = 0
    for r in rows:
        addr = int(r['addr'], 16)
        if addr in starts:
            continue
        label = r.get('asm_label', '')
        fixed = None
        if '__' in label:
            cls, _, meth = label.partition('__')
            fixed = by_name.get(f'{cls}::{meth}') or by_name.get(f'{cls}::{meth.rstrip("_")}')
        if fixed:
            r['addr'] = f'{fixed:x}'
            r['method'] = 'dump_symbol'
            r['confidence'] = 'high'
            moved += 1
        elif not r['method'].endswith('_inside'):
            r['method'] += '_inside'
            r['confidence'] = 'low'
            inside += 1

    dropped = []
    by_addr = {}
    for r in rows:
        by_addr.setdefault(int(r['addr'], 16), []).append(r)
    final_rows = []
    pair_addrs = {a: s for a, _, _, _, s in pairs}
    pair_names = {n: s for _, n, _, _, s in pairs}
    for r in rows:
        a = int(r['addr'], 16)
        if a in pair_addrs and r['final_name'] not in {n for pa, n, _, _, _ in pairs if pa == a}:
            dropped.append((r['addr'], r['final_name'], f'{pair_addrs[a]} pairs this address with another function'))
            continue
        if r['final_name'] in pair_names and a not in pair_addrs and (r['confidence'] == 'low' or pair_names[r['final_name']] != 'the oracle'):
            dropped.append((r['addr'], r['final_name'], f'{pair_names[r["final_name"]]} pairs this function with another address'))
            continue
        final_rows.append(r)
    existing = {(int(r['addr'], 16), r['final_name']) for r in final_rows}
    for a, n, note, method, _ in pairs:
        if (a, n) in existing:
            for r in final_rows:
                if int(r['addr'], 16) == a and r['final_name'] == n and (method == 'oracle' or r['confidence'] != 'high'):
                    r['method'], r['confidence'] = method, 'high'
            continue
        final_rows.append({k: '' for k in head} | {'addr': f'{a:x}', 'asm_label': f'sub_{a:x}', 'final_name': n,
                                                     'method': method, 'confidence': 'high', 'ref_file': note})
    final_rows.sort(key=lambda r: (int(r['addr'], 16), r['final_name']))

    print(f'{len(rows)} rows read; {inside} addresses inside another routine marked, {moved} moved to the dump\'s symbol, '
          f'{len(pairs)} proven pairs applied, {len(dropped)} rows dropped')
    for d in dropped:
        print('  dropped', *d)
    if args.write:
        with open(MAP, 'w', encoding='utf-8', newline='\n') as f:
            f.write('\t'.join(head) + '\n')
            for r in final_rows:
                f.write('\t'.join(r.get(k, '') for k in head) + '\n')
        print(f'{len(final_rows)} rows written to {MAP}')


if __name__ == '__main__':
    sys.exit(main())
