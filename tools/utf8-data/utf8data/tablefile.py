"""Convert '#'-separated text tables (msgStringTable.txt and friends).

table_reader.c reads a field up to '#', ends a line at '\r', and skips fields
that start with '//'.  Conversion is per field: each display field is
converted on its own, and a field that is not valid CP949 is kept and reported.
CP949 trail bytes never take the values of '#', '\r', '\n' or '/', so the field
structure is the same before and after.
"""
import collections
import re

from . import cp949, rules

_FIELD = re.compile(rb'([^#\r\n]*)([#\r\n]|$)')


class TableResult:
    def __init__(self):
        self.output = None
        self.counts = collections.Counter()
        self.invalid = []


def fields(data):
    """The fields table_reader.c would produce, plus line-mode fields, for checks."""
    out = []
    field_start = 0
    i = 0
    n = len(data)
    while i < n:
        c = data[i]
        if c == 0x0A:
            field_start = i + 1
        elif c == 0x0D:
            f = data[field_start:i]
            if f and not f.startswith(b'//'):
                out.append(('line', f))
            field_start = i + 1
        elif c == 0x23:
            if not data[field_start:field_start + 2] == b'//':
                out.append(('field', data[field_start:i]))
                field_start = i + 1
        i += 1
    return out


def convert(spec, data):
    res = TableResult()
    if spec.rules != rules.ALL_LINES:
        res.output = data
        res.counts['kept'] = sum(1 for _ in re.finditer(rb'[^\n]*[\x80-\xff][^\n]*', data))
        return res
    if cp949.is_utf8(data):
        # Decided per file, as in luafile: the whole table is UTF-8 already.  (A
        # single short CP949 field can be valid UTF-8 by accident.)
        res.output = data
        res.counts['already utf-8'] = sum(1 for line in data.split(b'\n') if cp949.has_high(line))
        return res
    out = []
    for line in data.split(b'\n'):
        if not cp949.has_high(line):
            out.append(line)
            continue
        if line.lstrip().startswith(b'//'):
            kind = 'comment'
        else:
            kind = 'display'
        parts = []
        for m in _FIELD.finditer(line):
            text, sep = m.group(1), m.group(2)
            if cp949.has_high(text):
                if cp949.is_cp949(text):
                    text = cp949.to_utf8(text)
                    res.counts[kind] += 1
                else:
                    res.invalid.append(text)
                    res.counts['invalid cp949'] += 1
            parts.append(text + sep)
            if not sep:
                break
        out.append(b''.join(parts))
    res.output = b'\n'.join(out)
    return res


def verify(spec, original, output):
    problems = []
    if output.startswith(cp949.BOM):
        problems.append('output starts with a UTF-8 BOM')
    a, b = fields(original), fields(output)
    if len(a) != len(b):
        return problems + ['field count changed: %d -> %d' % (len(a), len(b))]
    for i, ((ka, fa), (kb, fb)) in enumerate(zip(a, b)):
        if ka != kb:
            problems.append('field %d changed kind' % i)
            break
        if fa == fb:
            continue
        if spec.rules != rules.ALL_LINES or not cp949.is_cp949(fa) or cp949.to_utf8(fa) != fb:
            problems.append('field %d changed unexpectedly' % i)
            break
    return problems
