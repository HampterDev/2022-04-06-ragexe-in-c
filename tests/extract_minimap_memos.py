"""Test the production memo handlers unchanged, with only UI/stdio boundaries replaced."""
from pathlib import Path
import re
import sys

source = Path(sys.argv[1]).read_text(encoding="utf-8")
parts = ["/* Generated from minimap_wnd.c; do not edit. */"]
for macro in ("MEMO_MAX", "MEMO_EDIT_WND", "CMD_MEMO_OK",
              "MEMO_PATH_FORMAT", "MEMO_LINE_FORMAT"):
    parts.append(re.search(r"^#define " + macro + r" .+$", source, re.M).group())
parts.append("#define MARK_KINDS 3")
for struct in ("UIMiniMapWnd", "UIEditctrlWnd"):
    parts.append(re.search(r"^struct " + struct + r" \{.*?^\};", source, re.M | re.S).group())
parts.append("static int minimap_load_memos(struct UIMiniMapWnd *self);")
for function in ("memo_path", "memo_save_error", "memo_append_separator",
                 "minimap_save_memos", "memo_save", "ui_editctrl_wnd_send_msg"):
    match = re.search(r"^(?:static )?(?:void|int) " + function + r"\([^;{]*\{.*?^\}",
                      source, re.M | re.S)
    if match is None:
        if function in ("memo_save_error", "memo_append_separator"):
            continue
        raise ValueError(f"Missing production function: {function}")
    parts.append(match.group())
Path(sys.argv[2]).write_text("\n\n".join(parts) + "\n", encoding="utf-8")
