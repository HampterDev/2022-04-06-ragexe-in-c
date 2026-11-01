#include "contents/character_settings_internal.h"
#include "platform/client_config.h"
#include "session/state.h"
#include "session/skill_levels.h"
#include "skills/skill_bundle.h"
#include "skills/skill_info.h"
#include <stdio.h>
#include <string.h>

static int read_skill_level(HKEY key, int skill_id)
{
    char value_name[100];
    DWORD type = REG_DWORD, size = 4;
    int level;

    memset(value_name, 0, sizeof value_name);
    sprintf(value_name, "%s%d", skill_get_id_name(skill_id), g_session1.shortcutCharSlot);
    if (RegQueryValueExA(key, value_name, NULL, &type, (BYTE *)&level, &size) == ERROR_SUCCESS
        && type == REG_DWORD && size == sizeof level && level >= 0 && level <= 65535
        && skill_info_is_level_used(skill_id))
        return level;
    return 0;
}

void character_settings_seed(void)
{
    char path[0x200];
    HKEY key;
    int id;

    strcpy(path, g_registryKeyPath);
    strcat(path, "\\SkillUseLevelInfo");
    if (RegOpenKeyExA(HKEY_CURRENT_USER, path, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        return;
    for (id = 1; id < 0x157c; id++)
        player_skill_bundle_set_chosen_level(&g_session1.skills, id, read_skill_level(key, id));
    for (id = 0x1f40; id < 0x1f7d; id++)
        session_set_homun_skill_use_level(&g_session1, id, read_skill_level(key, id));
    for (id = 0x2008; id < 0x210a; id++)
        session_set_merc_skill_use_level(&g_session1, id, read_skill_level(key, id));
    RegCloseKey(key);
}
