"""Show the original's arm for switch values: asm_arm.py TABLE VALUE... [--lines N]

TABLE is a jump table name from case_sweep.tsv (`jump_table_<addr>`); each VALUE (hex or
decimal) is resolved through the compare and lookup in front of the table's jump, as
case_sweep.py resolves it, and the arm's label and its first lines are printed.
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import case_sweep as cs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('table')
    ap.add_argument('values', nargs='+')
    ap.add_argument('--herc', default=os.path.normpath(os.path.join(cs.REPO, '..', 'herc-client')))
    ap.add_argument('--lines', type=int, default=14)
    args = ap.parse_args()
    asm = cs.Asm(args.herc)
    tables = [t for t in asm.tables if t.name == args.table]
    if not tables:
        sys.exit(f'no jump through {args.table}')
    texts = {}

    def lines_of(file):
        if file not in texts:
            with open(os.path.join(args.herc, file), encoding='utf-8', errors='replace') as f:
                texts[file] = f.read().split('\n')
        return texts[file]

    for t in tables:
        print(f'{t.name} in {t.function} at {t.where}: {len(t.values or {})} values, default {t.default}')
        for v in args.values:
            value = cs.parse_int(v)
            target = (t.values or {}).get(value)
            if not target:
                print(f'  {value:#x}: not a case of this table')
                continue
            file = t.where.split(':')[0]
            lines = lines_of(file)
            at = next((i for i, l in enumerate(lines) if l.startswith(target + ':')), None)
            if at is None:
                for other in texts or []:
                    pass
                print(f'  {value:#x} -> {target} (label not in {file})')
                continue
            print(f'  {value:#x} -> {target} ({file}:{at + 1})')
            for l in lines[at + 1:at + 1 + args.lines]:
                if l.strip():
                    print('      ' + l.rstrip())


if __name__ == '__main__':
    main()
