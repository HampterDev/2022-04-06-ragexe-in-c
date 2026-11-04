"""Extract the startup loads of the managers that are allocated on first use (EZ2STR effects, map
sky and, on main, reputation, item names, item reform and enchants) and the code that later uses
the first two; test_manager_alloc_failure links the four main-only managers' files whole.

The loads come from core/client_managers.c (the manager table's adapters) when it exists, else from
app_init_managers (core/startup.c): its statements that name the two instances or their load
helpers, and the helpers. Either way the result defines startup_loads(), which runs them in order.
From core/map_sky.c and effects/ez2str_effect_mgr.c come the instances and the functions their
users call. Usage: ROOT OUT"""
import re
import sys
from pathlib import Path

root, out = Path(sys.argv[1]) / 'src', Path(sys.argv[2])
out.mkdir(parents=True, exist_ok=True)


def read(name):
    return (root / name).read_bytes().decode('utf-8').replace('\r\n', '\n')


def function(text, name, required=True):
    match = (re.search(r'^(?:static )?[^\n(]*\b' + name + r'\([^)]*\)\n\{\n.*?^\}\n', text, re.M | re.S) or
             re.search(r'^(?:static )?[^\n(]*\b' + name + r'\([^)]*\) \{[^\n]*\}\n', text, re.M))
    if not match:
        if required:
            sys.exit(name + ' not found')
        return ''
    return match.group(0)


def line(text, pattern):
    match = re.search(pattern, text, re.M)
    if not match:
        sys.exit(pattern + ' not found')
    return match.group(0) + '\n'


parts = []
sky = read('core/map_sky.c')
parts.append(line(sky, r'^static struct MapSkyMgr \*g_mapSkyMgrInstance;'))
for name in ('map_sky_mgr_instance', 'map_sky_mgr_construct', 'map_sky_mgr_load', 'map_sky_mgr_apply'):
    parts.append(function(sky, name))
ez = read('effects/ez2str_effect_mgr.c')
parts.append(line(ez, r'^static struct Ez2StrEffectMgr \*g_ez2strMgr;'))
for name in ('name_less', 'find_effect_id', 'lower_case', 'ez2str_effect_mgr_instance',
             'ez2_str_effect_mgr_find_by_effect_id', 'ez2_str_effect_mgr_get_effect_ids'):
    parts.append(function(ez, name))
parts.append(function(ez, 'ez2_str_effect_mgr_get_effect_id', required=False))
parts.append('#define HAVE_GET_EFFECT_ID %d\n' % ('ez2_str_effect_mgr_get_effect_id(' in ez))

table = root / 'core' / 'client_managers.c'
if table.exists():
    text = read('core/client_managers.c')
    parts.append(function(text, 'load_ez2str_effects'))
    parts.append(function(text, 'load_map_sky'))
    parts.append('static int load_results[2];\n'
                 'static void startup_loads(void)\n{\n'
                 '    load_results[0] = load_ez2str_effects();\n'
                 '    load_results[1] = load_map_sky();\n}\n')
else:
    text = read('core/startup.c')
    helpers = ('load_ez2str_effects', 'load_map_sky', 'load_repute', 'load_item_db_names', 'load_item_reform',
               'load_enchant')
    instances = ('ez2str_effect_mgr_instance', 'map_sky_mgr_instance', 'repute_mgr_instance',
                 'item_db_to_id_instance', 'item_reform_mgr', 'enchant_mgr_instance')
    for name in helpers:
        parts.append(function(text, name, required=False))
    body = function(text, 'app_init_managers')
    pattern = '|'.join([re.escape(i) + r'\(\)' for i in instances] + [re.escape(h) + r'\(\);' for h in helpers])
    calls = [l for l in body.splitlines() if re.search(pattern, l)]
    if len(calls) != 6:
        sys.exit('expected six manager loads in app_init_managers, found %d' % len(calls))
    parts.append('static int load_results[2];\n'
                 'static void startup_loads(void)\n{\n' + '\n'.join(calls) + '\n}\n')
(out / 'manager_alloc_failure.inc').write_text('\n'.join(parts), encoding='utf-8')
