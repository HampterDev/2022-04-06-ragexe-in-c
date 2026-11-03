import os
import tempfile
import unittest

from helpers import K, U
from utf8data import cp949, pipeline, rules, tablefile

MSG = rules.find('msgStringTable.txt')
ILLUST = rules.find('Num2CardIllustNameTable.txt')

TABLE = (b'// ' + K('주석') + b'#x#\r\n'
         b'Red Potion#\r\n'
         + K('빨간 포션') + b'#\r\n'
         b'4001#' + K('럭키') + b'#\r\n'
         b'\xcd^#\r\n'
         + K('여러') + b'\r\n' + K('줄') + b'#\r\n')


class TableTest(unittest.TestCase):
    def test_display_fields_convert_one_by_one(self):
        res = tablefile.convert(MSG, TABLE)
        lines = res.output.split(b'\n')
        self.assertEqual(lines[0], b'// ' + U('주석') + b'#x#\r')
        self.assertEqual(lines[1], b'Red Potion#\r')
        self.assertEqual(lines[2], U('빨간 포션') + b'#\r')
        self.assertEqual(lines[3], b'4001#' + U('럭키') + b'#\r')
        self.assertEqual(lines[4], b'\xcd^#\r')                     # not CP949: kept and reported
        self.assertEqual(res.invalid, [b'\xcd^'])
        self.assertEqual(lines[5], U('여러') + b'\r')
        self.assertEqual(tablefile.verify(MSG, TABLE, res.output), [])

    def test_field_structure_is_preserved(self):
        res = tablefile.convert(MSG, TABLE)
        self.assertEqual([k for k, _ in tablefile.fields(TABLE)], [k for k, _ in tablefile.fields(res.output)])

    def test_resource_tables_are_left_alone(self):
        data = b'4001#' + K('포링카드') + b'#\r\n'
        self.assertEqual(tablefile.convert(ILLUST, data).output, data)

    def test_utf8_table_passes_through(self):
        data = b'4001#' + U('럭키') + b'#\r\n'
        self.assertEqual(tablefile.convert(MSG, data).output, data)

    def test_verify_catches_changes(self):
        self.assertTrue(tablefile.verify(MSG, b'a#b#', b'a#'))
        self.assertTrue(tablefile.verify(MSG, b'a#b#', b'a#c#'))
        self.assertTrue(tablefile.verify(MSG, b'a#', cp949.BOM + b'a#'))

    def test_output_never_has_a_bom(self):
        with tempfile.TemporaryDirectory() as tmp:
            pipeline.write(os.path.join(tmp, 'a', 'ok.txt'), U('럭키'))
            with open(os.path.join(tmp, 'a', 'ok.txt'), 'rb') as f:
                self.assertFalse(f.read().startswith(cp949.BOM))
            with self.assertRaises(ValueError):
                pipeline.write(os.path.join(tmp, 'bom.txt'), cp949.BOM + U('럭키'))


if __name__ == '__main__':
    unittest.main()
