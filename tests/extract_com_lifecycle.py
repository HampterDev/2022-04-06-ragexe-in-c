"""Exercise the exact production startup tail and final-window cleanup.

The harness substitutes external subsystems, starts just before COM initialization,
and preserves all control flow from there to WinMain's return. No game data,
windows, audio devices, sockets or external applications are opened.
"""
from pathlib import Path
import sys

winmain = Path(sys.argv[1]).read_text(encoding="utf-8")
app = Path(sys.argv[2]).read_text(encoding="utf-8")
start = winmain.index("    frame_timer_reset();")
end = winmain.rindex("}")
assert winmain.count("    frame_timer_reset();") == 1
assert not winmain[end + 1:].strip()
tail = winmain[start:end]
pump_start = app.index("void app_last_pump(void)\n{")
pump_end = app.index("\n}", pump_start) + 2
pump = app[pump_start:pump_end]
legacy = "g_openLauncherPageAtExit" in tail
prefix = """
static int run_startup(void)
{
    struct InitResult init = {0, -1};
    HINSTANCE instance = NULL;
    HRESULT com_result;
    UINT period = 0;
    int result;
"""
if legacy:
    prefix += """
    WIN32_FIND_DATAA found, replay_found, navi_found;
    HANDLE find, replay_find, navi_find;
"""
if "instance_mutex" in tail:
    prefix += """
    HANDLE instance_mutex;
"""
Path(sys.argv[3]).write_text(
    "#define TEST_LEGACY_EXIT " + str(int(legacy)) + "\n" + pump + "\n"
    + prefix + tail + "}\n", encoding="utf-8")
