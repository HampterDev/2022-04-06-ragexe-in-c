#include "ui/window_access.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui/controls/edit.h"
#include "ui/items/item_tags.h"
#include "platform/language_api.h"

static struct { void *base, *data; size_t size; } guarded[64];
static size_t guarded_count;
static void guarded_free(void *data)
{
    size_t i;
    if (!data) return;
    for (i = 0; i < guarded_count && guarded[i].data != data; ++i) {}
    assert(i < guarded_count);
    assert(VirtualFree(guarded[i].base, 0, MEM_RELEASE));
    guarded[i] = guarded[--guarded_count];
}
void *mem_alloc(size_t size)
{
    SYSTEM_INFO info;
    char *base, *next;
    size_t available;
    GetSystemInfo(&info);
    available = ((size + info.dwPageSize - 1) / info.dwPageSize) * info.dwPageSize;
    base = VirtualAlloc(NULL, available + 2 * info.dwPageSize, MEM_RESERVE, PAGE_NOACCESS);
    assert(base && guarded_count < 64);
    next = VirtualAlloc(base + info.dwPageSize, available, MEM_COMMIT, PAGE_READWRITE);
    assert(next);
    guarded[guarded_count].base = base;
    guarded[guarded_count].data = next;
    guarded[guarded_count++].size = size;
    return next;
}
void mem_free(void *p)
{
    guarded_free(p);
}
void fatal_error(const char *message)
{
    (void)message;
    abort();
}
#include "../src/std/string.c"

static void draw_rewrap(struct UIEditCtrl3 *self, struct TextArray *lines);
#define ui_edit_ctrl3_rewrap draw_rewrap
#include "../src/ui/controls/edit3_draw.c"
#undef ui_edit_ctrl3_rewrap

struct UIWindowMgr g_windowMgr;
struct SkinMgr g_skinMgr;
static struct Language language;
struct Language *g_language = &language;
static struct UIWindow *focus;
static const char *ime_input = "";
static int inject_empty, drawn_count, key_calls, char_calls, updated, invalidated;
static char drawn[64][256];

static void draw_rewrap(struct UIEditCtrl3 *self, struct TextArray *lines)
{
    ui_edit_ctrl3_rewrap(self, lines);
    if (inject_empty) {
        assert(text_array_resize(lines, 2));
        assert(owned_text_assign(&lines->items[0], text_view_cstr("")));
        assert(owned_text_assign(&lines->items[1], text_view_cstr("x")));
        assert(byte_array_resize(&self->line_flags, 2));
    }
}
const char *language_char_next(const char *text) { return CharNextExA(949, text, 0); }
const char *language_charset_tag(struct Language *self, unsigned char folder)
{ assert(self == g_language && !folder); return ""; }
int ui_window_text_width(struct UIWindow *self, const char *text, int len, int face, int height, int a, int b)
{
    int width = 0, i;
    assert(self && len >= 0 && !face && height == 12 && !a && !b);
    if (!len) len = (int)strlen(text);
    for (i = 0; i < len; ++i) if (text[i] != '\r' && text[i] != '\n') ++width;
    return width;
}
void ui_window_text_out(struct UIWindow *self, int x, int y, const char *text, int len,
                      int face, int height, COLORREF colour, int a, int b)
{
    (void)x; (void)y; (void)colour;
    assert(self && !len && !face && height == 12 && !a && !b && drawn_count < 64);
    strcpy_s(drawn[drawn_count++], sizeof drawn[0], text);
}
void ui_window_fill_rect(struct UIWindow *self, int x, int y, int width, int height, COLORREF colour)
{ (void)x; (void)y; (void)colour; assert(self && width >= 0 && height == 14); }
void ui_skin_color_chip(struct SkinMgr *self, int x, int y, int *r, int *g, int *b)
{ assert(self == &g_skinMgr && x == 6 && y == 2); *r = *g = *b = 0x11; }
struct UIWindow *ui_window_mgr_get_focus_edit(struct UIWindowMgr *self)
{ assert(self == &g_windowMgr); return focus; }
void ui_edit_ctrl_sync_from_ime(struct UIEditCtrl *edit)
{ string_assign_cstr(&edit->text, ime_input); }
BYTE ui_edit_ctrl_caret_shown(void) { return 0; }
void ui_edit_ctrl_place_candidate_wnd(struct UIEditCtrl *edit, int x) { (void)edit; (void)x; }
void ui_edit_ctrl_place_key_stroke_wnd(struct UIEditCtrl *edit, int x) { (void)edit; (void)x; }
void ui_edit_ctrl3_place_navi_links(struct UIEditCtrl3 *edit, struct TextView line, int y, int index, int first, int last)
{ (void)y; assert(edit && line.data && index >= first && index < last); }
void language_get_selection(struct Language *self, int *anchor, int *caret)
{ assert(self == g_language); *anchor = self->anchor; *caret = self->caret; }
void language_move_caret_forward(struct Language *self, int count)
{ assert(self == g_language && count >= 0); self->caret += count; self->anchor = self->caret; }
void language_move_caret_back(struct Language *self, int count)
{ assert(self == g_language && count >= 0); self->caret -= count; self->anchor = self->caret; }
void language_on_char(struct Language *self, int ch) { assert(self == g_language); (void)ch; ++char_calls; }
void language_on_key_down(struct Language *self, int key) { assert(self == g_language); (void)key; ++key_calls; }
int language_replace_selection(struct Language *self, const char *text)
{ assert(self == g_language && !strcmp(text, "\r\n")); return 1; }
void ui_scroll_bar_set_pos(struct UIScrollBar *bar, int pos) { assert(bar); bar->pos = pos; }
int ui_window_send_msg_base(struct UIWindow *self, struct UIWindow *sender, int msg, int a, int b, int c, int d)
{ (void)self; (void)sender; (void)msg; (void)a; (void)b; (void)c; (void)d; return 0; }
int ui_edit_ctrl_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int a, int b, int c, int d)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)window; return ui_window_send_msg_base(&self->base, sender, msg, a, b, c, d); }
void ui_window_invalidate_base(struct UIWindow *window) { assert(window); ++invalidated; }
static void update(struct UIEditCtrl3 *edit) { assert(edit); ++updated; }
void ui_window_move_base(struct UIWindow *window, int x, int y) { window->x = x; window->y = y; }
static const struct UIEditCtrl3Ops edit_ops = { .edit = { .base = { .invalidate = ui_window_invalidate_base }, .draw_text = ui_edit_ctrl3_draw_text }, .update_bar = update };
static struct UIWindowOps button_ops = { .move = ui_window_move_base, .invalidate = ui_window_invalidate_base };

static void cleanup(struct UIEditCtrl3 *edit)
{
    string_free(&edit->edit.text); string_free(&edit->line_above);
    string_free(&edit->line_below); text_array_clear(&edit->lines);
    text_array_clear(&edit->shown_lines); byte_array_clear(&edit->line_flags);
    item_tag_buttons_clear(&edit->tag_buttons);
    assert(!guarded_count);
}
static void draw(struct UIEditCtrl3 *edit, const char *text)
{
    drawn_count = 0;
    string_assign_cstr(&edit->edit.text, text);
    ui_edit_ctrl_draw_text(&edit->edit);
}
int main(void)
{
    struct UIEditCtrl3 edit = {0};
    struct UIScrollBar bar = {0};
    struct UIItemTagButton first = {0}, last = {0};
    struct String joined = {0};
    unsigned i;
    edit.edit.base.ops = &edit_ops.edit.base;
    edit.edit.base.width = 26; edit.edit.base.height = 42; edit.line_height = 14;
    bar.base.ops = &button_ops; edit.bar = &bar;
    draw(&edit, ""); assert(!drawn_count && !edit.shown_lines.size);
    draw(&edit, "x"); assert(drawn_count == 1 && !strcmp(drawn[0], "x"));
    inject_empty = 1; draw(&edit, "xy"); inject_empty = 0;
    assert(drawn_count == 2 && !*drawn[0] && !strcmp(drawn[1], "x"));
    draw(&edit, "a\r\nb");
    assert(drawn_count == 2 && !strcmp(drawn[0], "a") && !strcmp(drawn[1], "b"));
    assert(byte_array_at(&edit.line_flags, 0) == 1 && byte_array_at(&edit.line_flags, (size_t)-1) == 0);
    language.anchor = language.caret = 0; EDIT3_CARET_LINE(&edit) = 0;
    assert(ui_edit_ctrl3_send_msg((struct UIWindow *)(&edit), NULL, UI_MSG_KEY, VK_BACK, 0, 0, 0) == 3 && !char_calls);
    language.anchor = language.caret = 1;
    ui_edit_ctrl3_send_msg((struct UIWindow *)(&edit), NULL, UI_MSG_KEY, VK_RIGHT, 0, 0, 0); assert(key_calls == 2);
    draw(&edit, "\ra");
    assert(drawn_count == 1 && !*drawn[0]);
    draw(&edit, "a\r"); assert(drawn_count == 1 && !strcmp(drawn[0], "a\r"));
    language.anchor = language.caret = 0; EDIT3_CARET_LINE(&edit) = 0;
    ui_edit_ctrl3_send_msg((struct UIWindow *)(&edit), NULL, UI_MSG_KEY, VK_END, 0, 0, 0);
    focus = &edit.edit.base; ime_input = "a\r"; language.anchor = 1; language.caret = 2;
    edit.edit.sel_start = 1; edit.edit.sel_end = 2; edit.edit.base.width = 6;
    draw(&edit, "a\r"); focus = NULL;
    assert(drawn_count == 2 && !strcmp(drawn[1], "\r"));

    text_array_clear(&edit.lines);
    edit.edit.base.width = 7; edit.edit.base.height = 14; edit.first_line = 2;
    first.text.button.base.ops = last.text.button.base.ops = &button_ops;
    first.line = 0; last.line = 2; last.line_x = 7;
    item_tag_buttons_append(&edit.tag_buttons, &first); item_tag_buttons_append(&edit.tag_buttons, &last);
    draw(&edit, "abc");
    assert(drawn_count == 1 && !strcmp(drawn[0], "c") && edit.lines.size == 3);
    assert(!text_array_at(&edit.lines, 0).size && !strcmp(text_array_at(&edit.lines, 2).data, "c"));
    assert(first.text.button.base.x == -100 && last.text.button.base.x == 9);
    ui_edit_ctrl3_join_lines(&edit, &joined); assert(!strcmp(string_cstr(&joined), "c") && !edit.lines.size);
    string_free(&joined);
    edit.first_line = 0; edit.edit.base.width = 26; edit.edit.base.height = 42;
    for (i = 0; i < 200; ++i) draw(&edit, "한국\r\nx");
    assert(updated >= 200 && !strcmp(drawn[0], "한국") && !strcmp(drawn[1], "x"));
    cleanup(&edit);
    puts("Multiline drawing: guarded short lines, CR/LF, flag bounds, keys, scroll gaps, tag placement, joining and repeated redraw passed.");
    return 0;
}
