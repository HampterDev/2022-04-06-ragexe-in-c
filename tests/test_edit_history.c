#include "ui/window_access.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ui/controls/edit.h"
#include "ui/chat/chat_window.h"
#include <stdlib.h>

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
static void truncate_text(struct UIEditCtrl *edit) { ui_edit_ctrl_truncate_text(edit, 0); }
static struct UIWindowOps parent_ops = { .invalidate = invalidate };

static struct UIWindow parent = { .ops = &parent_ops };
static int focus_calls, lost_calls, destroyed, freed, popup_destroyed;
static int chosen = -1;
static struct UIWindow *chosen_sender;
static struct UIChatEditCtrl *heap_chat;
static struct UIWhisperEditCtrl *heap_whisper;

struct UIWindow *ui_edit_ctrl_construct(struct UIWindow *window)
{
    struct UIEditCtrl *edit = (struct UIEditCtrl *)window;
    memset(edit, 0, sizeof *edit);
    edit->base.width = 500; edit->base.parent = &parent; edit->max_chars = 255;
    return window;
}
void ui_edit_ctrl_destruct(struct UIEditCtrl *edit)
{
    ++destroyed;
    string_free(&edit->text); string_free(&edit->tooltip);
    string_free(&edit->raw_text);
}
void *mem_alloc(size_t size)
{
    void *pointer = malloc(size);
    assert(pointer);
    return pointer;
}
void mem_free(void *pointer)
{
    if (pointer == heap_chat || pointer == heap_whisper) ++freed;
    free(pointer);
}
void fatal_error(const char *message)
{
    (void)message;
    abort();
}
void ui_edit_ctrl_on_focus(struct UIWindow *window)
{
    struct UIEditCtrl *edit = (struct UIEditCtrl *)window;
    ++focus_calls;
    if (focus == &edit->base) {
        strcpy_s(input, sizeof input, string_cstr(&edit->text));
        language.input_length = strlen(input);
    }
}
void ui_edit_ctrl_on_focus_lost(struct UIWindow *window)
{
    struct UIEditCtrl *edit = (struct UIEditCtrl *)window; ++lost_calls; ui_edit_ctrl_sync_from_ime(edit); }
intptr_t ui_edit_ctrl_send_msg(struct UIWindow *window, struct UIWindow *sender,
                       int msg, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{
    struct UIEditCtrl *edit = (struct UIEditCtrl *)window; assert(edit && sender == &parent && msg == 999 && a == 1 && b == 2 && c == 3 && d == 4); return 123; }
static void destroy_popup(struct UIWindow *window)
{ (void)window; ++popup_destroyed; }
static intptr_t chat_message(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{
    assert(window == g_windowMgr.new_chat_wnd && msg == COMBO_MSG_CHOSEN && !b && !c && !d);
    chosen = a; chosen_sender = sender; return 0;
}
static struct UIWindowOps chat_window_ops = { .send_msg = chat_message };
static struct UIWindowOps popup_ops = { .destroy = destroy_popup };
const struct UIEditCtrlOps ui_chat_edit_ctrl_ops = {
    .base = {
        .destroy = ui_chat_edit_ctrl_destroy,
        .on_focus = ui_edit_ctrl_on_focus,
    },
    .set_text = ui_edit_ctrl_set_text_default,
    .truncate = truncate_text,
};
const struct UIEditCtrlOps ui_whisper_edit_ctrl_ops = {
    .base = {
        .destroy = ui_whisper_edit_ctrl_destroy,
        .on_focus = ui_whisper_edit_ctrl_on_focus,
    },
    .set_text = ui_whisper_edit_ctrl_set_text,
    .truncate = truncate_text,
};

static void chat_step(struct UIChatEditCtrl *edit, int direction)
{ assert(!ui_chat_edit_ctrl_send_msg((struct UIWindow *)(edit), NULL, direction < 0 ? 0x12 : 0x13, 0, 0, 0, 0)); }
static void whisper_step(struct UIWhisperEditCtrl *edit, int direction)
{ assert(!ui_whisper_edit_ctrl_send_msg((struct UIWindow *)(edit), NULL, direction < 0 ? 0x12 : 0x13, 0, 0, 0, 0)); }
static void current(struct UIEditCtrl *edit, const char *expected)
{
    if (strcmp(string_cstr(&edit->text), expected))
        fprintf(stderr, "Expected '%s', got '%s'\n", expected, string_cstr(&edit->text));
    assert(!strcmp(string_cstr(&edit->text), expected));
}
static void entry(struct TextArray *array, unsigned index, const char *expected)
{ assert(!strcmp(text_array_at(array, index).data, expected)); }
int main(void)
{
    struct UIChatEditCtrl chat;
    struct UIWhisperEditCtrl whisper;
    struct UINewChatWnd window = {0};
    struct UIWindow popup = { .ops = &popup_ops };
    char name[32]; unsigned i, old_index;

    ui_chat_edit_ctrl_construct(&chat);
    assert(!chat.history.size && chat.history.capacity == 30 && !chat.index && !chat.popup);
    chat_step(&chat, -1); chat_step(&chat, 1); ui_chat_edit_ctrl_commit(&chat);
    assert(!chat.history.size && !focus_calls);
    for (i = 0; i < 32; ++i) {
        sprintf_s(name, sizeof name, "line%u", i);
        ui_edit_ctrl_set_text(&chat.edit, name); ui_chat_edit_ctrl_commit(&chat);
        current(&chat.edit, "");
    }
    assert(chat.history.size == 30 && chat.index == 30);
    entry(&chat.history, 0, "line2"); entry(&chat.history, 29, "line31");
    ui_edit_ctrl_set_text(&chat.edit, "draft 이름");
    chat_step(&chat, -1); current(&chat.edit, "line31");
    chat_step(&chat, -1); current(&chat.edit, "line30");
    chat_step(&chat, 1); chat_step(&chat, 1); current(&chat.edit, "draft 이름");
    chat_step(&chat, 1); current(&chat.edit, "draft 이름");
    chat_step(&chat, -1); chat_step(&chat, -1);
    ui_edit_ctrl_set_text(&chat.edit, "replacement"); ui_chat_edit_ctrl_commit(&chat);
    assert(chat.history.size == 30 && chat.index == 29);
    entry(&chat.history, 28, "replacement"); current(&chat.edit, "line31");
    for (i = 0; i < 40; ++i) chat_step(&chat, -1);
    assert(!chat.index); current(&chat.edit, "line2");
    chat.index = chat.history.size;
    focus = &chat.edit.base;
    strcpy_s(input, sizeof input, "IME 입력"); language.input_length = strlen(input);
    ui_chat_edit_ctrl_commit(&chat); entry(&chat.history, 29, "IME 입력");
    current(&chat.edit, ""); focus = NULL;
    assert(ui_chat_edit_ctrl_send_msg((struct UIWindow *)(&chat), &parent, 999, 1, 2, 3, 4) == 123);
    chat.popup = (struct UIItemTagCtrl *)&popup;
    ui_chat_edit_ctrl_destruct(&chat);
    assert(!chat.history.items && !chat.typed.data && popup_destroyed == 1);

    ui_whisper_edit_ctrl_construct(&whisper);
    focus = &whisper.edit.base;
    ui_whisper_edit_ctrl_set_text(&whisper.edit, "");
    whisper_step(&whisper, -1); whisper_step(&whisper, 1);
    ui_whisper_edit_ctrl_on_focus_lost((struct UIWindow *)(&whisper)); assert(!whisper.history.size && !whisper.index);
    for (i = 0; i < 31; ++i) {
        ui_whisper_edit_ctrl_set_text(&whisper.edit, "");
        sprintf_s(name, sizeof name, "name%u", i);
        ui_whisper_edit_ctrl_set_text(&whisper.edit, name);
        ui_whisper_edit_ctrl_on_focus_lost((struct UIWindow *)(&whisper));
    }
    assert(whisper.history.size == 30 && whisper.index == 29);
    entry(&whisper.history, 0, "name1"); current(&whisper.edit, "name30");
    ui_whisper_edit_ctrl_set_text(&whisper.edit, "");
    ui_whisper_edit_ctrl_set_text(&whisper.edit, "name1");
    old_index = whisper.index; ui_whisper_edit_ctrl_on_focus_lost((struct UIWindow *)(&whisper));
    assert(whisper.index == old_index && whisper.history.size == 30);
    whisper_step(&whisper, -1); whisper_step(&whisper, -1);
    ui_whisper_edit_ctrl_set_text(&whisper.edit, "new while browsing");
    ui_whisper_edit_ctrl_on_focus_lost((struct UIWindow *)(&whisper));
    assert(whisper.history.size == 31 && whisper.index == 30);
    entry(&whisper.history, 30, "new while browsing");
    whisper_step(&whisper, 1); current(&whisper.edit, "");
    whisper_step(&whisper, 1); assert(whisper.index == 31);
    for (i = 0; i < 40; ++i) whisper_step(&whisper, -1);
    assert(!whisper.index); current(&whisper.edit, "name1");
    ui_whisper_edit_ctrl_on_focus((struct UIWindow *)(&whisper));
    assert(language.anchor == 0 && language.caret == 5);
    assert(ui_whisper_edit_ctrl_send_msg((struct UIWindow *)(&whisper), &parent, 999, 1, 2, 3, 4) == 123);

    window.frame.base.ops = &chat_window_ops;
    window.whisper = &whisper; g_windowMgr.new_chat_wnd = &window.frame.base;
    whisper.index = 0;
    ui_window_mgr_add_whisper_name(&g_windowMgr, "external");
    assert(whisper.history.size == 32 && !whisper.index);
    ui_window_mgr_add_whisper_name(&g_windowMgr, "external"); assert(whisper.history.size == 32);
    whisper.index = 32; ui_window_mgr_add_whisper_name(&g_windowMgr, "newest");
    assert(whisper.index == 33 && whisper.history.size == 33);
    ui_window_mgr_set_whisper_name(&g_windowMgr, "external");
    assert(chosen == 31 && whisper.index == 31 && chosen_sender == &whisper.edit.base);
    ui_window_mgr_set_whisper_name(&g_windowMgr, "new selection");
    assert(chosen == 33 && whisper.index == 33 && whisper.history.size == 34);
    ui_window_mgr_set_whisper_name(&g_windowMgr, NULL); assert(whisper.history.size == 34);
    g_windowMgr.new_chat_wnd = NULL;
    ui_window_mgr_add_whisper_name(&g_windowMgr, "ignored"); assert(whisper.history.size == 34);
    ui_whisper_edit_ctrl_destruct(&whisper); assert(!whisper.history.items);
    focus = NULL;

    heap_chat = malloc(sizeof *heap_chat); assert(heap_chat);
    heap_whisper = malloc(sizeof *heap_whisper); assert(heap_whisper);
    ui_chat_edit_ctrl_construct(heap_chat);
    ui_window_destroy(UI_WINDOW(heap_chat));
    ui_whisper_edit_ctrl_construct(heap_whisper);
    ui_window_destroy(UI_WINDOW(heap_whisper));
    assert(freed == 2 && destroyed == 4 && lost_calls >= 33);
    puts("Edit history: drafts, cap, browsing overwrite, duplicate names, IME, external additions and lifecycle passed.");
    return 0;
}
