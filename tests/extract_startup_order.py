"""Extract the manager startup and shutdown sequences for test_startup_order.

From src/core/startup.c: init_failed and app_init_managers; from src/core/app.c:
app_destroy_singletons. Each must be found exactly once. Writes startup_order.inc, which
includes the manager table's header first: the two functions run g_clientManagers, which
build_startup_order.bat compiles from src/core/client_managers.c and manager_table.c.
Usage: extract_startup_order.py ROOT OUT
"""
from pathlib import Path
import re
import sys

root, output = map(Path, sys.argv[1:])
src = root / 'src'


def read(name):
    return (src / name).read_bytes().decode('utf-8').replace('\r\n', '\n')


def function(text, signature):
    matches = list(re.finditer(r'^' + re.escape(signature) + r'\n\{\n.*?^\}\n', text, re.M | re.S))
    assert len(matches) == 1, signature
    return matches[0].group(0)


startup = read('core/startup.c')
parts = [
    '#include "core/client_managers.h"\n',
    '/* src/core/startup.c */\n' + function(startup, 'static struct InitResult *init_failed(struct InitResult *out, const char *what)'),
    function(startup, 'struct InitResult *app_init_managers(struct InitResult *out)'),
    '/* src/core/app.c */\n' + function(read('core/app.c'), 'void app_destroy_singletons(void)'),
]
output.mkdir(parents=True, exist_ok=True)
(output / 'startup_order.inc').write_text('\n'.join(parts), encoding='utf-8')
