#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "owned_storage_test_allocator.h"

#include "../src/ui/mail/rodex_contents.c"
#include "../src/ui/controls/edit_tag_colour.c"
#include "rodex_contents_unreached.h"

void *mem_alloc(size_t size) { return checked_malloc(size); }
void mem_free(void *memory)
{
    checked_free(memory);
}
void fatal_error(const char *message) { (void)message; abort(); }

static struct Rodex mailbox;
struct Rodex *g_rodex = &mailbox;
int g_rodexBodyMaxChars = 500;
static struct UIRodexContentsWnd *active_mail;
static struct { struct UIWindowOps window; void (*set_text)(struct UIWindow *, const char *); } edit_methods;
static struct UIWindowOps mail_methods;
static const char *expected_body;
static size_t expected_tags;
static int parsed_links, plain_text, added;
static struct UIItemTagButton *tags[2];
static struct UIItemTagButton untouched[2];

static void accept_body(struct UIWindow *window, const char *text)
{
    struct UIEditCtrl3 *edit = active_mail->body;
    size_t i;
    assert(window == &edit->edit.base);
    assert(!strcmp(text, expected_body));
    assert(edit->extra_line == 1 && edit->edit.disabled == 1);
    assert(window->id == ID_BODY && edit->edit.max_chars == 500);
    assert(window->x == 15 && window->y == 81);
    assert(window->width == 267 && window->height == 192);
    if (expected_tags) {
        edit->tag_buttons.items = checked_malloc(expected_tags * sizeof *edit->tag_buttons.items);
        for (i = 0; i < expected_tags; ++i) {
            tags[i] = checked_malloc(sizeof *tags[i]);
            memset(tags[i], 0x5c, sizeof *tags[i]);
            untouched[i] = *tags[i];
            edit->tag_buttons.items[i] = tags[i];
        }
    }
    edit->tag_buttons.size = edit->tag_buttons.capacity = expected_tags;
}
static void set_body_text(struct UIWindow *window, const char *text)
{ ++plain_text; accept_body(window, text); }
static int edit_message(struct UIWindow *window, struct UIWindow *sender, int message, int a, int b, int c, int d)
{
    assert(!sender && message == UI_MSG_PARSE_LINKS && !b && !c && !d);
    ++parsed_links;
    accept_body(window, (const char *)a);
    return 0;
}
void ui_window_move_base(struct UIWindow *window, int x, int y) { window->x = x; window->y = y; }
void ui_window_invalidate_base(struct UIWindow *window) { assert(window == &active_mail->body->edit.base); }
struct UIEditCtrl3 *ui_rodex_edit_ctrl3_construct(struct UIEditCtrl3 *edit)
{
    memset(edit, 0, sizeof *edit);
    edit->edit.base.ops = &edit_methods.window;
    edit->caret_eol = 0x39;
    return edit;
}
void ui_window_create(struct UIWindow *window, int width, int height)
{ window->width = width; window->height = height; }
void ui_edit_ctrl_set_back_colour(struct UIWindow *window, int r, int g, int b)
{ assert(window == &active_mail->body->edit.base && r == 0xbd && g == 0xce && b == 0xf7); }
void ui_edit_ctrl_set_text_colour(struct UIWindow *window, int r, int g, int b, int second)
{ assert(window == &active_mail->body->edit.base && r == 0x31 && g == 0x31 && b == 0x31 && second == -1); }
void ui_window_add_child(struct UIWindow *parent, struct UIWindow *child)
{ assert(parent == &active_mail->frame.base && child == &active_mail->body->edit.base); ++added; }
char *msg_str(int id) { assert(id == 42); return "편지 %s"; }
struct UIBasicButton *ui_basic_button_construct(struct UIBasicButton *button) { (void)button; abort(); }
BYTE ui_basic_button_set_label(struct UIWindow *window, int width, const char *text)
{ (void)window; (void)width; (void)text; abort(); }
struct UIBitmapButton *ui_bitmap_button_construct(struct UIBitmapButton *button) { (void)button; abort(); }
BYTE ui_bitmap_button_set_bitmap(struct UIBitmapButton *button, const char *path, int state, int flag)
{ (void)button; (void)path; (void)state; (void)flag; abort(); }

static struct String own(const char *text)
{
    struct String result;
    string_init(&result);
    string_assign_cstr(&result, text);
    return result;
}
static void check_mail(const char *title, const char *body, const char *expanded, size_t tag_count, int navigation, int nr)
{
    struct UIRodexContentsWnd window;
    size_t i;
    memset(&window, 0, sizeof window);
    active_mail = &window;
    window.frame.base.ops = &mail_methods;
    string_init(&window.sender);
    string_init(&window.title);
    expected_body = expanded;
    expected_tags = tag_count;
    added = parsed_links = plain_text = 0;
    ui_rodex_contents_wnd_set_mail(&window, own("보낸 사람"), own(title), own(body), 0, 0, MAIL_FROM_SERVER);
    assert(added == 1 && parsed_links == navigation && plain_text == !navigation);
    assert(window.no_reply == nr);
    assert(!strcmp(string_cstr(&window.sender), "보낸 사람"));
    assert(!strcmp(string_cstr(&window.title), "제목"));
    assert(window.body->extra_line == 1 && window.body->caret_eol == 0x39);
    assert(window.body->tag_buttons.size == tag_count && window.body->tag_buttons.capacity == tag_count);
    for (i = 0; i < tag_count; ++i) {
        untouched[i].text.normal_colour = RODEX_TAG_COLOUR;
        untouched[i].text.text_colour = RODEX_TAG_COLOUR;
        untouched[i].text.hover_colour = RODEX_TAG_COLOUR;
        assert(!memcmp(tags[i], &untouched[i], sizeof *tags[i]));
        checked_free(tags[i]);
    }
    checked_free(window.body->tag_buttons.items);
    checked_free(window.body);
    string_free(&window.sender);
    string_free(&window.title);
    assert(!outstanding);
}
int main(void)
{
    edit_methods.window.move = ui_window_move_base;
    edit_methods.window.invalidate = ui_window_invalidate_base;
    edit_methods.window.send_msg = edit_message;
    edit_methods.set_text = set_body_text;
    check_mail("제목", "", "", 0, 0, 0);
    check_mail("<NR>제목", "<MSG>42,도착</MSG>", "편지 도착", 0, 0, 1);
    check_mail("제목", "<ITEML>검</ITEML> <ITEML>방패</ITEML>",
               "<ITEML>검</ITEML> <ITEML>방패</ITEML>", 2, 0, 0);
    check_mail("제목", "<NAVIL>마을0102</NAVIL> <ITEML>검</ITEML>",
               "<NAVIL>마을0102</NAVIL> <ITEML>검</ITEML>", 1, 1, 0);
    puts("RoDEX mail setup: native extra-line flag, item-link colours, navigation and message expansion passed");
    return 0;
}
