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


if __name__ == '__main__':
    unittest.main()
