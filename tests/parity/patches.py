"""herc-client's patches, each with the feature flags that switch it, from a herc-client checkout.

Every change herc-client makes to the 2022 client is tagged HERC PATCH (<name>), in the lifted
assembly (text/*.inc) and in the C sources (src/*.c, src/*.h). A patch inside a feature flag's
conditional (fasm's `if HERC_FEATURE_X` in the assembly, `#if HERC_FEATURE_X` in C) belongs to
that flag, as does a tag in a C file's header comment when the file's conditionals name flags, and
a patch named after a flag. A patch with none of these is a fix that ships unconditionally, which
the flag inventory alone does not see. patches(herc) returns {name: {'flags': set of short flag
names, 'what': the tag's first description, 'where': the first file:line}}.
"""
import glob
import os
import re

TAG = re.compile(r'HERC PATCH \(([a-z0-9_]+)\)')
FLAG = re.compile(r'HERC_FEATURE_([A-Z0-9_]+)')
ASM_IF = re.compile(r'^\s*if\s+(.*)$')
ASM_ELSE = re.compile(r'^\s*else(\s+if\s+(.*))?\s*$')
ASM_END = re.compile(r'^\s*end\s+if\b')
C_IF = re.compile(r'^\s*#\s*(if|ifdef|ifndef)\b(.*)$')
C_ELIF = re.compile(r'^\s*#\s*elif\b(.*)$')
C_ELSE = re.compile(r'^\s*#\s*else\b')
C_END = re.compile(r'^\s*#\s*endif\b')


def _describe(line, name):
    """The words after the tag on its line, as far as 160 characters."""
    rest = line.split(f'HERC PATCH ({name})', 1)[1]
    rest = re.sub(r"^[\s:)]*(original '.*?'(\s*\([^)]*\))?\s*)?-?\s*", '', rest)
    return rest.strip(' */;-\n')[:160]


def _conditional(line, asm):
    """('open', flags), ('else', flags), ('close', None) or None for a line of a conditional."""
    if asm:
        if ASM_END.match(line):
            return 'close', None
        m = ASM_ELSE.match(line)
        if m:
            return 'else', set(FLAG.findall(m.group(2) or ''))
        m = ASM_IF.match(line)
        if m and not line.lstrip().startswith(';'):
            return 'open', set(FLAG.findall(m.group(1)))
        return None
    m = C_IF.match(line)
    if m:
        return 'open', set(FLAG.findall(m.group(2)))
    m = C_ELIF.match(line)
    if m:
        return 'else', set(FLAG.findall(m.group(1)))
    if C_ELSE.match(line):
        return 'else', set()
    if C_END.match(line):
        return 'close', None
    return None


def _scan(path, asm, found):
    with open(path, encoding='utf-8', errors='replace') as f:
        lines = f.readlines()
    file_flags = set() if asm else set(FLAG.findall(''.join(l for l in lines if C_IF.match(l) or C_ELIF.match(l))))
    header = not asm                     # still in a C file's opening comment
    stack = []                           # the flags each open conditional names
    for number, line in enumerate(lines, 1):
        if header and (C_IF.match(line) or line.lstrip().startswith('#include')):
            header = False
        cond = _conditional(line, asm)
        if cond:
            kind, flags = cond
            if kind == 'open':
                stack.append(flags)
            elif kind == 'else' and stack:
                stack[-1] = stack[-1] | flags
            elif kind == 'close' and stack:
                stack.pop()
            continue
        for name in TAG.findall(line):
            flags = set().union(*stack) if stack else set()
            if not flags and header:
                flags = set(file_flags)
            entry = found.setdefault(name, {'flags': set(), 'what': '',
                                            'where': f'{os.path.basename(path)}:{number}'})
            entry['flags'] |= flags
            if not entry['what']:
                entry['what'] = _describe(line, name)


def patches(herc):
    found = {}
    for path in sorted(glob.glob(os.path.join(herc, 'text', '*.inc'))):
        _scan(path, True, found)
    for path in sorted(glob.glob(os.path.join(herc, 'src', '*.c')) + glob.glob(os.path.join(herc, 'src', '*.h'))):
        _scan(path, False, found)
    with open(os.path.join(herc, 'features.conf'), encoding='utf-8', errors='replace') as f:
        known = set(re.findall(r'^HERC_FEATURE_([A-Z0-9_]+)=', f.read(), re.M))
    for name, entry in found.items():
        if name.upper() in known:
            entry['flags'].add(name.upper())
    return found


def flagless(herc):
    """The patches under no feature flag: fixes that always ship."""
    return {name: e for name, e in patches(herc).items() if not e['flags']}


if __name__ == '__main__':
    import sys
    sys.stdout.reconfigure(encoding='utf-8')
    herc = sys.argv[1] if len(sys.argv) > 1 else r'C:\parity\reference'
    every = patches(herc)
    free = {n: e for n, e in every.items() if not e['flags']}
    print(f'{len(every)} patches, {len(free)} under no feature flag')
    for name in sorted(free):
        print(f'{name}\t{free[name]["where"]}\t{free[name]["what"]}')
