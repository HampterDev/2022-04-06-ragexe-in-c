"""Convert the display strings of one Lua data file (source or 5.1 bytecode).

Classification is done on the loaded data: the file is run in luadump, every
string it produces is classified by its field path (rules.classify), and a
string *value* is converted only if every place it appears is DISPLAY.  The
file is then rewritten at the token (source) or constant (bytecode) level,
so nothing but those string bytes changes.
"""
import collections

from . import cp949, dump, luabc, lualex, rules


class LuaResult:
    def __init__(self):
        self.output = None
        self.kind = None                      # 'bytecode' or 'source'
        self.counts = collections.Counter()   # display/resource/keep/conflict/... per string value
        self.converted_slots = 0              # constants or literals rewritten
        self.kept = collections.Counter()     # value -> class, for non-display non-ASCII strings
        self.paths = {}                       # value -> one example field path
        self.conflicts = []                   # values used as display and as something else
        self.unreferenced = []                # non-ASCII strings no dump path reaches
        self.invalid = []                     # display strings that are not valid CP949
        self.already_utf8 = []                # display strings that are already UTF-8
        self.convert = set()                  # the converted values
        self.comments_converted = 0
        self.fully_utf8 = False
        self.file_utf8 = False                # every non-ASCII string was UTF-8 already


def path_text(path):
    if path and path[0] == '@call':
        return '%s(arg %d)' % (path[2].decode('cp949', 'replace'), path[3])
    return ' / '.join(dump.key_text(k) if k[0] != 'n' else k[1] for k in path)


def classify_values(spec, rows):
    """value -> set of classes, plus one example path per value."""
    classes = collections.defaultdict(set)
    example = {}
    for path, val in rows:
        if path and path[0] != '@call':
            for k in path:
                if k[0] == 's' and cp949.has_high(k[1]):
                    classes[k[1]].add('key')
                    example.setdefault(k[1], path)
        if val[0] == 's' and cp949.has_high(val[1]):
            classes[val[1]].add(rules.classify(spec, path))
            example.setdefault(val[1], path)
    return classes, example


def _chunk_strings(data):
    if data[:4] == luabc.SIGNATURE:
        return [c.value for _, _, c in luabc.string_constants(luabc.read(data))]
    return [lualex.string_value(t) for t in lualex.literals(lualex.tokenize(data))]


def convert(spec, data, dump_rows=None):
    res = LuaResult()
    rows = dump_rows if dump_rows is not None else dump.run([data], call_main=spec.main)
    classes, example = classify_values(spec, rows)
    strings = [v for v in set(_chunk_strings(data)) if v and cp949.has_high(v)]
    # The encoding is decided per file: short CP949 words are often valid UTF-8 by
    # accident (킴 = C5 B4 = U+0174), so a single string cannot tell.
    res.file_utf8 = bool(strings) and all(cp949.is_utf8(v) for v in strings)
    for value, cls in classes.items():
        res.paths[value] = path_text(example[value])
        if cls == {rules.DISPLAY}:
            if res.file_utf8:
                res.already_utf8.append(value)
                res.counts['already utf-8'] += 1
            elif not cp949.is_cp949(value):
                res.invalid.append(value)
                res.counts['invalid cp949'] += 1
            else:
                res.convert.add(value)
                res.counts['display'] += 1
        elif rules.DISPLAY in cls:
            res.conflicts.append((value, sorted(cls)))
            res.counts['conflict'] += 1
        else:
            c = 'key' if 'key' in cls else (rules.RESOURCE if rules.RESOURCE in cls else rules.KEEP)
            res.kept[value] = c
            res.counts[c] += 1
    for value in strings:
        if value not in classes:
            res.unreferenced.append(value)
            res.counts['unreferenced'] += 1

    if data[:4] == luabc.SIGNATURE:
        res.kind = 'bytecode'
        chunk = luabc.read(data)
        for _, _, c in luabc.string_constants(chunk):
            if c.value in res.convert:
                luabc.set_string(chunk, c, cp949.to_utf8(c.value))
                res.converted_slots += 1
        res.output = luabc.write(chunk)
        res.fully_utf8 = all(cp949.is_utf8(c.value) for _, _, c in luabc.string_constants(chunk))
    else:
        res.kind = 'source'
        tokens = lualex.tokenize(data)
        out = []
        for t in tokens:
            if t.kind in ('string', 'longstring') and lualex.string_value(t) in res.convert:
                out.append(lualex.rewrite_literal(t, cp949.to_utf8))
                res.converted_slots += 1
            else:
                out.append(t.text)
        remaining = [t for t, o in zip(tokens, out) if t.kind in ('string', 'longstring')
                     and not cp949.is_utf8(lualex.string_value(lualex.Token(t.kind, 0, o)))]
        if not remaining and res.converted_slots:
            # Nothing CP949 is left in the strings: convert comments too, so the
            # output is one clean UTF-8 file for editors.
            for i, t in enumerate(tokens):
                if t.kind == 'comment' and cp949.has_high(t.text) and not cp949.is_utf8(t.text) \
                        and cp949.is_cp949(t.text):
                    out[i] = cp949.to_utf8(t.text)
                    res.comments_converted += 1
        res.output = b''.join(out)
        res.fully_utf8 = cp949.is_utf8(res.output)
    return res


def expected_rows(rows, convert_set):
    out = []
    for path, val in rows:
        if val[0] == 's' and val[1] in convert_set:
            val = ('s', cp949.to_utf8(val[1]))
        out.append((path, val))
    return out


def verify(spec, original, output, convert_set, original_rows=None):
    """Return a list of problems (empty when the output is equivalent)."""
    problems = []
    rows_o = original_rows if original_rows is not None else dump.run([original], call_main=spec.main)
    try:
        rows_n = dump.run([output], call_main=spec.main)
    except dump.DumpError as e:
        return ['output does not load: %s' % e]
    want = expected_rows(rows_o, convert_set)
    if rows_n != want:
        calls_n = sorted(r for r in rows_n if r[0][0] == '@call')
        calls_w = sorted(r for r in want if r[0][0] == '@call')
        vals_n = [r for r in rows_n if r[0][0] != '@call']
        vals_w = [r for r in want if r[0][0] != '@call']
        if vals_n != vals_w:
            bad = next((i for i, (a, b) in enumerate(zip(vals_n, vals_w)) if a != b), min(len(vals_n), len(vals_w)))
            problems.append('loaded values differ at row %d (%d vs %d rows)' % (bad, len(vals_n), len(vals_w)))
        if [(p[2], p[3], v) for p, v in calls_n] != [(p[2], p[3], v) for p, v in calls_w] and \
                sorted((p[2], p[3], v) for p, v in calls_n) != sorted((p[2], p[3], v) for p, v in calls_w):
            problems.append('recorded client calls differ')
    if original[:4] == luabc.SIGNATURE:
        a, b = luabc.read(original), luabc.read(output)
        if luabc.structure_without_strings(a) != luabc.structure_without_strings(b):
            problems.append('bytecode differs outside string constants')
        for (pa, ia, ca), (pb, ib, cb) in zip(luabc.string_constants(a), luabc.string_constants(b)):
            if cb.value != ca.value and not (ca.value in convert_set and cb.value == cp949.to_utf8(ca.value)):
                problems.append('constant %r[%d] changed unexpectedly' % (pa, ia))
                break
    return problems
