#include "fileio/resource_api.h"
#include "platform/code_page.h"
#include "core/table_reader.h"
#include "actor/weather.h"
#include "platform/client_config.h"
#include "core/text_util.h"

unsigned char g_startupSecondMod4;
#include "session/state.h"
#include "ui/window_manager_state.h"
#include <stdlib.h>
#include <string.h>
#include "platform/clientinfo.h"
#include "platform/clock.h"

const char *g_serverAddress = "";
const char *g_serverPort = "";
const char *g_serverDomain = "";
const char *g_windowClassName = "";
const char *g_registryKeyPath = "";

int g_nameFontHeight = 12;

char g_accountServerAddr[129] = { 0 };

static BYTE g_extendedSlot = 0;

int g_serviceType = 0;

unsigned char g_useCharsetTag;
BYTE g_hideAccountList;
BYTE g_passwordEncrypt;
BYTE g_passwordEncrypt2;
int g_serverType;
unsigned int g_codePage;
int g_clientVersion;
BYTE g_showNameA;
BYTE g_showNameB;
int g_languageIndex;
struct Vec g_yellowAids;
struct XmlDocument g_clientInfoDocument;
struct Vec g_loadingScreens;
struct Vec g_adminAids;
BYTE g_clientInfoRead;
static struct Language g_languageDefault;
static struct Once g_languageDefaultGuard;
static struct LanguageTC g_languageTC;
static struct Once g_languageTCGuard;
static struct Language g_languageTH;
static struct Once g_languageTHGuard;
static struct Language g_languageARB;
static struct Once g_languageARBGuard;

#define UI_FOLDER "유저인터페이스"

static unsigned char uint_less(const void *a, const void *b, void *pred)
{
    return *(const unsigned *)a < *(const unsigned *)b;
}

void uint_sort(unsigned *first, unsigned *last)
{
    std_sort(first, last, sizeof(unsigned), uint_less, NULL);
}

static void loading_screens_push(const char *name)
{
    string_vec_push_text(&g_loadingScreens, name, strlen(name));
}

static const char *child_text(struct XmlElement *node, const char *name)
{
    struct XmlElement *child = xml_find_child(node, name);

    if (!child)
        return NULL;
    return string_cstr(xml_node_text(child));
}

struct XmlElement *get_client_info(void)
{
    return xml_find_child(&g_clientInfoDocument.root, "clientinfo");
}

void clientinfo_load(const char *xml_name)
{
    struct File file;
    struct PlatformDateTime now;

    file_construct(&file);
    if (file_open(&file, xml_name, 0, 1, 0)) {
        xml_parse(&g_clientInfoDocument, (const char *)file.buffer, (const char *)file.buffer + file.size);
        file_close(&file);
    }
    init_default_service_type_();
    init_client_info_();
    if (g_serviceType == 0 || g_serviceType == 6) {
        g_showNameA = 1;
        g_showNameB = 1;
        g_nameFontHeight = 0xd;
    } else {
        g_showNameA = 0;
        g_showNameB = 0;
        g_nameFontHeight = 0xc;
    }
    platform_date_time_local(&now);
    g_startupSecondMod4 = (BYTE)now.second & 3;
    g_registryKeyPath = "Software\\Gravity Soft\\Ragnarok";
    g_windowClassName = "Ragnarok";
    file_destruct(&file);
}

void init_client_info_(void)
{
    if ((unsigned)g_serviceType > 0x14 || g_serviceType == 0x10) {
        error_msg("Unknown ServiceType !!!", 0);
        init_language_();
        return;
    }
    if (g_serviceType != 0) {
        select_client_info2(0);
        init_language_();
        return;
    }
    if (xml_find_child(&g_clientInfoDocument.root, "clientinfo"))
        select_client_info2(0);
    snprintf(g_accountServerAddr, 0x81, "%s:%s", g_serverAddress, g_serverPort);
    init_language_();
}

void load_loading_screens(int count)
{
    char name[0x104];
    int i;

    if (count == 0) {
        switch (g_serviceType) {
        case 0: count = 7; break;
        case 1: count = 0xc; break;
        case 3: count = 4; break;
        case 5: case 7: count = 8; break;
        case 6: count = 9; break;
        default: count = 1; break;
        }
    } else if (count < 0) {
        return;
    }
    for (i = 0; i < count; i++) {
        sprintf(name, UI_FOLDER "\\loading%02d.jpg", i);
        if (res_mgr_is_available(g_resMgr(), name))
            loading_screens_push(name);
    }
}

void init_default_service_type_(void)
{
    struct XmlElement *root = xml_find_child(&g_clientInfoDocument.root, "clientinfo");
    struct TableReader reader = {0};
    struct String *s;
    size_t i;

    if (root)
        set_option(root);
    if (file_make_file_name("etcinfo.txt", 0)) {
        for (s = (struct String *)g_loadingScreens.first; s != (struct String *)g_loadingScreens.last; s++)
            string_free(s);
        g_loadingScreens.last = g_loadingScreens.first;
        table_reader_read(&reader, "etcinfo.txt", TABLE_FIELDS);
        for (i = 0; i < reader.count; i++) {
            char **field = reader.fields + i;
            if (strcmp(field[0], "loadingscreen") == 0 && i + 1 < reader.count) {
                int n = atoi(field[1]);
                if (n > 0)
                    load_loading_screens(n);
            } else if (strcmp(field[0], "event") == 0 && i + 1 < reader.count) {
                if (atoi(field[1]) > 0)
                    game_file_manager_add_archive(&g_fileMgr, "event.grf");
            } else if (strcmp(field[0], "weather") == 0 && i + 2 < reader.count) {
                struct String map, effect, map_arg, effect_arg;

                string_init(&map);
                string_assign_cstr(&map, field[1]);
                i += 2;
                string_init(&effect);
                string_assign_cstr(&effect, field[2]);
                text_to_lower(&map);
                text_to_lower(&effect);
                string_copy(&effect_arg, &effect);
                string_copy(&map_arg, &map);
                weather_add_script(&g_weather, map_arg, effect_arg);
                string_free(&effect);
                string_free(&map);
            }
        }
        table_reader_release(&reader);
    }
    if (g_loadingScreens.first == g_loadingScreens.last)
        load_loading_screens(0);
}

static void language_default_destroy(void)
{
    language_destruct(&g_languageDefault);
}

static void language_tc_destroy(void)
{
    language_destruct(&g_languageTC.base);
}

static void language_th_destroy(void)
{
    language_destruct(&g_languageTH);
}

static void language_arb_destroy(void)
{
    language_destruct(&g_languageARB);
}

void init_language_(void)
{
    BYTE tag;

    if (once_begin(&g_languageDefaultGuard)) {
        language_construct(&g_languageDefault);
        atexit(language_default_destroy);
        once_complete(&g_languageDefaultGuard);
    }
    if (once_begin(&g_languageTCGuard)) {
        language_tc_construct(&g_languageTC);
        atexit(language_tc_destroy);
        once_complete(&g_languageTCGuard);
    }
    if (once_begin(&g_languageTHGuard)) {
        language_th_construct(&g_languageTH);
        atexit(language_th_destroy);
        once_complete(&g_languageTHGuard);
    }
    if (once_begin(&g_languageARBGuard)) {
        language_construct(&g_languageARB);
        atexit(language_arb_destroy);
        once_complete(&g_languageARBGuard);
    }
    g_language = &g_languageDefault;
    tag = g_useCharsetTag;
    switch (g_serviceType) {
    case 0:  g_languageIndex = 0;   g_codePage = 949;  break;
    case 2:  g_languageIndex = 2;   g_codePage = 932;  break;
    case 3:  g_languageIndex = 3;   g_codePage = 936;  g_language = &g_languageTC.base; break;
    case 4:  g_languageIndex = 4;   g_codePage = 950;  g_language = &g_languageTC.base; break;
    case 5:  g_languageIndex = 5;   g_codePage = 874;  g_language = &g_languageTH; break;
    case 0xa: g_languageIndex = 1;  tag = 1; g_codePage = 0;    break;
    case 0xb: g_languageIndex = 1;  tag = 0; g_codePage = 0;    break;
    case 0xc: case 0x12:
             g_languageIndex = 6;   tag = 0; g_codePage = 1252; break;
    case 0xe: g_languageIndex = 1;  tag = 0; g_codePage = 1251; break;
    case 0xf: g_languageIndex = 7;  tag = 0; g_codePage = 1258; break;
    case 0x11: g_languageIndex = 1; tag = 0; g_codePage = 1145; break;
    case 0x13: g_languageIndex = 0xb; g_codePage = 1256; g_language = &g_languageARB; break;
    case 0x10:
    default:
        if (g_serviceType == 0x10 || (unsigned)g_serviceType > 0x14)
            error_msg("no language setting !!!", 0);
        g_codePage = 0;
        g_languageIndex = 1;
        break;
    }
    g_useCharsetTag = g_serverType == 3 ? 1 : tag;
}

BYTE clientinfo_is_admin_aid(int aid)
{
    int *p;

    for (p = (int *)g_adminAids.first; p != (int *)g_adminAids.last; p++)
        if (*p == aid)
            break;
    return p != (int *)g_adminAids.last;
}

BYTE clientinfo_is_yellow_aid(int aid)
{
    int *p;

    for (p = (int *)g_yellowAids.first; p != (int *)g_yellowAids.last; p++)
        if (*p == aid)
            break;
    return p != (int *)g_yellowAids.last;
}

static int read_admins(struct XmlElement *list, struct Vec *ids)
{
    struct XmlElement *admin;

    vec_clear(ids);
    admin = xml_find_child(list, "admin");
    if (!admin)
        return 0;
    for (; admin; admin = xml_find_next(admin, "admin")) {
        int id = atoi(string_cstr(xml_node_text(admin)));

        vec_push(ids, &id, sizeof id);
    }
    uint_sort((unsigned *)ids->first, (unsigned *)ids->last);
    return 1;
}

static void read_connection(struct XmlElement *node)
{
    struct XmlElement *list;
    const char *text;

    set_option(node);
    if ((text = child_text(node, "address")) != NULL)
        g_serverAddress = text;
    if ((text = child_text(node, "port")) != NULL)
        g_serverPort = text;
    if ((text = child_text(node, "domain")) != NULL)
        g_serverDomain = text;
    if ((text = child_text(node, "version")) != NULL)
        g_clientVersion = atoi(text);
    if ((text = child_text(node, "langtype")) != NULL)
        g_serviceType = atoi(text);
    list = xml_find_child(node, "aid");
    if (list)
        read_admins(list, &g_adminAids);
    else
        vec_clear(&g_adminAids);
    list = xml_find_child(node, "yellow");
    if (!list) {
        vec_assign(&g_yellowAids, g_adminAids.first, g_adminAids.last, sizeof(unsigned));
        return;
    }
    read_admins(list, &g_yellowAids);
}

static struct XmlElement *find_connection(int index)
{
    struct XmlElement *root = xml_find_child(&g_clientInfoDocument.root, "clientinfo");
    struct XmlElement *conn;

    if (!root)
        return NULL;
    conn = xml_find_child(root, "connection");
    for (; conn && index; index--)
        conn = xml_find_next(conn, "connection");
    return conn;
}

void select_client_info_(int connection, int subconnection)
{
    struct XmlElement *conn = find_connection(connection), *sub;

    if (!conn)
        return;
    sub = xml_find_child(conn, "subconnection");
    if (!sub) {
        read_connection(conn);
        init_language_();
        return;
    }
    for (; sub && subconnection; subconnection--)
        sub = xml_find_next(sub, "subconnection");
    if (sub)
        read_connection(sub);
    init_language_();
}

void select_client_info2(int connection)
{
    struct XmlElement *conn = find_connection(connection);

    if (!conn)
        return;
    read_connection(conn);
    init_language_();
}

static const char *const service_names[] = {
    "korea", "america", "japan", "china", "taiwan", "thai", "indonesia", "philippine", "malaysia",
    "singapore", "germany", "india", "brazil", "australia", "russia", "vietnam",
};

void set_option(struct XmlElement *node)
{
    struct XmlElement *el;
    struct String *text;
    char name[0x104];
    int i;

    el = xml_find_child(node, "servicetype");
    if (el) {
        for (i = 0; i < 0x10; i++)
            if (string_equal_cstr(xml_node_text(el), service_names[i]))
                break;
        if (i < 0x10)
            g_serviceType = i;
        else if (string_equal_cstr(xml_node_text(el), "chile"))
            g_serviceType = 0x11;
        else if (string_equal_cstr(xml_node_text(el), "france"))
            g_serviceType = 0x12;
        else if (string_equal_cstr(xml_node_text(el), "uae"))
            g_serviceType = 0x13;
        else if (string_equal_cstr(xml_node_text(el), "eu"))
            g_serviceType = 0x14;
        else
            error_msg("ServiceType Error !!", 0);
    } else if (!g_clientInfoRead) {
        error_msg("No ServiceType !!", 0);
    }

    el = xml_find_child(node, "servertype");
    if (el) {
        text = xml_node_text(el);
        if (string_equal_cstr(text, "primary"))
            g_serverType = g_serverType == 0xd ? 0xd : 0;
        else if (string_equal_cstr(xml_node_text(el), "sakray"))
            g_serverType = 1;
        else if (string_equal_cstr(xml_node_text(el), "local"))
            g_serverType = 2;
        else if (string_equal_cstr(xml_node_text(el), "pk"))
            g_serverType = 5;
        else
            error_msg("ServerType Error !!", 0);
    } else if (!g_clientInfoRead) {
        error_msg("No ServerType !!", 0);
    }

    if (xml_find_child(node, "hideaccountlist"))
        g_hideAccountList = 1;
    if (xml_find_child(node, "passwordencrypt"))
        g_passwordEncrypt = 1;
    if (xml_find_child(node, "passwordencrypt2")) {
        g_passwordEncrypt = 1;
        g_passwordEncrypt2 = 1;
    }
    if (xml_find_child(node, "extendedslot"))
        g_extendedSlot = 1;
    if (xml_find_child(node, "readfolder"))
        g_readFolderFirst = 1;
    el = xml_find_child(node, "loading");
    if (el) {
        for (el = xml_find_child(el, "image"); el; el = xml_find_next(el, "image")) {
            sprintf(name, UI_FOLDER "\\%s", string_cstr(xml_node_text(el)));
            loading_screens_push(name);
        }
    }
    g_clientInfoRead = 1;
}

void session_set_text_type(int a, int b)
{
    if (g_serviceType != 0 && g_serviceType != 6 && g_serviceType != 0xa && g_serviceType != 0xc)
        return;
    g_showNameA = a != 0;
    g_showNameB = b != 0;
}
