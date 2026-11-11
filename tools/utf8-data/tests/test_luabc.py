import os
import unittest

from helpers import K, U, compile_lua, needs_lua
from utf8data import cp949, dump, luabc

SOURCE = (b'QuestInfoList = {\n'
          b'  [1000] = { Title = "' + K('공갈') + b'", IconName = "ico_nq.bmp", Exp = 1.5,\n'
          b'             Description = { "' + K('거북이') + b'", "' + K('등딱지') + b'" } },\n'
          b'}\n'
          b'function f(x) if x then return "' + K('함수') + b'" end return nil end\n')


@needs_lua
class BytecodeTest(unittest.TestCase):
    def setUp(self):
        self.data = compile_lua(SOURCE)

    def test_read_write_is_byte_identical(self):
        self.assertEqual(luabc.write(luabc.read(self.data)), self.data)

    def test_finds_constants_in_nested_functions(self):
        values = [c.value for _, _, c in luabc.string_constants(luabc.read(self.data))]
        self.assertIn(K('공갈'), values)
        self.assertIn(K('함수'), values)

    def test_rewriting_a_constant_changes_only_that_constant(self):
        chunk = luabc.read(self.data)
        for _, _, c in luabc.string_constants(chunk):
            if c.value == K('공갈'):
                luabc.set_string(chunk, c, cp949.to_utf8(c.value))
        out = luabc.write(chunk)
        self.assertNotEqual(out, self.data)
        self.assertEqual(len(out) - len(self.data), len(U('공갈')) - len(K('공갈')))
        self.assertEqual(luabc.structure_without_strings(luabc.read(out)),
                         luabc.structure_without_strings(luabc.read(self.data)))
        rows = dict(dump.run([out]))
        title = (('s', b'QuestInfoList'), ('n', '1000'), ('s', b'Title'))
        self.assertEqual(rows[title], ('s', U('공갈')))
        desc = (('s', b'QuestInfoList'), ('n', '1000'), ('s', b'Description'), ('n', '1'))
        self.assertEqual(rows[desc], ('s', K('거북이')))

    def test_string_with_nul_is_refused(self):
        chunk = luabc.read(self.data)
        _, _, c = next(luabc.string_constants(chunk))
        with self.assertRaises(luabc.BytecodeError):
            luabc.set_string(chunk, c, b'a\0b')

    def test_rejects_non_bytecode(self):
        with self.assertRaises(luabc.BytecodeError):
            luabc.read(b'tbl = {}')


# The game's .lub files were written by 32-bit Lua: 4-byte sizes. compile_lua makes 8-byte ones,
# so this chunk of the same form (tests/fixtures/lua51_win32.lub, docs/portability.md) keeps the
# real format covered.
WIN32_CHUNK = os.path.join(dump.ROOT, 'tests', 'fixtures', 'lua51_win32.lub')


class Win32ChunkTest(unittest.TestCase):
    def setUp(self):
        with open(WIN32_CHUNK, 'rb') as f:
            self.data = f.read()

    def test_sizes_are_four_bytes(self):
        self.assertEqual(self.data[8], 4)

    def test_read_write_is_byte_identical(self):
        self.assertEqual(luabc.write(luabc.read(self.data)), self.data)

    def test_rewriting_a_constant_keeps_four_byte_lengths(self):
        chunk = luabc.read(self.data)
        _, _, c = next(luabc.string_constants(chunk))
        self.assertEqual(c.value, K('안녕') + b'\0suffix')
        value = U('안녕') + b' suffix'
        luabc.set_string(chunk, c, value)
        self.assertEqual(c.raw[:4], (len(value) + 1).to_bytes(4, 'little'))
        out = luabc.write(chunk)
        self.assertEqual(len(out) - len(self.data), len(value) - len(K('안녕') + b'\0suffix'))
        self.assertEqual(luabc.structure_without_strings(luabc.read(out)),
                         luabc.structure_without_strings(luabc.read(self.data)))
        self.assertEqual(luabc.write(luabc.read(out)), out)

    @needs_lua
    def test_luadump_loads_the_rewritten_chunk(self):
        chunk = luabc.read(self.data)
        _, _, c = next(luabc.string_constants(chunk))
        luabc.set_string(chunk, c, U('안녕') + b' suffix')
        self.assertEqual(dump.run([luabc.write(chunk)]), [])


if __name__ == '__main__':
    unittest.main()
