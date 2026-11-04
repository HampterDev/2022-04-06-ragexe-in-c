#include "fileio/resource_api.h"
#include "platform/code_page.h"
#include "core/table_reader.h"
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

char g_accountServerAddr[129] = { 0 };

static BYTE g_extendedSlot = 0;

BYTE g_hideAccountList;
BYTE g_passwordEncrypt;
BYTE g_passwordEncrypt2;
int g_serverType;
/* Text in memory is UTF-8; conversions to and from it name this code page. */
unsigned int g_codePage = CODE_PAGE_UTF8;
int g_clientVersion;
struct Vec g_yellowAids;
struct XmlDocument g_clientInfoDocument;
struct Vec g_loadingScreens;
struct Vec g_adminAids;
BYTE g_clientInfoRead;
static struct Language g_languageDefault;

#define UI_FOLDER "유저인터페이스"

/* Without a loadingscreen count in etcinfo.txt the client looks for loading00-06.jpg,
   the Korean count, because <servicetype>korea</servicetype> was in effect at that point. */
#define DEFAULT_LOADING_SCREENS 7

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

void (*g_clientInfoWeatherScript)(struct String map, struct String effect);

static void load_loading_screens(int count)
{
    char name[0x104];
    int i;

    for (i = 0; i < count; i++) {
        sprintf(name, UI_FOLDER "\\loading%02d.jpg", i);
        if (res_mgr_is_available(g_resMgr(), name))
            loading_screens_push(name);
    }
}

static void read_etc_info(void)
{
    struct TableReader reader = {0};
    struct String *s;
    size_t i;

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
                if (g_clientInfoWeatherScript) {
                    g_clientInfoWeatherScript(map_arg, effect_arg);
                } else {
                    string_free(&map_arg);
                    string_free(&effect_arg);
                }
                string_free(&effect);
                string_free(&map);
            }
        }
        table_reader_release(&reader);
    }
    if (g_loadingScreens.first == g_loadingScreens.last)
        load_loading_screens(DEFAULT_LOADING_SCREENS);
}

static void language_default_destroy(void)
{
    language_destruct(&g_languageDefault);
}

void clientinfo_load(const char *xml_name)
{
    struct XmlElement *root;
    struct File file;
    struct PlatformDateTime now;

    file_construct(&file);
    if (file_open(&file, xml_name, 0, 1, 0)) {
        xml_parse(&g_clientInfoDocument, (const char *)file.buffer, (const char *)file.buffer + file.size);
        file_close(&file);
    }
    root = get_client_info();
    if (root)
        set_option(root);
    read_etc_info();
    select_client_info2(0);
    snprintf(g_accountServerAddr, 0x81, "%s:%s", g_serverAddress, g_serverPort);
    language_construct(&g_languageDefault);
    atexit(language_default_destroy);
    g_language = &g_languageDefault;
    platform_date_time_local(&now);
    g_startupSecondMod4 = (BYTE)now.second & 3;
    g_registryKeyPath = "Software\\Gravity Soft\\Ragnarok";
    g_windowClassName = "Ragnarok";
    file_destruct(&file);
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

void select_client_info2(int connection)
{
    struct XmlElement *conn = find_connection(connection);

    if (!conn)
        return;
    read_connection(conn);
}

void set_option(struct XmlElement *node)
{
    struct XmlElement *el;
    struct String *text;
    char name[0x104];

    el = xml_find_child(node, "servertype");
    if (el) {
        text = xml_node_text(el);
        if (string_equal_cstr(text, "primary"))
            g_serverType = 0;
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
