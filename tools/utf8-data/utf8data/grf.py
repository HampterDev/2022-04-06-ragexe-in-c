"""Read-only access to GRF 0x200 archives (the format of data.grf and revel.grf).

Entry names are CP949 bytes. Lookups lowercase ASCII letters only, which is what
the client does (_strlwr in the C locale, src/fileio/grf_pak.c hash_set_string).
"""
import struct
import zlib

FLAG_FILE = 0x01
FLAG_MIXCRYPT = 0x02
FLAG_DES_HEADER = 0x04


def ascii_lower(name: bytes) -> bytes:
    return bytes(b + 32 if 0x41 <= b <= 0x5A else b for b in name)


class GrfError(Exception):
    pass


class Grf:
    def __init__(self, path):
        self.path = path
        self._fh = open(path, 'rb')
        header = self._fh.read(46)
        if header[:15] != b'Master of Magic':
            raise GrfError(f'{path}: not a GRF archive')
        table_offset, seed, count, version = struct.unpack('<IIII', header[30:46])
        if version != 0x200:
            raise GrfError(f'{path}: unsupported GRF version {version:#x}')
        self._fh.seek(46 + table_offset)
        packed, unpacked = struct.unpack('<II', self._fh.read(8))
        table = zlib.decompress(self._fh.read(packed))
        if len(table) != unpacked:
            raise GrfError(f'{path}: file table size mismatch')
        self.entries = {}
        pos = 0
        while pos < len(table):
            end = table.index(b'\0', pos)
            name = table[pos:end]
            pos = end + 1
            csize, asize, rsize, flags, offset = struct.unpack('<IIIBI', table[pos:pos + 17])
            pos += 17
            if flags & FLAG_FILE:
                self.entries[ascii_lower(name)] = (name, csize, asize, rsize, flags, offset)

    def close(self):
        self._fh.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    def find(self, name: bytes):
        """Return the stored (original-case) entry name, or None."""
        entry = self.entries.get(ascii_lower(name.replace(b'/', b'\\')))
        return entry[0] if entry else None

    def read(self, name: bytes) -> bytes:
        entry = self.entries.get(ascii_lower(name.replace(b'/', b'\\')))
        if entry is None:
            raise KeyError(name)
        _, csize, asize, rsize, flags, offset = entry
        if flags & (FLAG_MIXCRYPT | FLAG_DES_HEADER):
            raise GrfError(f'{name!r}: encrypted GRF entries are not supported')
        self._fh.seek(46 + offset)
        raw = self._fh.read(asize)
        data = zlib.decompress(raw[:csize]) if csize else b''
        if len(data) != rsize:
            raise GrfError(f'{name!r}: size mismatch after decompression')
        return data
