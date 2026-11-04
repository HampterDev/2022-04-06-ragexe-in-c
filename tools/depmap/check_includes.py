"""Include check: no file includes a header of a library its library may not use.

Usage: check_includes.py [--table FILE] [--src DIR] [--strict] [--verbose]

Reads every `#include` of every file under src (all of them, whatever `#if` they sit in) and
resolves it as the compiler does: first next to the including file, then from src, the one /I
root all client includes are written from. An include that resolves to no file under src is a
system or vendor header and is not checked. The include is allowed when the included file's
library is the including file's library or one it may use (tools/libraries.txt), or when the
table lists it as an exception.

Exit code 1 when an include breaks the table, when a file under src belongs to no library or to
two, or (with --strict) when an exception no longer matches an include.
"""
import re
import sys
from collections import defaultdict
from pathlib import Path

import libtable

INCLUDE = re.compile(r'^\s*#\s*include\s*([<"])([^>"]+)[>"]')


def includes_of(path):
    """[(line number, quote, name)] for every #include line of a file."""
    out = []
    text = path.read_text(encoding='utf-8', errors='replace')
    for no, line in enumerate(text.splitlines(), 1):
        m = INCLUDE.match(line)
        if m:
            out.append((no, m.group(1), m.group(2)))
    return out


def resolve(src, rel, quote, name):
    """The src-relative path an include names, or None for a header outside src."""
    candidates = []
    if quote == '"':
        candidates.append((src / rel).parent / name)
    candidates.append(src / name)
    for c in candidates:
        try:
            r = c.resolve().relative_to(src.resolve()).as_posix()
        except ValueError:
            continue
        if c.is_file():
            return r
    return None


def check(table, src, strict=False, verbose=False, out=sys.stdout):
    try:
        owners = table.assign(src)
    except libtable.TableError as e:
        print(f'check_includes: {e}', file=out)
        return 1
    violations = defaultdict(list)       # (from lib, to lib) -> [(file, line, header)]
    used = set()
    n_includes = 0
    for rel in sorted(owners):
        lib = owners[rel]
        for no, quote, name in includes_of(src / rel):
            target = resolve(src, rel, quote, name)
            if target is None or target == rel:
                continue
            n_includes += 1
            tlib = owners[target]
            if table.may_use(lib, tlib):
                continue
            key = (rel, target)
            if key in table.include_exceptions:
                used.add(key)
                continue
            violations[(lib, tlib)].append((rel, no, target))
    unused = sorted(set(table.include_exceptions) - used)
    status = 0
    if violations:
        status = 1
        n = sum(len(v) for v in violations.values())
        print(f'check_includes: {n} includes break tools/libraries.txt:', file=out)
        for (a, b), vs in sorted(violations.items()):
            print(f'  {a} may not use {b}:', file=out)
            for rel, no, target in vs:
                print(f'    src/{rel}:{no}: includes {target}', file=out)
    if unused:
        print(f'check_includes: {len(unused)} include exceptions match no include:', file=out)
        for rel, target in unused:
            e = table.include_exceptions[(rel, target)]
            print(f'    {e.name}: {rel} {target}', file=out)
        if strict:
            status = 1
    if status == 0:
        print(f'check_includes: passed: {len(owners)} files in {len(table.order)} libraries, '
              f'{n_includes} includes, {len(used)} of them exceptions', file=out)
    if verbose:
        for e in table.exceptions:
            if e.includes:
                print(f'  exception {e.name}: {len(e.includes)} includes', file=out)
    return status


def main(argv):
    args = list(argv)
    opts = {'--table': None, '--src': None}
    for k in list(opts):
        if k in args:
            i = args.index(k)
            opts[k] = args[i + 1]
            del args[i:i + 2]
    root = Path(__file__).resolve().parent.parent.parent
    table = libtable.load(opts['--table'])
    src = Path(opts['--src']) if opts['--src'] else root / 'src'
    return check(table, src, strict='--strict' in args, verbose='--verbose' in args)


if __name__ == '__main__':
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    try:
        sys.exit(main(sys.argv[1:]))
    except libtable.TableError as e:
        print(f'check_includes: {e}')
        sys.exit(1)
