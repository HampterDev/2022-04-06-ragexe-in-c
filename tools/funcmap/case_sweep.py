"""The case-value sweep: every jump table in the original, as the set of switch values it
handles, against the case labels of the lifted C switch that handles the same values.

    python -I tools/funcmap/case_sweep.py [--asm PATH] [--out report.tsv] [--min-share 0.5]

A jump table in the assembly listing (`text/*.inc`) is read with the compare in front of
it: `sub/add/lea` for the base, `cmp R, N` and `ja default` for the range and the default arm,
the optional `movzx R, byte [R+lookup_table_Y]` for the arm index. Its values are base + i for
every i whose arm is not the default arm: the switch's case constants, which a C switch spells
as `case` labels. Each table is matched to the lifted C switches that share its values: those
sharing at least `--min-share` of them (one switch may have become several tables, and the lift
has split switches over several functions and files, so every such C switch counts and their
labels are joined). The values the table has and no matched switch has are the report: a
dropped jump-table target in the lift, or a case the lift handles elsewhere (an if before the
switch, a table), which the reader tells apart at the two sites the report names.

A table whose values match no C switch is listed too, with its values, since a C switch whose
labels are names (enums, macros) rather than numbers cannot be matched this way.
"""
import argparse
import os
import re
import sys
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, '..', '..'))

JMP = re.compile(r'^\s*jmp\s+dword\s+\[(\w+)\*4\+(jump_table_[0-9a-f]+)\]')
MOVZX = re.compile(r'^\s*movzx\s+(\w+),\s*byte\s+\[(\w+)\+(lookup_table_[0-9a-f]+)\]')
CMP = re.compile(r'^\s*cmp\s+(\w+),\s*(-?0x[0-9a-f]+|-?\d+)\s*$')
JA = re.compile(r'^\s*ja\s+(\w+)')
SUB = re.compile(r'^\s*sub\s+(\w+),\s*(0x[0-9a-f]+|\d+)\s*$')
ADD = re.compile(r'^\s*add\s+(\w+),\s*(0x[0-9a-f]+|\d+)\s*$')
LEA = re.compile(r'^\s*lea\s+(\w+),\s*\[(\w+)([+-])(0x[0-9a-f]+|\d+)\]')
DEC = re.compile(r'^\s*dec\s+(\w+)\s*$')
LABEL = re.compile(r'^([A-Za-z_][A-Za-z0-9_]*):(.*)$')
TABLE_DEF = re.compile(r'^(jump_table_[0-9a-f]+):\s*$')
LOOKUP_DEF = re.compile(r'^(lookup_table_[0-9a-f]+):\s*$')
DD = re.compile(r'^\s*dd\s+([A-Za-z_][A-Za-z0-9_]*)\s*(?:;.*)?$')
DB = re.compile(r'^\s*db\s+(.*?)\s*(?:;.*)?$')
NOT_FUNCTIONS = ('label_', 'xt_', 'jump_table_', 'lookup_table_', 'data_', 'jtfix_')
C_CASE = re.compile(r'\bcase\s+([^:;{}]+?)\s*:')
C_SWITCH = re.compile(r'\bswitch\s*\(')
C_DEF = re.compile(r'^(?!\s)(?!#|typedef|struct\s+\w+\s*(\{|;)|extern|return|else|case|default)'
                   r'[A-Za-z_][A-Za-z0-9_ \t\*]*?\b([A-Za-z_][A-Za-z0-9_]*)\s*\(')


def parse_int(text):
    text = text.strip()
    neg = text.startswith('-')
    if neg:
        text = text[1:]
    v = int(text, 16) if text.lower().startswith('0x') else int(text)
    return -v if neg else v


class Table:
    def __init__(self, name, function, where):
        self.name, self.function, self.where = name, function, where
        self.values = {}          # case value -> target label


class Asm:
    def __init__(self, listing):
        self.jump = {}
        self.lookup = {}
        self.tables = []
        files = []
        for sub in ('text', 'rdata'):
            d = os.path.join(listing, sub)
            if os.path.isdir(d):
                files += [(sub, n) for n in sorted(os.listdir(d)) if n.endswith('.inc')]
        texts = {}
        for sub, name in files:
            with open(os.path.join(listing, sub, name), encoding='utf-8', errors='replace') as f:
                lines = f.read().split('\n')
            texts[f'{sub}/{name}'] = lines
            self._collect_defs(lines)
        for file, lines in texts.items():
            if file.startswith('text/'):
                self._collect_sites(file, lines)

    def _collect_defs(self, lines):
        i = 0
        while i < len(lines):
            m = TABLE_DEF.match(lines[i])
            if m:
                targets = []
                j = i + 1
                while j < len(lines) and DD.match(lines[j]):
                    targets.append(DD.match(lines[j]).group(1))
                    j += 1
                self.jump[m.group(1)] = targets
                i = j
                continue
            m = LOOKUP_DEF.match(lines[i])
            if m:
                bytes_ = []
                j = i + 1
                while j < len(lines) and DB.match(lines[j]):
                    for tok in DB.match(lines[j]).group(1).split(','):
                        tok = tok.strip()
                        if tok:
                            bytes_.append(parse_int(tok))
                    j += 1
                self.lookup[m.group(1)] = bytes_
                i = j
                continue
            i += 1

    def _collect_sites(self, file, lines):
        function = None
        for i, line in enumerate(lines):
            m = LABEL.match(line)
            if m and not m.group(1).startswith(NOT_FUNCTIONS) and 'chunk of' not in m.group(2):
                function = m.group(1)
            j = JMP.match(line)
            if not j:
                continue
            reg, name = j.group(1), j.group(2)
            table = Table(name, function, f'{file}:{i + 1}')
            lookup = None
            limit = default = None
            base = 0
            k = i - 1
            steps = 0
            while k >= 0 and steps < 10:
                s = lines[k].strip()
                if not s:
                    k -= 1
                    continue
                steps += 1
                mz = MOVZX.match(lines[k])
                if mz and lookup is None and limit is None:
                    lookup = self.lookup.get(mz.group(3))
                    reg = mz.group(2)
                    k -= 1
                    continue
                ja = JA.match(lines[k])
                if ja and default is None:
                    default = ja.group(1)
                    k -= 1
                    continue
                c = CMP.match(lines[k])
                if c and default is not None and limit is None:
                    limit = parse_int(c.group(2))
                    reg = c.group(1)
                    k -= 1
                    continue
                if limit is not None:
                    ms = SUB.match(lines[k])
                    ma = ADD.match(lines[k])
                    ml = LEA.match(lines[k])
                    md = DEC.match(lines[k])
                    if ms and ms.group(1) == reg:
                        base = parse_int(ms.group(2))
                    elif ma and ma.group(1) == reg:
                        base = -((parse_int(ma.group(2)) ^ 0xffffffff) + 1) if parse_int(ma.group(2)) >= 0x80000000 else -parse_int(ma.group(2))
                        base = -base if False else base
                        base = (0x100000000 - parse_int(ma.group(2))) if parse_int(ma.group(2)) >= 0x80000000 else -parse_int(ma.group(2))
                    elif ml and ml.group(1) == reg:
                        base = parse_int(ml.group(4)) if ml.group(3) == '-' else -parse_int(ml.group(4))
                    elif md and md.group(1) == reg:
                        base = 1
                    break
                k -= 1
            targets = self.jump.get(name, [])
            if limit is None or not targets:
                table.values = None
                table.note = 'no compare in front' if limit is None else 'no table body'
            else:
                for idx in range(limit + 1):
                    arm = lookup[idx] if lookup and idx < len(lookup) else idx
                    if arm >= len(targets):
                        continue
                    target = targets[arm]
                    if target != default:
                        table.values[base + idx] = target
                table.note = ''
            table.default = default
            self.tables.append(table)


class CSwitch:
    def __init__(self, function, where):
        self.function, self.where = function, where
        self.values = set()
        self.symbolic = 0


def c_switches(src):
    out = []
    for dp, _, files in os.walk(src):
        for name in sorted(files):
            if not name.endswith('.c'):
                continue
            path = os.path.join(dp, name)
            rel = os.path.relpath(path, src).replace('\\', '/')
            with open(path, encoding='utf-8', errors='replace') as f:
                lines = f.read().split('\n')
            function = '?'
            stack = []          # (switch, depth at its block)
            depth = 0
            for i, line in enumerate(lines):
                m = C_DEF.match(line)
                if m and depth == 0 and not line.rstrip().endswith(';'):
                    function = m.group(2)
                for _ in C_SWITCH.finditer(line):
                    sw = CSwitch(function, f'{rel}:{i + 1}')
                    out.append(sw)
                    stack.append([sw, None])
                if stack and not line.lstrip().startswith(('//', '*', '/*')):
                    sw = stack[-1][0]
                    for c in C_CASE.finditer(line):
                        text = c.group(1).strip()
                        try:
                            if text.startswith("'") and text.endswith("'") and len(text) in (3, 4):
                                sw.values.add(ord(text[1]) if len(text) == 3 else ord(text[2]))
                            else:
                                sw.values.add(parse_int(text))
                        except ValueError:
                            sw.symbolic += 1
                for ch in line:
                    if ch == '{':
                        depth += 1
                        if stack and stack[-1][1] is None:
                            stack[-1][1] = depth
                    elif ch == '}':
                        if stack and stack[-1][1] == depth:
                            stack.pop()
                        depth -= 1
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--asm', help='the folder of the assembly listing', default=os.path.normpath(os.path.join(REPO, '..', 'herc-client')))
    ap.add_argument('--out', default=os.path.join(HERE, 'case_sweep.tsv'))
    ap.add_argument('--min-share', type=float, default=0.5)
    args = ap.parse_args()

    asm = Asm(args.asm)
    switches = c_switches(os.path.join(REPO, 'src'))
    by_value = defaultdict(set)
    for sw in switches:
        for v in sw.values:
            by_value[v].add(sw)

    rows, unmatched, unread = [], [], []
    for t in asm.tables:
        if t.values is None:
            unread.append(t)
            continue
        if not t.values:
            continue
        counts = defaultdict(int)
        for v in t.values:
            for sw in by_value.get(v, ()):
                counts[sw] += 1
        need = max(2, int(len(t.values) * args.min_share + 0.999))
        matched = [sw for sw, n in counts.items() if n >= need]
        if not matched:
            unmatched.append(t)
            continue
        # The lift split many switches over several functions and files, so every C switch that
        # shares a few of the table's values also covers it: a value none of them has is the lead.
        lenient_need = max(3, int(len(t.values) * 0.1 + 0.999))
        covering = [sw for sw, n in counts.items() if n >= lenient_need]
        covered = set()
        for sw in covering:
            covered |= sw.values
        strict = set()
        for sw in matched:
            strict |= sw.values
        missing = sorted(v for v in t.values if v not in covered)
        strict_missing = sum(1 for v in t.values if v not in strict)
        rows.append((len(missing), t, matched, missing, strict_missing, covering))
    rows.sort(key=lambda r: (-r[0], r[1].where))
    with open(args.out, 'w', encoding='utf-8', newline='\n') as f:
        f.write('missing\tstrict_missing\ttable_values\ttable\tasm_function\tasm_where\tc_switches\tcovering\tmissing_values\n')
        for n, t, matched, missing, strict_missing, covering in rows:
            f.write('\t'.join([str(n), str(strict_missing), str(len(t.values)), t.name, t.function or '', t.where,
                               ' '.join(sorted(f'{sw.function}@{sw.where}' for sw in matched)),
                               ' '.join(sorted({sw.function for sw in covering})),
                               ' '.join(f'{v:#x}' for v in missing)]) + '\n')
        f.write('\n# tables matched by no C switch (values, table, function, where, the values)\n')
        for t in sorted(unmatched, key=lambda t: -len(t.values)):
            vals = sorted(t.values)
            shown = ' '.join(f'{v:#x}' for v in vals[:24]) + (' ...' if len(vals) > 24 else '')
            f.write(f'#\t{len(vals)}\t{t.name}\t{t.function}\t{t.where}\t{shown}\n')
        f.write('\n# tables whose compare the reader could not parse\n')
        for t in unread:
            f.write(f'#\t{t.name}\t{t.function}\t{t.where}\t{t.note}\n')
    flagged = sum(1 for r in rows if r[0])
    print(f'{len(asm.tables)} tables: {len(rows)} matched to C switches, {flagged} with missing values, '
          f'{len(unmatched)} unmatched, {len(unread)} unread -> {args.out}')
    for n, t, matched, missing, strict_missing, covering in rows[:45]:
        if not n:
            break
        names = ' '.join(sorted({sw.function for sw in covering}))[:70]
        shown = ' '.join(f'{v:#x}' for v in missing[:12]) + (' ...' if len(missing) > 12 else '')
        print(f'  {n:>3} of {len(t.values):>4} missing  {t.function:34s} {t.where:34s} -> {names:70s} {shown}')


if __name__ == '__main__':
    sys.exit(main())
