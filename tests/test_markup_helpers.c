#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui/chat/markup_links.h"
#include "session/session_items.h"
#include "ui/window_mgr.h"

static unsigned allocations, queries, notices;
static int queried_ids[1024];
static char long_message[6001];
struct Session g_session1;
struct UIWindowMgr g_windowMgr;

void *mem_alloc(size_t size)
{
    void *p = malloc(size);
    assert(p);
    ++allocations;
    return p;
}
void mem_free(void *p)
{
    assert(p && allocations);
    --allocations;
    free(p);
}
void fatal_error(const char *message)
{
    (void)message;
    abort();
}
#include "../src/std/string.c"

char *msg_str(int id)
{
    static char *messages[] = {
        "zero", "Hello %s", "%04d apples", "%.2f", "한국%s", "%s/%d/%.1f", "",
        "<MSG>0</MSG>", "<MSG>8</MSG>", "x<MSG>9</MSG>", "%s%s", "percent %",
        "%n", "%*s", "%99999999999999s", "%% %d", "참고", "%s tail", "%s %s",
        NULL, "<MSG>21</MSG>", "<MSG>20</MSG>", "<MSG>0</MSG><MSG>1,한국</MSG>",
        "%lld", "%.300f", "%0400d"
    };
    if (id == 19) return long_message;
    if (id == 0xe8d) return "item not owned";
    if (id < 0 || (unsigned)id >= sizeof messages / sizeof messages[0]) return "unknown";
    return messages[id];
}
struct ItemInfo *session_get_inventory_item_by_id(struct Session *session, struct ItemInfo *out, int id, BYTE unused)
{
    assert(session == &g_session1 && !unused);
    assert(queries < sizeof queried_ids / sizeof queried_ids[0]);
    queried_ids[queries++] = id;
    memset(out, 0, sizeof *out);
    string_assign_cstr(&out->name, "name");
    string_assign_cstr(&out->id_text, "id");
    out->index = id >= 100 && id < 200 && id % 3 == 0;
    return out;
}
BYTE session_is_wearing_item_id(struct Session *session, int id)
{ assert(session == &g_session1); return id >= 100 && id < 200 && id % 3 == 1; }
BYTE session_has_cart_item_id(struct Session *session, int id)
{ assert(session == &g_session1); return id >= 100 && id < 200 && id % 3 == 2; }
int ui_window_mgr_send_msg_(struct UIWindowMgr *self, int msg, int text, int colour, int c, int d)
{
    assert(self == &g_windowMgr && msg == UIWM_MSG_CHAT_LINE && colour == 0xff && !c && !d);
    assert(!strcmp((const char *)text, "item not owned"));
    ++notices;
    return 0;
}

static void message(struct String *out, const char *input, const char *expected)
{
    assert(navi_links_expand_msg_tags(out, text_view_cstr(input)) == out);
    assert(out->size == strlen(expected) && !strcmp(string_cstr(out), expected));
}
static void filter(const char *input, int blocked, unsigned count)
{
    unsigned before = allocations;

    queries = notices = 0;
    assert(chat_filter_is_blocked(text_view_cstr(input)) == blocked);
    assert(notices == (unsigned)blocked && queries == count && allocations == before);
}

static void guard_tests(void)
{
    static const char input[] = "prefix<NAVIL>map0102</NAVIL><NR><MSG>1,한국</MSG>";
    SYSTEM_INFO info;
    DWORD old;
    char *memory;
    struct String out = {0};
    size_t i;

    GetSystemInfo(&info);
    memory = VirtualAlloc(NULL, info.dwPageSize * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(memory && VirtualProtect(memory + info.dwPageSize, info.dwPageSize, PAGE_NOACCESS, &old));
    for (i = 0; i < sizeof input; ++i) {
        char *edge = memory + info.dwPageSize - i;
        struct TextView view;

        memcpy(edge, input, i);
        view = text_view(edge, i);
        navi_links_has_link(view);
        navi_links_extract_link(view);
        navi_links_strip_nr(&out, view);
        navi_links_expand_msg_tags(&out, view);
        assert(!chat_filter_is_blocked(view));
    }
    string_free(&out);
    assert(VirtualFree(memory, 0, MEM_RELEASE));
}

int main(void)
{
    struct String out = {0}, other = {0};
    struct TextView view;
    const char input[] = "prefix<NAVIL>map0102</NAVIL>suffix";
    unsigned i;

    assert(text_view_find(text_view("a\0b", 3), text_view("\0b", 2), 0) == 1);
    assert(text_view_find(text_view("abc", 3), text_view("", 0), 3) == 3);
    assert(text_view_find(text_view("abc", 3), text_view("a", 1), 4) == STRING_NPOS);
    for (i = 0; i < 10000; ++i) {
        navi_links_to_base62(&out, i);
        assert(out.size >= 2 && navi_links_base62(string_view(&out)) == (int)i);
    }
    navi_links_to_base62(&out, UINT_MAX);
    assert(!strcmp(string_cstr(&out), "4GFfc3"));
    assert(navi_links_has_link(text_view_cstr("x</NAVIL>")));
    assert(navi_links_has_link(text_view_cstr("x<ITEML>")));
    assert(navi_links_has_link(text_view("a\0</ITEML>", 10)));
    assert(!navi_links_has_link(text_view_cstr("<NAVIL <iteml>")));
    view = navi_links_extract_link(text_view_cstr(input));
    assert(view.data == input + 6 && view.size == 22);
    view = navi_links_extract_link(text_view_cstr("x<NAVIL>unfinished"));
    assert(view.size == 17 && !memcmp(view.data, "<NAVIL>unfinished", 17));
    assert(navi_links_strip_nr(&out, text_view_cstr("a<NR>b<NR>c")));
    assert(!strcmp(string_cstr(&out), "ab<NR>c"));
    assert(navi_links_strip_nr(&out, string_view(&out)));
    assert(!strcmp(string_cstr(&out), "abc"));
    assert(!navi_links_strip_nr(&out, string_view(&out)));
    memset(long_message, 'x', sizeof long_message - 1);
    message(&out, "plain 한국", "plain 한국");
    message(&out, "x<MSG>0</MSG>y", "xzeroy");
    message(&out, "<MSG>1,세계</MSG>", "Hello 세계");
    message(&out, "<MSG>2,12</MSG>", "0012 apples");
    message(&out, "<MSG>3,1.25</MSG>", "1.25");
    message(&out, "<MSG>5,hi,7,1.26</MSG>", "hi/7/1.3");
    message(&out, "<MSG>1,MsgStr(16)</MSG>", "Hello 참고");
    message(&out, "<MSG>10,,x</MSG>", "x");
    message(&out, "<MSG>18,one,</MSG>", "one %s");
    message(&out, "<MSG>6</MSG>", "");
    message(&out, "<MSG>7</MSG>", "zero");
    message(&out, "<MSG>22</MSG>", "zeroHello 한국");
    message(&out, "<MSG>19</MSG>", long_message);
    message(&out, "<MSG>0</MSG><MSG>broken", "<MSG>0</MSG><MSG>broken");
    message(&out, "<MSG>8</MSG>", "<MSG>8</MSG>");
    message(&out, "<MSG>9</MSG>", "<MSG>9</MSG>");
    message(&out, "<MSG>20</MSG>", "<MSG>20</MSG>");
    message(&out, "</MSG><MSG>0</MSG>", "</MSG>zero");
    message(&out, "<MSG>11,x</MSG>", "percent %");
    message(&out, "<MSG>12,123</MSG>", "%n");
    message(&out, "<MSG>13,x</MSG>", "%*s");
    message(&out, "<MSG>14,x</MSG>", "%99999999999999s");
    message(&out, "<MSG>15,42</MSG>", "% 42");
    message(&out, "<MSG>23,42</MSG>", "%lld");
    navi_links_expand_msg_tags(&out, text_view_cstr("<MSG>24,1</MSG>"));
    assert(out.size == 302 && out.data[0] == '1' && out.data[1] == '.');
    for (i = 2; i < out.size; ++i) assert(out.data[i] == '0');
    navi_links_expand_msg_tags(&out, text_view_cstr("<MSG>25,7</MSG>"));
    assert(out.size == 400 && out.data[399] == '7');
    string_assign_cstr(&out, "xx<MSG>1,alias</MSG>");
    navi_links_expand_msg_tags(&out, text_view(out.data + 2, out.size - 2));
    assert(!strcmp(string_cstr(&out), "Hello alias"));
    message(&other, "<MSG>0</MSG>", "zero");
    assert(!strcmp(string_cstr(&out), "Hello alias"));
    {
        size_t original_size;

        string_assign_cstr(&out, "<MSG>17,");
        string_reserve(&out, out.size + 1024 * 1024);
        memset(out.data + out.size, 'x', 1024 * 1024);
        out.size += 1024 * 1024;
        out.data[out.size] = 0;
        string_append_cstr(&out, "</MSG>");
        original_size = out.size;
        navi_links_expand_msg_tags(&out, string_view(&out));
        assert(out.size == original_size && !memcmp(out.data, "<MSG>17,", 8));
        assert(!strcmp(out.data + out.size - 6, "</MSG>"));
        string_clear(&out);
        for (i = 0; i < 4100; ++i) string_append_cstr(&out, "<MSG>0</MSG>");
        original_size = out.size;
        navi_links_expand_msg_tags(&out, string_view(&out));
        assert(out.size == original_size && !memcmp(out.data, "<MSG>0</MSG>", 12));
    }
    filter("plain", 0, 0);
    filter("</ITEML>", 0, 0);
    filter("<ITEML>0000001C!</ITEML>", 0, 1); assert(queried_ids[0] == 100);
    filter("<ITEML>0000001D!</ITEML>", 0, 1); assert(queried_ids[0] == 101);
    filter("<ITEML>0000001E!</ITEML>", 0, 1); assert(queried_ids[0] == 102);
    filter("a<ITEML>0000001C!</ITEML>b<ITEML>0000001E!</ITEML>", 0, 2);
    filter("<ITEML>000000zz!</ITEML>", 1, 1);
    filter("<ITEML>short</ITEML>", 1, 0);
    filter("<ITEML>missing close", 1, 0);
    filter("</ITEML><ITEML>0000001C!</ITEML>", 1, 0);
    guard_tests();
    for (i = 0; i < 1000; ++i) {
        message(&out, "<MSG>1,한국</MSG>", "Hello 한국");
        assert(navi_links_strip_nr(&other, text_view_cstr("<NR>title")));
        filter("<ITEML>0000001C!</ITEML>", 0, 1);
    }
    string_free(&other);
    string_free(&out);
    assert(!allocations);
    puts("Markup helpers: stateless checks, borrowed extraction, no-reply ownership, bounded message formatting, filter queries and cleanup passed.");
    return 0;
}
