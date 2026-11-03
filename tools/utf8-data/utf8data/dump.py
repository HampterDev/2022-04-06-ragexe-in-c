"""Run build/utf8-data/luadump.exe and parse its canonical value dump.

A dump is a list of (path, value) pairs.  A path is a tuple of keys; a key or
value is one of
    ('n', '<%.17g>')  ('s', bytes)  ('b', '0'|'1')  ('p', bytes)  # stand-in name
    ('t',)  ('f',)  ('u',)  ('r', '<path text>')  ('tk',)
Call records use the path ('@call', seq, callee bytes, argument index).
"""
import os
import subprocess
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..', '..'))
LUADUMP = os.path.join(ROOT, 'build', 'utf8-data', 'luadump.exe')


class DumpError(Exception):
    pass


def _atom(text):
    tag, _, rest = text.partition(':')
    if tag in ('s', 'p'):
        return (tag, bytes.fromhex(rest))
    if tag in ('n', 'b', 'r'):
        return (tag, rest)
    return (tag,)


def parse(output: bytes):
    rows = []
    for line in output.split(b'\n'):
        if not line:
            continue
        parts = line.decode('ascii').split(' ')
        if parts[0] == 'V':
            rows.append((tuple(_atom(p) for p in parts[1:-1]), _atom(parts[-1])))
        elif parts[0] == 'C':
            seq, callee, arg, value = parts[1], bytes.fromhex(parts[2]), int(parts[3]), parts[4]
            rows.append((('@call', int(seq), callee, arg), _atom(value)))
        else:
            raise DumpError('unexpected dump line: %r' % line[:80])
    return rows


def run(chunks, call_main=False, exe=LUADUMP):
    """chunks: list of bytes (file contents) loaded in order into one environment."""
    if not os.path.isfile(exe):
        raise DumpError('%s is missing; run tools\\utf8-data\\build_luadump.bat' % exe)
    with tempfile.TemporaryDirectory() as tmp:
        paths = []
        for i, data in enumerate(chunks):
            p = os.path.join(tmp, 'chunk%d' % i)
            with open(p, 'wb') as f:
                f.write(data)
            paths.append(p)
        args = [exe] + (['--main'] if call_main else []) + paths
        proc = subprocess.run(args, capture_output=True)
    if proc.returncode != 0:
        raise DumpError(proc.stderr.decode('utf-8', 'replace').strip() or 'luadump failed')
    return parse(proc.stdout)


def key_text(atom):
    """Human-readable form of a key, used by rules and reports."""
    tag = atom[0]
    if tag == 's':
        return atom[1].decode('cp949', 'replace')
    if tag == 'n':
        return '#'
    if tag == 'p':
        return '[' + atom[1].decode('cp949', 'replace') + ']'
    return '<' + tag + '>'
