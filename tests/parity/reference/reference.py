"""Builds the herc-client reference the parity harness records from.

    python -I reference.py [--herc PATH] [--commit REV] [--out DIR] [--set KEY=VALUE ...]

Exports REV (default HEAD) of the herc-client checkout at PATH into DIR, copies the files git
does not track that the build needs (vendor\\*.lib, shield.local.conf, tools\\local_paths.json),
applies the reference's patches and builds DIR\\build\\main_rebuild.exe with Shield left out:

  - parity_probe.c joins src\\ (the state probe parity.dll calls), with parity_world.c and .h
    from ragexe's src\\parity (the world section both clients' probes write).
  - lifted.inc exports the client's own 64-bit divide and remainder outside the Shield block:
    C code divides 64-bit values (src\\meterwnd.c since 2026-10-08), and with Shield left out
    nothing else provides them, so a Shield-off build does not link.

Two features are off in the reference:

  - HERC_FEATURE_SHIELD: the harness runs without the Shield server.
  - HERC_FEATURE_TEXT_CACHE: the cache's slots are chosen by a hash of the font's GDI handle,
    which differs from run to run, so which entries it evicts, and when it frees their pixels,
    differs too. That frees CRT blocks in a different order in each run, and every allocation
    after it lands at a different address; the renderer settles draws at the same depth by
    address, so overlapping sprites swapped places between two replays. What the cache puts
    in a canvas is GDI's own output byte for byte, so the pictures are the same without it.

--set changes a features.conf value for this build (HERC_FEATURE_X=0, ...).
"""
import argparse
import glob
import io
import os
import shutil
import subprocess
import sys
import tarfile

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_HERC = os.path.normpath(os.path.join(HERE, '..', '..', '..', '..', 'herc-client'))
RAGEXE_SRC = os.path.normpath(os.path.join(HERE, '..', '..', '..', 'src', 'parity'))

HELPERS = (b"\tpublic __aulldiv as '__aulldiv'\t; unsigned 64-bit divide, edx:eax",
           b"\tpublic __aullrem as '__aullrem'\t; unsigned 64-bit remainder, edx:eax")
SHIELD_BLOCK = b'; ---- shield (HERC PATCH shield)'


def export(herc, commit, out):
    if os.path.exists(out):
        sys.exit(f'{out} exists; remove it or choose another --out')
    data = subprocess.run(['git', '-C', herc, 'archive', commit], capture_output=True, check=True).stdout
    with tarfile.open(fileobj=io.BytesIO(data)) as t:
        t.extractall(out, filter='data')
    for lib in glob.glob(os.path.join(herc, 'vendor', '*.lib')):
        shutil.copy2(lib, os.path.join(out, 'vendor'))
    for rel in ('shield.local.conf', os.path.join('tools', 'local_paths.json')):
        if os.path.exists(os.path.join(herc, rel)):
            shutil.copy2(os.path.join(herc, rel), os.path.join(out, rel))


def patch_helpers(out):
    """Moves the two helper exports in front of the Shield block when they sit inside it."""
    p = os.path.join(out, 'lifted.inc')
    b = open(p, 'rb').read()
    nl = b'\r\n' if b'\r\n' in b else b'\n'
    shield_at = b.find(SHIELD_BLOCK)
    if shield_at < 0 or all(b.find(h) < shield_at for h in HELPERS):
        return False
    for h in HELPERS:
        assert b.count(h + nl) == 1, h
        b = b.replace(h + nl, b'')
    head = b"; the client's own 64-bit unsigned divide and remainder, for C code that divides 64-bit values"
    b = b.replace(SHIELD_BLOCK, head + nl + HELPERS[0] + nl + HELPERS[1] + nl + SHIELD_BLOCK, 1)
    open(p, 'wb').write(b)
    return True


def features_file(out, sets):
    """The reference's features.conf: Shield and the text cache off, plus the --set values, line
    endings kept."""
    src = open(os.path.join(out, 'features.conf'), 'rb').read()
    nl = b'\r\n' if b'\r\n' in src else b'\n'
    lines = src.split(nl)
    wanted = dict(s.split('=', 1) for s in ['HERC_FEATURE_SHIELD=0', 'HERC_FEATURE_TEXT_CACHE=0'] + sets)
    for key, value in wanted.items():
        k = key.encode()
        hits = [i for i, l in enumerate(lines) if l.split(b'=', 1)[0].strip() == k]
        if not hits:
            sys.exit(f'{key} is not in features.conf')
        for i in hits:
            lines[i] = k + b'=' + value.encode()
    path = os.path.join(out, 'reference_features.conf')
    open(path, 'wb').write(nl.join(lines))
    return path


def build(out, features):
    env = dict(os.environ)
    root = env.get('SystemRoot', r'C:\WINDOWS')
    env['PATH'] = ';'.join([root + r'\System32', root, root + r'\System32\Wbem',
                            root + r'\System32\WindowsPowerShell\v1.0', r'C:\Program Files\Git\cmd',
                            env.get('ProgramFiles(x86)', r'C:\Program Files (x86)') + r'\Microsoft Visual Studio\Installer',
                            r'C:\Python312', r'C:\Python312\Scripts', r'C:\Program Files\CMake\bin'])
    env['HERC_FEATURES'] = features
    env.pop('NoDefaultCurrentDirectoryInExePath', None)
    log = os.path.join(out, 'reference_build.log')
    with open(log, 'w') as f:
        r = subprocess.run(['cmd.exe', '/c', os.path.join(out, 'build.bat')], cwd=out, env=env, stdout=f,
                           stderr=subprocess.STDOUT)
    exe = os.path.join(out, 'build', 'main_rebuild.exe')
    if r.returncode or not os.path.exists(exe):
        sys.exit(f'the reference build failed; see {log}')
    return exe


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--herc', default=DEFAULT_HERC)
    ap.add_argument('--commit', default='HEAD')
    ap.add_argument('--out', default=r'C:\parity\reference')
    ap.add_argument('--set', action='append', default=[])
    args = ap.parse_args()
    rev = subprocess.run(['git', '-C', args.herc, 'rev-parse', '--short', args.commit], capture_output=True,
                         check=True, text=True).stdout.strip()
    export(args.herc, rev, args.out)
    moved = patch_helpers(args.out)
    shutil.copy2(os.path.join(HERE, 'parity_probe.c'), os.path.join(args.out, 'src', 'parity_probe.c'))
    for name in ('parity_world.c', 'parity_world.h'):
        shutil.copy2(os.path.join(RAGEXE_SRC, name), os.path.join(args.out, 'src', name))
    exe = build(args.out, features_file(args.out, args.set))
    with open(os.path.join(args.out, 'reference.txt'), 'w') as f:
        f.write(f'herc-client {rev}\nhelper exports moved: {moved}\nset: {" ".join(args.set) or "-"}\n')
    print(f'reference herc-client {rev} built: {exe}')


if __name__ == '__main__':
    main()
