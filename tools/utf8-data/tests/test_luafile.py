import unittest

from helpers import K, U, compile_lua, needs_lua
from utf8data import cp949, dump, luafile, rules

SPEC = rules.Spec('test', rules.DISK_MAIN, 'lua', [
    (('tbl', '*', 'name'), rules.DISPLAY),
    (('tbl', '*', 'desc', '#'), rules.DISPLAY),
    (('tbl', '*', 'sprite'), rules.RESOURCE),
    (('@call', 'AddItem', 2), rules.DISPLAY),
    (('@call', 'AddItem', 3), rules.RESOURCE),
], main=True)

SOURCE = (b'tbl = {\n'
          b'  [501] = { name = "' + K('빨간 포션') + b'", sprite = "' + K('빨간포션') + b'",\n'
          b'            desc = { "' + K('체력을 회복한다.') + b'", "^ffffff_^000000" }, memo = "' + K('메모') + b'" },\n'
          b'  [502] = { name = "' + K('사과') + b'", sprite = "' + K('사과') + b'", desc = { "' + K('킴') + b'" } },\n'
          b'}\n'
          b'byname = { ["' + K('키') + b'"] = 1 }\n'
          b'function main()\n'
          b'  for id, d in pairs(tbl) do AddItem(id, d.name, d.sprite) end\n'
          b'  return true, "good"\n'
          b'end\n'
          b'-- ' + K('주석') + b'\n')


@needs_lua
class LuaFileTest(unittest.TestCase):
    def check(self, data):
        res = luafile.convert(SPEC, data)
        self.assertEqual(luafile.verify(SPEC, data, res.output, res.convert), [])
        self.assertFalse(res.output.startswith(cp949.BOM))
        rows = dict(dump.run([res.output], call_main=True))
        get = lambda *keys: rows[tuple(('n', k) if isinstance(k, int) and not isinstance(k, bool)  # noqa: E731
                                       else ('s', k.encode() if isinstance(k, str) else k) for k in keys)]
        n = lambda i: str(i)  # noqa: E731
        self.assertEqual(rows[(('s', b'tbl'), ('n', n(501)), ('s', b'name'))], ('s', U('빨간 포션')))
        self.assertEqual(rows[(('s', b'tbl'), ('n', n(501)), ('s', b'sprite'))], ('s', K('빨간포션')))
        self.assertEqual(rows[(('s', b'tbl'), ('n', n(501)), ('s', b'desc'), ('n', '1'))],
                         ('s', U('체력을 회복한다.')))
        # a field with no rule stays CP949
        self.assertEqual(rows[(('s', b'tbl'), ('n', n(501)), ('s', b'memo'))], ('s', K('메모')))
        # 사과 is a name and a sprite at once: it cannot be both, so it stays CP949
        self.assertEqual(rows[(('s', b'tbl'), ('n', n(502)), ('s', b'name'))], ('s', K('사과')))
        self.assertIn(K('사과'), [v for v, _ in res.conflicts])
        # 킴 is valid UTF-8 by accident but the file is CP949, so it is converted
        self.assertEqual(rows[(('s', b'tbl'), ('n', n(502)), ('s', b'desc'), ('n', '1'))], ('s', U('킴')))
        # a non-ASCII key is never touched
        self.assertIn((('s', b'byname'), ('s', K('키'))), rows)
        self.assertEqual(res.kept[K('키')], 'key')
        return res

    def test_source_file(self):
        res = self.check(SOURCE)
        self.assertEqual(res.kind, 'source')
        self.assertIn(K('주석'), res.output)     # comments stay: the file still holds CP949 strings
        self.assertEqual(res.output.count(b'\n'), SOURCE.count(b'\n'))

    def test_bytecode_file(self):
        data = compile_lua(SOURCE)
        res = self.check(data)
        self.assertEqual(res.kind, 'bytecode')

    def test_function_only_strings_are_unreferenced_and_kept(self):
        data = compile_lua(b'function f() return "' + K('몬스터') + b'\\\\" end\n')
        res = luafile.convert(SPEC, data)
        self.assertEqual(res.output, data)
        self.assertEqual(res.unreferenced, [K('몬스터') + b'\\'])

    def test_utf8_file_passes_through(self):
        data = b'tbl = { [1] = { name = "' + U('빨간 포션') + b'" } }\nfunction main() return true, "good" end\n'
        res = luafile.convert(SPEC, data)
        self.assertEqual(res.output, data)
        self.assertTrue(res.file_utf8)

    def test_comments_converted_when_nothing_cp949_is_left(self):
        data = b'-- ' + K('주석') + b'\ntbl = { [1] = { name = "' + K('사과') + b'" } }\n'
        res = luafile.convert(SPEC, data)
        self.assertTrue(cp949.is_utf8(res.output))
        self.assertIn(U('주석'), res.output)
        self.assertEqual(res.comments_converted, 1)


if __name__ == '__main__':
    unittest.main()
