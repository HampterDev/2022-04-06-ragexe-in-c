import unittest

from helpers import K, U
from utf8data import cp949


class Cp949Test(unittest.TestCase):
    def test_hangul_converts_to_utf8(self):
        self.assertEqual(cp949.to_utf8(K('빨간포션')), U('빨간포션'))
        self.assertEqual(cp949.to_utf8(K('ATK +5%, 공격속도')), U('ATK +5%, 공격속도'))

    def test_ascii_is_unchanged(self):
        self.assertEqual(cp949.to_utf8(b'Red Potion#\r'), b'Red Potion#\r')

    def test_invalid_cp949_is_refused_not_guessed(self):
        with self.assertRaises(cp949.NotCp949):
            cp949.to_utf8(b'\xcd^')        # lead byte without a valid trail byte

    def test_short_cp949_words_can_be_valid_utf8(self):
        # Why the encoding is decided per file: 킴 is C5 B4, which is also U+0174.
        self.assertTrue(cp949.is_utf8(K('킴')))
        self.assertTrue(cp949.is_cp949(K('킴')))

    def test_damage_model(self):
        # An editor that opens CP949 as UTF-8 and saves turns bytes that are not part of
        # a valid UTF-8 sequence into U+FFFD; valid-looking pairs survive as odd letters.
        self.assertEqual(cp949.damage(K('워그')), U('�' * 4))
        self.assertEqual(cp949.damage(K('미케닉')), U('���ɴ�'))
        self.assertTrue(cp949.is_damaged(cp949.damage(K('워그'))))
        self.assertFalse(cp949.is_damaged(U('워그')))


if __name__ == '__main__':
    unittest.main()
