"""External symbols of the client's compiled objects.

Shared by the dependency map (depmap.py) and the link check (check_links.py).

Inputs:
- MSVC: the output of `dumpbin /symbols` for one or more objects. `symbols.bat` writes one file
  for all objects of a build folder; each object starts with a "Dump of file NAME.obj" line.
- GCC/Clang: the output of `nm -P -A` for the objects (one "OBJECT: SYMBOL TYPE ..." line per
  symbol).

For every object, `defs` maps each external symbol it defines to 'func', 'data' or 'communal'
and `refs` is the set of external symbols it uses without defining them. Names are kept as the
compiler wrote them (`_name`, `@name@8`, `_name@4` on x86), so a reference matches its definition
exactly; `plain()` gives the C name for messages and tables.

Communal symbols: MSVC compiles a C global without an initializer (`int g_x;`) as a communal
symbol, which dumpbin lists as UNDEF with its size as the value; the linker allocates it. It has
no defining section, so the dependency map has never drawn an edge for it. Here every object that
lists it with a size counts as defining it ('communal'), and an object that lists it with value 0
references it. GCC's `nm` shows such globals as B (or C with -fcommon).
"""
import re
from pathlib import Path

_DUMPBIN_FILE = re.compile(r'^Dump of file (.+)$')
# 01E 00000000 SECT4  notype ()    External     | _able_to_make_effect_send_msg
_DUMPBIN_SYM = re.compile(
    r'^[0-9A-F]{3,8} ([0-9A-F]{8}) (SECT[0-9A-F]+|UNDEF)\s+\w+\s*(\(\))?\s+External\s+\|\s+(\S+)')


class ObjectSymbols:
    __slots__ = ('name', 'defs', 'refs')

    def __init__(self, name):
        self.name = name
        self.defs = {}
        self.refs = set()


def plain(sym):
    """The C name of a decorated symbol: `_f` and `_f@8` (x86 cdecl, stdcall), `@f@8` (fastcall)."""
    if sym.startswith('__imp_'):
        sym = sym[6:]
    if sym.startswith('@'):
        sym = sym[1:]
    elif sym.startswith('_'):
        sym = sym[1:]
    return sym.split('@')[0]


def _finish(obj):
    # A symbol an object both lists as communal and references is its own.
    obj.refs -= obj.defs.keys()


def read_dumpbin(path, objects=None):
    """Parse `dumpbin /symbols` output. Returns {object file name: ObjectSymbols}."""
    objects = {} if objects is None else objects
    cur = None
    if Path(path).suffix.lower() == '.sym':
        # One file per object (the map's former layout): the object is named after the file.
        cur = objects.setdefault(Path(path).stem + '.obj', ObjectSymbols(Path(path).stem + '.obj'))
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            m = _DUMPBIN_FILE.match(line)
            if m:
                if cur is not None:
                    _finish(cur)
                name = Path(m.group(1).strip()).name
                cur = objects.setdefault(name, ObjectSymbols(name))
                continue
            if cur is None or not line[:1].isalnum():
                continue
            m = _DUMPBIN_SYM.match(line)
            if not m:
                continue
            value, sect, func, sym = m.groups()
            if sym.startswith('__imp_'):
                continue    # a DLL import: never a client definition
            if sect == 'UNDEF':
                if int(value, 16):
                    cur.defs[sym] = 'communal'
                else:
                    cur.refs.add(sym)
            else:
                cur.defs.setdefault(sym, 'func' if func else 'data')
    if cur is not None:
        _finish(cur)
    return objects


_NM_DEF = set('TDBRGSVWCuiI')


def read_nm(path, objects=None):
    """Parse `nm -P -A` output ("OBJECT: SYMBOL TYPE [VALUE [SIZE]]" or "ARCHIVE[MEMBER]: ...")."""
    objects = {} if objects is None else objects
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            head, sep, rest = line.rpartition(': ')
            if not sep:
                continue
            parts = rest.split()
            if len(parts) < 2:
                continue
            sym, kind = parts[0], parts[1]
            obj = objects.setdefault(head, ObjectSymbols(head))
            if kind == 'U':
                obj.refs.add(sym)
            elif kind == 'C':
                obj.defs[sym] = 'communal'
            elif kind in _NM_DEF:
                obj.defs.setdefault(sym, 'func' if kind in 'TW' else 'data')
    for obj in objects.values():
        _finish(obj)
    return objects


def read(path, objects=None):
    """Read a dumpbin or nm listing, whichever `path` holds."""
    with open(path, encoding='utf-8', errors='replace') as f:
        head = f.read(4096)
    if 'Dump of file' in head or 'COFF SYMBOL TABLE' in head:
        return read_dumpbin(path, objects)
    return read_nm(path, objects)


def source_of_object(name, sources_by_stem, sources_by_path):
    """The `src`-relative source of an object, or None for one that is not built from `src`.

    build.bat puts every object into one folder as `<stem>.obj` (basenames are unique under
    `src`); CMake keeps the tree (`.../src/foo/bar.c.o` or `.../src/foo/bar.c.obj`)."""
    n = name.replace('\\', '/')
    i = n.rfind('/src/')
    if i >= 0:
        rel = re.sub(r'\.(o|obj)$', '', n[i + 5:])
        return rel if rel in sources_by_path else None
    stem = re.sub(r'\.(o|obj)$', '', n.rsplit('/', 1)[-1])
    if stem.endswith('.c'):
        stem = stem[:-2]
    return sources_by_stem.get(stem)
