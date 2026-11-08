#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/xml.h"
#include "login/loginmode.h"
#include "platform/clientinfo.h"


BYTE g_hideAccountList;
static struct XmlDocument doc;
static struct XmlElement *info;
static int cases;

__declspec(noreturn) void fatal_error(const char *message) { (void)message; abort(); }
struct XmlElement *get_client_info(void) { return info; }

static __declspec(noinline) void fill_stack(void)
{
    volatile unsigned char junk[0x4000];
    size_t i;

    for (i = 0; i < sizeof junk; ++i)
        junk[i] = 0xa5;
}

static void load(const char *xml)
{
    xml_element_construct(&doc.root, "XMLDocument");
    doc.copy = NULL;
    assert(xml_parse(&doc, xml, xml + strlen(xml)));
    info = xml_find_child(&doc.root, "clientinfo");
    assert(info);
}

static void unload(void)
{
    xml_document_release(&doc);
    xml_element_destruct(&doc.root);
}

static void check_entries(struct Vec *list, const char *const *displays, size_t count)
{
    const struct LoginConnEntry *e = (const struct LoginConnEntry *)list->first;
    size_t i;

    assert((size_t)((const struct LoginConnEntry *)list->last - e) == count);
    for (i = 0; i < count; ++i) {
        assert(strcmp(string_cstr(&e[i].display), displays[i]) == 0);
        assert(e[i].version == 0 && e[i].service_type == 0);
        ++cases;
    }
}

static void check_construct(void)
{
    struct LoginConnEntry entry, copy;

    memset(&entry, 0xa5, sizeof entry);
    login_conn_entry_construct(&entry);
    assert(entry.version == 0 && entry.service_type == 0 && entry.display.size == 0);
    memset(&copy, 0x5a, sizeof copy);
    login_conn_entry_copy(&copy, &entry);
    assert(copy.version == 0 && copy.service_type == 0);
    login_conn_entry_destruct(&copy);
    login_conn_entry_destruct(&entry);
    ++cases;
}

int main(void)
{
    static const char *const servers[] = { "First", "Second" };
    static const char *const subs[] = { "Sub A", "Sub B", "Sub C" };
    static const char *const single[] = { "Second" };
    static struct LoginMode mode;
    BYTE shown;

    check_construct();
    load("<clientinfo>"
         "<connection><display>First</display><desc>one</desc><balloon>b1</balloon>"
         "<subconnection><display>Sub A</display></subconnection>"
         "<subconnection><display>Sub B</display><desc>d</desc></subconnection>"
         "<subconnection><display>Sub C</display><balloon>c</balloon></subconnection></connection>"
         "<connection><display>Second</display></connection>"
         "</clientinfo>");
    fill_stack();
    shown = login_mode_load_connections(&mode);
    assert(shown == 1);
    check_entries(&mode.connections, servers, 2);
    fill_stack();
    login_mode_load_sub_connections(&mode, 0);
    check_entries(&mode.subConnections, subs, 3);
    login_conn_entries_clear(&mode.subConnections);
    fill_stack();
    login_mode_load_sub_connections(&mode, 1);
    check_entries(&mode.subConnections, single, 1);
    login_conn_entries_free(&mode.subConnections);
    login_conn_entries_free(&mode.connections);
    unload();
    printf("Login connection entries: %d entries built with version and service_type zero.\n", cases);
    return 0;
}
