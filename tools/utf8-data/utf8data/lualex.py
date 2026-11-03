"""A byte-exact Lua 5.1 tokenizer, enough to find and rewrite string literals.

tokenize() returns tokens that cover the input completely (whitespace and
comments included), so ''.join(t.text) == source.  String literal values are
decoded with Lua 5.1's rules (llex.c read_string / read_long_string).
"""
import re


class LexError(Exception):
    pass


class Token:
    __slots__ = ('kind', 'start', 'text')

    def __init__(self, kind, start, text):
        self.kind = kind      # name number string longstring comment op ws
        self.start = start
        self.text = text

    def __repr__(self):
        return 'Token(%s, %d, %r)' % (self.kind, self.start, self.text[:30])


_WS = b' \t\r\n\v\f'
_NAME = re.compile(rb'[A-Za-z_][A-Za-z0-9_]*')
_NUMBER = re.compile(rb'0[xX][0-9A-Fa-f]+|(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?')
_OPS = (b'...', b'..', b'==', b'~=', b'<=', b'>=')


def _long_bracket(src, pos):
    """If src[pos:] opens a long bracket, return (level, content start)."""
    m = re.compile(rb'\[(=*)\[').match(src, pos)
    if not m:
        return None
    return len(m.group(1)), m.end()


def _long_end(src, start, level):
    close = b']' + b'=' * level + b']'
    end = src.find(close, start)
    if end < 0:
        raise LexError('unfinished long string/comment at %d' % start)
    return end + len(close)


def tokenize(src: bytes):
    tokens = []
    pos, n = 0, len(src)
    while pos < n:
        c = src[pos]
        start = pos
        if c in _WS:
            while pos < n and src[pos] in _WS:
                pos += 1
            kind = 'ws'
        elif src.startswith(b'--', pos):
            lb = _long_bracket(src, pos + 2)
            if lb:
                pos = _long_end(src, lb[1], lb[0])
            else:
                end = src.find(b'\n', pos)
                pos = n if end < 0 else end
            kind = 'comment'
        elif c in b'"\'':
            pos += 1
            while True:
                if pos >= n:
                    raise LexError('unfinished string at %d' % start)
                d = src[pos]
                if d == c:
                    pos += 1
                    break
                if d in b'\r\n':
                    raise LexError('unfinished string at %d' % start)
                if d == 0x5C:  # backslash: skip the escaped character (or line break pair)
                    pos += 1
                    if pos < n and src[pos] in b'\r\n':
                        pair = src[pos:pos + 2]
                        pos += 2 if pair in (b'\r\n', b'\n\r') else 1
                        continue
                pos += 1
            kind = 'string'
        elif c == 0x5B and _long_bracket(src, pos):
            lb = _long_bracket(src, pos)
            pos = _long_end(src, lb[1], lb[0])
            kind = 'longstring'
        else:
            m = _NAME.match(src, pos)
            if m:
                pos = m.end()
                kind = 'name'
            else:
                m = _NUMBER.match(src, pos)
                if m and (c in b'0123456789' or c == 0x2E and pos + 1 < n and src[pos + 1] in b'0123456789'):
                    pos = m.end()
                    kind = 'number'
                else:
                    for op in _OPS:
                        if src.startswith(op, pos):
                            pos += len(op)
                            break
                    else:
                        if c >= 0x80:
                            raise LexError('unexpected byte %#x at %d' % (c, pos))
                        pos += 1
                    kind = 'op'
        tokens.append(Token(kind, start, src[start:pos]))
    return tokens


_ESCAPES = {ord('a'): 7, ord('b'): 8, ord('f'): 12, ord('n'): 10, ord('r'): 13, ord('t'): 9, ord('v'): 11}


def string_value(token: Token) -> bytes:
    text = token.text
    if token.kind == 'longstring':
        level = text.index(b'[', 1) - 1
        body = text[level + 2:len(text) - level - 2]
        if body[:2] in (b'\r\n', b'\n\r'):
            body = body[2:]
        elif body[:1] in (b'\r', b'\n'):
            body = body[1:]
        return re.sub(rb'\r\n|\n\r|\r|\n', b'\n', body)
    if token.kind != 'string':
        raise ValueError('not a string token')
    body = text[1:-1]
    out = bytearray()
    i = 0
    while i < len(body):
        b = body[i]
        if b != 0x5C:
            out.append(b)
            i += 1
            continue
        i += 1
        e = body[i]
        if e in _ESCAPES:
            out.append(_ESCAPES[e])
            i += 1
        elif e in b'\r\n':
            out.append(10)
            i += 2 if body[i:i + 2] in (b'\r\n', b'\n\r') else 1
        elif 0x30 <= e <= 0x39:
            j = i
            while j < len(body) and j - i < 3 and 0x30 <= body[j] <= 0x39:
                j += 1
            v = int(body[i:j])
            if v > 255:
                raise LexError('escape sequence too large')
            out.append(v)
            i = j
        else:
            out.append(e)
            i += 1
    return bytes(out)


def short_literal(value: bytes, quote: bytes = b'"') -> bytes:
    """A short string literal whose value is exactly `value`; high bytes stay literal."""
    out = bytearray(quote)
    for b in value:
        if b == 0x5C:
            out += b'\\\\'
        elif b == quote[0]:
            out += b'\\' + quote
        elif b == 10:
            out += b'\\n'
        elif b == 13:
            out += b'\\r'
        elif b < 0x20 or b == 0x7F:
            out += b'\\%03d' % b
        else:
            out.append(b)
    out += quote
    return bytes(out)


def rewrite_literal(token: Token, convert) -> bytes:
    """Return new token text whose value is convert(old value).

    The raw text between the delimiters is converted in place first, which keeps
    escapes and formatting; if that does not give the right value (high bytes
    written as numeric escapes), a fresh short literal is produced."""
    old = string_value(token)
    new_value = convert(old)
    text = token.text
    if token.kind == 'string':
        candidate = text[:1] + convert(text[1:-1]) + text[-1:]
    else:
        level = text.index(b'[', 1) - 1
        head, tail = text[:level + 2], text[len(text) - level - 2:]
        candidate = head + convert(text[level + 2:len(text) - level - 2]) + tail
    try:
        if string_value(Token(token.kind, 0, candidate)) == new_value:
            return candidate
    except Exception:
        pass
    return short_literal(new_value, text[:1] if token.kind == 'string' else b'"')


def literals(tokens):
    return [t for t in tokens if t.kind in ('string', 'longstring')]


def anchors(tokens):
    """For each string token, the key it sits under: the index of a top-level
    statement `X[k] = ...` or the key of a field directly inside a top-level
    table constructor `X = { [k] = ..., k = ... }`.  Returns {token index: text}."""
    code = [(i, t) for i, t in enumerate(tokens) if t.kind not in ('ws', 'comment')]
    result = {}
    depth = 0
    stmt_anchor = None          # from X[k] = ...
    field_anchor = None         # current depth-1 field key
    j = 0
    while j < len(code):
        i, t = code[j]
        if depth == 0 and t.kind == 'name':
            # parse  Name {'.' Name | '[' expr ']'} '='
            k = j + 1
            index = None
            while k < len(code):
                tt = code[k][1]
                if tt.text == b'.' and k + 1 < len(code) and code[k + 1][1].kind == 'name':
                    k += 2
                elif tt.text == b'[':
                    level, m = 1, k + 1
                    while m < len(code) and level:
                        if code[m][1].text == b'[':
                            level += 1
                        elif code[m][1].text == b']':
                            level -= 1
                        m += 1
                    index = b''.join(x[1].text for x in code[k + 1:m - 1])
                    k = m
                else:
                    break
            if k < len(code) and code[k][1].text == b'=':
                stmt_anchor = index
                field_anchor = None
                j = k + 1
                continue
        if t.text == b'{':
            depth += 1
        elif t.text == b'}':
            depth -= 1
        elif depth == 1 and t.text == b'[':
            level, m = 1, j + 1
            while m < len(code) and level:
                if code[m][1].text == b'[':
                    level += 1
                elif code[m][1].text == b']':
                    level -= 1
                m += 1
            if m < len(code) and code[m][1].text == b'=':
                field_anchor = b''.join(x[1].text for x in code[j + 1:m - 1])
                j = m + 1
                continue
        elif depth == 1 and t.kind == 'name' and j + 1 < len(code) and code[j + 1][1].text == b'=':
            field_anchor = t.text
            j += 2
            continue
        elif depth == 1 and t.text == b',':
            field_anchor = None
        if t.kind in ('string', 'longstring'):
            a = stmt_anchor if stmt_anchor is not None else field_anchor
            if a is not None:
                result[i] = a
        j += 1
    return result
