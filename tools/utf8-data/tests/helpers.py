import os
import subprocess
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))

from utf8data import dump  # noqa: E402

HAVE_LUADUMP = os.path.isfile(dump.LUADUMP)
needs_lua = unittest.skipUnless(HAVE_LUADUMP, 'build_luadump.bat has not been run')


def K(text):
    """CP949 bytes of a Python string."""
    return text.encode('cp949')


def U(text):
    return text.encode('utf-8')


def compile_lua(source: bytes) -> bytes:
    """Lua 5.1 bytecode of `source`, made by the vendored Lua (32-bit, like the .lub files)."""
    with tempfile.TemporaryDirectory() as tmp:
        src, out = os.path.join(tmp, 'in.lua'), os.path.join(tmp, 'out.lub')
        with open(src, 'wb') as f:
            f.write(source)
        subprocess.run([dump.LUADUMP, '--compile', src, out], check=True)
        with open(out, 'rb') as f:
            return f.read()
