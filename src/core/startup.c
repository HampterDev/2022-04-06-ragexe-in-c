#include "actor/map_type_table.h"
#include "contents/town_info.h"
#include "core/select_package.h"
#include "contents/champion_monster.h"
#include "contents/item_db_to_id.h"
#include "contents/item_reform.h"
#include "contents/enchant_mgr.h"
#include "core/map_sky.h"
#include "effects/effect_mgr.h"
#include "effects/ez2str_effect_mgr.h"
#include "session/option_pairs.h"
#include "effects/particle.h"
#include "contents/repute_api.h"
#include "contents/player_names.h"
#include "render/device.h"
#include "security/themida.h"
#include "contents/schedule.h"
#include "contents/whisper_block.h"
#include "web/work_manager.h"
#include "web/service_managers.h"
#include "web/http_post.h"
#include "resources/state_icon_sheet.h"
#include "ui/social/entry_queue_state.h"
#include "contents/quest.h"
#include "ui/quests/signboard_list.h"
#include "platform/client_options.h"
#include "platform/client_config.h"
#include "session/state.h"
#include "contents/connection_api.h"
#include <windows.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "std/std.h"
#include "render/render_api.h"
#include "core/app.h"
#include "platform/language.h"
#include "world/world_imports.h"
#include "contents/clan_info.h"
#include "contents/guild_mgr.h"
#include "session/session_state.h"
#include "ui/mail/rodex_box.h"
#include "ui/media/connect_sns.h"
#include "ui/misc/roulette_state.h"
#include "ui/net/packet_crypt.h"
#include "ui/social/party_board.h"
#include "ui/render/graphic_setting.h"
#include "contents/sound.h"

char g_deviceName[40] = { 0 };
uint32_t g_showTipsAtStartup = 1;
uint32_t g_soundMode;

const char *g_setupRegistryKeyPath = "Software\\Gravity Soft\\RenewSetup";
DWORD g_isVoodoo;
GUID g_deviceGuid;

#define SETTINGS_ROOT HKEY_CURRENT_USER

int read_registry(void)
{
    HKEY probe, key;
    DWORD type, size;
    LONG failed;

    failed = RegOpenKeyExA(SETTINGS_ROOT, g_setupRegistryKeyPath, 0, KEY_QUERY_VALUE, &probe);
    if (failed)
        return 0;
    RegCloseKey(probe);
    failed = RegOpenKeyExA(SETTINGS_ROOT, g_registryKeyPath, 0, KEY_QUERY_VALUE, &probe);
    if (failed)
        return 0;
    RegCloseKey(probe);

    if (RegOpenKeyExA(SETTINGS_ROOT, g_registryKeyPath, 0, KEY_QUERY_VALUE, &key) == 0) {
        type = 4;
        size = 4;
        RegQueryValueExA(key, "ISVOODOO", 0, &type, (BYTE *)&g_isVoodoo, &size);
        type = REG_DWORD;
        size = sizeof(DWORD);
        RegQueryValueExA(key, "SHOWTIPSATSTARTUP", 0, &type, (BYTE *)&g_showTipsAtStartup, &size);
        type = REG_DWORD;
        size = sizeof(DWORD);
        if (RegQueryValueExA(key, "TRILINEARFILTER", 0, &type, (BYTE *)&g_trilinearFilter, &size))
            g_trilinearFilter = 0;
        if (g_showTipsAtStartup != 1)
            g_showTipsAtStartup = g_showTipsAtStartup != 0;
        type = 1;
        size = 0x28;
        RegQueryValueExA(key, "DEVICENAME", 0, &type, (BYTE *)g_deviceName, &size);
        type = 3;
        size = 0x10;
        RegQueryValueExA(key, "GUIDDEVICE", 0, &type, (BYTE *)&g_deviceGuid, &size);
        type = 3;
        size = 0x10;
        RegQueryValueExA(key, "GUIDDRIVER", 0, &type, (BYTE *)(GUID *)&g_driverGuid, &size);
        RegCloseKey(key);
    }
    g_isVoodooInUse = stale_guid();

    if (RegOpenKeyExA(SETTINGS_ROOT, g_registryKeyPath, 0, KEY_QUERY_VALUE, &key) == 0) {
        type = 4;
        size = 4;
        RegQueryValueExA(key, "SOUNDMODE", 0, &type, (BYTE *)&g_soundMode, &size);
        type = REG_DWORD;
        size = sizeof(DWORD);
        RegQueryValueExA(key, "SPEAKERTYPE", 0, &type, (BYTE *)&g_sound_speaker_type, &size);
        type = REG_DWORD;
        size = sizeof(DWORD);
        RegQueryValueExA(key, "DIGITALRATETYPE", 0, &type, (BYTE *)&g_sound_rate_type, &size);
        type = REG_DWORD;
        size = sizeof(DWORD);
        RegQueryValueExA(key, "DIGITALBITSTYPE", 0, &type, (BYTE *)&g_sound_bits_type, &size);
        type = REG_DWORD;
        size = sizeof(DWORD);
        if (RegQueryValueExA(key, "NUMSAMPLETYPE", 0, &type, (BYTE *)&g_sound_sample_type, &size))
            g_sound_sample_type = 2;
        type = 1;
        size = 0x400;
        RegQueryValueExA(key, "PROVIDERNAME", 0, &type, (BYTE *)g_sound_provider_name, &size);
        RegCloseKey(key);
    }
    return 1;
}

static void upper_ascii(char *name)
{
    char *p;

    for (p = name; *p; p++)
        if ((islower)(*p))
            *p += 0xe0;
}

static void unknown_name(char name[0x104])
{
    memcpy(name, "unknown", 8);
    memset(name + 8, 0, 0xfc);
}

typedef HANDLE (WINAPI *SnapshotFn)(DWORD flags, DWORD pid);
typedef BOOL (WINAPI *ProcessWalkFn)(HANDLE snapshot, PROCESSENTRY32 *entry);

char create_depends_dll_msg(void)
{
    static const char *const exes[] = {
        "RAGEXE.EXE", "SAKEXE.EXE", "RAGEXERE.EXE", "RAGTESTEXE.EXE", "SAKTESTEXE.EXE",
        "ADTESTEXE.EXE", "RAGEXE_M.EXE", "SAKEXE_M.EXE"
    };
    HMODULE kernel;
    SnapshotFn snapshot;
    ProcessWalkFn first, next;
    HANDLE snap;
    PROCESSENTRY32 entry;
    char name[0x104];
    BOOL more;
    int i;
    unsigned k;

    kernel = LoadLibraryA("kernel32.dll");
    if (!kernel)
        return 0;
    snapshot = (SnapshotFn)GetProcAddress(kernel, "CreateToolhelp32Snapshot");
    first = (ProcessWalkFn)GetProcAddress(kernel, "Process32First");
    next = (ProcessWalkFn)GetProcAddress(kernel, "Process32Next");
    if (!snapshot || !first || !next) {
        FreeLibrary(kernel);
        return 0;
    }
    snap = snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) {
        FreeLibrary(kernel);
        FreeLibrary(kernel);
        return 0;
    }
    unknown_name(name);
    if (first(snap, &entry)) {
        do {
            more = next(snap, &entry);
            for (i = (int)strlen(entry.szExeFile) - 1; i > 0; i--)
                if (entry.szExeFile[i] == '\\') {
                    i++;
                    break;
                }
            strcpy(name, entry.szExeFile + i);
            upper_ascii(name);
            puts(name);
            for (k = 0; k < sizeof exes / sizeof exes[0]; k++)
                if (strcmp(name, exes[k]) == 0) {
                    FreeLibrary(kernel);
                    return 1;
                }
        } while (more);
    }
    CloseHandle(snap);
    FreeLibrary(kernel);
    return 0;
}

typedef BOOL (WINAPI *EnumProcessesFn)(DWORD *pids, DWORD bytes, DWORD *needed);
typedef BOOL (WINAPI *EnumProcessModulesFn)(HANDLE process, HMODULE *modules, DWORD bytes, DWORD *needed);
typedef DWORD (WINAPI *GetModuleBaseNameFn)(HANDLE process, HMODULE module, char *name, DWORD size);

char i_process_construct(void)
{
    HMODULE psapi;
    EnumProcessesFn enum_processes;
    EnumProcessModulesFn enum_modules;
    GetModuleBaseNameFn base_name;
    DWORD pids[0x400], needed, count, i;
    HMODULE module;
    HANDLE process;
    char name[0x104];

    psapi = LoadLibraryA("psapi.dll");
    if (!psapi)
        return 0;
    unknown_name(name);
    enum_processes = (EnumProcessesFn)GetProcAddress(psapi, "EnumProcesses");
    enum_modules = (EnumProcessModulesFn)GetProcAddress(psapi, "EnumProcessModules");
    base_name = (GetModuleBaseNameFn)GetProcAddress(psapi, "GetModuleBaseNameA");
    if (!enum_processes || !enum_modules || !base_name || !enum_processes(pids, sizeof pids, &needed)) {
        FreeLibrary(psapi);
        return 0;
    }
    count = needed >> 2;
    for (i = 0; i < count; i++) {
        process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pids[i]);
        if (process && enum_modules(process, &module, 4, &needed)) {
            base_name(process, module, name, 0x104);
            upper_ascii(name);
            puts(name);
            if (strcmp(name, "RAGEXE.EXE") == 0 || strcmp(name, "SAKEXE.EXE") == 0
                || strcmp(name, "RAGEXERE.EXE") == 0) {
                FreeLibrary(psapi);
                return 1;
            }
        }
        CloseHandle(process);
    }
    FreeLibrary(psapi);
    return 0;
}

HWND g_hMainWnd;

void app_activate(int active)
{
    g_windowActive = active;
    if (active) {
        RECT client, clip;
        POINT top_left, bottom_right;

        SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
        if (g_session1.mouseExclusiveOption != 1)
            return;
        GetClientRect(g_hMainWnd, &client);
        top_left.x = client.left;
        top_left.y = client.top;
        bottom_right.x = client.right;
        bottom_right.y = client.bottom;
        ClientToScreen(g_hMainWnd, &top_left);
        ClientToScreen(g_hMainWnd, &bottom_right);
        clip.left = top_left.x;
        clip.top = top_left.y;
        clip.right = bottom_right.x;
        clip.bottom = bottom_right.y;
        ClipCursor(&clip);
    } else {
        struct IntInt64Pair *var;
        DWORD packet;
        int key = 2, flag;

        SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
        var = tree_find(&g_session1.instantVars, &key, tree_less_int);
        flag = var ? (int)var->value : 0;
        if (flag) {
            packet = 0x07f40b11;
            rag_connection_send_packet(rag_connection_instance(), 4, &packet);
            session_set_pair(&g_session1, 2, 0);
        }
    }
}

int app_init_3d(void)
{
    struct SurfaceDesc mode;
    int flags = g_fullscreenMode ? 7 : 6;

    mode.refreshRate = 0;
    mode.width = g_displayWidth;
    mode.height = g_displayHeight;
    mode.bitCount = g_bitsPerPixel;
    if (c3d_device_init(&g_3dDevice, g_hMainWnd, &g_driverGuid, &g_deviceGuid, &mode, flags) < 0) {
        error_msg("Cannot init d3d OR grf file has problem.", 0);
        return 0;
    }
    return 1;
}

static struct InitResult *init_failed(struct InitResult *out, const char *what)
{
    out->ok = 0;
    string_init(&out->what);
    string_assign_cstr(&out->what, what);
    return out;
}

struct InitResult *app_init_managers(struct InitResult *out)
{
    if (!themida_startup())
        return init_failed(out, "CThemidaMgr::CreateInstance()");
    if (!reassembly_packet_mgr_create())
        return init_failed(out, "CReassemblyPacketMgr::CreateInstance()");
    ez2_str_effect_mgr_load(ez2str_effect_mgr_instance());
    if (!effect_mgr_create())
        return init_failed(out, "CEffectMgr::CreateInstance()");
    if (!effect_memory_create())
        return init_failed(out, "Particle pools");
    if (!entry_queue_mgr_create())
        return init_failed(out, "CEntryQueueMgr::CreateInstance()");
    if (!time_scheduler_create())
        return init_failed(out, "CTimeScheduler::CreateInstance()");
    town_info_create();
    if (!town_info_load())
        MessageBoxA(0, "TownInfo file Init", "Error", 0);
    if (!icon_sheet_create())
        return init_failed(out, "CCooltimerMgr::CreateInstance()");
    if (!renew_quest_create())
        return init_failed(out, "CRenewQuestMgr::CreateInstance()");
    renew_quest_load();
    if (!clan_info_create())
        return init_failed(out, "CClanInfoMgr::CreateInstance()");
    champion_monster_create();
    if (!champion_monster_load())
        MessageBoxA(0, "ChampionMonster file Init", "Error", 0);
    if (!map_property_mgr_create())
        return init_failed(out, "CMapPropertyMgr::CreateInstance()");
    session_load_map_info_table(&g_session1);
    if (!rodex_create_instance())
        return init_failed(out, "CRodexSystemMgr::CreateInstance()");
    if (!roulette_create_instance())
        return init_failed(out, "CRoulletteMgr::CreateInstance()");
    if (!http_client_start())
        return init_failed(out, "CCURLMgr::CreateInstance()");
    if (!account_user_data_create())
        return init_failed(out, "CAccountLinkedUserDataMgr::CreateInstance()");
    if (!char_user_data_create())
        return init_failed(out, "CCharacterLinkedUserDataMgr::CreateInstance()");
    if (!twitter_create())
        return init_failed(out, "CTwitterDataMgr::CreateInstance()");
    if (!async_work_mgr_create(2))
        return init_failed(out, "CAsyncWorkMgr::CreateInstance()");
    if (!async_work_mgr_start())
        return init_failed(out, "InstanceCAsyncWorkMgr()->Start()");
    if (!whisper_block_create())
        return init_failed(out, "CWhisperBlockMgr::CreateInstance()");
    if (!player_names_create())
        return init_failed(out, "CPlayerNameDictionary::CreateInstance()");
    if (!guild_mgr_create())
        return init_failed(out, "CGuildMgr::CreateInstance()");
    if (!emblem_mgr_create())
        return init_failed(out, "CEmblemDataMgr::CreateInstance()");
    if (!connect_sns_mgr_create_instance())
        return init_failed(out, "CConnectSnsMgr::CreateInstance()");
    if (signboards_load(g_session1.luaState))
        signboards_place();
    if (!adventurer_agency_create())
        return init_failed(out, "CAdventurerAgencyMgr::CreateInstance()");
    if (!ui_request_join_party_mgr_create())
        return init_failed(out, "UIRequestJoinPartyMgr::CreateInstance()");
    repute_mgr_load(repute_mgr_instance());
    item_db_to_id_load(item_db_to_id_instance());
    item_reform_mgr_load(item_reform_mgr());
    enchant_mgr_load(enchant_mgr_instance());
    map_sky_mgr_load(map_sky_mgr_instance());
    select_package_create();
    select_package_load(g_selectPackageMgr);
    out->ok = 1;
    string_init(&out->what);
    string_assign_cstr(&out->what, "good");
    return out;
}
