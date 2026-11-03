import unittest

from helpers import K, U
from utf8data import cp949, lualex

SOURCE = (b'-- comment ' + K('주석') + b'\r\n'
          b'tbl = {\r\n'
          b'\t[501] = {\r\n'
          b'\t\tname = "' + K('빨간포션') + b'",\r\n'
          b'\t\tpath = "' + K('여') + b'\\\\' + K('활용병') + b'",\r\n'
          b'\t\tesc = "\\191\\169\\n",\r\n'
          b'\t\tlong = [==[\r\n' + K('설명') + b']==],\r\n'
          b'\t},\r\n'
          b'}\r\n'
          b'JobNameTable = { [jobtbl.JT_PORING] = "a", [jobtbl.JT_X] = "b" }\r\n'
          b'StateIconList[EFST_IDs.EFST_HIDING] = { descript = { { "c" } } }\r\n')


class LexerTest(unittest.TestCase):
    def test_tokens_cover_the_source(self):
        self.assertEqual(b''.join(t.text for t in lualex.tokenize(SOURCE)), SOURCE)

    def test_string_values_follow_lua_51(self):
        values = [lualex.string_value(t) for t in lualex.literals(lualex.tokenize(SOURCE))]
        self.assertEqual(values[0], K('빨간포션'))
        self.assertEqual(values[1], K('여') + b'\\' + K('활용병'))
        self.assertEqual(values[2], b'\xbf\xa9\n')
        self.assertEqual(values[3], K('설명'))       # first newline of a long string is dropped

    def test_rewrite_keeps_escapes_and_layout(self):
        t = lualex.literals(lualex.tokenize(SOURCE))[1]
        new = lualex.rewrite_literal(t, cp949.to_utf8)
        self.assertEqual(new, b'"' + U('여') + b'\\\\' + U('활용병') + b'"')

    def test_rewrite_falls_back_for_numeric_escapes(self):
        t = lualex.Token('string', 0, b'"\\191\\169"')          # 여 written as escapes
        new = lualex.rewrite_literal(t, cp949.to_utf8)
        self.assertEqual(lualex.string_value(lualex.Token('string', 0, new)), U('여'))

    def test_rewrite_long_string(self):
        t = lualex.literals(lualex.tokenize(SOURCE))[3]
        new = lualex.rewrite_literal(t, cp949.to_utf8)
        self.assertTrue(new.startswith(b'[==[') and new.endswith(b']==]'))
        self.assertEqual(lualex.string_value(lualex.Token('longstring', 0, new)), U('설명'))

    def test_short_literal_round_trips(self):
        for value in (b'a"b\\c\nd\re\x00f', U('한글'), K('한글')):
            tok = lualex.Token('string', 0, lualex.short_literal(value))
            self.assertEqual(lualex.string_value(tok), value)

    def test_anchors_name_the_table_key(self):
        tokens = lualex.tokenize(SOURCE)
        anchors = lualex.anchors(tokens)
        by_value = {lualex.string_value(tokens[i]): a for i, a in anchors.items()}
        self.assertEqual(by_value[b'a'], b'jobtbl.JT_PORING')
        self.assertEqual(by_value[b'b'], b'jobtbl.JT_X')
        self.assertEqual(by_value[b'c'], b'EFST_IDs.EFST_HIDING')
        self.assertEqual(by_value[K('빨간포션')], b'501')


if __name__ == '__main__':
    unittest.main()
