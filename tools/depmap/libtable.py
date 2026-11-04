"""Read tools/libraries.txt, the client's library table.

Shared by check_includes.py, check_links.py and the dependency map. The format is described at
the top of tools/libraries.txt.
"""
import fnmatch
import re
import sys
from pathlib import Path

LIBRARY_FIELDS = ('uses', 'paths', 'note')
EXCEPTION_FIELDS = ('why', 'remove', 'include', 'link')


class TableError(Exception):
    pass


class Library:
    def __init__(self, name, line):
        self.name = name
        self.line = line
        self.uses = []
        self.paths = []
        self.note = ''
        self.allowed = set()     # every library it may use: uses, and what those may use


class Exception_:
    def __init__(self, name, line):
        self.name = name
        self.line = line
        self.why = ''
        self.remove = ''
        self.includes = []       # (from file, header)
        self.links = []          # (from file, symbol)


class Table:
    def __init__(self, path):
        self.path = Path(path)
        self.libraries = {}
        self.order = []
        self.exceptions = []
        self._parse()
        self._resolve_uses()
        self.include_exceptions = {}
        self.link_exceptions = {}
        for e in self.exceptions:
            for key in e.includes:
                if key in self.include_exceptions:
                    raise TableError(f'{self.path}:{e.line}: include exception {key} listed twice')
                self.include_exceptions[key] = e
            for key in e.links:
                if key in self.link_exceptions:
                    raise TableError(f'{self.path}:{e.line}: link exception {key} listed twice')
                self.link_exceptions[key] = e

    # ---- parsing ----
    def _parse(self):
        cur = None          # Library or Exception_
        field = None
        for no, raw in enumerate(self.path.read_text(encoding='utf-8').splitlines(), 1):
            line = raw.split('#', 1)[0].rstrip()
            if not line:
                continue
            if line[0].isspace():
                if cur is None or field is None:
                    raise TableError(f'{self.path}:{no}: continuation line without a field')
                self._add(cur, field, line.split(), no, continuation=True)
                continue
            words = line.split()
            key, rest = words[0], words[1:]
            if key == 'library':
                if len(rest) != 1:
                    raise TableError(f'{self.path}:{no}: library takes one name')
                if rest[0] in self.libraries:
                    raise TableError(f'{self.path}:{no}: library {rest[0]} defined twice')
                cur = Library(rest[0], no)
                self.libraries[cur.name] = cur
                self.order.append(cur.name)
                field = None
            elif key == 'exception':
                if len(rest) != 1:
                    raise TableError(f'{self.path}:{no}: exception takes one name')
                cur = Exception_(rest[0], no)
                self.exceptions.append(cur)
                field = None
            else:
                fields = LIBRARY_FIELDS if isinstance(cur, Library) else EXCEPTION_FIELDS
                if cur is None or key not in fields:
                    raise TableError(f'{self.path}:{no}: unknown field {key!r}')
                field = key
                self._add(cur, field, rest, no)
        for e in self.exceptions:
            if not e.why or not e.remove:
                raise TableError(f'{self.path}:{e.line}: exception {e.name} needs a why and a remove')
            if not e.includes and not e.links:
                raise TableError(f'{self.path}:{e.line}: exception {e.name} lists no include or link')

    def _add(self, cur, field, words, no, continuation=False):
        if field == 'uses':
            cur.uses += words
        elif field == 'paths':
            for w in words:
                if w.startswith('/') or '\\' in w or '..' in w:
                    raise TableError(f'{self.path}:{no}: path {w!r} must be relative to src, with /')
                cur.paths.append(w)
        elif field in ('note', 'why', 'remove'):
            setattr(cur, field, (getattr(cur, field) + ' ' + ' '.join(words)).strip())
        elif field in ('include', 'link'):
            # include FROM HEADER...; link FROM SYMBOL...; a continuation line adds to the same FROM.
            if not continuation:
                if len(words) < 2:
                    raise TableError(f'{self.path}:{no}: {field} takes a source file and at least one '
                                     f'{"header" if field == "include" else "symbol"}')
                cur.last_from, words = words[0], words[1:]
            target = cur.includes if field == 'include' else cur.links
            for w in words:
                target.append((cur.last_from, w))

    def _resolve_uses(self):
        for lib in self.libraries.values():
            for u in lib.uses:
                if u != '*' and u not in self.libraries:
                    raise TableError(f'{self.path}:{lib.line}: {lib.name} uses unknown library {u}')
                if u == lib.name:
                    raise TableError(f'{self.path}:{lib.line}: {lib.name} uses itself')
        state = {}

        def visit(name, chain):
            if state.get(name) == 'done':
                return self.libraries[name].allowed
            if state.get(name) == 'active':
                raise TableError(f'{self.path}: libraries use each other: {" -> ".join(chain + [name])}')
            state[name] = 'active'
            lib = self.libraries[name]
            allowed = set()
            for u in lib.uses:
                if u == '*':
                    allowed |= set(self.libraries) - {name}
                    continue
                allowed.add(u)
                allowed |= visit(u, chain + [name])
            if name in allowed:
                raise TableError(f'{self.path}: libraries use each other through {name}')
            lib.allowed = allowed
            state[name] = 'done'
            return allowed

        for name in self.order:
            if '*' in self.libraries[name].uses:
                continue
            visit(name, [])
        for name in self.order:
            lib = self.libraries[name]
            if '*' in lib.uses:
                lib.allowed = set(self.libraries) - {name}
                if any(name in o.allowed for o in self.libraries.values() if o is not lib):
                    raise TableError(f'{self.path}: {name} uses every library, so no library may use it')

    # ---- paths ----
    @staticmethod
    def _rank(pattern, path):
        """How specifically `pattern` names `path` (higher wins), or None if it does not match."""
        if pattern.endswith('/'):
            if path.startswith(pattern):
                return (0, pattern.count('/'))
            return None
        if not re.search(r'[*?\[]', pattern):
            return (2, 0) if path == pattern else None
        # `*` and `?` stay inside one path component.
        if fnmatch.fnmatchcase(path, pattern) and path.count('/') == pattern.count('/'):
            return (1, len(pattern.replace('*', '').replace('?', '')))
        return None

    def library_of(self, path):
        """The library that owns a src-relative path; raises TableError if none or two do."""
        best, owners = None, set()
        for name in self.order:
            for p in self.libraries[name].paths:
                r = self._rank(p, path)
                if r is None:
                    continue
                if best is None or r > best:
                    best, owners = r, {name}
                elif r == best:
                    owners.add(name)
        if not owners:
            raise TableError(f'{path}: no library lists it')
        if len(owners) > 1:
            raise TableError(f'{path}: equally specific paths of {", ".join(sorted(owners))} name it')
        return owners.pop()

    def may_use(self, user, used):
        return user == used or used in self.libraries[user].allowed

    def assign(self, src_root):
        """{src-relative path: library} for every file under src; TableError lists the problems."""
        src = Path(src_root)
        result, problems, every = {}, [], []
        for f in sorted(src.rglob('*')):
            if not f.is_file():
                continue
            rel = f.relative_to(src).as_posix()
            every.append(rel)
            try:
                result[rel] = self.library_of(rel)
            except TableError as e:
                problems.append(str(e))
        unused = []
        for name in self.order:
            for p in self.libraries[name].paths:
                if not any(self._rank(p, rel) is not None and result.get(rel, name) == name
                           for rel in every):
                    unused.append(f'{name}: path {p} names no file the library owns')
        if problems or unused:
            raise TableError('\n'.join(problems + unused))
        return result


def load(path=None):
    if path is None:
        path = Path(__file__).resolve().parent.parent / 'libraries.txt'
    return Table(path)


if __name__ == '__main__':
    # libtable.py [table] [src]: print each library with its size and what it may use.
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    root = Path(__file__).resolve().parent.parent.parent
    t = load(sys.argv[1] if len(sys.argv) > 1 else None)
    owners = t.assign(sys.argv[2] if len(sys.argv) > 2 else root / 'src')
    src = Path(sys.argv[2]) if len(sys.argv) > 2 else root / 'src'
    for name in t.order:
        fs = [f for f, l in owners.items() if l == name]
        c = [f for f in fs if f.endswith('.c')]
        lines = sum((src / f).read_text(encoding='utf-8', errors='replace').count('\n') + 1 for f in c)
        lib = t.libraries[name]
        uses = 'everything' if '*' in lib.uses else (', '.join(lib.uses) or '-')
        print(f'{name:16s} {len(c):4d} .c {len(fs) - len(c):4d} other {lines:7d} lines  uses {uses}')
