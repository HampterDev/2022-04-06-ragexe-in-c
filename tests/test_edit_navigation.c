#include "ui/window_access.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/ui/controls/edit_navi_links.c"

static unsigned heap_live, detached, moved, attached;
static int fail_button;
static struct UINaviLinkButton *heap_button;
static const char *fitted_name;
static char input[512];
static const char *display_name = "Name";
static struct UIEditCtrl *active_edit;
static struct Language language;
struct Language *g_language = &language;
struct UIWindowMgr g_windowMgr;
const struct String text_tag_NAVIL = STRING_LITERAL("<NAVIL>");
const struct String text_tag_NAVIL_END = STRING_LITERAL("</NAVIL>");

void *mem_alloc(size_t n)
{
    void *p;
    if (fail_button && n == sizeof(struct UINaviLinkButton)) return NULL;
    p = malloc(n); assert(p); ++heap_live;
    if (n == sizeof(struct UINaviLinkButton)) { assert(!heap_button); heap_button = p; }
    return p;
}
void mem_free(void *p)
{  if (p) { assert(heap_live); --heap_live; free(p); } }
void fatal_error(const char *message) { (void)message; abort(); }

struct UIWindow *ui_window_mgr_get_focus_edit(struct UIWindowMgr *self)
{ assert(self == &g_windowMgr); return active_edit ? &active_edit->base : NULL; }
static struct UIWindow *rodex_body;
struct UIEditCtrl3 *ui_rodex_compose_body(struct UIWindow *compose)
{ assert(compose == g_windowMgr.rodex_compose_wnd); return (struct UIEditCtrl3 *)rodex_body; }
const char *language_input_text(const struct Language *self)
{ assert(self == g_language); return input; }
void language_cancel_input(struct Language *self) { assert(self == g_language); }
int language_replace_selection(struct Language *self, const char *text)
{ assert(self == g_language); strcpy_s(input, sizeof input, text); return 1; }
void language_set_selection(struct Language *self, int anchor, int caret)
{ assert(self == g_language); self->anchor = anchor; self->caret = caret; }
struct String *ui_window_fit_plain_text(struct UIWindow *self, struct String *out, struct TextView text, int width)
{
    assert(self && width >= 0);
    if (fitted_name) text = text_view_cstr(fitted_name);
     string_assign(out, text.data, text.size); return out;
}
int ui_window_text_width(struct UIWindow *self, const char *text, int len, int face, int height, int a, int b)
{ assert(self && !face && height == 12 && !a && !b); return 7 * (len ? len : (int)strlen(text)); }
struct String *navi_links_readable_link(struct String *out, struct TextView text, int brackets)
{
    (void)text; assert(!brackets);
    string_assign_cstr(out, display_name); return out;
}
struct String *navi_links_readable_links(struct String *out, struct TextView text)
{
    (void)text;
    string_assign_cstr(out, "<");
    string_append_cstr(out, display_name);
    string_append_cstr(out, ">"); return out;
}
struct TextView navi_links_extract_link(struct TextView text) { return text; }
void ui_window_move_base(struct UIWindow *button, int x, int y)
{ ++moved; button->x = x; button->y = y; }
static struct UIWindowOps button_ops = { .move = ui_window_move_base };
struct UINaviLinkButton *ui_navi_link_button_construct(struct UINaviLinkButton *self)
{
    memset(self, 0, sizeof *self); self->text.button.base.ops = &button_ops;
    string_init(&self->link); return self;
}
struct UINaviLinkButton *ui_navi_link_button_create(void)
{
    struct UINaviLinkButton *self = mem_alloc(sizeof *self);
    return self ? ui_navi_link_button_construct(self) : NULL;
}
void ui_text_button_set_text(struct UIWindow *self, const char *text, DWORD colour,
                         int face, int height, BYTE outline, BYTE underline)
{ assert(self && text && colour == NAVI_LINK_COLOUR && !face && height == 12 && !outline && !underline); }
void ui_window_add_child(struct UIWindow *self, struct UIWindow *button)
{
    assert(self && button && ((struct UIEditCtrl *)self)->navigation_link == (struct UINaviLinkButton *)button);
    ++attached; button->parent = self;
}
void ui_window_remove_child(struct UIWindow *self, struct UIWindow *button, BYTE destroy)
{
    assert(self && button && destroy && !((struct UIEditCtrl *)self)->navigation_link);
    ++detached; button->parent = NULL;
    if ((struct UINaviLinkButton *)button == heap_button) {
        string_free(&heap_button->link);
        mem_free(heap_button);
        heap_button = NULL;
    }
}
int ui_window_send_msg_base(struct UIWindow *self, struct UIWindow *sender, int msg, int a, int b, int c, int d)
{ (void)self; (void)sender; (void)msg; (void)a; (void)b; (void)c; (void)d; return 0; }
struct UIHooks g_uiHooks;

struct ItemInfo *item_info_copy(struct ItemInfo *self, const struct ItemInfo *from) { (void)self; (void)from; abort(); }
struct String *item_info_get_display_name(const struct ItemInfo *self, struct String *out)
{ (void)self; (void)out; abort(); }

struct Fixture {
    struct UIEditCtrl edit;
    struct UINaviLinkButton button;
};
static void set_text(struct UIEditCtrl *edit, const char *text)
{
    string_assign_cstr(&edit->text, text);
    strcpy_s(input, sizeof input, text);
}
static struct UIEditCtrlOps edit_ops = { .set_text = set_text };
static void init_edit(struct UIEditCtrl *edit, struct UINaviLinkButton *button, const char *text, int caret)
{
    memset(edit, 0, sizeof *edit);
    edit->base.ops = &edit_ops.base;
    string_init(&edit->text); string_init(&edit->raw_text);
    string_assign_cstr(&edit->text, text);
    edit->base.width = 400; edit->text_y = 10; edit->max_chars = 255;
    ui_navi_link_button_construct(button);
    button->text.button.base.id = UI_ID_NAVI_LINK;
    button->text.button.base.parent = &edit->base;
    string_assign_cstr(&button->link, "<NAVIL>encoded</NAVIL>");
    edit->navigation_link = button;
    active_edit = edit; language.caret = language.anchor = caret;
    strcpy_s(input, sizeof input, text); detached = moved = attached = 0;
    memset(&g_windowMgr, 0, sizeof g_windowMgr);
    display_name = "Name"; fitted_name = NULL; fail_button = 0;
}
static void init(struct Fixture *f, const char *text, int caret)
{
    init_edit(&f->edit, &f->button, text, caret);
}
static void finish_edit(struct UIEditCtrl *edit, struct UINaviLinkButton *button)
{
    if (edit->navigation_link) drop_button(edit);
    string_free(&button->link); string_free(&edit->text); string_free(&edit->raw_text);
    assert(!heap_live && !heap_button);
}
static void finish(struct Fixture *f)
{
    finish_edit(&f->edit, &f->button);
}
static void check_one_line(void)
{
    struct Fixture f;
    const struct { const char *text; int caret; const char *result; } deletions[] = {
        { "x<Name!", 6, "x!" }, { "xName>!", 1, "x!" },
        { "Name Name Name <Name!", 20, "Name Name Name !" },
        { "Name Name Name Name>!", 15, "Name Name Name !" }
    };
    size_t i;

    init(&f, "x<Name>!", 3);
    ui_edit_ctrl_layout_navi_links(&f.edit);
    assert(moved == 1 && !detached && f.button.text.button.base.x == 9);
    assert(language.caret == 7 && language.anchor == 7);
    assert(!strcmp(string_cstr(&f.edit.raw_text), "x<NAVIL>encoded</NAVIL>!"));
    language.caret = 5;
    ui_edit_ctrl_layout_navi_links(&f.edit);
    assert(language.caret == 1);
    finish(&f);
    for (i = 0; i < sizeof deletions / sizeof *deletions; ++i) {
        init(&f, deletions[i].text, deletions[i].caret);
        ui_edit_ctrl_layout_navi_links(&f.edit);
        assert(!f.edit.navigation_link && detached == 1);
        assert(!strcmp(input, deletions[i].result));
        assert(!strcmp(string_cstr(&f.edit.raw_text), deletions[i].result));
        ui_edit_ctrl_layout_navi_links(&f.edit);
        assert(detached == 1);
        finish(&f);
    }
    puts("Checking half-deleted links with caret elsewhere..."); fflush(stdout);
    for (i = 0; i < 2; ++i) {
        const char *text = i ? "xName>!" : "x<Name";
        init(&f, text, 0);
        ui_edit_ctrl_layout_navi_links(&f.edit);
        assert(detached == 1 && !f.edit.navigation_link && !strcmp(input, text));
        assert(!strcmp(string_cstr(&f.edit.raw_text), text));
        finish(&f);
    }
    init(&f, "Name Name Name <Name>!", 0);
    ui_edit_ctrl_layout_navi_links(&f.edit);
    assert(!detached && !strcmp(string_cstr(&f.edit.raw_text), "Name Name Name <NAVIL>encoded</NAVIL>!"));
    finish(&f);
    init(&f, "<Name x <Name>!", 0);
    ui_edit_ctrl_layout_navi_links(&f.edit);
    assert(!detached && !strcmp(string_cstr(&f.edit.raw_text), "<Name x <NAVIL>encoded</NAVIL>!"));
    finish(&f);
    init(&f, "<Name>!", 0);
    f.edit.scroll = 50;
    ui_edit_ctrl_layout_navi_links(&f.edit);
    assert(!detached && f.button.text.button.base.x == 1000);
    assert(!strcmp(string_cstr(&f.edit.raw_text), "<NAVIL>encoded</NAVIL>!"));
    finish(&f);
    init(&f, "앞<이름>뒤", 0);
    display_name = "이름";
    ui_edit_ctrl_layout_navi_links(&f.edit);
    assert(!detached && f.button.text.button.base.x == 16);
    assert(!strcmp(string_cstr(&f.edit.raw_text), "앞<NAVIL>encoded</NAVIL>뒤"));
    finish(&f);
    init(&f, "앞<이름뒤", (int)strlen("앞<이름"));
    display_name = "이름";
    ui_edit_ctrl_layout_navi_links(&f.edit);
    assert(detached == 1 && !strcmp(input, "앞뒤"));
    finish(&f);
    for (i = 0; i < 3; ++i) {
        init(&f, "no matching link", 0);
        if (i == 1) display_name = "";
        if (i == 2) fitted_name = "";
        ui_edit_ctrl_layout_navi_links(&f.edit);
        assert(detached == 1 && !f.edit.navigation_link);
        assert(!strcmp(string_cstr(&f.edit.raw_text), "no matching link"));
        finish(&f);
    }
}
static void check_other_focus(void)
{
    struct Fixture f;
    struct UIEditCtrl other = {0};
    struct UIWindow compose = {0};

    init(&f, "x<Name!", 6);
    rodex_body = &f.edit.base;
    g_windowMgr.rodex_compose_wnd = &compose;
    active_edit = &other;
    strcpy_s(input, sizeof input, "another edit's input");
    ui_edit_ctrl_layout_navi_links(&f.edit);
    assert(detached == 1 && !strcmp(input, "another edit's input"));
    assert(!strcmp(string_cstr(&f.edit.raw_text), "x<Name!"));
    finish(&f);
}
static void check_multiline(void)
{
    struct UIEditCtrl3 edit = {0};
    struct UINaviLinkButton button;
    const char *text[] = { "<other>", "xx<Name>!", "tail" };
    size_t i;

    init_edit(&edit.edit, &button, "<other>\r\nxx<Name>!\r\ntail", 13);
    for (i = 0; i < 3; ++i) assert(text_array_append(&edit.lines, text_view_cstr(text[i])));
    button.line = 1;
    ui_edit_ctrl3_place_navi_links(&edit, text_array_at(&edit.lines, 0), 10, 0, 0, 3);
    assert(!strcmp(text_array_at(&edit.lines, 0).data, "<other>") && !moved && language.caret == 13);
    ui_edit_ctrl3_place_navi_links(&edit, text_array_at(&edit.lines, 1), 24, 1, 0, 3);
    assert(!strcmp(text_array_at(&edit.lines, 1).data, "xx<NAVIL>encoded</NAVIL>!"));
    assert(button.line == 1 && button.text.button.base.x == 16 && button.text.button.base.y == 24);
    assert(language.caret == 17 && !detached);
    ui_edit_ctrl3_place_navi_links(&edit, text_array_at(&edit.lines, 2), 38, 2, 2, 3);
    assert(button.text.button.base.x == -1000 && button.text.button.base.y == -1000);
    assert(!strcmp(text_array_at(&edit.lines, 2).data, "tail"));
    text_array_clear(&edit.lines);
    finish_edit(&edit.edit, &button);
}
static void parse(struct UIEditCtrl *edit, const char *text)
{
    ui_edit_ctrl_send_msg((struct UIWindow *)(edit), NULL, UI_MSG_PARSE_LINKS, (int)text, 0, 0, 0);
}
static void check_replace(void)
{
    struct Fixture f;
    init(&f, "before ", 0);
    f.edit.navigation_link = NULL;
    parse(&f.edit, "<NAVIL>first</NAVIL>");
    assert(attached == 1 && !detached && f.edit.navigation_link == heap_button);
    assert(!strcmp(string_cstr(&f.edit.text), "before <Name>"));
    assert(language.caret == 13 && f.edit.sel_start == 13 && f.edit.sel_end == 13);
    parse(&f.edit, "<NAVIL>second</NAVIL>");
    assert(attached == 2 && detached == 1 && f.edit.navigation_link == heap_button);
    ui_edit_ctrl_layout_navi_links(&f.edit);
    assert(!strcmp(string_cstr(&f.edit.raw_text), "before <NAVIL>second</NAVIL>"));
    parse(&f.edit, "plain text");
    assert(detached == 2 && !f.edit.navigation_link && !heap_button);
    assert(!strcmp(string_cstr(&f.edit.text), "plain text"));
    finish(&f);
    init(&f, "prefix<Name>suffix", 0);
    parse(&f.edit, "<NAVIL>replacement</NAVIL>");
    assert(detached == 1 && attached == 1);
    ui_edit_ctrl_layout_navi_links(&f.edit);
    assert(!strcmp(string_cstr(&f.edit.raw_text), "prefix<NAVIL>replacement</NAVIL>suffix"));
    finish(&f);
    init(&f, "<Name>", 0);
    f.edit.max_chars = 6;
    parse(&f.edit, "<NAVIL>too long</NAVIL>");
    assert(detached == 1 && !attached && !f.edit.navigation_link);
    assert(!strcmp(string_cstr(&f.edit.text), "<Name>"));
    finish(&f);
    init(&f, "", 0);
    f.edit.navigation_link = NULL; fail_button = 1;
    parse(&f.edit, "<NAVIL>allocation failure</NAVIL>");
    assert(!f.edit.navigation_link && !attached && !heap_button);
    assert(!strcmp(string_cstr(&f.edit.text), "<Name>"));
    finish(&f);
}
static HANDLE completed;
static DWORD WINAPI watchdog(void *unused)
{
    (void)unused;
    if (WaitForSingleObject(completed, 5000) != WAIT_OBJECT_0) {
        fputs("Navigation edit did not return within five seconds.\n", stderr);
        ExitProcess(2);
    }
    return 0;
}
int main(void)
{
    HANDLE thread;
    completed = CreateEvent(NULL, TRUE, FALSE, NULL); assert(completed);
    thread = CreateThread(NULL, 0, watchdog, NULL, 0, NULL); assert(thread);
    check_one_line(); check_other_focus(); check_multiline(); check_replace();
    SetEvent(completed); assert(WaitForSingleObject(thread, INFINITE) == WAIT_OBJECT_0);
    CloseHandle(thread); CloseHandle(completed);
    puts("Edit navigation: bounded layout, Korean bytes, multiline caret, replacement and failed allocation passed.");
    return 0;
}
