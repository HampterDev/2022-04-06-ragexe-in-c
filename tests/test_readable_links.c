#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mbstring.h>
#include <mbctype.h>
#include "ui/chat/readable_links.h"
#include "ui/chat/chat_window.h"
#include "ui/window_manager_state.h"
#include "ui/draw.h"
#include "iteminfo/item_api.h"
#include "iteminfo/item_link.h"
#include "iteminfo/item_table.h"
#include "render/textdc.h"
#include "core/text_encoding.h"
#include "contents/player_names.h"
#include "owned_storage_test_allocator.h"

static unsigned legacy_live, conversion_live, conversion_calls, fail_wide, fail_narrow;

void *mem_alloc(size_t size) { void *p = malloc(size); assert(p); ++legacy_live; return p; }
void mem_free(void *p)
{  if (p) { assert(legacy_live); --legacy_live; free(p); } }
__declspec(noreturn) void fatal_error(const char *message) { (void)message; abort(); }

wchar_t *encoding_to_wide(unsigned page, const char *text)
{
    wchar_t *wide;
    int count;
    assert(page == CP_THREAD_ACP);
    if (fail_wide) return NULL;
    count = MultiByteToWideChar(949, 0, text, -1, NULL, 0); assert(count);
    wide = malloc(count * sizeof *wide); assert(wide); ++conversion_live;
    assert(MultiByteToWideChar(949, 0, text, -1, wide, count));
    return wide;
}
char *encoding_from_wide(unsigned page, const wchar_t *wide)
{
    char *text;
    int count;
    assert(page == CP_THREAD_ACP);
    if (++conversion_calls == fail_narrow) return NULL;
    count = WideCharToMultiByte(949, 0, wide, -1, NULL, 0, NULL, NULL); assert(count);
    text = malloc(count); assert(text); ++conversion_live;
    assert(WideCharToMultiByte(949, 0, wide, -1, text, count, NULL, NULL));
    return text;
}
static void conversion_free(void *p)
{ assert(p && conversion_live); --conversion_live; free(p); }
#define free conversion_free
#include "../src/ui/controls/window_fit_plain.c"
#include "../src/ui/controls/window_fit.c"
#undef free

struct UIWindowMgr g_windowMgr;
static const char *name62 = "Sword", *name63 = "갑옷";
static const char *resolved_name;
static struct ItemTableEntry definition;
const struct ItemTableEntry *item_table_get(int id)
{
    definition.name = id == 62 ? name62 : id == 63 ? name63 : "Unknown Item";
    return &definition;
}
struct String *item_info_get_display_name(const struct ItemInfo *item, struct String *out)
{
    if (resolved_name && strstr(string_cstr(&item->name), "[pending]"))
        string_format(out, "%s Sword", resolved_name);
    else string_assign(out, string_cstr(&item->name), item->name.size);
    return out;
}

struct Session g_session1;
static unsigned children, rebuilds, pending_names, fail_child;
void *mem_try_alloc(size_t size) { return fail_child ? NULL : mem_alloc(size); }
char *msg_str(int id) { assert(id == 0x245); return "[pending]"; }
struct TextView player_name_lookup(unsigned id) { assert(id == 0); return text_view_cstr(resolved_name ? resolved_name : ""); }
void session_add_pending_name(struct Session *session, int id) { assert(session == &g_session1 && id == 0); ++pending_names; }
struct UIItemTagButton *ui_item_tag_button_construct(struct UIItemTagButton *tag)
{
    memset(tag, 0, sizeof *tag);
    string_init(&tag->text.button.text); string_init(&tag->text.button.tooltip);
    item_info_construct(&tag->item); return tag;
}
void ui_text_button_set_text(struct UIWindow *window, const char *text, DWORD colour, int face, int height, BYTE outlined, BYTE underlined)
{
    struct UIItemTagButton *tag = (struct UIItemTagButton *)window;
    assert(colour == 0x82f2e1 && !face && height == 12 && !outlined && !underlined);
    string_assign_cstr(&tag->text.button.text, text); window->height = 14;
}
void ui_bitmap_button_set_tool_tip(struct UIBitmapButton *button, const char *text)
{ struct UIItemTagButton *tag = (struct UIItemTagButton *)button; string_assign_cstr(&tag->text.button.tooltip, text); }
void ui_text_button_fit_width(struct UITextButton *button, int width, int height)
{ assert(height == 14); button->button.base.width = width; }
void ui_window_add_child(struct UIWindow *parent, struct UIWindow *child)
{ assert(parent && child->id == 0x1c8); ++children; }
void ui_chat_his_box_rebuild(struct UIChatHisBox *box) { assert(box); ++rebuilds; }
void **ptr_vector_emplace_realloc4(struct Vec *v, void **where, void *const *value)
{
    size_t n = v->first ? (void **)v->last - (void **)v->first : 0;
    void **items = mem_alloc((n + 4) * sizeof *items);
    assert(where == v->last);
    if (n) memcpy(items, v->first, n * sizeof *items);
    items[n] = *value; mem_free(v->first);
    v->first = items; v->last = items + n + 1; v->end = items + n + 4; return items + n;
}
static void history_init(struct UIChatHisBox *box, struct ListNode *head)
{
    memset(box, 0, sizeof *box); memset(head, 0, sizeof *head);
    head->next = head->prev = head; box->item_tags.head = head;
    box->list.base.width = 1000; box->list.base.canvas = (struct DcBitmap *)box; box->list.scroll_x = 3;
    children = rebuilds = pending_names = 0;
}
static void history_destroy(struct UIChatHisBox *box)
{
    struct ListNode *node = box->item_tags.head->next;
    while (node != box->item_tags.head) {
        struct ListNode *next = node->next;
        struct Vec *windows = LIST_VALUE(node, struct Vec);
        struct UIItemTagButton **p;
        for (p = windows->first; p != windows->last; ++p) {
            string_free(&(*p)->text.button.text); string_free(&(*p)->text.button.tooltip);
            item_info_destruct(&(*p)->item); mem_free(*p);
        }
        vec_free(windows); mem_free(node); node = next;
    }
}

struct Measure { int length, face, height, bold; unsigned hash; };
static struct Measure measures[8192];
static size_t measure_count;
static void measured(const char *text, int length, int face, int height, int bold)
{
    struct Measure *m;
    int i;
    assert(measure_count < sizeof measures / sizeof *measures);
    m = &measures[measure_count++];
    m->length = length; m->face = face; m->height = height; m->bold = bold; m->hash = 5381;
    for (i = 0; i < length; ++i) m->hash = m->hash * 33u + (unsigned char)text[i];
}
int ui_window_text_width(struct UIWindow *window, const char *text, int length,
                       int face, int height, int bold, int extra)
{
    assert(window && window->canvas && !extra);
    if (!length) length = (int)strlen(text);
    measured(text, length, face, height, bold);
    return length * (face + 1);
}
struct TextDC *text_dc_construct(struct TextDC *dc, struct DcBitmap *bitmap)
{ assert(bitmap); memset(dc, 0, sizeof *dc); return dc; }
void text_dc_select_font(struct TextDC *dc, int face, int height, int bold, int italic)
{ assert(!italic); dc->face = face; dc->size = height; dc->bold = (BYTE)bold; }
void text_dc_text_extent(struct TextDC *dc, const char *text, int length, SIZE *out)
{ measured(text, length, dc->face, dc->size, dc->bold); out->cx = length * (dc->face + 1); out->cy = dc->size; }
void text_dc_destruct(struct TextDC *dc) { (void)dc; }
void ui_window_move_base(struct UIWindow *window, int x, int y) { window->x = x; window->y = y; }

static struct UIWindow window = {0};
static const char link62[] = "<ITEML>00001110</ITEML>";
static const char link63[] = "<ITEML>00001111</ITEML>";
static unsigned tags;
static char prefixes[4][128], captions[4][128];
static void tag(void *context, const struct ItemInfo *item, const char *name,
                 const char *caption, const char *prefix)
{
    assert(context == &window && item->identified && *name && tags < 4);
    strcpy_s(prefixes[tags], sizeof prefixes[tags], prefix);
    strcpy_s(captions[tags], sizeof captions[tags], caption);
    ++tags;
}

static void readability(void)
{
    struct String out = {0}, input = {0};
    struct UINewChatWnd chat = {0};
    struct UIWindow other = {0}, *boxes[] = {&window, &other};
    struct ItemLinkSpan span;
    size_t cursor;
    other.width = 1; other.canvas = (struct DcBitmap *)&other;
    chat.boxes.first = boxes; chat.boxes.last = boxes + 2; chat.boxes.end = boxes + 2;
    string_format(&input, "앞 %s 뒤 %s", link62, link63);
    ui_chat_his_box_readable_item_links(&window, &out, string_view(&input));
    assert(!strcmp(string_cstr(&out), "앞 <Sword> 뒤 <갑옷>"));
    ui_chat_his_box_readable_item_links(&window, &input, string_view(&input));
    assert(input.size == out.size && !memcmp(input.data, out.data, out.size));
    string_format(&input, "</ITEML>plain <ITEML>outer %s orphan<ITEML>", link62);
    ui_chat_his_box_readable_item_links(&window, &out, string_view(&input));
    assert(!strcmp(string_cstr(&out), "</ITEML>plain <ITEML>outer <Sword> orphan<ITEML>"));
    ui_chat_his_box_readable_item_links(&window, &out, text_view_cstr("<ITEML>short</ITEML><ITEML>00001001</ITEML>"));
    assert(!strcmp(string_cstr(&out), "<ITEML>short</ITEML><ITEML>00001001</ITEML>"));
    cursor = 0;
    assert(item_link_next(string_view(&input), &cursor, &span));
    assert(span.payload.size == 8 && !memcmp(span.payload.data, "00001110", 8));
    assert(!item_link_next(string_view(&input), &cursor, &span) && cursor == input.size);
    ui_chat_his_box_readable_item_links(&window, &out, text_view("a\0b", 3));
    assert(out.size == 3 && !memcmp(out.data, "a\0b", 3));
    name62 = link63;
    ui_new_chat_wnd_readable_links(&chat.frame.base, &out, text_view_cstr(link62));
    assert(!strcmp(string_cstr(&out), "<<갑옷>>"));
    name62 = "Sword";
    chat.boxes.last = boxes;
    ui_new_chat_wnd_readable_links(&chat.frame.base, &out, text_view_cstr(link62));
    assert(!strcmp(string_cstr(&out), link62));
    chat.boxes.last = boxes + 1;
    g_windowMgr.new_chat_wnd = &chat.frame.base;
    string_format(&input, "<URL>%s</URL><INFO>secret</INFO><QUEST>완료</QUEST>", link62);
    ui_window_fit_text(&window, &out, 1000, string_cstr(&input), 2, 19, 1);
    assert(!strcmp(string_cstr(&out), "<Sword>완료"));
    g_windowMgr.new_chat_wnd = NULL;
    string_free(&input); string_free(&out);
}

static void lines(void)
{
    struct String rest = {0}, out = {0};
    string_format(&rest, "ab%sxy%sz", link62, link63);
    tags = 0;
    assert(item_link_build_line(&window, &out, &rest, "ab", tag, &window));
    assert(!strcmp(string_cstr(&out), "ab") && !tags);
    assert(item_link_build_line(&window, &out, &rest, "<Sword>xy", tag, &window));
    assert(!strcmp(string_cstr(&out), "<Sword>xy") && tags == 1);
    assert(!strcmp(prefixes[0], "") && !strcmp(captions[0], "<Sword>"));
    assert(item_link_build_line(&window, &out, &rest, "<갑옷>z", tag, &window));
    assert(!strcmp(string_cstr(&out), "<갑옷>z") && tags == 2 && !rest.size);
    string_format(&rest, "ab%safter", link62);
    tags = 0;
    assert(item_link_build_line(&window, &out, &rest, "ab123", tag, &window));
    assert(!strcmp(string_cstr(&out), "ab") && !tags);
    assert(!strncmp(string_cstr(&rest), link62, strlen(link62)));
    string_assign_cstr(&rest, "plain tail");
    assert(!item_link_build_line(&window, &out, &rest, "plain", tag, &window));
    assert(!strcmp(string_cstr(&out), "plain") && !strcmp(string_cstr(&rest), "plain tail"));
    string_assign_cstr(&rest, "ab<ITEML>");
    assert(item_link_build_line(&window, &out, &rest, "ab<", tag, &window));
    assert(!strcmp(string_cstr(&out), "ab<") && !strcmp(string_cstr(&rest), "ITEML>"));
    string_free(&rest); string_free(&out);
}

static void fitting(void)
{
    struct String out = {0};
    ui_window_fit_text(&window, &out, 5, "abcdef", 0, 12, 0);
    assert(!strcmp(string_cstr(&out), "ab..."));
    ui_window_fit_text(&window, &out, 3, "한국", 0, 12, 0);
    assert(!strcmp(string_cstr(&out), "한..."));
    ui_window_fit_text(&window, &out, 3, "나다", 0, 12, 0);
    assert(!strcmp(string_cstr(&out), "..."));
    string_assign_cstr(&out, "before <URL>abcdef</URL> after");
    ui_window_fit_text(&window, &out, 5, out.data + 12, 0, 12, 0);
    assert(!strcmp(string_cstr(&out), "ab..."));
    fail_wide = 1;
    ui_window_fit_text(&window, &out, 1, "abcdef", 0, 12, 0);
    assert(!strcmp(string_cstr(&out), "abcdef")); fail_wide = 0;
    conversion_calls = 0; fail_narrow = 2;
    ui_window_fit_text(&window, &out, 1, "abcdef", 0, 12, 0);
    assert(!strcmp(string_cstr(&out), "abcde...")); fail_narrow = 0;
    string_free(&out);
}

static void boundaries(void)
{
    SYSTEM_INFO info;
    char *memory, *end;
    struct String out = {0};
    const char *text = "가<ITEML>00001110</ITEML>나</ITEML><ITEML>";
    size_t i, size = strlen(text);
    DWORD old;
    GetSystemInfo(&info);
    memory = VirtualAlloc(NULL, info.dwPageSize * 2, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE); assert(memory);
    end = memory + info.dwPageSize;
    assert(VirtualProtect(end, info.dwPageSize, PAGE_NOACCESS, &old));
    for (i = 0; i <= size; ++i) {
        size_t cursor = 0, previous = 0;
        struct ItemLinkSpan span;
        memcpy(end - i, text, i);
        while (item_link_next(text_view(end - i, i), &cursor, &span)) {
            assert(cursor > previous && span.begin >= previous && span.end <= i); previous = cursor;
        }
        assert(cursor == i);
        ui_chat_his_box_readable_item_links(&window, &out, text_view(end - i, i));
        assert(out.size < 100);
    }
    assert(VirtualFree(memory, 0, MEM_RELEASE));
    string_free(&out);
}

static void history_buttons(void)
{
    struct UIChatHisBox box;
    struct ListNode head;
    struct String rest = {0}, out = {0};
    struct String stored;
    struct UIItemTagButton **buttons;
    history_init(&box, &head);
    name62 = "[pending] Sword";
    string_format(&rest, "ab%s xy %s!", link62, link63);
    ui_chat_his_box_build_item_line(&box, &out, "ab<[pending] Sword> xy <갑옷>!", &rest);
    assert(!rest.size && children == 2 && pending_names == 1 && box.item_tags.size == 1);
    buttons = LIST_VALUE(head.next, struct Vec)->first;
    assert(buttons[0]->line_x == -1 && buttons[0]->line == 0);
    assert(buttons[0]->text.hover_colour == 0x82f2e1 && !strcmp(string_cstr(&buttons[0]->text.button.tooltip), "[pending] Sword"));
    string_init(&stored);
    string_assign_cstr(&stored, string_cstr(&out));
    box.list.items.first = &stored; box.list.items.last = box.list.items.end = &stored + 1;
    resolved_name = "제작자";
    ui_chat_his_box_resolve_pending_name(&box, "someone else");
    assert(rebuilds == 1 && strstr(string_cstr(&stored), "[pending]"));
    ui_chat_his_box_resolve_pending_name(&box, resolved_name);
    assert(rebuilds == 2 && !strcmp(string_cstr(&stored), "ab<제작자 Sword> xy <갑옷>!"));
    assert(!strcmp(string_cstr(&buttons[0]->text.button.text), "<제작자 Sword>"));
    assert(!strcmp(string_cstr(&buttons[0]->text.button.tooltip), "<제작자 Sword>"));
    assert(buttons[1]->line_x == (int)strlen("ab<제작자 Sword> xy ") - box.list.scroll_x);
    buttons[0]->line = -1; buttons[1]->line = 99;
    ui_chat_his_box_resolve_pending_name(&box, resolved_name); assert(rebuilds == 3);
    buttons[0]->line = 0;
    string_assign_cstr(&stored, "missing");
    ui_chat_his_box_resolve_pending_name(&box, resolved_name); assert(!strcmp(string_cstr(&stored), "missing"));
    string_free(&stored); history_destroy(&box); resolved_name = NULL; name62 = "Sword";
    history_init(&box, &head); fail_child = 1;
    string_assign_cstr(&rest, link62);
    ui_chat_his_box_build_item_line(&box, &out, "<Sword>", &rest);
    assert(!children && box.item_tags.size == 1 && !rest.size && !strcmp(string_cstr(&out), "<Sword>"));
    fail_child = 0; history_destroy(&box);
    string_free(&rest); string_free(&out);
}

#ifdef READABLE_TEXT_REFERENCE
#define free conversion_free
#include "readable_text_reference.c"
#undef free

static void history_snapshot(struct UIChatHisBox *box, struct String *out, const char *line, const char *rest)
{
    struct ListNode *node;
    string_format(out, "%s|%s|%u|%u|%u|%u", line, rest, children, rebuilds, pending_names, box->item_tags.size);
    for (node = box->item_tags.head->next; node != box->item_tags.head; node = node->next) {
        struct Vec *windows = LIST_VALUE(node, struct Vec);
        struct UIItemTagButton **p;
        for (p = windows->first; p != windows->last; ++p) {
            struct UIItemTagButton *tag = *p;
            string_append_format(out, "|%s|%s|%d,%d,%d,%d,%d,%d,%u", string_cstr(&tag->text.button.text),
                string_cstr(&tag->text.button.tooltip), tag->line, tag->line_x, tag->aid,
                tag->text.button.base.id, tag->text.button.base.width, tag->text.button.base.height, tag->text.hover_colour);
        }
    }
}
static void reference_history_cases(void)
{
    struct String input = {0}, display = {0}, rest = {0}, out = {0}, expected = {0}, actual = {0};
    struct String pending_expected = {0};
    unsigned leading, pending, i, pass, count = 0;
    for (leading = 0; leading < 8; ++leading) for (pending = 0; pending < 2; ++pending)
    for (i = 0; i < 4; ++i) {
        name62 = pending ? "[pending] Sword" : "Sword";
        string_format(&input, "%.*s%s xy %s%.*s", leading, "abcdefgh", link62, i & 1 ? link62 : link63, i, "xyz");
        ui_chat_his_box_readable_item_links(&window, &display, string_view(&input));
        for (pass = 0; pass < 2; ++pass) {
            struct UIChatHisBox box;
            struct ListNode head;
            struct String old, remaining, stored, unused;
            history_init(&box, &head);
            measure_count = 0;
            if (!pass) {
                string_init(&remaining);
                string_assign_cstr(&remaining, string_cstr(&input));
                reference_UIChatHisBox_BuildItemLine(&box, &old, string_cstr(&display), &remaining);
                string_assign(&out, string_cstr(&old), old.size); string_assign(&rest, string_cstr(&remaining), remaining.size);
                string_free(&old); string_free(&remaining);
            } else {
                string_assign(&rest, string_cstr(&input), input.size);
                ui_chat_his_box_build_item_line(&box, &out, string_cstr(&display), &rest);
            }
            history_snapshot(&box, pass ? &actual : &expected, string_cstr(&out), string_cstr(&rest));
            if (pass) assert(actual.size == expected.size && !memcmp(actual.data, expected.data, actual.size));
            string_init(&stored);
            string_assign_cstr(&stored, string_cstr(&out));
            box.list.items.first = &stored; box.list.items.last = box.list.items.end = &stored + 1;
            if (pending) {
                resolved_name = "제작자";
                if (!pass) {
                    reference_UIChatHisBox_ResolvePendingName(&box, &unused, resolved_name); string_free(&unused);
                } else ui_chat_his_box_resolve_pending_name(&box, resolved_name);
                history_snapshot(&box, &out, string_cstr(&stored), "");
                if (!pass) string_assign(&pending_expected, string_cstr(&out), out.size);
                else assert(out.size == pending_expected.size && !memcmp(out.data, pending_expected.data, out.size));
                resolved_name = NULL;
            }
            string_free(&stored); history_destroy(&box);
        }
        ++count;
    }
    name62 = "Sword";
    string_free(&input); string_free(&display); string_free(&rest);
    string_free(&out); string_free(&expected); string_free(&actual);
    string_free(&pending_expected);
    printf("Chat history: %u archived button/placement/pending-name comparisons passed.\n", count);
}

static void reference_cases(void)
{
    static const char *inputs[] = {
        "", "ordinary text", "한글 가나다 abc", "<URL>한글</URL> and <QUEST>quest</QUEST>",
        "<NAVI>place</NAVI><TIPBOX>tip</TIPBOX><ITEM>item</ITEM>",
        "before<INFO>hidden</INFO>after", "<NAVIL>Prontera<INFO>prontera,50,60,0,0</INFO></NAVIL>",
        "<ITEML>00001110</ITEML>", "before <ITEML>00001111</ITEML> after",
        "<URL><ITEML>00001110</ITEML></URL> + <ITEML>00001111</ITEML>",
        "before <ITEML>outer<ITEML>00001110</ITEML> after", "literal <QUEST>unfinished"
    };
    struct UINewChatWnd chat = {0};
    struct UIWindow *boxes[] = {&window, &window};
    struct Measure expected[8192];
    struct String actual = {0};
    unsigned i, tabs, face, checks = 0;
    int width;
    chat.boxes.first = boxes; chat.boxes.end = boxes + 2;
    for (tabs = 0; tabs <= 2; ++tabs) {
        chat.boxes.last = boxes + tabs;
        g_windowMgr.new_chat_wnd = tabs ? &chat.frame.base : NULL;
        for (i = 0; i < sizeof inputs / sizeof *inputs; ++i) {
            struct String source, old;
            size_t count;
            string_init(&source);
            string_assign_cstr(&source, inputs[i]);
            measure_count = 0;
            reference_UINewChatWnd_ReadableLinks(&chat, &old, &source);
            count = measure_count; memcpy(expected, measures, count * sizeof *expected);
            measure_count = 0;
            ui_new_chat_wnd_readable_links(&chat.frame.base, &actual, string_view(&source));
            assert(actual.size == old.size && !memcmp(actual.data, string_cstr(&old), old.size));
            assert(count == measure_count && !memcmp(expected, measures, count * sizeof *expected));
            string_free(&old); string_free(&source); ++checks;
            for (face = 0; face < 3; ++face) for (width = -2; width <= 80; width += 2) {
                measure_count = 0;
                reference_UIWindow_FitText(&window, &old, width, inputs[i], face, 12 + face, face & 1);
                count = measure_count; memcpy(expected, measures, count * sizeof *expected);
                measure_count = 0;
                ui_window_fit_text(&window, &actual, width, inputs[i], face, 12 + face, face & 1);
                assert(actual.size == old.size && !memcmp(actual.data, string_cstr(&old), old.size));
                assert(count == measure_count && !memcmp(expected, measures, count * sizeof *expected));
                string_free(&old); ++checks;
            }
        }
    }
    string_free(&actual); g_windowMgr.new_chat_wnd = NULL;
    printf("Readable text: %u archived output and font measurement comparisons passed.\n", checks);
}
#endif

int main(void)
{
    unsigned i;
    _setmbcp(949);
    window.width = 1000; window.canvas = (struct DcBitmap *)&window;
    readability(); lines(); fitting(); boundaries(); history_buttons();
    for (i = 0; i < 100; ++i) { measure_count = 0; readability(); lines(); fitting(); }
#ifdef READABLE_TEXT_REFERENCE
    reference_cases(); reference_history_cases();
#endif
    assert(!outstanding && !legacy_live && !conversion_live);
    puts("Readable links: counted/aliased/Korean text, malformed tags, tabs, line consumption, font fitting, guarded boundaries, child buttons and pending maker names passed.");
    return 0;
}
