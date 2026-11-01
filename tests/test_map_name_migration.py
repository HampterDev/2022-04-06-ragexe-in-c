import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


TOOL = Path(__file__).resolve().parents[1] / "tools/migrate_map_names.py"
spec = importlib.util.spec_from_file_location("map_names", TOOL)
map_names = importlib.util.module_from_spec(spec)
spec.loader.exec_module(map_names)


class MigrationTests(unittest.TestCase):
    def test_pairs_comments_duplicates_and_empty_display(self):
        source = b"// comment#ignored#\r\na.rsw#old#\r\nb.rsw##a.rsw#new#\n"
        self.assertEqual(map_names.read_names(source), {b"a.rsw": b"new", b"b.rsw": b""})

    def test_byte_strings_do_not_change_encoding(self):
        self.assertEqual(map_names.lua_string(b' A"\\\xb0\xa112'), '" A\\034\\092\\176\\16112"')
        self.assertEqual(map_names.lua_string(bytes([0, 9, 10, 255])), '"\\000\\009\\010\\255"')

    def test_bad_input_fails_instead_of_silently_losing_names(self):
        for source in (b"", b"// only a comment", b"map#name", b"map#", b"#name#", b"map#name#tail",
                       b"map#nul\0#", b"\xef\xbb\xbfmap#name#"):
            with self.subTest(source=source), self.assertRaises(ValueError):
                map_names.read_names(source)

    def test_preserves_existing_lua_and_rejects_compiled_or_repeated_base(self):
        base = b'mapTbl = {}\r\nfunction main() return true, "\xb0\xa1" end\r\n'
        output = map_names.convert(b"map#name#", base)
        self.assertIn(base, output)
        self.assertIn(b'AddMapDisplayName(map, name)', output)
        for invalid in (b"\x1bLua123", output):
            with self.subTest(base=invalid), self.assertRaises(ValueError):
                map_names.convert(b"map#name#", invalid)

    def test_command_writes_new_file_and_preserves_inputs_and_existing_output(self):
        with tempfile.TemporaryDirectory() as folder:
            folder = Path(folder)
            source, output = folder / 'names.txt', folder / 'mapInfo_true.lub'
            original = b'map#Name#\n'
            source.write_bytes(original)
            command = [sys.executable, str(TOOL), str(source), '--output', str(output)]
            self.assertEqual(subprocess.run(command, capture_output=True).returncode, 0)
            first = output.read_bytes()
            self.assertNotEqual(subprocess.run(command, capture_output=True).returncode, 0)
            self.assertEqual(source.read_bytes(), original)
            self.assertEqual(output.read_bytes(), first)


if __name__ == '__main__':
    unittest.main()
