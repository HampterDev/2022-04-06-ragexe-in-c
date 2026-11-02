import importlib.util
import os
from pathlib import Path
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("source_encoding", ROOT / "tools/source_encoding.py")
encoding = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(encoding)


class SourceEncodingTests(unittest.TestCase):
    def test_legacy_bytes_and_adjacent_ascii(self):
        self.assertEqual(encoding.encode_literals('"안녕A7"'), r'"\276\310\263\347A7"')

    def test_comments_and_explicit_unicode_stay_utf8(self):
        source = '// "한글"\n/* "한글" */ L"한글" u"한글" U"한글" u8"한글"'
        self.assertEqual(encoding.encode_literals(source), source)

    def test_escaped_quotes_and_backslashes(self):
        source = r'"a\"한글\\"'
        self.assertEqual(encoding.encode_literals(source), r'"a\"\307\321\261\333\\"')

    def test_line_splicing_precedes_comments_and_literals(self):
        source = '// comment \\\n"한글"\n"안\\\n녕"'
        expected = '// comment \\\n"한글"\n"\\276\\310\\\n\\263\\347"'
        self.assertEqual(encoding.encode_literals(source), expected)

    def test_no_silent_replacement(self):
        with self.assertRaises(UnicodeEncodeError):
            encoding.encode_literals('"😀"')

    def test_tree_mirrors_sources_and_keeps_unchanged_outputs(self):
        with tempfile.TemporaryDirectory() as temp:
            source, output = Path(temp, "src"), Path(temp, "out")
            (source / "core").mkdir(parents=True)
            (source / "core" / "a.c").write_text('const char *s = "한글";\n', encoding="utf-8")
            (source / "b.h").write_text("int b;\n", encoding="utf-8")
            (source / "notes.txt").write_text("x\n", encoding="utf-8")
            (output / "gone").mkdir(parents=True)
            (output / "gone" / "old.c").write_text("int old;\n", encoding="utf-8")
            encoding.encode_tree(source, output)
            self.assertEqual((output / "core" / "a.c").read_text(encoding="utf-8"),
                             'const char *s = "\\307\\321\\261\\333";\n')
            self.assertEqual((output / "b.h").read_text(encoding="utf-8"), "int b;\n")
            self.assertFalse((output / "notes.txt").exists())
            self.assertFalse((output / "gone" / "old.c").exists())
            before = (output / "b.h").stat().st_mtime_ns
            os.utime(output / "b.h", ns=(before - 10**9, before - 10**9))
            encoding.encode_tree(source, output)
            self.assertEqual((output / "b.h").stat().st_mtime_ns, before - 10**9)

    def test_all_production_sources_can_be_transformed(self):
        for path in (ROOT / "src").rglob("*"):
            if path.suffix in encoding.SOURCE_SUFFIXES:
                with self.subTest(path=path.relative_to(ROOT)):
                    encoding.encode_literals(path.read_text(encoding="utf-8-sig"))


if __name__ == "__main__":
    unittest.main()
