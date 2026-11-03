"""Validate the converted output against client-files.

    python tools/utf8-data/validate.py [--client-files DIR] [--out DIR] [--report DIR]

For every file in the output folder: no BOM; Lua files load with the vendored
Lua 5.1; every display field is valid UTF-8 and equals the UTF-8 form of its
source; every other field is byte-identical to the source; bytecode is
identical outside string constants; repaired files differ from the damaged
copy only where a string was restored.
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
    results = pipeline.validate_all(args.client_files, args.out)
    text = pipeline.validation_report(results)
    os.makedirs(args.report, exist_ok=True)
    with open(os.path.join(args.report, 'validate-report.txt'), 'w', encoding='utf-8') as f:
        f.write(text)
    sys.stdout.write(text)
    return 1 if any(r['problems'] for r in results) or not results else 0


if __name__ == '__main__':
    sys.exit(main())
