import unittest

import helpers  # noqa: F401  (sys.path)
from utf8data import rules

S = lambda t: ('s', t.encode('ascii'))   # noqa: E731
N = ('n', '1')
P = ('p', b'jobtbl.JT_PORING')


class ClassifyTest(unittest.TestCase):
    def setUp(self):
        self.item = rules.find(r'System\iteminfo_true.lub')
        self.quest = rules.find(r'System\OngoingQuestInfoList_True')
        self.pet = rules.find(r'Lua Files\DataInfo\PetInfo')
        self.icon = rules.find(r'Lua Files\StateIcon\StateIconInfo')

    def test_item_names_display_resource_names_kept(self):
        self.assertEqual(rules.classify(self.item, (S('tbl'), N, S('identifiedDisplayName'))), rules.DISPLAY)
        self.assertEqual(rules.classify(self.item, (S('tbl'), N, S('identifiedResourceName'))), rules.RESOURCE)
        self.assertEqual(rules.classify(self.item, (S('tbl'), N, S('identifiedDescriptionName'), N)),
                         rules.DISPLAY)

    def test_unknown_fields_are_kept(self):
        self.assertEqual(rules.classify(self.item, (S('tbl'), N, S('slotCount'))), rules.KEEP)
        self.assertEqual(rules.classify(self.quest, (S('QuestInfoList'), N, S('IconName'))), rules.KEEP)

    def test_numeric_and_standin_keys(self):
        self.assertEqual(rules.classify(self.quest, (S('QuestInfoList'), N, S('Description'), N)), rules.DISPLAY)
        self.assertEqual(rules.classify(self.quest, (S('QuestInfoList'), N, S('Description'), S('x'))), rules.KEEP)
        self.assertEqual(rules.classify(self.pet, (S('PetIllustNameTable'), P)), rules.RESOURCE)
        self.assertEqual(rules.classify(self.pet, (S('PetStringTable'), P)), rules.KEEP)

    def test_double_star_matches_any_depth(self):
        path = (S('StateIconList'), ('p', b'EFST_IDs.EFST_HIDING'), S('descript'), N, N)
        self.assertEqual(rules.classify(self.icon, path), rules.DISPLAY)
        self.assertEqual(rules.classify(self.icon, (S('StateIconList'),)), rules.DISPLAY)
        self.assertEqual(rules.classify(self.icon, (S('Other'), N)), rules.KEEP)

    def test_call_arguments(self):
        self.assertEqual(rules.classify(self.item, ('@call', 1, b'AddItem', 2)), rules.DISPLAY)
        self.assertEqual(rules.classify(self.item, ('@call', 1, b'AddItem', 3)), rules.RESOURCE)
        self.assertEqual(rules.classify(self.item, ('@call', 1, b'AddItem', 6)), rules.KEEP)
        self.assertEqual(rules.classify(self.item, ('@call', 1, b'Other', 2)), rules.KEEP)

    def test_every_spec_has_a_known_loader(self):
        loaders = {rules.CFILE_LUA, rules.DISK_LUA, rules.DISK_MAIN, rules.CFILE_TABLE}
        for spec in rules.SPECS:
            self.assertIn(spec.loader, loaders, spec.name)
            if spec.kind == 'table':
                self.assertIn(spec.rules, ([], rules.ALL_LINES), spec.name)


if __name__ == '__main__':
    unittest.main()
