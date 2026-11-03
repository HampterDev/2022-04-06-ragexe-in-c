import unittest

from helpers import K, U, compile_lua, needs_lua
from utf8data import cp949, dump, lualex, repair


def damaged(text):
    return cp949.damage(K(text))


class PickTest(unittest.TestCase):
    def test_unique_fit(self):
        self.assertEqual(repair.pick(damaged('워그'), {K('워그'): 'a', b'ACIDUS': 'b'}), (K('워그'), 'a'))

    def test_ambiguous_fit_is_refused(self):
        # 물리 and 마법 damage into the same four U+FFFD
        self.assertEqual(damaged('물리'), damaged('마법'))
        original, why = repair.pick(damaged('물리'), {K('물리'): 'a', K('마법'): 'b'})
        self.assertIsNone(original)
        self.assertIn('2 different', why)

    def test_damaged_candidates_never_count(self):
        original, _ = repair.pick(damaged('워그'), {damaged('워그'): 'already damaged copy'})
        self.assertIsNone(original)


class TableRepairTest(unittest.TestCase):
    def test_lines_restored_from_reference_and_converted(self):
        ref = b'Event#\r\n\xa2\xaec#\r\nB#\r\n'
        bad = b'Event Manager#\r\n' + cp949.damage(b'\xa2\xaec#\r') + b'\nB#\r\n'
        rep = repair.repair_table_lines('t', bad, ref, 'ref')
        self.assertEqual(rep.output, b'Event Manager#\r\n' + cp949.to_utf8(b'\xa2\xaec#\r') + b'\nB#\r\n')
        self.assertEqual(len(rep.restored), 1)
        self.assertEqual(rep.unrecovered, [])


@needs_lua
class LuaRepairTest(unittest.TestCase):
    GRF = (b'JobNameTable = {\n'
           b'  [jobtbl.JT_DECOY_A] = "' + K('미케닉_로봇_마법') + b'",\n'
           b'  [jobtbl.JT_DECOY_B] = "' + K('미케닉_로봇_물리') + b'",\n'
           b'  [jobtbl.JT_ACIDUS] = "' + K('워그') + b'",\n'
           b'  [jobtbl.JT_ACIDUS] = "ACIDUS",\n'
           b'}\n')

    def damaged_file(self):
        return (b'JobNameTable = {\n'
                b'  [jobtbl.JT_DECOY_A] = "' + damaged('미케닉_로봇_마법') + b'",\n'
                b'  [jobtbl.JT_DECOY_B] = "' + damaged('미케닉_로봇_물리') + b'",\n'
                b'  [jobtbl.JT_ACIDUS] = "' + damaged('워그') + b'",\n'
                b'  [jobtbl.JT_LOST] = "' + damaged('사라짐') + b'",\n'
                b'}\n')

    def test_overwritten_entries_are_found_in_bytecode(self):
        pool = repair.bytecode_assignments(compile_lua(self.GRF))
        self.assertEqual(pool['jobtbl.JT_ACIDUS'], {K('워그'), b'ACIDUS'})
        # a dump only sees the last value of a duplicate key
        dpool = repair.dump_candidates(dump.run([compile_lua(self.GRF)]))
        self.assertEqual(dpool['jobtbl.JT_ACIDUS'], {b'ACIDUS'})

    def test_key_decides_between_equal_damage(self):
        pool = repair.merge((repair.bytecode_assignments(compile_lua(self.GRF)), 'grf'))
        rep = repair.repair_lua_source('jobName.lua', self.damaged_file(), pool, display=False)
        values = [lualex.string_value(t) for t in lualex.literals(lualex.tokenize(rep.output))]
        self.assertEqual(values, [K('미케닉_로봇_마법'), K('미케닉_로봇_물리'), K('워그'), damaged('사라짐')])
        self.assertEqual(len(rep.by_key), 2)
        self.assertEqual([w for w, _, _ in rep.unrecovered], ['line 5 [jobtbl.JT_LOST]'])

    def test_display_repairs_are_written_as_utf8(self):
        pool = repair.merge((repair.bytecode_assignments(compile_lua(self.GRF)), 'grf'))
        rep = repair.repair_lua_source('x', self.damaged_file(), pool, display=True)
        self.assertIn(b'"' + U('워그') + b'"', rep.output)


if __name__ == '__main__':
    unittest.main()
