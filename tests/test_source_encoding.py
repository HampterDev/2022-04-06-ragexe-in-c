import importlib.util
from pathlib import Path
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

    def test_all_production_sources_can_be_transformed(self):
        for path in (ROOT / "src").rglob("*"):
            if path.suffix in (".c", ".h"):
                with self.subTest(path=path.relative_to(ROOT)):
                    encoding.encode_literals(path.read_text(encoding="utf-8-sig"))


if __name__ == "__main__":
    unittest.main()
