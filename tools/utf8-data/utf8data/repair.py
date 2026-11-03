"""Restore strings that an editor destroyed by opening CP949 files as UTF-8.

Each damaged string holds U+FFFD where CP949 bytes were.  A recovered original
is accepted only if cp949.damage(original) reproduces the damaged bytes exactly
and it is the only candidate that does; candidates come from data.grf and
client-files git history, matched by line (tables) or by table key (Lua).
"""
import struct

from . import cp949, dump, luabc, lualex


class Repair:
    def __init__(self, name):
        self.name = name
        self.restored = []      # (where, original bytes, source)
        self.unrecovered = []   # (where, damaged bytes, reason)
        self.by_key = []        # (where, original, other originals with the same damage)
        self.output = None


# ----------------------------------------------------------------- candidates

def dump_candidates(rows):
    """anchor text -> set of string values found under that key (path[1])."""
    pool = {}
    for path, val in rows:
        if path[0] == '@call' or len(path) < 2 or val[0] != 's':
            continue
        k = path[1]
        anchor = k[1].decode('cp949', 'replace') if k[0] in ('s', 'p') else None
        if anchor is not None:
            pool.setdefault(anchor, set()).add(val[1])
    return pool


def bytecode_assignments(data):
    """anchor text -> set of string constants stored with SETTABLE under that key,
    including entries a later duplicate key overwrites (they never reach a dump)."""
    chunk = luabc.read(data)
    pool = {}
    for _, p in chunk.protos():
        k = p.constants
        n = struct.unpack('<I', p.code[:4])[0]
        code = struct.unpack('<%dI' % n, p.code[4:4 + 4 * n])
        reg = {}

        def rk(x):
            if x & 0x100:
                c = k[x & 0xFF]
                return ('k', c.value) if c.type == luabc.TSTRING else ('n',)
            return reg.get(x)

        def text(sym):
            if not sym:
                return None
            if sym[0] == 'k':
                return sym[1].decode('cp949', 'replace') if sym[1] is not None else None
            if sym[0] == 'g':
                return sym[1]
            if sym[0] == 'idx':
                base, key = text(sym[1]), text(sym[2])
                return base + '.' + key if base and key else None
            return None

        for ins in code:
            op = ins & 0x3F
            a = (ins >> 6) & 0xFF
            c = (ins >> 14) & 0x1FF
            b = (ins >> 23) & 0x1FF
            bx = ins >> 14
            if op == 1:      # LOADK
                kc = k[bx]
                reg[a] = ('k', kc.value) if kc.type == luabc.TSTRING else ('n',)
            elif op == 5:    # GETGLOBAL
                reg[a] = ('g', k[bx].value.decode('cp949', 'replace'))
            elif op == 6:    # GETTABLE
                reg[a] = ('idx', reg.get(b), rk(c))
            elif op == 0:    # MOVE
                reg[a] = reg.get(b)
            elif op == 9:    # SETTABLE
                key, val = text(rk(b)), rk(c)
                if key and val and val[0] == 'k' and val[1] is not None:
                    pool.setdefault(key, set()).add(val[1])
            else:
                reg.pop(a, None)
    return pool


def source_candidates(data):
    tokens = lualex.tokenize(data)
    anchors = lualex.anchors(tokens)
    pool = {}
    for i, t in enumerate(tokens):
        if i in anchors:
            pool.setdefault(anchors[i].decode('cp949', 'replace').replace(' ', ''), set()).add(
                lualex.string_value(t))
    return pool


def merge(*pools):
    out = {}
    for pool, label in pools:
        for anchor, values in pool.items():
            for v in values:
                out.setdefault(anchor, {}).setdefault(v, label)
    return out


def pick(damaged, candidates):
    """The unique original that damages into `damaged`, or (None, reason)."""
    hits = {v: src for v, src in candidates.items()
            if not cp949.is_damaged(v) and cp949.is_cp949(v) and cp949.damage(v) == damaged}
    if len(hits) == 1:
        (v, src), = hits.items()
        return v, src
    if not hits:
        return None, 'no candidate damages into this string'
    return None, '%d different candidates fit' % len(hits)


# ----------------------------------------------------------------- repairs

def repair_lua_source(name, damaged_file, pool, display):
    """Restore damaged literals of a Lua source file.  pool: anchor -> {value: source}.
    display=True writes restored strings as UTF-8, False keeps them as CP949 bytes."""
    rep = Repair(name)
    tokens = lualex.tokenize(damaged_file)
    anchors = lualex.anchors(tokens)
    everything = {v for values in pool.values() for v in values
                  if not cp949.is_damaged(v) and cp949.is_cp949(v)}
    out = []
    for i, t in enumerate(tokens):
        text = t.text
        if t.kind in ('string', 'longstring'):
            value = lualex.string_value(t)
            if cp949.is_damaged(value):
                line = damaged_file.count(b'\n', 0, t.start) + 1
                anchor = anchors.get(i)
                where = 'line %d [%s]' % (line, anchor.decode('ascii', 'replace') if anchor else '?')
                if anchor is None:
                    rep.unrecovered.append((where, value, 'no table key to match on'))
                else:
                    original, src = pick(value, pool.get(anchor.decode('cp949', 'replace').replace(' ', ''), {}))
                    if original is None:
                        rep.unrecovered.append((where, value, src))
                    else:
                        new_value = cp949.to_utf8(original) if display else original
                        text = lualex.short_literal(new_value, t.text[:1] if t.kind == 'string' else b'"')
                        rep.restored.append((where, original, src))
                        others = sorted(v for v in everything if v != original and cp949.damage(v) == value)
                        if others:
                            rep.by_key.append((where, original, others))
        out.append(text)
    rep.output = b''.join(out)
    return rep


def repair_table_lines(name, damaged_file, reference_file, reference_label):
    """Restore damaged lines of a '#' table from a reference copy, line by line; the
    result is converted to UTF-8 (tables are display text)."""
    rep = Repair(name)
    ref = reference_file.split(b'\n')
    out = []
    for i, line in enumerate(damaged_file.split(b'\n')):
        if not cp949.is_damaged(line):
            out.append(line)
            continue
        where = 'line %d' % (i + 1)
        cands = {}
        if i < len(ref):
            cands[ref[i]] = reference_label + ' line %d' % (i + 1)
        original, src = pick(line, cands)
        if original is None:
            # fall back to any reference line that fits uniquely
            original, src = pick(line, {r: reference_label + ' line %d' % (j + 1) for j, r in enumerate(ref)})
        if original is None:
            rep.unrecovered.append((where, line, src))
            out.append(line)
        else:
            rep.restored.append((where, original, src))
            out.append(cp949.to_utf8(original))
    rep.output = b'\n'.join(out)
    return rep
