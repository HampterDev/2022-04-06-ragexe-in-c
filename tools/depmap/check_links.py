"""Link check: no object uses a symbol that only a library it may not use defines.

Usage: check_links.py LISTING... [--nm OBJDIR] [--table FILE] [--src DIR] [--strict]

LISTING is the `dumpbin /symbols` output for the objects of a build (tools\\check_libraries.bat
writes it for build.bat's object folder) or an `nm -P -A` listing of a GCC or Clang build's
objects; with --nm the check runs `nm -P -A` itself on every .o under OBJDIR (CMake's
CMakeFiles/ragexe.dir, which the CTest library_links of cmake/Client.cmake passes). Each
object is matched to its source under src (build.bat names objects after the source's basename,
which is unique; CMake keeps the source path). Objects not built from src, such as the vendored
libraries and build\\gen\\effect_data_embedded.c, are not checked and define nothing here.

For every external symbol an object uses, the objects that define it are looked up; communal
globals (`int g_x;` without an initializer) count as defined by the objects that declare them
that way. The use is allowed when one of the definers is in the object's own library or in one it
may use, or when the table lists (source, symbol) as an exception. A symbol that no client object
defines (the C runtime, Windows, the vendored libraries) is not checked.

Exit code 1 when a use breaks the table, when a file under src belongs to no library or to two,
or (with --strict) when an exception no longer matches a use.
"""
import subprocess
import sys
import tempfile
from collections import defaultdict
from pathlib import Path

import libtable
import objsyms


def check(table, src, listings, strict=False, out=sys.stdout):
    try:
        owners = table.assign(src)
    except libtable.TableError as e:
        print(f'check_links: {e}', file=out)
        return 1
    sources = {f for f in owners if f.endswith('.c')}
    by_stem = {}
    for f in sources:
        by_stem.setdefault(Path(f).stem, f)
    objects = {}
    for listing in listings:
        objsyms.read(listing, objects)
    source_of = {}
    for name in objects:
        rel = objsyms.source_of_object(name, by_stem, sources)
        if rel is not None:
            if rel in source_of.values():
                print(f'check_links: two objects for src/{rel}', file=out)
                return 1
            source_of[name] = rel
    if not source_of:
        print('check_links: the listing names no object built from src', file=out)
        return 1
    definers = defaultdict(set)          # symbol -> libraries
    for name, rel in source_of.items():
        for sym in objects[name].defs:
            definers[sym].add(owners[rel])
    violations = defaultdict(list)       # (from lib, to lib) -> [(file, symbol)]
    used = set()
    n_refs = 0
    for name, rel in sorted(source_of.items(), key=lambda kv: kv[1]):
        lib = owners[rel]
        allowed = table.libraries[lib].allowed | {lib}
        for sym in sorted(objects[name].refs):
            libs = definers.get(sym)
            if not libs:
                continue
            n_refs += 1
            if libs & allowed:
                continue
            key = (rel, objsyms.plain(sym))
            if key in table.link_exceptions:
                used.add(key)
                continue
            target = min(libs, key=table.order.index)
            violations[(lib, target)].append(key)
    unused = sorted(set(table.link_exceptions) - used)
    status = 0
    if violations:
        status = 1
        n = sum(len(v) for v in violations.values())
        print(f'check_links: {n} symbol uses break tools/libraries.txt:', file=out)
        for (a, b), vs in sorted(violations.items()):
            print(f'  {a} may not use {b}:', file=out)
            for rel, sym in vs:
                print(f'    src/{rel}: uses {sym}', file=out)
    if unused:
        print(f'check_links: {len(unused)} link exceptions match no use:', file=out)
        for rel, sym in unused:
            print(f'    {table.link_exceptions[(rel, sym)].name}: {rel} {sym}', file=out)
        if strict:
            status = 1
    missing = len(sources) - len(source_of)
    if status == 0:
        print(f'check_links: passed: {len(source_of)} objects ({missing} sources without one), '
              f'{n_refs} uses of client symbols, {len(used)} of them exceptions', file=out)
    return status


def nm_listing(objdir, out):
    """Write `nm -P -A` of every object under objdir to the open file out."""
    objs = sorted(str(p) for p in Path(objdir).rglob('*') if p.suffix in ('.o', '.obj') and p.is_file())
    if not objs:
        raise SystemExit(f'check_links: no objects under {objdir}')
    for i in range(0, len(objs), 200):
        r = subprocess.run(['nm', '-P', '-A'] + objs[i:i + 200], capture_output=True, text=True,
                           encoding='utf-8', errors='replace')
        if r.returncode:
            raise SystemExit(f'check_links: nm failed: {r.stderr.strip()}')
        out.write(r.stdout)


def main(argv):
    args = list(argv)
    opts = {'--table': None, '--src': None, '--nm': None}
    for k in list(opts):
        if k in args:
            i = args.index(k)
            opts[k] = args[i + 1]
            del args[i:i + 2]
    strict = '--strict' in args
    listings = [a for a in args if not a.startswith('--')]
    root = Path(__file__).resolve().parent.parent.parent
    table = libtable.load(opts['--table'])
    src = Path(opts['--src']) if opts['--src'] else root / 'src'
    if opts['--nm']:
        with tempfile.TemporaryDirectory() as tmp:
            listing = Path(tmp) / 'nm.txt'
            with open(listing, 'w', encoding='utf-8') as out:
                nm_listing(opts['--nm'], out)
            return check(table, src, listings + [str(listing)], strict=strict)
    if not listings:
        print(__doc__)
        return 2
    return check(table, src, listings, strict=strict)


if __name__ == '__main__':
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    try:
        sys.exit(main(sys.argv[1:]))
    except libtable.TableError as e:
        print(f'check_links: {e}')
        sys.exit(1)
