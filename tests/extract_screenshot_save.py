"""Compile the screenshot handler from production source, without unrelated modes."""
from pathlib import Path
import re
import sys

source = Path(sys.argv[1]).read_text(encoding="utf-8")
parts = ["static int g_screenShotCounter = 0;\n"]
for name in ("file_name_sanitize", "watermark_code", "screenshot_save_error", "mode_screen_shot"):
    match = re.search(r"^(?:static )?void " + name + r"\([^;{]*\{.*?^\}", source, re.M | re.S)
    if match:
        parts.append(match.group() + "\n")
    elif name in ("file_name_sanitize", "mode_screen_shot"):
        raise SystemExit("Missing production function: " + name)
Path(sys.argv[2]).write_text("\n".join(parts), encoding="utf-8")
