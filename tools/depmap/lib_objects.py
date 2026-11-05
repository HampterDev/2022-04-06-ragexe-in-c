"""Response files for linking the client from its libraries (build.bat libs).

Usage: lib_objects.py OBJDIR LISTING OUTDIR

Writes OUTDIR/<library>.rsp with the objects of each library of tools/libraries.txt except app,
OUTDIR/app.rsp with app's objects, and OUTDIR/libraries.txt with the library names from the
highest to the lowest, the order of the link line. Objects that define no external symbol (the
other platform's implementation files, empty on this one; LISTING is their dumpbin listing) are
left out, since lib.exe warns about them and nothing could pull them in. build\\gen's
effect_data_embedded.obj goes with the library of effects/effect_data.c, its only user.
"""
import sys
from pathlib import Path

import libtable
import objsyms


def main(argv):
    objdir, listing, outdir = Path(argv[0]), argv[1], Path(argv[2])
    root = Path(__file__).resolve().parent.parent.parent
    table = libtable.load()
    owners = table.assign(root / 'src')
    objects = objsyms.read(listing, {})
    defines = {name.lower(): bool(o.defs) for name, o in objects.items()}
    outdir.mkdir(parents=True, exist_ok=True)
    per_lib = {name: [] for name in table.order}
    for rel, lib in sorted(owners.items()):
        if not rel.endswith('.c'):
            continue
        obj = Path(rel).stem + '.obj'
        if not defines.get(obj.lower(), False):
            continue
        per_lib[lib].append(objdir / obj)
    per_lib[owners['effects/effect_data.c']].append(objdir / 'effect_data_embedded.obj')
    for lib, objs in per_lib.items():
        (outdir / f'{lib}.rsp').write_text(''.join(f'"{o}"\n' for o in objs), encoding='ascii')
    order = [lib for lib in reversed(table.order) if lib != 'app']
    (outdir / 'libraries.txt').write_text(''.join(f'{lib}\n' for lib in order), encoding='ascii')
    print(f'lib_objects: {sum(len(v) for v in per_lib.values())} objects in {len(per_lib)} libraries')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
