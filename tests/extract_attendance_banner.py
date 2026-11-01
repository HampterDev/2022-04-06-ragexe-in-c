from pathlib import Path
import re
import sys

source = Path(sys.argv[1]).read_text(encoding='utf-8')
pieces = ['#include "ui/quests/quests_std.h"\n#include "ui/controls/check_radio.h"\n#include "ui/controls/buttons.h"\n']
for name in ('BANNER_SWITCH_MS', 'BANNER_COUNT'):
    pieces.append(re.search(r'^#define ' + name + r'[^\n]*\n', source, re.M).group())
pieces.append(re.search(r'^struct UICheckAttendanceWnd_Banner \{.*?^\};\n', source, re.M|re.S).group())
names = ('ui_check_attendance_wnd_banner_send_msg', 'ui_check_attendance_wnd_banner_on_process')
for name in names:
    match = re.search(r'^(?:void|int) ' + name + r'\([^;]*?\)\n\{.*?^\}\n', source, re.M|re.S)
    assert match, name
    pieces.append(match.group())
Path(sys.argv[2]).write_text('\n'.join(pieces), encoding='utf-8')
