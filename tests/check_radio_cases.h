#include "world/tooltip.h"
static unsigned notices, tooltip_calls, mouse_moves, show_calls, cursor_calls;
static int notice_msg[4], notice_a[4], notice_b[4], tooltip_x, tooltip_y, shown;
static char tooltip_text[512];

struct Mode *client_current_mode(void) { return (struct Mode *)&parent; }
void mode_show_tooltip(struct Mode *mode, const char *text, int x, int y, DWORD colour, BYTE flag)
{
    assert(mode == client_current_mode() && colour == 0xffffffff && !flag);
    assert(strlen(text) < sizeof tooltip_text);
    strcpy(tooltip_text, text); tooltip_x = x; tooltip_y = y; ++tooltip_calls;
}
void mode_set_cursor_action(struct Mode *mode, int action)
{ assert(mode == client_current_mode() && action == CURSOR_HAND); ++cursor_calls; }
void ui_window_to_screen(struct UIWindow *wnd, int *x, int *y)
{ *x += wnd->x; *y += wnd->y; }
struct UIWindow *ui_window_get_ancestor(struct UIWindow *wnd) { return wnd->parent; }
void ui_window_set_show_base(struct UIWindow *wnd, int show)
{ assert(wnd); shown = show; ++show_calls; }
void ui_window_on_mouse_move_base(struct UIWindow *wnd, int x, int y)
{ assert(wnd); (void)x; (void)y; ++mouse_moves; }
void ui_window_on_l_btn_dbl_clk_base(struct UIWindow *wnd, int x, int y) { abort(); }
void ui_window_track_mouse_base(struct UIWindow *wnd, int x, int y) { abort(); }
void ui_window_on_l_btn_up_base(struct UIWindow *wnd, int x, int y) { abort(); }
static intptr_t receive_notice(struct UIWindow *wnd, struct UIWindow *sender, int msg, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{
    assert(wnd == &parent && sender && notices < 4 && !c && !d);
    notice_msg[notices] = msg; notice_a[notices] = a; notice_b[notices] = b;
    ++notices;
    return 0;
}
static void reset_control_events(void)
{
    reset_events(); notices = tooltip_calls = mouse_moves = show_calls = cursor_calls = 0;
    tooltip_text[0] = 0;
}
static void test_native_check_box(void)
{
    struct UICheckBox *box = ui_check_box_construct(mem_alloc(sizeof *box));
    struct UIWindow *wnd = &box->base;
    char label[] = "선택 항목";
    assert(!box->disabled && !box->no_draw && !box->send_notice && box->notice_value == -1);
    assert(!box->label.size && !box->checked_bitmap.size && !box->unchecked_tooltip.size);
    assert(wnd->id == UI_ID_CHECKBOX && !wnd->state && box->label_height == 12);
    wnd->parent = &parent; wnd->height = 40; wnd->x = 17; wnd->y = 30;
    parent_methods.send_msg = receive_notice;
    resource.width = 27; resource.height = 18;
    reset_control_events();
    ui_check_box_set_bitmaps(wnd, "선택.bmp", "해제.bmp", label);
    label[0] = '!';
    assert(!strcmp(string_cstr(&box->label), "선택 항목"));
    assert(wnd->width == 27 + (int)strlen("선택 항목") * 6 + 3 && loads == 3 && resizes == 1);
    ui_check_box_set_bitmaps(wnd, string_cstr(&box->checked_bitmap) + 2,
                         string_cstr(&box->unchecked_bitmap), string_cstr(&box->label) + 2);
    assert(!strcmp(string_cstr(&box->checked_bitmap), "택.bmp"));
    assert(!strcmp(string_cstr(&box->label), "택 항목"));
    reset_control_events(); expected_bitmap_y = 11;
    ui_check_box_on_draw((struct UIWindow *)(box));
    assert(draws == 1 && labels == 1 && !clears && !strcmp(loaded_path, "해제.bmp"));
    expect_text(0, 30, 0, 12, 0, 0, "택 항목");
    box->disabled = 1;
    reset_control_events(); ui_check_box_on_draw((struct UIWindow *)(box)); ui_check_box_on_l_btn_down((struct UIWindow *)(box), 0, 0);
    ui_check_box_set_check(wnd, 1); ui_check_box_on_mouse_shape((struct UIWindow *)(box), 0, 0); ui_check_box_on_mouse_hover((struct UIWindow *)(box), 0, 0);
    assert(clears == 1 && !draws && !notices && !wnd->state && !mouse_moves && !cursor_calls);
    box->disabled = 0;
    reset_control_events(); ui_check_box_set_hidden(wnd, 2); ui_check_box_on_draw((struct UIWindow *)(box));
    assert(box->no_draw == 2 && shown == 3 && show_calls == 1 && invalidations == 1 && !draws);
    ui_check_box_set_hidden(wnd, 0);
    ui_check_box_set_tool_tip(wnd, "일반 도움말");
    ui_check_box_set_tool_tip(wnd, NULL);
    string_assign_cstr(&box->unchecked_tooltip, "선택 전");
    reset_control_events(); ui_check_box_on_mouse_hover((struct UIWindow *)(box), 3, 4);
    assert(mouse_moves == 1 && tooltip_calls == 1 && tooltip_x == 17 && tooltip_y == 10);
    assert(!strcmp(tooltip_text, "선택 전"));
    box->send_notice = 1; box->notice_value = 314;
    reset_control_events(); ui_check_box_on_l_btn_down((struct UIWindow *)(box), 0, 0);
    assert(wnd->state == 1 && invalidations == 2 && notices == 2);
    assert(notice_msg[0] == UI_MSG_COMMAND && notice_a[0] == UI_ID_CHECKBOX && notice_b[0] == 1);
    assert(notice_msg[1] == UI_MSG_CHECK_NOTICE && notice_a[1] == 314);
    reset_control_events(); ui_check_box_on_mouse_hover((struct UIWindow *)(box), 0, 0); ui_check_box_on_mouse_shape((struct UIWindow *)(box), 0, 0);
    assert(!strcmp(tooltip_text, "일반 도움말") && cursor_calls == 1);
    reset_control_events(); ui_check_box_set_check_quiet(box, 0);
    assert(wnd->state == 0 && !notices && invalidations == 2);
    reset_control_events(); ui_check_box_set_check(wnd, 1);
    assert(wnd->state == 1 && notices == 1 && notice_b[0] == 1);
    reset_control_events(); ui_check_box_set_bitmaps_fit(box, "fit-on.bmp", "fit-off.bmp");
    assert(wnd->width == 27 && wnd->height == 18 && loads == 2);
    ui_check_box_set_bitmaps(wnd, "fit-on.bmp", "fit-off.bmp", NULL);
    assert(!box->label.size);
    string_clear(&box->tooltip);
    reset_control_events(); ui_check_box_on_mouse_hover((struct UIWindow *)(box), 0, 0);
    assert(mouse_moves == 1 && !tooltip_calls);
    ui_check_box_destroy((struct UIWindow *)(box));
}
static void test_native_radio(void)
{
    struct UIRadioBtn *radio = ui_radio_btn_construct(mem_alloc(sizeof *radio));
    struct UIWindow *wnd = &radio->base;
    char label[] = "항목 선택";
    assert(wnd->id == UI_ID_RADIO && !wnd->state && radio->enabled == 1 && radio->value == 0);
    assert(!radio->label.size && !radio->tooltip.size && radio->label_height == 12);
    wnd->parent = &parent; wnd->width = 100; wnd->height = 40; wnd->x = 17; wnd->y = 30;
    ui_radio_btn_set_label(wnd, label); label[0] = '!';
    assert(!strcmp(string_cstr(&radio->label), "항목 선택"));
    ui_radio_btn_set_label(wnd, string_cstr(&radio->label) + 2);
    reset_control_events(); ui_radio_btn_on_draw((struct UIWindow *)(radio));
    assert(draws == 1 && clears == 1 && !strcmp(loaded_path, UI_SKIN "radiobtn_off.bmp"));
    expect_text(0, 13, 0, 12, 0, 0, "목 선택");
    wnd->state = 1; radio->alt_on_bitmap = 1; radio->enabled = 0;
    reset_control_events(); ui_radio_btn_on_draw((struct UIWindow *)(radio)); ui_radio_btn_on_l_btn_down((struct UIWindow *)(radio), 0, 0);
    assert(!strcmp(loaded_path, UI_SKIN "radiobtn_on1.bmp") && !notices);
    expect_text(0, 13, 0, 12, UI_GREY_TEXT, 0, "목 선택");
    string_assign_cstr(&radio->on_bitmap, "켜짐.bmp");
    string_assign_cstr(&radio->off_bitmap, "꺼짐.bmp");
    resource.width = 15; resource.height = 20;
    reset_control_events(); expected_bitmap_y = 10; ui_radio_btn_on_draw((struct UIWindow *)(radio));
    assert(draws == 1 && alpha == 1 && !strcmp(loaded_path, "켜짐.bmp"));
    expect_text(0, 18, 13, 12, UI_GREY_TEXT, 0, "목 선택");
    radio->enabled = 2;
    reset_control_events(); expected_bitmap_y = 10; ui_radio_btn_on_draw((struct UIWindow *)(radio));
    expect_text(0, 18, 13, 12, UI_GREY_TEXT, 0, "목 선택");
    radio->enabled = 1; radio->value = 77;
    reset_control_events(); ui_radio_btn_on_l_btn_down((struct UIWindow *)(radio), 0, 0);
    assert(notices == 1 && notice_msg[0] == UI_MSG_COMMAND && notice_a[0] == UI_ID_RADIO && notice_b[0] == 77);
    string_assign_cstr(&radio->tooltip, "설명");
    reset_control_events(); ui_radio_btn_on_mouse_hover((struct UIWindow *)(radio), 0, 0);
    assert(mouse_moves == 1 && tooltip_calls == 1 && tooltip_x == 27 && tooltip_y == 10 && !strcmp(tooltip_text, "설명"));
    reset_control_events(); ui_radio_btn_send_msg((struct UIWindow *)(radio), NULL, UI_MSG_SET_STATE, 0, 0, 0, 0);
    assert(!wnd->state && invalidations == 1);
    ui_radio_btn_destroy((struct UIWindow *)(radio));
}
static void test_native_bitmap_radio(void)
{
    struct UIWindow *wnd = ui_bmp_radio_btn_create(317);
    struct UIBmpRadioBtn *radio = (struct UIBmpRadioBtn *)wnd;
    const char *paths[] = {"기본.bmp", "강조.bmp", "선택.bmp", "사용불가.bmp"};
    const COLORREF shadows[] = {UP_SHADOW, HOVER_SHADOW, CHOSEN_SHADOW, DISABLED_SHADOW};
    char caption[] = "분류 제목";
    char counted[] = {'a', 0, 'b', 'c'};
    int look, i;
    assert(((struct Allocation *)radio - 1)->size == sizeof *radio);
    assert(radio->value == 317 && !radio->disabled && !radio->hover && !radio->styles && !radio->style_count);
    wnd->parent = &parent; wnd->width = 100; wnd->height = 40;
    ui_bmp_radio_btn_set_caption(wnd, text_view_cstr(caption)); caption[0] = '!';
    assert(!strcmp(string_cstr(&radio->label), "분류 제목"));
    ui_bmp_radio_btn_set_caption(wnd, string_view(&radio->label));
    ui_bmp_radio_btn_set_caption(wnd, text_view_cstr(string_cstr(&radio->label) + 2));
    assert(!strcmp(string_cstr(&radio->label), "류 제목"));
    ui_bmp_radio_btn_set_caption(wnd, text_view(counted, sizeof counted));
    ui_bmp_radio_btn_set_text_style(wnd, -9, 18, 0, 0, 0, RADIO_CENTRED, RADIO_CENTRED);
    assert(measured_length == sizeof counted && !memcmp(measured_text, counted, sizeof counted));
    ui_bmp_radio_btn_set_caption(wnd, text_view_cstr("버튼"));
    resource.width = 27; resource.height = 18;
    for (i = 0; i < 4; ++i) assert(ui_bmp_radio_btn_set_bitmap(wnd, paths[i], i));
    reset_control_events(); missing = 1;
    assert(!ui_bmp_radio_btn_set_bitmap(wnd, "missing.bmp", 0) && !resizes);
    assert(!strcmp(string_cstr(&radio->bitmaps[0]), paths[0])); missing = 0;
    assert(ui_bmp_radio_btn_set_bitmap(wnd, "ignored.bmp", 99));
    assert(!strcmp(string_cstr(&radio->bitmaps[0]), paths[0]));
    assert(ui_bmp_radio_btn_set_bitmap(wnd, string_cstr(&radio->bitmaps[0]) + 2, 0));
    assert(!strcmp(string_cstr(&radio->bitmaps[0]), "본.bmp"));
    assert(ui_bmp_radio_btn_set_bitmap(wnd, paths[0], 0));
    for (look = 0; look < 4; ++look) {
        radio->disabled = (BYTE)(look == 3); radio->hover = (BYTE)(look == 1); wnd->state = look == 2;
        reset_control_events(); ui_bmp_radio_btn_on_draw((struct UIWindow *)(radio));
        assert(draws == 1 && labels == 2 && !strcmp(loaded_path, paths[look]));
        expect_text(0, 38 + (look < 2), 13 + (look < 2), 12, shadows[look], 0, "버튼");
        expect_text(1, 38 + (look >= 2), 13 + (look >= 2), 12, look == 3 ? DISABLED_TEXT : 0, 0, "버튼");
        ui_bmp_radio_btn_set_text_style(wnd, look, 20 + look, 0x112233 + look, 2, 1, 10 + look, 20 + look);
    }
    for (i = 0; i < 24; ++i) ui_bmp_radio_btn_set_text_style(wnd, 100 + i, 12, 0, 0, 0, i, i);
    assert(radio->style_count == 29);
    ui_bmp_radio_btn_set_text_style(wnd, 0, 33, 0xabcdef, 3, 0, 7, 9);
    assert(radio->style_count == 29);
    radio->disabled = 2; radio->hover = 2; wnd->state = 2;
    reset_control_events(); ui_bmp_radio_btn_on_draw((struct UIWindow *)(radio));
    assert(draws == 1 && labels == 1 && !strcmp(loaded_path, paths[0]));
    expect_text(0, 7, 9, 33, 0xabcdef, 3, "버튼");
    radio->disabled = 1;
    reset_control_events(); ui_bmp_radio_btn_on_draw((struct UIWindow *)(radio));
    expect_text(0, 13, 23, 23, shadows[3], 0, "버튼");
    expect_text(1, 14, 24, 23, 0x112236, 2, "버튼");
    string_assign_cstr(&radio->tooltip, "세 설명");
    reset_control_events(); ui_bmp_radio_btn_on_mouse_hover((struct UIWindow *)(radio), 0, 0);
    assert(tooltip_calls == 1 && !strcmp(tooltip_text, "세 설명"));
    reset_control_events(); ui_bmp_radio_btn_on_l_btn_down((struct UIWindow *)(radio), 0, 0);
    assert(notices == 1 && notice_b[0] == 317);
    ui_bmp_radio_btn_hover_on((struct UIWindow *)(radio)); assert(radio->hover == 1);
    ui_bmp_radio_btn_hover_off((struct UIWindow *)(radio)); assert(radio->hover == 0);
    ui_bmp_radio_btn_send_msg((struct UIWindow *)(radio), NULL, UI_MSG_SET_STATE, -4, 0, 0, 0);
    assert(wnd->state == -4 && invalidations == 3);
    ui_bmp_radio_btn_destroy((struct UIWindow *)(radio));
    radio = (struct UIBmpRadioBtn *)ui_bmp_radio_btn_create(0);
    assert(radio && radio->value == 0); ui_bmp_radio_btn_destroy((struct UIWindow *)(radio));
    fail_at = allocation_calls + 1;
    assert(!ui_bmp_radio_btn_create(0)); fail_at = 0;
}
