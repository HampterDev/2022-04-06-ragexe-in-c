"""Exercise the numeric reader and its actual startup call site in isolation."""
from pathlib import Path
import sys

source = Path(sys.argv[1]).read_text(encoding="utf-8")
prefix = source[:source.index("static struct TableReader item_table_reader;")]
start = source.index("unsigned char read_int_file(")
end = source.index("\n}\n", start) + 3
startup = source[source.index("    moveInfo.first = moveInfo.last = moveInfo.end = NULL;", end):]
assert startup.endswith("}\n") and startup.count("read_int_file(") == 1
out = prefix + """
void restriction_error_box(HWND, const char *, const char *, unsigned);
#include "platform/window.h"
#define MessageBoxA restriction_error_box
#define platform_message_box restriction_error_box
""" + source[start:end] + "void exercise_item_restrictions(void)\n{\n    struct Vec moveInfo;\n" + startup
Path(sys.argv[2]).write_text(out, encoding="utf-8")
