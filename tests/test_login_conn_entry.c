#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/xml.h"
#include "login/loginmode.h"
#include "platform/clientinfo.h"

/* Load the connection list with the production login_mode_load_connections from a
   parsed clientinfo.xml, after filling the stack with a pattern, and construct and
   copy an entry over filled memory. An entry is its five strings and nothing else:
   the unused version field, which the copy took from an entry that never set it,
   is gone. */

STATIC_ASSERT(conn_entry_is_its_strings, sizeof(struct LoginConnEntry) == 5 * sizeof(struct String));

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

static void check_text(const struct String *s, const char *expected)
{
    assert(s->size == strlen(expected) && strcmp(string_cstr(s), expected) == 0);
}

static void check_construct(void)
{
    struct LoginConnEntry entry, copy;

    memset(&entry, 0xa5, sizeof entry);
    login_conn_entry_construct(&entry);
    check_text(&entry.display, "");
    check_text(&entry.port, "");
    string_assign_cstr(&entry.display, "Shown");
    string_assign_cstr(&entry.desc, "Described");
    string_assign_cstr(&entry.balloon, "Balloon");
    string_assign_cstr(&entry.address, "127.0.0.1");
    string_assign_cstr(&entry.port, "6900");
    memset(&copy, 0x5a, sizeof copy);
    login_conn_entry_copy(&copy, &entry);
    check_text(&copy.display, "Shown");
    check_text(&copy.desc, "Described");
    check_text(&copy.balloon, "Balloon");
    check_text(&copy.address, "127.0.0.1");
    check_text(&copy.port, "6900");
    string_free(&copy.port); string_free(&copy.address); string_free(&copy.balloon);
    string_free(&copy.desc); string_free(&copy.display);
    string_free(&entry.port); string_free(&entry.address); string_free(&entry.balloon);
    string_free(&entry.desc); string_free(&entry.display);
    ++cases;
}

int main(void)
{
    static struct LoginMode mode;
    const struct LoginConnEntry *e;

    check_construct();
    load("<clientinfo>"
         "<connection><display>First</display><desc>one</desc><balloon>b1</balloon></connection>"
         "<connection><display>Second</display></connection>"
         "</clientinfo>");
    fill_stack();
    assert(login_mode_load_connections(&mode) == 1);
    e = (const struct LoginConnEntry *)mode.connections.first;
    assert((const struct LoginConnEntry *)mode.connections.last - e == 2);
    check_text(&e[0].display, "First");
    check_text(&e[0].desc, "one");
    check_text(&e[0].balloon, "b1");
    check_text(&e[1].display, "Second");
    check_text(&e[1].desc, "");
    check_text(&e[1].address, "");
    cases += 2;
    login_conn_entries_free(&mode.connections);
    unload();
    printf("Login connection entries: %d entries built from their five strings only.\n", cases);
    return 0;
}
