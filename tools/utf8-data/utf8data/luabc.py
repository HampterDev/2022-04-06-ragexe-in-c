"""Lossless reader and writer for Lua 5.1 bytecode chunks (luac output).

Only string constants are meant to be edited.  Every other byte - header,
instructions, numbers, debug info, upvalue and local names, the source name -
is kept as the raw bytes it was read from, so write(read(x)) == x and a chunk
whose constants are unchanged is rewritten byte for byte.

Layout (lundump.c): header(12) then a function:
    source:string linedefined:int lastlinedefined:int
    nups:byte numparams:byte is_vararg:byte maxstacksize:byte
    code:int n, n*Instruction
    constants:int n, n*(type:byte value)   nil=0 bool=1(+byte) number=3(+lua_Number) string=4(+string)
    protos:int n, n*function
    lineinfo:int n, n*int   locvars:int n, n*(string int int)   upvalues:int n, n*string
A string is size_t length (including the trailing NUL; 0 means NULL) then bytes.
"""

SIGNATURE = b'\x1bLua'
TNIL, TBOOLEAN, TNUMBER, TSTRING = 0, 1, 3, 4


class BytecodeError(Exception):
    pass


class Constant:
    __slots__ = ('type', 'raw', 'value')

    def __init__(self, type_, raw, value=None):
        self.type = type_      # constant tag
        self.raw = raw         # the payload bytes exactly as read (after the tag)
        self.value = value     # bytes for strings (without NUL), None for NULL strings


class Proto:
    def __init__(self):
        self.head = b''        # source .. maxstacksize, raw
        self.code = b''        # count + instructions, raw
        self.constants = []
        self.protos = []
        self.tail = b''        # lineinfo, locvars, upvalues, raw

    def walk(self, path=()):
        yield path, self
        for i, p in enumerate(self.protos):
            yield from p.walk(path + (i,))


class Chunk:
    def __init__(self, header, main):
        self.header = header
        self.main = main

    def protos(self):
        return self.main.walk()


class _Reader:
    def __init__(self, data):
        self.data = data
        self.pos = 0
        if data[:4] != SIGNATURE:
            raise BytecodeError('not a Lua bytecode chunk')
        if data[4] != 0x51 or data[5] != 0:
            raise BytecodeError('not Lua 5.1 official-format bytecode')
        self.little = data[6] == 1
        self.sz_int, self.sz_size, self.sz_instr, self.sz_num = data[7], data[8], data[9], data[10]
        self.header = data[:12]
        self.pos = 12

    def take(self, n):
        if self.pos + n > len(self.data):
            raise BytecodeError('truncated chunk')
        b = self.data[self.pos:self.pos + n]
        self.pos += n
        return b

    def uint(self, size):
        return int.from_bytes(self.take(size), 'little' if self.little else 'big')

    def string_raw(self):
        start = self.pos
        n = self.uint(self.sz_size)
        self.take(n)
        return self.data[start:self.pos], n

    def function(self):
        p = Proto()
        start = self.pos
        self.string_raw()
        self.take(2 * self.sz_int + 4)
        p.head = self.data[start:self.pos]
        start = self.pos
        n = self.uint(self.sz_int)
        self.take(n * self.sz_instr)
        p.code = self.data[start:self.pos]
        n = self.uint(self.sz_int)
        for _ in range(n):
            tag = self.take(1)[0]
            if tag == TNIL:
                p.constants.append(Constant(tag, b''))
            elif tag == TBOOLEAN:
                p.constants.append(Constant(tag, self.take(1)))
            elif tag == TNUMBER:
                p.constants.append(Constant(tag, self.take(self.sz_num)))
            elif tag == TSTRING:
                raw, length = self.string_raw()
                if length == 0:
                    value = None
                else:
                    body = raw[self.sz_size:]
                    if body[-1:] != b'\0':
                        raise BytecodeError('string constant without trailing NUL')
                    value = body[:-1]
                p.constants.append(Constant(tag, raw, value))
            else:
                raise BytecodeError('bad constant type %d' % tag)
        n = self.uint(self.sz_int)
        for _ in range(n):
            p.protos.append(self.function())
        start = self.pos
        n = self.uint(self.sz_int)
        self.take(n * self.sz_int)
        n = self.uint(self.sz_int)
        for _ in range(n):
            self.string_raw()
            self.take(2 * self.sz_int)
        n = self.uint(self.sz_int)
        for _ in range(n):
            self.string_raw()
        p.tail = self.data[start:self.pos]
        return p


def read(data: bytes) -> Chunk:
    r = _Reader(data)
    main = r.function()
    if r.pos != len(data):
        raise BytecodeError('%d trailing bytes after the main function' % (len(data) - r.pos))
    chunk = Chunk(r.header, main)
    chunk._fmt = (r.little, r.sz_int, r.sz_size)
    return chunk


def _uint_bytes(value, size, little):
    if value >= 1 << (8 * size):
        raise BytecodeError('value %d does not fit in %d bytes' % (value, size))
    return value.to_bytes(size, 'little' if little else 'big')


def set_string(chunk: Chunk, constant: Constant, value: bytes):
    """Replace a string constant's value, re-encoding its length prefix."""
    if constant.type != TSTRING or constant.value is None:
        raise BytecodeError('not a string constant')
    if b'\0' in value:
        raise BytecodeError('string constant may not contain NUL')
    little, _, sz_size = chunk._fmt
    constant.value = value
    constant.raw = _uint_bytes(len(value) + 1, sz_size, little) + value + b'\0'


def _write_function(p, out, fmt):
    little, sz_int, _ = fmt
    out.append(p.head)
    out.append(p.code)
    out.append(_uint_bytes(len(p.constants), sz_int, little))
    for c in p.constants:
        out.append(bytes([c.type]))
        out.append(c.raw)
    out.append(_uint_bytes(len(p.protos), sz_int, little))
    for sub in p.protos:
        _write_function(sub, out, fmt)
    out.append(p.tail)


def write(chunk: Chunk) -> bytes:
    out = [chunk.header]
    _write_function(chunk.main, out, chunk._fmt)
    return b''.join(out)


def string_constants(chunk: Chunk):
    """Yield (proto path, index, Constant) for every non-NULL string constant."""
    for path, p in chunk.protos():
        for i, c in enumerate(p.constants):
            if c.type == TSTRING and c.value is not None:
                yield path, i, c


def structure_without_strings(chunk: Chunk):
    """Everything in the chunk except string constant values, for equivalence checks."""
    parts = [chunk.header]
    for path, p in chunk.protos():
        parts.append(repr(path).encode())
        parts.append(p.head)
        parts.append(p.code)
        parts.append(p.tail)
        for c in p.constants:
            parts.append(bytes([c.type]))
            parts.append(b'<str>' if c.type == TSTRING and c.value is not None else c.raw)
    return b'\x00|'.join(parts)
