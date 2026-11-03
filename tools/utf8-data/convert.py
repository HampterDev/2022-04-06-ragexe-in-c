"""Convert the purist client's data to UTF-8 and repair the damaged files.

    python tools/utf8-data/convert.py [--client-files DIR] [--out DIR] [--report DIR]

Needs build/utf8-data/luadump.exe (tools/utf8-data/build_luadump.bat).
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from utf8data import pipeline  # noqa: E402

ROOT = os.path.normpath(os.path.join(HERE, '..', '..'))
GIT = os.path.dirname(ROOT)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--client-files', default=os.path.join(GIT, 'client-files'))
    ap.add_argument('--out', default=os.path.join(GIT, 'client-files-utf8'))
    ap.add_argument('--report', default=os.path.join(ROOT, 'build', 'utf8-data'))
    args = ap.parse_args()
    if os.path.normcase(os.path.abspath(args.out)).startswith(os.path.normcase(os.path.abspath(args.client_files)) + os.sep):
        ap.error('--out must not be inside --client-files')
    text, entries, repairs = pipeline.run(args.client_files, args.out, args.report)
    sys.stdout.write(text)
    bad = [e['name'] for e in entries if e.get('problems')] + [r['file'] for r in repairs if r['problems']]
    if bad:
        print('PROBLEMS in: %s' % ', '.join(bad))
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
