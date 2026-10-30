#include "ui/window_access.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ui/controls/edit.h"

struct UIWindowMgr g_windowMgr;
static struct Language language;
struct Language *g_language = &language;
unsigned int g_codePage = 949;
static struct UIWindow *focus;
static char input[1024];
static int unavailable, cancelled, replaced, invalidated;

struct UIWindow *ui_window_mgr_get_focus_edit(struct UIWindowMgr *self)
{ assert(self == &g_windowMgr); return focus; }
int language_get_text(struct Language *self, struct TextView *out)
{
    assert(self == g_language);
    if (unavailable) return 0;
    *out = text_view_cstr(input); return 1;
}
void language_get_selection(struct Language *self, int *anchor, int *caret)
{ assert(self == g_language); *anchor = self->anchor; *caret = self->caret; }
void language_set_selection(struct Language *self, int anchor, int caret)
{ assert(self == g_language); self->anchor = anchor; self->caret = caret; }
void language_cancel_input(struct Language *self)
{ assert(self == g_language); ++cancelled; }
int language_replace_selection(struct Language *self, const char *text)
{
    assert(self == g_language); ++replaced;
    strcpy_s(input, sizeof input, text); self->input_length = strlen(input); return 1;
}
int ui_window_text_width(struct UIWindow *window, const char *text, int length,
                       int face, int height, int font_flag, int unused)
{
    assert(window && !face && height == 12 && !unused && font_flag >= 0);
    return 7 * (length ? length : (int)strlen(text));
}
static void invalidate(struct UIWindow *window) { assert(window); ++invalidated; }
static struct UIWindowOps parent_ops = { .invalidate = invalidate };
static const struct UIEditCtrlOps edit_ops = { .set_text = ui_edit_ctrl_set_text_default, .truncate = ui_edit_ctrl_truncate_default };
static const struct UIEditCtrlOps ime_ops = { .truncate = ui_edit_ctrl_truncate_to_ime };

int main(void)
{
    struct UIEditCtrl edit = {0};
    struct UIWindow parent = {0};
    char *storage;
    int i;

    edit.base.ops = &edit_ops.base;
    parent.ops = &parent_ops; edit.base.parent = &parent;
    edit.base.width = 500; edit.max_chars = 255;
    string_reserve(&edit.text, 255); storage = edit.text.data;
    ui_edit_ctrl_set_text(&edit, "앞이름뒤");
    assert(!strcmp(ui_edit_ctrl_get_text(&edit.base), "앞이름뒤"));
    assert(edit.sel_start == 0 && edit.sel_end == 8 && invalidated == 1);
    assert(!cancelled && !replaced && ui_edit_ctrl_get_text_size(&edit.base) == 8);
    ui_edit_ctrl_set_text(&edit, edit.text.data + 2);
    assert(!strcmp(edit.text.data, "이름뒤") && edit.text.data == storage);
    edit.max_chars = 3;
    ui_edit_ctrl_set_text(&edit, "가나다");
    assert(edit.text.size == 3 && !memcmp(edit.text.data, "가 ", 4));
    edit.max_chars = 0;
    ui_edit_ctrl_set_text(&edit, "가");
    assert(!edit.text.size && !*edit.text.data);
    edit.max_chars = 3;
    string_assign_cstr(&edit.text, "가나다");
    edit.base.ops = &ime_ops.base;
    ui_edit_ctrl_truncate(&edit);
    edit.base.ops = &edit_ops.base;
    assert(!strcmp(edit.text.data, "가 ") && !strcmp(input, "가"));
    assert(cancelled == 2 && replaced == 1);

    focus = &edit.base; edit.max_chars = 255;
    ui_edit_ctrl_set_text(&edit, "typed");
    assert(!strcmp(input, "typed") && language.anchor == 0 && language.caret == 5);
    for (i = 0; i < 1000; ++i) {
        strcpy_s(input, sizeof input, "입력 composition");
        language.input_length = strlen("입력 ");
        language.anchor = 1; language.caret = (int)strlen(input);
        ui_edit_ctrl_sync_from_ime(&edit);
        assert(!strcmp(edit.text.data, input) && edit.text.data == storage);
        assert(edit.sel_start == 1 && edit.sel_end == (int)strlen(input));
        assert(ui_edit_ctrl_get_text_size(&edit.base) == (int)strlen("입력 "));
    }
    unavailable = 1;
    strcpy_s(input, sizeof input, "unavailable input");
    ui_edit_ctrl_sync_from_ime(&edit);
    assert(!strcmp(edit.text.data, "입력 composition"));
    unavailable = 0;
    strcpy_s(input, sizeof input, "abcdef");
    language.anchor = language.caret = 100;
    ui_edit_ctrl_sync_from_ime(&edit);
    assert(language.anchor == 6 && language.caret == 6 && edit.sel_end == 6);
    edit.base.width = 10; edit.sel_end = 0;
    ui_edit_ctrl_sync_from_ime(&edit);
    assert(edit.scroll > 0 && edit.scroll <= edit.sel_end);
    language.anchor = language.caret = 0;
    ui_edit_ctrl_sync_from_ime(&edit);
    assert(!edit.scroll);
    ui_edit_ctrl_set_caret(&edit.base, 1000);
    assert(edit.sel_start == 6 && edit.sel_end == 6 && language.caret == 6);
    focus = NULL;
    string_assign_cstr(&edit.raw_text, "<NAVIL>경로</NAVIL>");
    assert(ui_edit_ctrl_link_text(&edit).data == edit.raw_text.data);
    assert(ui_edit_ctrl_link_text(&edit).size == strlen("<NAVIL>경로</NAVIL>"));
    string_free(&edit.raw_text); string_free(&edit.text);
    puts("Edit text: real setters, Korean character boundaries, aliasing, IME synchronization, retained capacity, selection and scrolling passed.");
    return 0;
}
