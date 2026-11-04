"""Whether build.bat's objects are current: stale_objects.py OBJDIR [--src DIR]

Exit code 0 when every .c under src has OBJDIR/<basename>.obj and no file under src (sources,
headers, .def tables) is newer than the oldest of those objects; 1 otherwise, with the reason.
tools\\check_libraries.bat runs build.bat first when this fails, so the link check never reads
objects of an older tree.
"""
import sys
from pathlib import Path


def main(argv):
    args = list(argv)
    root = Path(__file__).resolve().parent.parent.parent
    src = root / 'src'
    if '--src' in args:
        i = args.index('--src')
        src = Path(args[i + 1])
        del args[i:i + 2]
    obj = Path(args[0])
    oldest = None
    for c in sorted(src.rglob('*.c')):
        o = obj / (c.stem + '.obj')
        if not o.is_file():
            print(f'stale_objects: no {o.name} for src/{c.relative_to(src).as_posix()}')
            return 1
        t = o.stat().st_mtime
        oldest = t if oldest is None else min(oldest, t)
    if oldest is None:
        print('stale_objects: no sources')
        return 1
    for f in src.rglob('*'):
        if f.is_file() and f.stat().st_mtime > oldest:
            print(f'stale_objects: src/{f.relative_to(src).as_posix()} is newer than the objects in {obj}')
            return 1
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
