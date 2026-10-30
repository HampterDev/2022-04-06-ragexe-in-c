#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <zlib.h>
#include "owned_storage_test_allocator.h"
#define malloc checked_malloc
#define free checked_free
#define mem_alloc window_alloc
#include "../src/ui/chat/captcha_data.c"
#include "../src/ui/chat/captcha.c"
#include "../src/resources/bitmap_storage.c"
#include "../src/resources/bitmap_codecs.c"
#undef mem_alloc
#undef free
#undef malloc
#include "captcha_test_bitmap.h"
#include "captcha_window_test_boundaries.h"

static unsigned shown, refreshed, sent;
static int send_success = 1, sent_size;
static unsigned char sent_packet[20];
static const char *edit_text;
static void show(struct UIWindow *w, int visible) { w->visible = visible; shown += visible != 0; }
static void refresh(struct UIWindow *w) { (void)w; ++refreshed; }
static struct UIWindowOps window_ops = {.set_show = show, .invalidate = refresh};

static unsigned edges, string_blocks, closes;
static void *string_allocations[128];
static int drawn_key;
static struct UIWindow *focused_edit;
static struct { struct UIWindow *parent, *child; } children[16];
static void control_destroy(struct UIWindow *window)
{
    ui_window_destruct(window);
    checked_free(window);
}
static void text_destroy(struct UIWindow *window)
{
    struct UIStaticText *text = (struct UIStaticText *)window;
    string_free(&text->tooltip);
    control_destroy(window);
}
static struct UIWindowOps control_ops = {.destroy = control_destroy, .move = ui_window_move_base, .invalidate = refresh};
static struct UIWindowOps text_ops = {.destroy = text_destroy, .move = ui_window_move_base, .invalidate = refresh};
void *window_alloc(size_t bytes) { return checked_malloc(bytes); }
void *mem_alloc(size_t size) {
    unsigned i;
    void *p = malloc(size);
    assert(p);
    for (i = 0; i < 128 && string_allocations[i]; ++i) {}
    assert(i < 128);
    string_allocations[i] = p;
    ++string_blocks;
    return p;
}
void mem_free(void *p)
{
    unsigned i;
    if (!p) return;
    for (i = 0; i < 128 && string_allocations[i] != p; ++i) {}
    if (i < 128) {
        assert(string_blocks);
        string_allocations[i] = NULL;
        --string_blocks;
        free(p);
    } else {
        checked_free(p);
    }
}
void fatal_error(const char *message) { (void)message; abort(); }
struct UIWindow *ui_window_construct(struct UIWindow *w, int composites)
{ (void)composites; memset(w, 0, sizeof *w); w->ops = &control_ops; return w; }
struct UIFrameWnd *ui_frame_wnd_construct(struct UIFrameWnd *w, int composites)
{ memset(w, 0, sizeof *w); ui_window_construct(&w->base, composites); return w; }
void ui_window_create(struct UIWindow *w, int width, int height)
{
    w->width = width; w->height = height;
    if (w->ops->on_create) ui_window_on_create(UI_WINDOW(w), width, height);
}
void ui_window_add_child(struct UIWindow *w, struct UIWindow *child)
{ assert(edges < 16); children[edges].parent = w; children[edges++].child = child; }
void ui_window_destruct(struct UIWindow *w)
{
    unsigned i = 0;
    while (i < edges) {
        if (children[i].parent == w) {
            struct UIWindow *child = children[i].child;
            children[i] = children[--edges]; ui_window_destroy(UI_WINDOW(child));
        } else ++i;
    }
}
void ui_frame_wnd_destruct(struct UIFrameWnd *w) { ui_window_destruct(&w->base); }
void ui_window_move_base(struct UIWindow *w, int x, int y) { w->x = x; w->y = y; }
void ui_frame_wnd_move(struct UIWindow *window, int x, int y) {
    struct UIFrameWnd *w = (struct UIFrameWnd *)window; ui_window_move_base(&w->base, x, y); }
void ui_window_invalidate_base(struct UIWindow *w) { refresh(w); }
void ui_window_set_show_base(struct UIWindow *w, int visible) { show(w, visible); }
struct UIWindow *ui_window_add_sys_button(struct UIWindow *w) { return w; }
struct UIWindow *ui_window_add_close_button(struct UIWindow *w, const char *tip, int width)
{ (void)tip; (void)width; return w; }
BYTE ui_window_mgr_post_quit1(struct UIWindowMgr *w, int id) { (void)w; assert(id == 0x9b || id == 0x27); ++closes; return 1; }
void ui_window_draw_bitmap(struct UIWindow *w, int x, int y, struct BitmapRes *bitmap, int keyed)
{ assert(w && !x && !y && bitmap); drawn_key = keyed; }
void ui_window_mgr_set_focus_edit(struct UIWindowMgr *w, struct UIWindow *edit) { (void)w; assert(edit); focused_edit = edit; }
struct UIWindow *ui_static_text_construct(struct UIWindow *w)
{
    ui_window_construct(w, 0); w->ops = &text_ops;
    string_init(&((struct UIStaticText *)w)->tooltip); return w;
}
void ui_static_text_set_text_ellipsis(struct UIWindow *w, const char *text)
{ string_assign_cstr(&((struct UIStaticText *)w)->tooltip, text); }
struct UIWindow *ui_combo_box_def_wnd_construct(struct UIWindow *w) { return ui_window_construct(w, 0); }
struct UIWindow *ui_edit_ctrl_construct(struct UIWindow *w) { return ui_window_construct(w, 0); }
void ui_edit_ctrl_set_back_colour(struct UIWindow *w, int r, int g, int b) { (void)w; (void)r; (void)g; (void)b; }
void ui_edit_ctrl_set_border_colour(struct UIWindow *w, int r, int g, int b) { (void)w; (void)r; (void)g; (void)b; }
void ui_edit_ctrl_set_text_colour(struct UIWindow *w, int r, int g, int b, int extra)
{ (void)w; (void)r; (void)g; (void)b; (void)extra; }
struct UIBitmapButton *ui_bitmap_button_construct(struct UIBitmapButton *b)
{ ui_window_construct(&b->button.base, 0); b->bitmap_width = b->bitmap_height = 16; return b; }
BYTE ui_bitmap_button_set_bitmap(struct UIBitmapButton *b, const char *path, int state, int flag)
{ (void)b; (void)path; (void)state; (void)flag; return 1; }
void list_init(struct List *list)
{
    struct ListNode *head = malloc(sizeof *head);
    assert(head);
    head->next = head->prev = head;
    list->head = head;
    list->size = 0;
}
void list_free(struct List *list, ListDestroyFn destroy)
{
    assert(destroy);
    if (!list->head) return;
    assert(list->head->next == list->head && !list->size);
    free(list->head);
    list->head = NULL;
}
void dir_list_names_list(struct List *list, const char *folder, const char *pattern)
{ assert(list->head && folder && !strcmp(pattern, "*.bmp")); }
char *msg_str(int id) { return id == MSG_CAPTCHA_CONFIRM ? "Send %s?" : "%d"; }
const char *ui_edit_ctrl_get_text(struct UIWindow *w) { assert(w); return edit_text; }
int ui_edit_ctrl_get_text_size(struct UIWindow *w) { assert(w); return (int)strlen(edit_text); }
int ui_window_mgr_message_box(struct UIWindowMgr *w, const char *text, int type, int a, int b, int c,
                          const char *title, int width, int height, int d)
{ (void)w; (void)text; (void)type; (void)a; (void)b; (void)c; (void)title; (void)width; (void)height; (void)d; return 0; }
int ui_window_send_msg_base(struct UIWindow *w, struct UIWindow *sender, int msg, int a, int b, int c, int d)
{ (void)w; (void)sender; (void)msg; (void)a; (void)b; (void)c; (void)d; return 0; }
int ui_frame_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int a, int b, int c, int d)
{
    struct UIFrameWnd *w = (struct UIFrameWnd *)window; return ui_window_send_msg_base(&w->base, sender, msg, a, b, c, d); }
void language_cancel_input(struct Language *language) { (void)language; }

struct BitmapRes *bitmap_resource_init(struct BitmapRes *b)
{ memset(b, 0, sizeof *b); b->res.ops = NULL; return b; }

struct RagConnection *rag_connection_instance(void) { return (struct RagConnection *)1; }
char rag_connection_send_packet(struct RagConnection *c, int size, const void *data)
{
    const unsigned char *p = data;
    assert(c == (void *)1 && size <= sizeof sent_packet && p[1] == 0x0a);
    assert((size == 2 && p[0] == 0x5a) || (size == 18 && p[0] == 0x5c) || (size == 20 && p[0] == 0x52));
    sent_size = size; memcpy(sent_packet, data, size);
    if (!send_success) return 0;
    ++sent; return 1;
}

static void test_answers(void)
{
    struct UICaptchaRegisterWnd registration = {0};
    struct UIMacroDetectorWnd detector = {0};
    struct UIBitmapButton button = {0};
    struct UIWindow edit = {0};
    unsigned char packed = 1;
    unsigned i, before = sent;
    registration.frame.base.ops = detector.frame.base.ops = button.button.base.ops = &window_ops;
    registration.answer_edit = detector.answer_edit = &edit;
    registration.send_button = &button;
    registration.packed.bytes = &packed; registration.packed.size = 0x1234;
    string_init(&registration.file); string_assign_cstr(&registration.file, "test.bmp");
    string_init(&detector.last_answer);
    edit_text = "ABCD"; send_success = 0;
    captcha_register_send(&registration);
    assert(!button.button.disabled && sent == before);
    send_success = 1; captcha_register_send(&registration);
    assert(button.button.disabled && sent_size == 20 && sent == before + 1);
    assert(!memcmp(sent_packet + 2, "ABCD", 4) && sent_packet[18] == 0x34 && sent_packet[19] == 0x12);
    for (i = 6; i < 18; ++i) assert(sent_packet[i] == 0);
    captcha_register_send(&registration); assert(sent == before + 1);
    edit_text = "A"; send_success = 0;
    ui_macro_detector_wnd_send_msg(&detector.frame.base, NULL, UI_MSG_COMMAND, CAPTCHA_ID_SEND, 0, 0, 0);
    assert(!detector.last_answer.size && sent == before + 1);
    send_success = 1;
    ui_macro_detector_wnd_send_msg(&detector.frame.base, NULL, UI_MSG_COMMAND, CAPTCHA_ID_SEND, 0, 0, 0);
    assert(sent_size == 18 && sent_packet[2] == 'A');
    for (i = 3; i < 18; ++i) assert(!sent_packet[i]);
    assert(detector.last_answer.size == 1 && sent == before + 2);
    ui_macro_detector_wnd_send_msg(&detector.frame.base, NULL, UI_MSG_COMMAND, CAPTCHA_ID_SEND, 0, 0, 0);
    assert(sent == before + 2);
    string_free(&detector.last_answer); string_free(&registration.file);
    assert(!outstanding && !string_blocks);
}

static void receive(struct UIWindow *window, const unsigned char key[4], const struct CaptchaData *packed, int detector)
{
    size_t offset = 0;
    while (offset < packed->size) {
        size_t size = packed->size - offset;
        if (size > 113) size = 113;
        if (detector) captcha_detector_append(window, key, packed->bytes + offset, size);
        else captcha_preview_append(window, key, packed->bytes + offset, size);
        offset += size;
    }
}

static void test_windows(int custom, int bits, unsigned palette)
{
    const unsigned char key[4] = {0, 1, 0, 2}, other_key[4] = {0, 1, 0, 3};
    struct UICaptchaPreviewWnd preview = {0};
    struct UIMacroDetectorWnd detector = {0};
    struct UICaptchaImage image = {0};
    struct CaptchaData packed = {0};
    size_t size;
    unsigned char *raw = make_bitmap(custom, bits, palette, &size), *old_packed;
    DWORD *old_pixels;
    unsigned old_sent;
    preview.frame.base.ops = detector.frame.base.ops = image.base.ops = &window_ops;
    preview.image = detector.image = &image;
    bitmap_resource_init(&image.image);
    assert(captcha_data_pack(&packed, raw, size));
    assert(captcha_preview_begin(&preview.frame.base, key, packed.size));
    old_packed = preview.stream.packed.bytes;
    fail_at = allocation_calls + 1;
    assert(!captcha_preview_begin(&preview.frame.base, other_key, packed.size));
    fail_at = 0;
    assert(preview.stream.packed.bytes == old_packed && !memcmp(preview.key, key, 4));
    captcha_preview_append(&preview.frame.base, other_key, packed.bytes, 1);
    assert(!preview.stream.received);
    receive(&preview.frame.base, key, &packed, 0);
    assert(preview.frame.base.visible && image.image.pixels);
    assert(image.image.width == 220 && image.image.height == 90);
    old_pixels = image.image.pixels;
    assert(captcha_preview_begin(&preview.frame.base, key, packed.size));
    assert(!preview.stream.received);
    packed.bytes[packed.size - 1] ^= 1;
    receive(&preview.frame.base, key, &packed, 0);
    assert(!preview.frame.base.visible && image.image.pixels == old_pixels);
    packed.bytes[packed.size - 1] ^= 1;
    assert(captcha_preview_begin(&preview.frame.base, key, packed.size));
    fail_at = allocation_calls + 1;
    receive(&preview.frame.base, key, &packed, 0);
    fail_at = 0;
    assert(image.image.pixels == old_pixels && !preview.frame.base.visible);
    assert(captcha_detector_begin(&detector.frame.base, key, packed.size));
    old_sent = sent;
    captcha_detector_append(&detector.frame.base, other_key, packed.bytes, 1);
    assert(!detector.stream.received);
    packed.bytes[packed.size - 1] ^= 1;
    receive(&detector.frame.base, key, &packed, 1);
    assert(sent == old_sent && image.image.pixels == old_pixels);
    packed.bytes[packed.size - 1] ^= 1;
    assert(captcha_detector_begin(&detector.frame.base, key, packed.size));
    receive(&detector.frame.base, key, &packed, 1);
    assert(sent == old_sent + 1 && image.image.pixels != old_pixels);
    captcha_detector_append(&detector.frame.base, key, packed.bytes, 1);
    assert(sent == old_sent + 1);
    captcha_stream_clear(&preview.stream); captcha_stream_clear(&detector.stream);
    captcha_data_clear(&packed); bitmap_resource_clear(&image.image); free(raw);
    assert(!outstanding);
}

int main(void)
{
    int custom;
    struct UIWindow *(*factories[])(void) = {captcha_preview_create, captcha_register_create, captcha_detector_create};
    size_t i;
    for (i = 0; i < sizeof factories / sizeof *factories; ++i) {
        struct UIWindow *w;
        unsigned count, before = allocation_calls, failure, old_closes = closes;
        struct UICaptchaImage *image;
        focused_edit = NULL;
        w = factories[i](); assert(w && edges); count = allocation_calls - before;
        if (i == 2) assert(focused_edit == captcha_detector_edit(w) && closes == old_closes + 2);
        image = i == 0 ? ((struct UICaptchaPreviewWnd *)w)->image
            : i == 1 ? ((struct UICaptchaRegisterWnd *)w)->image : ((struct UIMacroDetectorWnd *)w)->image;
        ui_window_on_draw(UI_WINDOW(image)); assert(drawn_key == (i != 1));
        old_closes = closes;
        ui_window_destroy(UI_WINDOW(w)); assert(!edges && !outstanding && !string_blocks);
        for (failure = 1; failure <= count; ++failure) {
            focused_edit = NULL;
            fail_at = allocation_calls + failure;
            w = factories[i](); fail_at = 0;
            assert(!w && !edges && !outstanding && !string_blocks && !focused_edit && closes == old_closes);
        }
    }
    for (custom = 0; custom <= 1; ++custom) {
        test_windows(custom, 24, 0);
        test_windows(custom, 8, 0);
        test_windows(custom, 8, 16);
    }
    assert(shown == 6 && sent == 6 && refreshed >= 12);
    test_answers();
    puts("Captcha windows: factory failure cleanup, binary keys, real BMP/zlib decoding, replacement rollback and acknowledgments passed");
    return 0;
}
