"""Byte-level helpers for CP949 -> UTF-8 conversion.

Python's 'cp949' codec is Microsoft's code page 949 (Unified Hangul Code), the
encoding of the Korean client data.  Conversion is strict: a string that is not
valid CP949 is never guessed at; the caller keeps it and reports it.
"""

BOM = b'\xef\xbb\xbf'
REPLACEMENT = '�'.encode('utf-8')


class NotCp949(ValueError):
    pass


def has_high(data: bytes) -> bool:
    return any(b >= 0x80 for b in data)


def is_utf8(data: bytes) -> bool:
    try:
        data.decode('utf-8')
        return True
    except UnicodeDecodeError:
        return False


def is_cp949(data: bytes) -> bool:
    try:
        data.decode('cp949')
        return True
    except UnicodeDecodeError:
        return False


def to_utf8(data: bytes) -> bytes:
    """Convert CP949 bytes to UTF-8. ASCII passes through unchanged."""
    if not has_high(data):
        return data
    try:
        return data.decode('cp949').encode('utf-8')
    except UnicodeDecodeError as e:
        raise NotCp949('not valid CP949 at byte %d: %r' % (e.start, data[max(0, e.start - 8):e.end + 8]))


def damage(data: bytes) -> bytes:
    """What an editor does when it opens CP949 bytes as UTF-8 and saves: every byte that
    is not part of a valid UTF-8 sequence becomes U+FFFD.  Used to prove that a
    recovered original really is the source of a damaged string."""
    return data.decode('utf-8', 'replace').encode('utf-8')


def is_damaged(data: bytes) -> bool:
    return REPLACEMENT in data
