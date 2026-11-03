#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "owned_storage_test_allocator.h"

static void *moving_realloc(void *old, size_t size)
{
    void *next = checked_malloc(size);
    assert(next);
    if (old) {
        size_t before = ((struct Allocation *)old - 1)->size;
        memcpy(next, old, before < size ? before : size);
        checked_free(old);
    }
    return next;
}
#define malloc checked_malloc
#define realloc moving_realloc
#define free checked_free
#include "../src/ui/controls/button.c"
#include "../src/ui/controls/bmp_text_button.c"
#include "../src/ui/character/job_icon_button.c"
#include "../src/ui/character/descript_race.c"
#include "../src/ui/items/refine_buttons.c"
#define service_path check_radio_service_path
#include "../src/ui/controls/check_radio.c"
#undef service_path
#include "../src/ui/controls/bmp_radio_btn.c"
#undef free
#undef realloc
#undef malloc
#include "button_drawing_unreached.h"
#include "race_button_unreached.h"

static struct BitmapRes resource;
static struct UIWindow parent;
static struct UIWindowOps parent_methods;
static struct BitmapRes *drawn_bitmap;
static unsigned loads, draws, labels, invalidations, destructors;
static unsigned clears, resizes, width_calls, height_calls;
static struct TextDraw { int x, y, height, font; COLORREF colour; char text[512]; } text_draws[4];
static char measured_text[512];
static int measured_length, measured_height, measured_font;
static int tinted, alpha, missing, expected_bitmap_y;
static char loaded_path[512];
static struct UIButton *visibility_button;
static BYTE expected_show;
static unsigned visibility_events, capture_releases;

void *mem_alloc(size_t size) { return checked_malloc(size); }
void mem_free(void *memory)
{
    checked_free(memory);
}
void fatal_error(const char *message) { (void)message; abort(); }

struct UIWindow *ui_window_construct(struct UIWindow *window, int composites)
{ assert(!composites); memset(window, 0, sizeof *window); return window; }
void ui_window_destruct(struct UIWindow *window) { assert(window); ++destructors; }
struct ResMgr *g_resMgr(void) { return (struct ResMgr *)&resource; }
void *res_mgr_get(struct ResMgr *manager, const char *path)
{
    assert(manager == g_resMgr());
    assert(strlen(path) < sizeof loaded_path);
    strcpy(loaded_path, path);
    ++loads;
    return missing ? NULL : &resource;
}
const char *ui_skin_path(const char *path) { return path; }
void ui_window_draw_bitmap(struct UIWindow *window, int x, int y, struct BitmapRes *bitmap, int keep_alpha)
{
    assert(window && !x && y == expected_bitmap_y);
    assert(!bitmap || bitmap == &resource);
    drawn_bitmap = bitmap;
    alpha = keep_alpha;
    ++draws;
}
void ui_window_draw_bitmap_tinted(struct UIWindow *window, int tint, int x, int y,
                               struct BitmapRes *bitmap, int keep_alpha, BYTE remake)
{
    assert(!tint && !remake);
    ++tinted;
    ui_window_draw_bitmap(window, x, y, bitmap, keep_alpha);
}
void ui_window_text_out(struct UIWindow *window, int x, int y, const char *text, int len,
                      int face, int height, COLORREF colour, int font_a, int font_b)
{
    struct TextDraw *draw;
    assert(window && !len && !face && !font_b && labels < 4);
    draw = &text_draws[labels];
    assert(strlen(text) < sizeof draw->text);
    draw->x = x; draw->y = y; draw->height = height; draw->colour = colour; draw->font = font_a;
    strcpy(draw->text, text);
    ++labels;
}
void ui_window_clear_canvas(struct UIWindow *window, DWORD colour)
{ assert(window && colour == UI_TRANSPARENT); ++clears; }
void ui_window_resize_base(struct UIWindow *window, int width, int height)
{ window->width = width; window->height = height; ++resizes; }
void ui_window_invalidate_base(struct UIWindow *window)
{
    assert(window); ++invalidations;
    if (visibility_button) {
        assert(window == &visibility_button->base);
        assert(visibility_button->disabled == (BYTE)(expected_show ^ 1));
        assert(window->visible == 0x57);
        visibility_events = visibility_events * 10 + 2;
    }
}
void ui_window_mgr_release_capture(struct UIWindowMgr *manager)
{
    assert(manager == &g_windowMgr && visibility_button && !expected_show);
    assert(visibility_button->disabled == 0x39 && visibility_button->base.visible == 0x57);
    visibility_events = visibility_events * 10 + 1;
    ++capture_releases;
}
int ui_window_text_width(struct UIWindow *window, const char *text, int len, int face, int height, int font, int extra)
{
    size_t size = len ? (size_t)len : strlen(text);
    assert(window && !face && !extra && size < sizeof measured_text);
    memcpy(measured_text, text, size); measured_text[size] = 0;
    measured_length = len; measured_height = height; measured_font = font; ++width_calls;
    return (int)size * 6;
}
int ui_window_text_height(struct UIWindow *window, const char *text, int len, int face, int height, int font, int extra)
{ assert(window && text && len >= 0 && !face && height > 0 && font >= 0 && !extra); ++height_calls; return 14; }
void *object_as_type(void *object, enum ObjectType type)
{ assert(type == OBJECT_BITMAP_RESOURCE && (!object || object == &resource)); return object; }
static void invalidate_parent(struct UIWindow *window) { assert(window == &parent); ++invalidations; }
static void reset_events(void)
{
    loads = draws = labels = invalidations = clears = resizes = width_calls = height_calls = 0;
    expected_bitmap_y = 0;
    tinted = alpha = 0; drawn_bitmap = NULL; loaded_path[0] = 0;
    memset(text_draws, 0, sizeof text_draws); measured_text[0] = 0;
}
static void expect_text(unsigned i, int x, int y, int height, COLORREF colour, int font, const char *text)
{
    const struct TextDraw *draw = &text_draws[i];
    assert(i < labels && draw->x == x && draw->y == y && draw->height == height);
    assert(draw->colour == colour && draw->font == font && !strcmp(draw->text, text));
}
static void test_stock(void)
{
    struct UIButton button;
    int state;
    const char *paths[] = {UI_SKIN "sml_button_up.bmp", UI_SKIN "sml_button_act.bmp", UI_SKIN "sml_button_down.bmp"};
    memset(&button, 0xa5, sizeof button);
    ui_button_construct(&button);
    assert(!button.skinned && !button.disabled && !button.cmd_value);
    string_assign_cstr(&button.text, "버튼");
    button.skinned = 1;
    for (state = -1; state <= 3; ++state) {
        int valid = state >= 0 && state < 3;
        button.base.state = state;
        reset_events();
        ui_button_on_draw((struct UIWindow *)(&button));
        assert(draws == 1 && labels == 1 && loads == (unsigned)valid && !alpha);
        expect_text(0, 0, 0, 12, 0, 0, "버튼");
        assert(drawn_bitmap == (valid ? &resource : NULL));
        if (valid) assert(!strcmp(loaded_path, paths[state]));
    }
    button.skinned = 0;
    reset_events();
    ui_button_on_draw((struct UIWindow *)(&button));
    assert(!draws && !labels && !loads);
    ui_button_destruct(&button);
}
static void test_bitmap_and_job_icon(void)
{
    struct UIJobIconButton button;
    const char *paths[] = {"기본.bmp", "강조.bmp", "누름.bmp", "사용불가.bmp"};
    int state, disabled, grey, i;
    ui_job_icon_button_construct(&button);
    assert(button.draw_mode == 0 && button.bmp.bitmap_width == 0 && button.bmp.bitmap_height == 0);
    button.bmp.button.base.parent = &parent;
    resource.width = 27; resource.height = 19;
    for (i = 0; i < 4; ++i) assert(ui_bitmap_button_set_bitmap(&button.bmp, paths[i], i, 0));
    assert(button.bmp.bitmap_width == 27 && button.bmp.bitmap_height == 19);
    resource.width = 8; resource.height = 11;
    assert(ui_bitmap_button_set_bitmap(&button.bmp, paths[0], 0, 0));
    assert(button.bmp.bitmap_width == 27 && button.bmp.bitmap_height == 19);
    assert(ui_bitmap_button_set_bitmap(&button.bmp, paths[0], 0, 1));
    assert(button.bmp.bitmap_width == 8 && button.bmp.bitmap_height == 11);
    resource.width = 4; resource.height = 6;
    assert(ui_bitmap_button_set_bitmap(&button.bmp, "ignored.bmp", -1, 0x100));
    assert(button.bmp.bitmap_width == 8 && button.bmp.bitmap_height == 11);
    assert(!strcmp(string_cstr(&button.bmp.bitmaps[0]), paths[0]));
    assert(ui_bitmap_button_set_bitmap(&button.bmp, "ignored.bmp", 99, 0x101));
    assert(button.bmp.bitmap_width == 4 && button.bmp.bitmap_height == 6);
    assert(ui_bitmap_button_set_bitmap(&button.bmp, string_cstr(&button.bmp.bitmaps[0]) + 2, 0, 0));
    assert(!strcmp(loaded_path, "본.bmp") && !strcmp(string_cstr(&button.bmp.bitmaps[0]), "본.bmp"));
    assert(ui_bitmap_button_set_bitmap(&button.bmp, paths[0], 0, 0));
    missing = 1;
    assert(!ui_bitmap_button_set_bitmap(&button.bmp, "missing.bmp", 0, 0));
    assert(!strcmp(string_cstr(&button.bmp.bitmaps[0]), paths[0]));
    missing = 0;
    ui_bitmap_button_set_tool_tip(&button.bmp, "상세 설명");
    ui_bitmap_button_set_tool_tip(&button.bmp, NULL);
    assert(!strcmp(string_cstr(&button.bmp.button.tooltip), "상세 설명"));
    ui_bitmap_button_set_tool_tip(&button.bmp, string_cstr(&button.bmp.button.tooltip) + 2);
    assert(!strcmp(string_cstr(&button.bmp.button.tooltip), "세 설명"));
    for (state = -1; state <= 3; ++state) for (disabled = 0; disabled < 2; ++disabled) {
        int valid = state >= 0 && state < 3;
        button.bmp.button.base.state = state;
        button.bmp.button.disabled = (BYTE)disabled;
        button.bmp.redraw_parent = 1;
        reset_events();
        ui_bitmap_button_on_draw((struct UIWindow *)(&button.bmp));
        assert(draws == 1 && loads == (unsigned)(valid + disabled) && !alpha);
        assert(drawn_bitmap == ((valid || disabled) ? &resource : NULL) && invalidations == 1);
        if (valid || disabled) assert(!strcmp(loaded_path, paths[disabled ? 3 : state]));
        for (grey = 0; grey < 2; ++grey) {
            button.draw_mode = grey;
            reset_events();
            ui_job_icon_button_on_draw((struct UIWindow *)(&button));
            assert(draws == 1 && loads == (unsigned)(valid + disabled) && alpha == 1 && tinted == grey);
            assert(drawn_bitmap == ((valid || disabled) ? &resource : NULL) && invalidations == 2);
            if (valid || disabled) assert(!strcmp(loaded_path, paths[disabled ? 3 : state]));
        }
    }
    button.bmp.button.disabled = 1;
    button.bmp.button.base.state = -1;
    string_clear(&button.bmp.bitmaps[3]);
    button.bmp.redraw_parent = 0;
    reset_events(); ui_job_icon_button_on_draw((struct UIWindow *)(&button));
    assert(!drawn_bitmap && !loads && invalidations == 1);
    button.bmp.button.base.parent = NULL;
    reset_events(); ui_bitmap_button_on_draw((struct UIWindow *)(&button.bmp));
    assert(!drawn_bitmap && !loads && !invalidations);
    button.bmp.button.base.state = 0;
    missing = 1;
    reset_events(); ui_job_icon_button_on_draw((struct UIWindow *)(&button));
    assert(!drawn_bitmap && loads == 1 && !invalidations);
    missing = 0;
    ui_bitmap_button_destruct(&button.bmp);
}
static void test_basic_label(void)
{
    struct UIBasicButton *button = ui_basic_button_construct(mem_alloc(sizeof *button));
    struct UIWindow *window = &button->button.base;
    reset_events();
    assert(!ui_basic_button_set_label(window, 100, NULL) && !resizes && !invalidations);
    assert(ui_basic_button_set_label(window, 100, "abcdef"));
    assert(resizes == 1 && invalidations == 1 && window->height == UI_BASIC_BUTTON_HEIGHT);
    reset_events();
    assert(ui_basic_button_set_label(window, 24, string_cstr(&button->label) + 2));
    assert(!strcmp(measured_text, "cdef") && window->width == 24 && invalidations == 1);
    reset_events();
    assert(!ui_basic_button_set_label(window, 5, "too wide"));
    assert(!strcmp(string_cstr(&button->label), "too wide") && resizes == 1 && !invalidations);
    assert(ui_basic_button_set_label(window, 0, ""));
    ui_basic_button_destroy((struct UIWindow *)(button));
}
static void test_bitmap_caption(void)
{
    struct UIBmpTextButton *button = ui_bmp_text_button_construct(mem_alloc(sizeof *button));
    struct UIWindow *window = &button->bmp.button.base;
    const char *paths[] = {"up.bmp", "hover.bmp", "down.bmp", "disabled.bmp"};
    const COLORREF shadows[] = {0xd7d7d7, 0xc7a796, 0xffffff, 0xe2e2e2};
    char source[] = "긴 제목의 버튼";
    char counted[] = {'a', 0, 'b', 'c'};
    int state, disabled, i;
    assert(!button->styles && !button->style_count && !button->style_capacity);
    reset_events(); ui_bmp_text_button_set_caption(button, text_view(source, sizeof source - 1));
    source[0] = '!';
    assert(!strcmp(string_cstr(&button->caption), "긴 제목의 버튼") && invalidations == 1);
    ui_bmp_text_button_set_caption(button, text_view(string_cstr(&button->caption) + 3, button->caption.size - 3));
    assert(!strcmp(string_cstr(&button->caption), "제목의 버튼"));
    ui_bmp_text_button_set_caption(button, text_view(counted, sizeof counted));
    assert(button->caption.size == sizeof counted && !memcmp(button->caption.data, counted, sizeof counted));
    ui_bmp_text_button_set_text_style(button, -7, 18, 1, 2, 0, UI_TEXT_CENTRED, 0);
    assert(measured_length == sizeof counted && !memcmp(measured_text, counted, sizeof counted));
    ui_bmp_text_button_set_caption(button, text_view_cstr("버튼"));
    window->width = 100; window->height = 40; window->parent = &parent; button->bmp.redraw_parent = 1;
    for (i = 0; i < 4; ++i) assert(ui_bitmap_button_set_bitmap(&button->bmp, paths[i], i, 0));
    for (state = -1; state <= 3; ++state) for (disabled = 0; disabled < 2; ++disabled) {
        int look = disabled ? 3 : state;
        int valid = look >= 0 && look < 4 && (disabled || look < 3);
        int shifted = look >= 2;
        window->state = state; button->bmp.button.disabled = (BYTE)disabled;
        reset_events(); ui_bmp_text_button_on_draw((struct UIWindow *)(button));
        assert(clears == 1 && invalidations == 1 && draws == (unsigned)valid && labels == (unsigned)(valid * 2));
        if (valid) {
            assert(!strcmp(loaded_path, paths[look]) && width_calls == 1 && height_calls == 1);
            expect_text(0, 38 + !shifted, 13 + !shifted, 12, shadows[look], 0, "버튼");
            expect_text(1, 38 + shifted, 13 + shifted, 12, disabled ? 0x6f6f6f : 0, 0, "버튼");
        }
    }
    ui_bmp_text_button_set_text_style(button, 0, 18, 0x112233, 2, 1, 10, 20);
    reset_events();
    ui_bmp_text_button_set_text_style(button, 1, 20, 0x223344, 3, 1, UI_TEXT_CENTRED, UI_TEXT_CENTRED);
    assert(width_calls == 1 && height_calls == 1 && measured_height == 20 && measured_font == 3 && measured_length == 4);
    ui_bmp_text_button_set_text_style(button, 2, 22, 0x334455, 4, 0, -4, -6);
    ui_bmp_text_button_set_text_style(button, 3, 24, 0x445566, 5, 1, 9, 11);
    for (i = 0; i < 20; ++i) ui_bmp_text_button_set_text_style(button, 100 + i, 12, 0, 0, 0, i, -i);
    assert(button->style_count == 25);
    reset_events();
    ui_bmp_text_button_set_text_style(button, 0, 99, 0xffffff, 0, 0, UI_TEXT_CENTRED, UI_TEXT_CENTRED);
    assert(button->style_count == 25 && width_calls == 1 && height_calls == 1);
    for (state = 0; state < 4; ++state) {
        window->state = state == 3 ? -1 : state;
        button->bmp.button.disabled = (BYTE)(state == 3);
        reset_events(); ui_bmp_text_button_on_draw((struct UIWindow *)(button));
        assert(clears == 1 && invalidations == 1 && draws == 1 && !width_calls && !height_calls);
        assert(!strcmp(loaded_path, paths[state]));
        switch (state) {
        case 0: assert(labels == 2); expect_text(0, 11, 21, 18, shadows[0], 2, "버튼"); expect_text(1, 10, 20, 18, 0x112233, 2, "버튼"); break;
        case 1: assert(labels == 2); expect_text(0, 39, 14, 20, shadows[1], 3, "버튼"); expect_text(1, 38, 13, 20, 0x223344, 3, "버튼"); break;
        case 2: assert(labels == 1); expect_text(0, -3, -5, 22, 0x334455, 4, "버튼"); break;
        case 3: assert(labels == 2); expect_text(0, 9, 11, 24, shadows[3], 0, "버튼"); expect_text(1, 10, 12, 24, 0x445566, 5, "버튼"); break;
        }
    }
    ui_bmp_text_button_set_caption(button, text_view(NULL, 0));
    reset_events(); ui_bmp_text_button_on_draw((struct UIWindow *)(button));
    assert(draws == 1 && !labels && !width_calls && !height_calls);
    ui_bmp_text_button_destroy((struct UIWindow *)(button));
}
static void test_race_button_factory(void)
{
    int race;
    for (race = RACE_HUMAN; race <= RACE_DORAM; ++race) {
        struct UIWindow *window = ui_descript_race_create(race);
        struct UIDescriptRace *button = (struct UIDescriptRace *)window;
        assert(window && ((struct Allocation *)button - 1)->size == sizeof *button);
        assert(button->race == race && button->frame == 0 && button->animating == (race == RACE_HUMAN));
        assert(!button->base.button.skinned);
        reset_events();
        g_session1.doramMakeable = 0;
        ui_descript_race_send_msg((struct UIWindow *)(button), NULL, DESCRIPT_RACE_MSG_CHOOSE, 1, 0, 0, 0);
        assert(loads == (unsigned)(race == RACE_HUMAN));
        g_session1.doramMakeable = 1;
        ui_descript_race_send_msg((struct UIWindow *)(button), NULL, DESCRIPT_RACE_MSG_CHOOSE, 1, 0, 0, 0);
        assert(button->animating && strstr(loaded_path, race ? "img_doram_on.bmp" : "img_human_on.bmp"));
        ui_descript_race_send_msg((struct UIWindow *)(button), NULL, DESCRIPT_RACE_MSG_CHOOSE, 0, 0, 0, 0);
        assert(!button->animating && strstr(loaded_path, race ? "img_doram_off.bmp" : "img_human_off.bmp"));
        ui_descript_race_destroy((struct UIWindow *)(button));
    }
    fail_at = allocation_calls + 1;
    assert(!ui_descript_race_create(RACE_HUMAN));
    fail_at = 0;
}

static void test_button_visibility(void)
{
    const struct UIButtonOps refine_ops = {
        .base = { .invalidate = ui_window_invalidate_base },
        .set_show = ui_refine_button_set_show
    };
    struct UIButton button;
    unsigned variant, value;
    ui_button_construct(&button);
    visibility_button = &button;
    for (variant = 0; variant < 2; ++variant) {
        button.base.ops = variant ? &refine_ops.base : &ui_button_ops.base;
        for (value = 0; value < 256; ++value) {
            unsigned before = invalidations;
            expected_show = (BYTE)value;
            visibility_events = capture_releases = 0;
            button.disabled = 0x39;
            button.base.visible = 0x57;
            ui_button_show(&button, expected_show);
            assert(button.disabled == (BYTE)(value ^ 1) && button.base.visible == value);
            assert(invalidations == before + 1);
            assert(capture_releases == (unsigned)(variant && !value));
            assert(visibility_events == (variant && !value ? 12u : 2u));
        }
    }
    visibility_button = NULL;
    ui_button_destruct(&button);
    assert(!outstanding);
}

#include "check_radio_cases.h"

int main(void)
{
    parent_methods.invalidate = invalidate_parent;
    parent.ops = &parent_methods;
    test_stock();
    test_bitmap_and_job_icon();
    test_basic_label();
    test_bitmap_caption();
    test_race_button_factory();
    test_native_check_box();
    test_native_radio();
    test_native_bitmap_radio();
    assert(!outstanding && destructors == 10);
    test_button_visibility();
    puts("Button drawing: stock, bitmap, job icon, disabled, missing resource and invalid state passed");
    puts("Button text: owned captions, substring aliases, labels, counted text and growing state styles passed");
    puts("Race buttons: native allocation, initialization, race availability, selection and destruction passed");
    puts("Checkbox/radio controls: owned text, typed styles, borrowed aliases, drawing, notices, tooltips, factory failure and guarded lifetimes passed");
    puts("Button operations: typed dispatch, all 256 byte values, capture-before-disable and invalidation-before-visibility passed");
    return 0;
}
