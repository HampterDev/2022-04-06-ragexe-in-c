"""The empty-switch fingerprint sweep: the arms of every jump table in the original against the
arms of the switches in the lifted C that came from the same function.

    python -I tools/funcmap/switch_sweep.py [--herc PATH] [--out report.tsv] [--all]

The original's side is the reassembly's text chunks (herc-client `text/*.inc`): every function
(a `sub_<addr>` or named label; a label marked `chunk of` stays in its function) with the jump
tables it jumps through (`jmp dword [reg*4+jump_table_X]`) and, for each, the distinct targets of
its `dd` entries: the arms the compiler emitted, raw-byte `xt_` targets among them. The lift's
side is every function in `src/` with its `switch` statements and its arm groups (a run of
`case` labels sharing one body counts once; `default` is not counted, since a table holds the
default arm only when it also covers an in-range value).

An original function is paired with its C functions three ways, all taken together: the
function map (`function_map.tsv`, asm label -> `main_where`); the string literals it pushes
that occur in at most three original functions, matched against the literals of the C
functions (the lift split many large switches over several files, so every C function sharing
such a string is taken, and their arms are summed); and, for a function with no map row and
no telling string, nothing: it is listed as unpaired, with its arms, for a reader.

A paired function is flagged when the original has more distinct arms than the lift has case
groups plus one (the default arm a table may hold), or more tables than the lift has switches.
Both are the shape of a dropped jump-table target or a dropped switch. A flagged function is a
lead to read, not a verdict: the lift may spell a switch as a lookup table or an if-chain, or
the compiler may have merged arms the C spells apart (that direction is never flagged).
"""
import argparse
import os
import re
import sys
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, '..', '..'))
MAP = os.path.join(HERE, 'function_map.tsv')

JMP_TABLE = re.compile(r'jmp\s+dword\s+\[[^\]]*\b(jump_table_[0-9a-f]+)\]')
TABLE_DEF = re.compile(r'^(jump_table_[0-9a-f]+):\s*$')
DD_ENTRY = re.compile(r'^\s*dd\s+([A-Za-z_][A-Za-z0-9_]*)\s*(?:;.*)?$')
FUNC_LABEL = re.compile(r'^([A-Za-z_][A-Za-z0-9_]*):(.*)$')
ASM_STRING = re.compile(r';\s*"((?:[^"\\]|\\.)*)"')
NOT_FUNCTIONS = ('label_', 'xt_', 'jump_table_', 'lookup_table_', 'data_', 'jtfix_')
C_STRING = re.compile(r'"((?:[^"\\\n]|\\.)*)"')
C_DEF = re.compile(r'^(?!\s)(?!#|typedef|struct\s+\w+\s*(\{|;)|extern|return|else|case|default)'
                   r'[A-Za-z_][A-Za-z0-9_ \t\*]*?\b([A-Za-z_][A-Za-z0-9_]*)\s*\(')
CASE = re.compile(r'^\s*case\b')
DEFAULT = re.compile(r'^\s*default\s*:')
SWITCH = re.compile(r'\bswitch\s*\(')
MAX_STRING_FUNCTIONS = 3


def load_map():
    rows = []
    with open(MAP, encoding='utf-8') as f:
        head = f.readline().rstrip('\n').split('\t')
        for line in f:
            rows.append(dict(zip(head, line.rstrip('\n').split('\t'))))
    return rows


def c_unescape(s):
    out, i = [], 0
    while i < len(s):
        c = s[i]
        if c == '\\' and i + 1 < len(s):
            n = s[i + 1]
            out.append({'n': '\n', 't': '\t', 'r': '\r', '0': '\0', '\\': '\\', '"': '"', "'": "'"}.get(n, '\\' + n))
            i += 2
        else:
            out.append(c)
            i += 1
    return ''.join(out)


class AsmFunction:
    def __init__(self, label, file, line):
        self.label, self.file, self.line = label, file, line
        self.tables = []
        self.strings = set()
        self.end = line


class Asm:
    """The reassembly's functions: their tables, their string literals; every table's targets."""

    def __init__(self, herc):
        self.tables = {}
        self.functions = []
        text = os.path.join(herc, 'text')
        for name in sorted(os.listdir(text)):
            if name.endswith('.inc'):
                with open(os.path.join(text, name), encoding='utf-8', errors='replace') as f:
                    lines = f.read().split('\n')
                self._collect_tables(lines)
                self._collect_functions('text/' + name, lines)
        rdata = os.path.join(herc, 'rdata')
        if os.path.isdir(rdata):
            for name in sorted(os.listdir(rdata)):
                if name.endswith('.inc'):
                    with open(os.path.join(rdata, name), encoding='utf-8', errors='replace') as f:
                        self._collect_tables(f.read().split('\n'))

    def _collect_tables(self, lines):
        i = 0
        while i < len(lines):
            m = TABLE_DEF.match(lines[i])
            if not m:
                i += 1
                continue
            targets = []
            j = i + 1
            while j < len(lines):
                e = DD_ENTRY.match(lines[j])
                if not e:
                    break
                targets.append(e.group(1))
                j += 1
            self.tables[m.group(1)] = targets
            i = j

    def _collect_functions(self, file, lines):
        current = None
        for i, line in enumerate(lines, 1):
            m = FUNC_LABEL.match(line)
            if m and not m.group(1).startswith(NOT_FUNCTIONS) and 'chunk of' not in m.group(2):
                current = AsmFunction(m.group(1), file, i)
                self.functions.append(current)
                continue
            if current is None:
                continue
            current.end = i
            for t in JMP_TABLE.finditer(line):
                if t.group(1) not in current.tables:
                    current.tables.append(t.group(1))
            for s in ASM_STRING.finditer(line):
                text = s.group(1)
                if text.isascii() and len(text) >= 4:
                    current.strings.add(text)

    def arms(self, fn):
        return sum(len(set(self.tables.get(t, []))) for t in fn.tables)


C_CONST = re.compile(r'^static\s+const\s+char\s+\*?\s*([A-Za-z_][A-Za-z0-9_]*)\s*(?:\[\])?\s*=\s*"((?:[^"\\\n]|\\.)*)"')
IDENT = re.compile(r'[A-Za-z_][A-Za-z0-9_]*')


class CFunction:
    def __init__(self, name, where, body, constants):
        self.name, self.where = name, where
        self.switches, self.groups, self.defaults = switch_shape(body)
        self.strings = set()
        for line in body:
            for s in C_STRING.finditer(line):
                self._add(c_unescape(s.group(1)))
            if constants:
                for ident in IDENT.findall(line):
                    if ident in constants:
                        self._add(constants[ident])

    def _add(self, text):
        if text.isascii() and len(text) >= 4:
            self.strings.add(text)


def switch_shape(body):
    switches = sum(len(SWITCH.findall(line)) for line in body)
    groups = defaults = 0
    in_group = False
    for line in body:
        s = line.strip()
        if not s:
            continue
        if CASE.match(line):
            if not in_group:
                groups += 1
            in_group = s.endswith(':') or s.endswith(': {')
            continue
        if DEFAULT.match(line):
            defaults += 1
        in_group = False
    return switches, groups, defaults


def c_functions(src):
    """Every function definition under src, with its body."""
    out = []
    for dp, _, files in os.walk(src):
        for name in sorted(files):
            if not name.endswith('.c'):
                continue
            path = os.path.join(dp, name)
            rel = os.path.relpath(path, src).replace('\\', '/')
            with open(path, encoding='utf-8', errors='replace') as f:
                lines = f.read().split('\n')
            constants = {}
            for line in lines:
                m = C_CONST.match(line)
                if m:
                    constants[m.group(1)] = c_unescape(m.group(2))
            i = 0
            while i < len(lines):
                m = C_DEF.match(lines[i])
                if m and not lines[i].rstrip().endswith(';') and '=' not in lines[i].split('(')[0]:
                    j = i
                    opened = False
                    while j < len(lines) and j < i + 8:
                        if '{' in lines[j]:
                            opened = True
                            break
                        if ';' in lines[j]:
                            break
                        j += 1
                    if opened:
                        body, end = function_body(lines, i)
                        out.append(CFunction(m.group(2), f'{rel}:{i + 1}', body, constants))
                        i = end + 1
                        continue
                i += 1
    return out


def function_body(lines, start):
    depth = 0
    started = False
    i = start
    body = []
    while i < len(lines):
        body.append(lines[i])
        for ch in lines[i]:
            if ch == '{':
                depth += 1
                started = True
            elif ch == '}':
                depth -= 1
        if started and depth <= 0:
            break
        i += 1
    return body, i


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--herc', default=os.path.normpath(os.path.join(REPO, '..', 'herc-client')))
    ap.add_argument('--out', default=os.path.join(HERE, 'switch_sweep.tsv'))
    ap.add_argument('--all', action='store_true', help='report every paired function, flagged or not')
    args = ap.parse_args()

    asm = Asm(args.herc)
    with_tables = [fn for fn in asm.functions if fn.tables]
    cfuncs = c_functions(os.path.join(REPO, 'src'))
    by_name = defaultdict(list)
    for c in cfuncs:
        by_name[c.name].append(c)
    by_string = defaultdict(set)
    for c in cfuncs:
        for s in c.strings:
            by_string[s].add(c)
    string_users = defaultdict(int)
    for fn in asm.functions:
        for s in fn.strings:
            string_users[s] += 1
    mapped = {}
    for row in load_map():
        if row.get('main_where') and row.get('asm_label'):
            mapped.setdefault(row['asm_label'], set()).add(row['final_name'])

    report, unpaired = [], []
    for fn in with_tables:
        arms = asm.arms(fn)
        partners = set()
        how = []
        for name in mapped.get(fn.label, ()):
            if by_name.get(name):
                partners.update(by_name[name])
                how.append('map')
        telling = [s for s in fn.strings if string_users[s] <= MAX_STRING_FUNCTIONS and by_string.get(s)]
        for s in telling:
            partners.update(by_string[s])
        if telling:
            how.append(f'{len(telling)} strings')
        if not partners:
            unpaired.append((arms, fn))
            continue
        groups = sum(c.groups for c in partners)
        switches = sum(c.switches for c in partners)
        arm_gap = arms - (groups + 1)
        table_gap = len(fn.tables) - switches
        flagged = arm_gap > 0 or table_gap > 0
        if flagged or args.all:
            names = ' '.join(sorted(f'{c.name}@{c.where}' for c in partners))
            report.append((arm_gap, table_gap, fn.label, f'{fn.file}:{fn.line}-{fn.end}', len(fn.tables), arms,
                           switches, groups, '+'.join(how), names))
    report.sort(key=lambda r: (-r[0], -r[1], r[2]))
    unpaired.sort(key=lambda u: -u[0])
    with open(args.out, 'w', encoding='utf-8', newline='\n') as f:
        f.write('arm_gap\ttable_gap\tasm_label\tasm_where\tasm_tables\tasm_arms\tc_switches\tc_case_groups\tpaired_by\tc_functions\n')
        for r in report:
            f.write('\t'.join(str(x) for x in r) + '\n')
        f.write('\n# unpaired original functions with jump tables (arms, label, where)\n')
        for arms, fn in unpaired:
            f.write(f'#\t{arms}\t{fn.label}\t{fn.file}:{fn.line}-{fn.end}\t{len(fn.tables)} tables\n')
    flagged = sum(1 for r in report if r[0] > 0 or r[1] > 0)
    print(f'{len(with_tables)} original functions with jump tables: {len(report)} reported, {flagged} flagged, '
          f'{len(unpaired)} unpaired ({sum(a for a, _ in unpaired)} arms) -> {args.out}')
    for r in report[:30]:
        print(f'  arms {r[5]:>4} vs groups {r[7]:>4}+1  tables {r[4]:>2} vs switches {r[6]:>2}  {r[2]:36s} {r[8]:12s} {r[9][:90]}')


if __name__ == '__main__':
    sys.exit(main())
