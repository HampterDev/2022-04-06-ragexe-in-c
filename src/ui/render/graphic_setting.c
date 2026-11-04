#include "game/gamemode.h"
#include "game/display_settings.h"
#include "render/device.h"
#include "render/rp_pool.h"
#include "render/texture_cache.h"
#include "contents/sound.h"
#include "render/display_devices.h"
#include "actor/weather.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/character/char_info.h"
#include "ui/runtime.h"
#include "ui/ui_std.h"
#include "ui/render/render_std.h"
#include <stdlib.h>
#include <string.h>
#include "core/app.h"
#include "platform/display_depth.h"
#include "core/msg_str.h"
#include "core/skin_path.h"
#include "ui/skin_manager.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "render/render_api.h"
#include "session/session_names.h"
#include "ui/wndmgr/wndmgr.h"
#include "ui/render/graphic_setting.h"
#include "game/mouse_camera.h"

#define WID_GRAPHIC_SETTING 0x131
#define WID_COMMAND_LIST 0x70

#define ID_OPTION_BOX 0xcd
#define ID_SKIN 0x104
#define ID_APPLY 0x107
#define ID_RESET 0x163
#define ID_COMMAND_LIST 0x1f6
#define ID_DEVICE 0x1fa
#define ID_MODE 0x1fb

#define TAB_OPTIONS 0

#define OFF_SCREEN (-200)

#define MSG_CONFIRM_RESTART 0xc60
#define MSG_CONFIRM_APPLY 0xc5f
#define MSG_CONFIRM_RESET 0xc5e

#define CZ_SET_USERCONFIG 0xb93
#define UI_MSGBOX_ANSWER_NO 0xb4

#define GS_MSG_RELOAD 0x17
#define GS_MSG_REDRAW 0x3c

#define SIZE_TAB_CONTROL sizeof(struct UITabControl)
#define SIZE_SCROLL_BAR sizeof(struct UIScrollBar)

#define WND(self) (&(self)->frame.base)
#define W(ctrl) ((struct UIWindow *)(ctrl))
#define STR_AT(v, i) (&((struct String *)(v)->first)[i])
#define STR_COUNT(v) ((unsigned)(((BYTE *)(v)->last - (BYTE *)(v)->first) / (int)sizeof(struct String)))
#define MODE_AT(v, i) (&((struct DisplayMode *)(v)->first)[i])

void ui_graphic_setting_wnd_destroy(struct UIWindow *window);
void ui_graphic_setting_wnd_store_info(struct UIWindow *self);
void ui_graphic_setting_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_graphic_setting_wnd_on_draw(struct UIWindow *self);
intptr_t ui_graphic_setting_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_graphic_setting_wnd_ops = {
    .destroy = ui_graphic_setting_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_graphic_setting_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_graphic_setting_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_graphic_setting_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_graphic_setting_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

int g_displayDeviceIndex;
int g_displayModeIndex;
int g_bitsPerPixel;

#pragma pack(push, 1)
struct PacketSetUserConfig {
    unsigned short id;
    unsigned short option_group;
    int kind;
    int value;
};
#pragma pack(pop)

void ui_graphic_setting_wnd_apply_tab(struct UIGraphicSettingWnd *self);
void ui_graphic_setting_wnd_load_options(struct UIGraphicSettingWnd *self);

struct UIGraphicSettingWnd *ui_graphic_setting_wnd_construct(struct UIGraphicSettingWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    WND(self)->ops = &ui_graphic_setting_wnd_ops;
    memset(&self->device_names, 0, sizeof(self->device_names));
    memset(&self->mode_names, 0, sizeof(self->mode_names));
    memset(&self->modes, 0, sizeof(self->modes));
    self->device = 0;
    self->mode = 0;
    string_init(&self->device_name);
    memset(&self->info, 0, (BYTE *)&self->box_rodex_all - (BYTE *)&self->info);
    return self;
}

struct UIWindow *ui_graphic_setting_wnd_new(void)
{
    return (struct UIWindow *)ui_graphic_setting_wnd_construct(mem_alloc(sizeof(struct UIGraphicSettingWnd)));
}

void ui_graphic_setting_wnd_destruct(struct UIGraphicSettingWnd *self)
{
    WND(self)->ops = &ui_graphic_setting_wnd_ops;
    string_free(&self->device_name);
    vec_free(&self->modes);
    string_vec_free(&self->mode_names);
    string_vec_free(&self->device_names);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_graphic_setting_wnd_destroy(struct UIWindow *window)
{
    struct UIGraphicSettingWnd *self = (struct UIGraphicSettingWnd *)window;
    ui_graphic_setting_wnd_destruct(self);
    mem_free(self);
}

void ui_graphic_setting_wnd_build_mode_list(struct UIGraphicSettingWnd *self)
{
    const struct DisplayDevice *rec;
    unsigned count, i, hw, m;
    int has_16bit_modes = platform_display_has_16bit_modes();

    count = (unsigned)display_devices_count();
    self->modes.last = self->modes.first;
    string_vec_clear(&self->mode_names);
    for (i = 0, hw = 0; i < count; i++) {
        struct String name;

        rec = display_devices_at(i);
        if (!rec->hardware)
            continue;
        if (hw++ != (unsigned)self->device)
            continue;
        string_init(&name);
        string_assign_cstr(&name, rec->name);
        string_free(&self->device_name);
        self->device_name = name;
        string_init(&name);
        string_free(&name);
        self->device_guid = rec->device_guid;
        self->adapter_guid = rec->adapter_guid;
        for (m = 0; m < rec->mode_count; m++) {
            const struct DisplayMode *mode = &rec->modes[m];
            int width = (int)mode->width, height = (int)mode->height, bpp = (int)mode->bpp;

            if (!has_16bit_modes) {
                if ((unsigned)height >= 0x298 && bpp == 32 && (unsigned)width <= 0x2000
                    && (unsigned)height <= 0x1000) {
                    struct String text;
                    struct DisplayMode entry;

                    string_init(&text);
                    string_format(&text, "%d x %d x %d", width, height, bpp);
                    string_vec_push_copy(&self->mode_names, &text);
                    string_free(&text);
                    entry.width = width;
                    entry.height = height;
                    entry.bpp = 32;
                    vec_push(&self->modes, &entry, sizeof entry);
                }
            } else if ((unsigned)height >= 0x298 && (unsigned)width <= 0x2000 && (unsigned)height <= 0x1000
                       && (bpp == 16 || bpp == 32)) {
                struct String text;
                struct DisplayMode entry;

                string_init(&text);
                string_format(&text, "%d x %d x %d", width, height, bpp);
                string_vec_push_copy(&self->mode_names, &text);
                string_free(&text);
                entry.width = width;
                entry.height = height;
                entry.bpp = bpp;
                vec_push(&self->modes, &entry, sizeof entry);
            }
            if (width == g_displayWidth && height == g_displayHeight && bpp == g_bitsPerPixel) {
                g_displayModeIndex = (int)vec_count(&self->modes, sizeof(struct DisplayMode)) - 1;
                self->mode = g_displayModeIndex;
            }
        }
    }
}

int ui_graphic_setting_wnd_check_changed(struct UIGraphicSettingWnd *self)
{
    struct UIWindow *apply = W(self->btn_apply);
    int changed = 0;

    if (self->device != g_displayDeviceIndex || self->mode != g_displayModeIndex
        || settings_display_changed(self))
        changed = 2;
    else if (self->sprite_mode != g_spriteMode || self->texture_detail != g_textureMode
             || W(self->box_mouse)->state != (g_session1.mouseExclusiveOption != 0)
             || W(self->box_trilinear)->state != g_trilinearFilter)
        changed = 1;
    if (!changed)
        ui_window_move_base(apply, OFF_SCREEN, OFF_SCREEN);
    else
        ui_window_move_base(apply, W(self->btn_close)->x - apply->width - 10, WND(self)->height - apply->height - 3);
    return changed;
}

static struct UICheckBox *make_check_box(struct UIGraphicSettingWnd *self, int width, int msg)
{
    void *mem = mem_alloc(sizeof(struct UICheckBox));
    struct UICheckBox *box = mem ? ui_check_box_construct(mem) : NULL;

    ui_window_create(W(box), width, 0xc);
    ui_check_box_set_bitmaps(W(box), "유저인터페이스\\checkbox_1.bmp", "유저인터페이스\\checkbox_0.bmp", msg_str(msg));
    (void)self;
    return box;
}

static struct UIBitmapButton *make_bitmap_button(const char *up, const char *hover, const char *down)
{
    void *mem = mem_alloc(sizeof(struct UIBitmapButton));
    struct UIBitmapButton *button = mem ? ui_bitmap_button_construct(mem) : NULL;

    ui_bitmap_button_set_bitmap(button, up, 0, 0);
    ui_bitmap_button_set_bitmap(button, hover, 1, 0);
    ui_bitmap_button_set_bitmap(button, down, 2, 0);
    ui_window_create(W(button), button->bitmap_width, button->bitmap_height);
    return button;
}

static struct UINoneVisibleButton *make_combo(int id, int tooltip)
{
    void *mem = mem_alloc(sizeof(struct UINoneVisibleButton));
    struct UINoneVisibleButton *button = mem ? ui_none_visible_button_construct(mem) : NULL;

    ui_window_create(W(button), 0xb5, 0x10);
    W(button)->id = id;
    ui_button_set_tool_tip(&button->button, msg_str(tooltip));
    return button;
}

static struct UIScrollBar *make_bar(int x, int y, int max_pos, int page, int total)
{
    void *mem = mem_alloc(SIZE_SCROLL_BAR);
    struct UIScrollBar *bar = mem ? ui_scroll_bar_construct(mem) : NULL;

    ui_scroll_bar_create(bar, x, y, 0, 0xb5);
    ui_scroll_bar_set_range(bar, max_pos, page, total);
    return bar;
}

static int clamp_0_2(int value)
{
    return value < 0 ? 0 : value > 2 ? 2 : value;
}

void ui_graphic_setting_wnd_on_create(struct UIWindow *window, int width,
                                      int height)
{
    struct UIGraphicSettingWnd *self = (struct UIGraphicSettingWnd *)window;
    struct UIWindow *wnd = WND(self);
    const struct DisplayDevice *rec;
    unsigned count, i;
    void *mem;

    ui_window_add_sys_button(wnd);
    ui_window_add_close_button(wnd, "Alt+D", 0);
    string_vec_clear(&self->device_names);
    self->device = g_displayDeviceIndex;
    count = (unsigned)display_devices_count();
    for (i = 0; i < count; i++) {
        rec = display_devices_at(i);
        if (!rec->hardware)
            continue;
        string_vec_push_text(&self->device_names, rec->name, strlen(rec->name));
    }
    self->mode = g_displayModeIndex;
    ui_graphic_setting_wnd_build_mode_list(self);

    mem = mem_alloc(SIZE_TAB_CONTROL);
    self->tabs = (struct UITabControl *)(mem ? ui_tab_control_construct(mem) : NULL);
    ui_tab_control_setup(W(self->tabs), 1, 0x11, 0x14, 1, 0, 0, 0, 0, 0, 0, 0, 0);
    ui_window_move_base(&self->tabs->base, 0, 0x11);
    ui_window_add_child(wnd, W(self->tabs));
    ui_tab_control_add_tab(W(self->tabs), msg_str(0xc5b));
    ui_tab_control_add_tab(W(self->tabs), msg_str(0xc5c));

    self->btn_reset = make_bitmap_button(UI_SKIN "btn_reset.bmp", UI_SKIN "btn_reset_a.bmp",
                                         UI_SKIN "btn_reset_b.bmp");
    W(self->btn_reset)->id = ID_RESET;
    ui_bitmap_button_set_tool_tip(self->btn_reset, msg_str(0xc5d));
    ui_window_add_child(wnd, W(self->btn_reset));
    self->btn_apply = make_bitmap_button(UI_SKIN "btn_apply.bmp", UI_SKIN "btn_apply_a.bmp",
                                         UI_SKIN "btn_apply_b.bmp");
    W(self->btn_apply)->id = ID_APPLY;
    ui_window_add_child(wnd, W(self->btn_apply));
    self->btn_close = make_bitmap_button(UI_SKIN "btn_close.bmp", UI_SKIN "btn_close_a.bmp",
                                         UI_SKIN "btn_close_b.bmp");
    ui_window_move_base(&self->btn_close->button.base, wnd->width - W(self->btn_close)->width - 10,
             wnd->height - W(self->btn_close)->height - 3);
    W(self->btn_close)->id = UI_ID_CLOSE;
    ui_window_add_child(wnd, W(self->btn_close));

    mem = mem_alloc(sizeof(struct UIBasicButton));
    self->btn_commands = mem ? ui_basic_button_construct(mem) : NULL;
    {
        const char *label = msg_str(0xc7b);

        ui_basic_button_set_label(W(self->btn_commands),
                               ui_window_text_width(wnd, msg_str(0xc7b), 0, 0, 0xc, 0, 0) + 10, label);
    }
    W(self->btn_commands)->id = ID_COMMAND_LIST;
    ui_window_add_child(wnd, W(self->btn_commands));

    self->skin_box = make_combo(ID_SKIN, 0xc13);
    ui_window_add_child(wnd, W(self->skin_box));

    self->box_snap = make_check_box(self, 0x30, 0x7cf);
    ui_window_add_child(wnd, W(self->box_snap));
    self->box_skillsnap = make_check_box(self, 0x30, 0x7d0);
    ui_window_add_child(wnd, W(self->box_skillsnap));
    self->box_itemsnap = make_check_box(self, 0x30, 0x7d1);
    ui_window_add_child(wnd, W(self->box_itemsnap));
    self->box_fog = make_check_box(self, 0x30, 0xc61);
    ui_window_add_child(wnd, W(self->box_fog));
    self->box_aura = make_check_box(self, 0x5a, 0xc62);
    ui_window_add_child(wnd, W(self->box_aura));
    self->box_aura2 = make_check_box(self, 0x50, 0xc63);
    ui_window_add_child(wnd, W(self->box_aura2));
    self->box_effect = make_check_box(self, 0x30, 0xc64);
    ui_window_add_child(wnd, W(self->box_effect));
    self->box_lightmap = make_check_box(self, 0x46, 0xc65);
    ui_window_add_child(wnd, W(self->box_lightmap));
    self->box_noctrl = make_check_box(self, 0x30, 0x7d2);
    ui_window_add_child(wnd, W(self->box_noctrl));
    self->box_noshift = make_check_box(self, 0x3c, 0xc66);
    ui_window_add_child(wnd, W(self->box_noshift));

    self->bgm_volume = make_bar(0x3a, 0x3c, 0x7f, 1, 0x80);
    ui_window_add_child(wnd, W(self->bgm_volume));
    self->effect_volume = make_bar(0x3a, 0x3c, 0x7f, 1, 0x80);
    ui_window_add_child(wnd, W(self->effect_volume));

    self->box_bgm = make_check_box(self, 0x3c, 0xc11);
    ui_window_add_child(wnd, W(self->box_bgm));
    self->box_sound = make_check_box(self, 0x3c, 0xc11);
    ui_window_add_child(wnd, W(self->box_sound));
    self->box_zoom = make_check_box(self, 0x3c, 0xc11);
    ui_window_add_child(wnd, W(self->box_zoom));
    self->box_emblem_frame = make_check_box(self, 0x3c, 0xc11);
    ui_window_add_child(wnd, W(self->box_emblem_frame));
    self->box_extend_damage = make_check_box(self, 0x3c, 0xdf8);
    ui_window_add_child(wnd, W(self->box_extend_damage));

    self->device_box = make_combo(ID_DEVICE, 0xc67);
    ui_window_add_child(wnd, W(self->device_box));
    self->mode_box = make_combo(ID_MODE, 0xc68);
    ui_window_add_child(wnd, W(self->mode_box));

    self->sprite_mode = clamp_0_2(g_spriteMode);
    self->sprite_bar = make_bar(0x3a, 0x3c, 2, 1, 3);
    ui_scroll_bar_set_pos(self->sprite_bar, self->sprite_mode);
    ui_window_add_child(wnd, W(self->sprite_bar));
    self->texture_detail = clamp_0_2(g_textureMode);
    mem = mem_alloc(SIZE_SCROLL_BAR);
    self->texture_bar = mem ? ui_scroll_bar_construct(mem) : NULL;
    ui_scroll_bar_create(self->texture_bar, W(self->sprite_bar)->x, W(self->sprite_bar)->y + 0x2e, 0, 0xb5);
    ui_scroll_bar_set_range(self->texture_bar, 2, 1, 3);
    ui_scroll_bar_set_pos(self->texture_bar, self->texture_detail);
    ui_window_add_child(wnd, W(self->texture_bar));

    self->box_fullscreen = make_check_box(self, 0x3c, 0xc69);
    ui_check_box_set_check(W(self->box_fullscreen), g_fullscreenMode);
    settings_add_row(self, W(self->box_fullscreen));
    self->box_mouse = make_check_box(self, 0x50, 0xc6a);
    ui_check_box_set_check(W(self->box_mouse), g_session1.mouseExclusiveOption);
    ui_window_add_child(wnd, W(self->box_mouse));
    self->box_trilinear = make_check_box(self, 0x3c, 0x7ce);
    ui_check_box_set_check(W(self->box_trilinear), g_trilinearFilter);
    ui_window_add_child(wnd, W(self->box_trilinear));
    self->box_rodex_all = make_check_box(self, 0x3c, 0xf1c);
    ui_check_box_set_check(W(self->box_rodex_all), g_session1.userConfigEntry1 != 0);
    ui_window_add_child(wnd, W(self->box_rodex_all));

    ui_graphic_setting_wnd_load_options(self);
    ui_graphic_setting_wnd_apply_tab(self);
}

static void draw_label(struct UIWindow *wnd, int x, int y, const char *text)
{
    ui_window_text_out(wnd, x, y, text, 0, 0, 0xc, 0, 0, 0);
}

static void draw_combo(struct UIWindow *wnd, struct UIWindow *box)
{
    struct BitmapRes *arrow;

    ui_window_draw_outline(wnd, box->x, box->y, 0xb5, 0x12, 0xc0c0c0);
    arrow = res_mgr_get(g_resMgr(), ui_skin_path(UI_SKIN "basic_interface\\txtbox_btn_a.bmp"));
    ui_window_draw_bitmap(wnd, box->x + box->width - arrow->width, box->y, arrow, 0);
}

void ui_graphic_setting_wnd_on_draw(struct UIWindow *window)
{
    struct UIGraphicSettingWnd *self = (struct UIGraphicSettingWnd *)window;
    struct UIWindow *wnd = WND(self);
    struct UIWindow *box;

    ui_window_draw_title_bar(wnd, 0, "", 0);
    ui_window_draw_body(wnd, 0x11, 0, "");
    ui_window_fill_rect(wnd, W(self->tabs)->width, 0x22, wnd->width - W(self->tabs)->width, 1, 0xc3c3c3);
    ui_window_draw_button_bar(wnd, 2, 0, 1);
    ui_window_text_out(wnd, 0x11, 4, msg_str(0x946), 0, 0, 0xc, 0xffffff, 0, 0);
    ui_window_text_out(wnd, 0x10, 3, msg_str(0x946), 0, 0, 0xc, 0, 0, 0);
    if (self->tabs->selected == TAB_OPTIONS) {
        box = W(self->skin_box);
        draw_label(wnd, 0x12, box->y + 3, msg_str(0x5d9));
        draw_combo(wnd, box);
        draw_label(wnd, box->x + 3, box->y + 4, skin_mgr_get_current_name(&g_skinMgr));
        draw_label(wnd, 0x12, W(self->box_snap)->y, msg_str(0x116));
        draw_label(wnd, 0x12, W(self->box_fog)->y, msg_str(0xc6b));
        draw_label(wnd, 0x12, W(self->box_noctrl)->y, msg_str(0xc6c));
        draw_label(wnd, 0x12, W(self->bgm_volume)->y + 2, msg_str(0x5cd));
        draw_label(wnd, W(self->box_snap)->x, W(self->bgm_volume)->y + 2, msg_str(0x5d7));
        draw_label(wnd, W(self->box_snap)->x, W(self->effect_volume)->y + 2, msg_str(0x5d8));
        draw_label(wnd, 0x12, W(self->box_zoom)->y + 2, msg_str(0xdb5));
        return;
    }
    box = W(self->device_box);
    draw_label(wnd, 0x12, box->y + 3, msg_str(0xc6d));
    draw_combo(wnd, box);
    if (STR_COUNT(&self->device_names) > (unsigned)self->device)
        draw_label(wnd, box->x + 3, box->y + 4, string_cstr(STR_AT(&self->device_names, self->device)));
    box = W(self->mode_box);
    draw_label(wnd, 0x12, box->y + 3, msg_str(0xc6e));
    draw_combo(wnd, box);
    draw_label(wnd, box->x + 3, box->y + 4, string_cstr(STR_AT(&self->mode_names, self->mode)));
    draw_label(wnd, 0x12, W(self->sprite_bar)->y + 2, msg_str(0x144));
    draw_label(wnd, 0x12, W(self->texture_bar)->y + 2, msg_str(0x145));
    draw_label(wnd, 0x12, W(self->box_fullscreen)->y, msg_str(0xc6f));
    draw_label(wnd, 0x12, W(self->box_mouse)->y, msg_str(0xc70));
    settings_draw_labels(self);
    draw_label(wnd, 0x12, W(self->box_emblem_frame)->y, msg_str(0xdc6));
}

static void move_to(void *ctrl, int x, int y)
{
    ui_window_move(ctrl, x, y);
}

void ui_graphic_setting_wnd_apply_tab(struct UIGraphicSettingWnd *self)
{
    struct UIWindow *wnd = WND(self);
    struct UIWindow *at;

    if (self->tab == TAB_OPTIONS) {
        move_to(self->btn_apply, OFF_SCREEN, OFF_SCREEN);
        move_to(self->device_box, OFF_SCREEN, OFF_SCREEN);
        move_to(self->mode_box, OFF_SCREEN, OFF_SCREEN);
        move_to(self->sprite_bar, OFF_SCREEN, OFF_SCREEN);
        move_to(self->texture_bar, OFF_SCREEN, OFF_SCREEN);
        move_to(self->box_fullscreen, OFF_SCREEN, OFF_SCREEN);
        move_to(self->box_mouse, OFF_SCREEN, OFF_SCREEN);
        settings_hide_rows(self);
        move_to(self->box_emblem_frame, OFF_SCREEN, OFF_SCREEN);
        move_to(self->btn_reset, 5, wnd->height - W(self->btn_reset)->height - 3);
        move_to(self->btn_commands, W(self->btn_reset)->x + 5 + W(self->btn_reset)->width,
                wnd->height - W(self->btn_reset)->height - 3);
        move_to(self->skin_box, 0x5f, 0x2b);
        move_to(self->box_snap, W(self->skin_box)->x, W(self->skin_box)->y + 0x24);
        at = W(self->box_snap);
        move_to(self->box_skillsnap, at->x + 0x64, at->y);
        move_to(self->box_itemsnap, at->x + 0xc8, at->y);
        move_to(self->box_fog, at->x, at->y + 0x20);
        move_to(self->box_aura, at->x + 0x64, at->y + 0x20);
        move_to(self->box_aura2, at->x + 0xc8, at->y + 0x20);
        move_to(self->box_effect, at->x, at->y + 0x40);
        move_to(self->box_lightmap, at->x + 0x64, at->y + 0x40);
        move_to(self->box_noctrl, at->x, at->y + 0x60);
        move_to(self->box_noshift, at->x + 0x64, at->y + 0x60);
        move_to(self->bgm_volume, at->x + 0x32, at->y + 0x80);
        move_to(self->effect_volume, at->x + 0x32, at->y + 0xa0);
        move_to(self->box_bgm, at->x + 0x37 + W(self->bgm_volume)->width, at->y + 0x80);
        move_to(self->box_sound, at->x + 0x37 + W(self->effect_volume)->width, at->y + 0xa0);
        move_to(self->box_zoom, at->x, at->y + 0xc0);
        move_to(self->box_extend_damage, at->x + 0xc8, at->y + 0x40);
        move_to(self->box_effect, at->x, at->y + 0x32);
        move_to(self->box_lightmap, at->x + 0x64, at->y + 0x32);
        move_to(self->box_extend_damage, at->x + 0xc8, at->y + 0x32);
        move_to(self->box_rodex_all, at->x, at->y + 0xd7);
        return;
    }
    move_to(self->btn_reset, OFF_SCREEN, OFF_SCREEN);
    move_to(self->btn_commands, OFF_SCREEN, OFF_SCREEN);
    move_to(self->skin_box, OFF_SCREEN, OFF_SCREEN);
    move_to(self->box_snap, OFF_SCREEN, OFF_SCREEN);
    move_to(self->box_skillsnap, OFF_SCREEN, OFF_SCREEN);
    move_to(self->box_itemsnap, OFF_SCREEN, OFF_SCREEN);
    move_to(self->box_fog, OFF_SCREEN, OFF_SCREEN);
    move_to(self->box_aura, OFF_SCREEN, OFF_SCREEN);
    move_to(self->box_aura2, OFF_SCREEN, OFF_SCREEN);
    move_to(self->box_effect, OFF_SCREEN, OFF_SCREEN);
    move_to(self->box_lightmap, OFF_SCREEN, OFF_SCREEN);
    move_to(self->box_noctrl, OFF_SCREEN, OFF_SCREEN);
    move_to(self->box_noshift, OFF_SCREEN, OFF_SCREEN);
    move_to(self->bgm_volume, OFF_SCREEN, OFF_SCREEN);
    move_to(self->effect_volume, OFF_SCREEN, OFF_SCREEN);
    move_to(self->box_bgm, OFF_SCREEN, OFF_SCREEN);
    move_to(self->box_sound, OFF_SCREEN, OFF_SCREEN);
    move_to(self->box_zoom, OFF_SCREEN, OFF_SCREEN);
    move_to(self->box_extend_damage, OFF_SCREEN, OFF_SCREEN);
    ui_graphic_setting_wnd_check_changed(self);
    move_to(self->device_box, 0x96, 0x2b);
    at = W(self->device_box);
    move_to(self->mode_box, at->x, at->y + 0x20);
    move_to(self->sprite_bar, at->x, at->y + 0x40);
    move_to(self->texture_bar, at->x, at->y + 0x60);
    move_to(self->box_fullscreen, at->x, at->y + 0x80);
    move_to(self->box_mouse, at->x, at->y + 0xa0);
    settings_place_rows(self);
    move_to(self->box_emblem_frame, at->x, at->y + 0xe0);
    move_to(self->box_rodex_all, OFF_SCREEN, OFF_SCREEN);
}

void ui_graphic_setting_wnd_store_info(struct UIWindow *window)
{
    struct UIGraphicSettingWnd *self = (struct UIGraphicSettingWnd *)window;
    if (!self->info)
        return;
    self->info->width = WND(self)->width;
    self->info->height = WND(self)->height;
    self->info->x = WND(self)->x;
    self->info->y = WND(self)->y;
}

void ui_graphic_setting_wnd_load_options(struct UIGraphicSettingWnd *self)
{
    ui_check_box_set_check(W(self->box_snap), g_session1.monsterSnapOnNoSkill);
    ui_check_box_set_check(W(self->box_skillsnap), g_session1.monsterSnapOnSkill);
    ui_check_box_set_check(W(self->box_itemsnap), g_session1.itemSnap);
    ui_check_box_set_check(W(self->box_fog), g_session1.fogOn);
    ui_check_box_set_check(W(self->box_aura), g_session1.auraFull == 0);
    ui_check_box_set_check(W(self->box_aura2), g_session1.auraOff == 0);
    ui_check_box_set_check(W(self->box_effect), g_session1.effectOn);
    ui_check_box_set_check(W(self->box_lightmap), g_lightmap);
    ui_check_box_set_check(W(self->box_noctrl), g_session1.noCtrl);
    ui_check_box_set_check(W(self->box_noshift), g_session1.noShift);
    ui_scroll_bar_set_pos(self->bgm_volume, sound_bgm_volume());
    ui_window_invalidate_base(&self->bgm_volume->base);
    ui_scroll_bar_set_pos(self->effect_volume, sound_effect_volume());
    ui_window_invalidate_base(&self->effect_volume->base);
    ui_check_box_set_check(W(self->box_bgm), g_sound_settings.bgm_on);
    ui_check_box_set_check(W(self->box_sound), g_sound_settings.effects_on);
    ui_check_box_set_check(W(self->box_zoom), g_session1.zoomFarLimit);
    W(self->box_emblem_frame)->state = g_session1.emblemFrame;
    ui_window_invalidate_base(&self->box_emblem_frame->base);
    ui_check_box_set_check(W(self->box_extend_damage), g_session1.extendDamage);
}

static void place_list(struct UIGraphicSettingWnd *self, struct UIWindow *list, struct UIWindow *box, int selected,
                       int height)
{
    int x, y;

    ui_window_send_msg(list, NULL, COMBO_MSG_SELECT, selected, 0, 0, 0);
    ui_window_resize(list, 0xb5, height);
    ui_window_send_msg(list, NULL, COMBO_MSG_OPENER_ID, WID_GRAPHIC_SETTING, 0, 0, 0);
    x = box->x;
    y = box->y + 0x11;
    ui_window_to_screen(WND(self), &x, &y);
    ui_window_move(list, x + 1, y + 1);
}

static void open_string_list(struct UIGraphicSettingWnd *self, int id, struct Vec *names, int selected,
                             struct UIWindow *box)
{
    struct UIWindow *list;
    unsigned i;
    unsigned height = 0x50;

    if (g_windowMgr.combo_box_wnd)
        ui_window_mgr_post_quit1(&g_windowMgr, UI_WND_COMBO_BOX);
    list = ui_window_open(UI_WND_COMBO_BOX);
    list->id = id;
    for (i = 0; i < STR_COUNT(names); i++)
        ui_window_send_msg(list, NULL, COMBO_MSG_ADD_LINE, (intptr_t)string_cstr(STR_AT(names, i)), 0, 0, 0);
    if (STR_COUNT(names) * 16 < height)
        height = STR_COUNT(names) * 16;
    place_list(self, list, box, selected, (int)height);
}

static int on_command(struct UIGraphicSettingWnd *self, struct UIWindow *sender, int id, int value)
{
    int answer, count, i, height;
    struct UIWindow *list;

    switch (id) {
    case UI_ID_CANCEL:
    case UI_ID_CLOSE:
        ui_window_mgr_post_quit1(&g_windowMgr, WID_GRAPHIC_SETTING);
        return 0;
    case ID_OPTION_BOX:
        if (sender == W(self->box_snap)) {
            g_session1.monsterSnapOnNoSkill = value;
        } else if (sender == W(self->box_skillsnap)) {
            g_session1.monsterSnapOnSkill = value;
        } else if (sender == W(self->box_itemsnap)) {
            g_session1.itemSnap = value;
        } else if (sender == W(self->box_fog)) {
            g_session1.fogOn = value;
            game_mode_apply_fog(game_current_mode(), value);
        } else if (sender == W(self->box_aura) || sender == W(self->box_aura2)) {
            if (sender == W(self->box_aura))
                g_session1.auraFull = value == 0;
            else
                g_session1.auraOff = value == 0;
            game_mode_refresh_actor_effects(game_current_mode(), 0);
        } else if (sender == W(self->box_effect)) {
            const char *map = game_current_mode()->map_name;
            BYTE christmas;

            g_session1.effectOn = value;
            christmas = strcmp(map, "xmas_fild01.rsw") == 0;
            if (!christmas) {
                map = game_current_mode()->map_name;
                christmas = strcmp(map, "xmas.rsw") == 0;
            }
            if (christmas) {
                if (g_session1.effectOn)
                    weather_snow(&g_weather);
                else
                    weather_stop_effect(&g_weather);
            }
        } else if (sender == W(self->box_lightmap)) {
            g_lightmap = value;
            g_weather.ground_lighting = value == 0;
        } else if (sender == W(self->box_noctrl)) {
            g_session1.noCtrl = value;
        } else if (sender == W(self->box_noshift)) {
            g_session1.noShift = value;
        } else if (sender == W(self->box_bgm)) {
            g_sound_settings.bgm_on = value;
            mode_send_message(client_current_mode(), 0x5a, 0, 0, 0, 0);
        } else if (sender == W(self->box_sound)) {
            g_sound_settings.effects_on = value;
        } else if (sender == W(self->box_zoom)) {
            g_session1.zoomFarLimit = value;
            g_maxViewDistance = value ? 480.0f : 400.0f;
        } else if (sender == W(self->box_emblem_frame)) {
            g_session1.emblemFrame = value;
            ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
            ui_window_mgr_send_msg_(&g_windowMgr, 1, (intptr_t)msg_str(g_session1.emblemFrame ? 0xdc7 : 0xdc8), 0x64ffff, 0, 0);
        } else if (sender == W(self->box_extend_damage)) {
            g_session1.extendDamage = value;
        } else if (settings_is_display_box(sender, self) || sender == W(self->box_trilinear)) {
            ui_graphic_setting_wnd_check_changed(self);
        } else if (sender == W(self->box_rodex_all)) {
            struct PacketSetUserConfig packet;
            int len;

            packet.id = CZ_SET_USERCONFIG;
            packet.option_group = 0;
            packet.kind = 1;
            packet.value = value;
            len = rag_connection_get_packet_length(rag_connection_instance(), CZ_SET_USERCONFIG);
            rag_connection_send_packet(rag_connection_instance(), len, &packet);
            return 0;
        }
        list = ui_window_find(WID_COMMAND_LIST);
        if (list)
            ui_window_invalidate(list);
        return 0;
    case ID_SKIN:
        if (g_windowMgr.combo_box_wnd)
            ui_window_mgr_post_quit1(&g_windowMgr, UI_WND_COMBO_BOX);
        count = (int)(((BYTE *)g_skinMgr.paths.last - (BYTE *)g_skinMgr.paths.first) / (int)sizeof(struct String));
        list = ui_window_open(UI_WND_COMBO_BOX);
        if (!list)
            return 0;
        list->id = ID_SKIN;
        ui_window_send(list, NULL, COMBO_MSG_ADD_LINE, (intptr_t)msg_str(0xc12), 0, 0, 0);
        for (i = 0; i < count; i++)
            ui_window_send(list, NULL, COMBO_MSG_ADD_LINE, (intptr_t)skin_mgr_get_name(&g_skinMgr, i), 0, 0, 0);
        height = (count + 1) * 16;
        if (height >= 0x50)
            height = 0x50;
        place_list(self, list, W(self->skin_box), g_skinMgr.current + 1, height);
        return 0;
    case ID_APPLY:
        switch (ui_graphic_setting_wnd_check_changed(self)) {
        case 2:
            answer = ui_window_mgr_message_box(&g_windowMgr, display_prompt_text(self, MSG_CONFIRM_RESTART),
                                            UI_MSGBOX_QUESTION, 1, 0, 0, UI_MSGBOX_TITLE, 0x118, 0x78, 0);
            if (answer == UI_MSGBOX_ANSWER_NO)
                return 0;
            if (g_displayDeviceIndex != self->device || g_displayModeIndex != self->mode) {
                const struct DisplayMode *mode;

                g_displayDeviceIndex = self->device;
                g_displayModeIndex = self->mode;
                snprintf(g_deviceName, sizeof(g_deviceName), "%s",
                            string_cstr(&self->device_name));
                g_deviceGuid = self->device_guid;
                g_driverGuid = self->adapter_guid;
                mode = MODE_AT(&self->modes, self->mode);
                g_displayWidth = mode->width;
                g_displayHeight = mode->height;
                g_bitsPerPixel = mode->bpp;
            }
            if (apply_display_mode(self) == 1)
                return 0;
            connection_disconnect(rag_connection_transport(rag_connection_instance()));
            mode_send_message(client_current_mode(), 2, 0, 0, 0, 0);
            return 0;
        case 1: {
            int detail;
            BYTE refresh = 0;

            answer = ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_CONFIRM_APPLY), UI_MSGBOX_QUESTION, 1, 0,
                                            0, UI_MSGBOX_TITLE, 0x118, 0x78, 0);
            if (answer == UI_MSGBOX_ANSWER_NO)
                return 0;
            g_spriteMode = self->sprite_mode;
            if (self->sprite_mode == 0) {
                g_sprHalfWidth = 1;
                g_sprHalfHeight = 1;
            } else {
                g_sprHalfWidth = 0;
                g_sprHalfHeight = self->sprite_mode == 1;
            }
            g_textureMode = self->texture_detail;
            detail = self->texture_detail == 0 ? 4 : self->texture_detail == 1 ? 2 : 1;
            if (detail != g_textureDivisor) {
                g_textureDivisor = detail;
                refresh = 1;
            }
            if (g_trilinearFilter != W(self->box_trilinear)->state) {
                g_trilinearFilter = W(self->box_trilinear)->state;
                c3d_device_enable_mipmap(&g_3dDevice);
                texcache_reload_all(&g_textureCache);
            } else if (refresh) {
                texcache_reload_all(&g_textureCache);
            }
            if (W(self->box_mouse)->state != (g_session1.mouseExclusiveOption != 0))
                g_session1.mouseExclusiveOption = W(self->box_mouse)->state != 0;
            ui_graphic_setting_wnd_check_changed(self);
            ui_window_invalidate_base(WND(self));
            return 0;
        }
        default:
            return 0;
        }
    case ID_RESET:
        if (ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_CONFIRM_RESET), UI_MSGBOX_QUESTION, 1, 0, 0,
                                   UI_MSGBOX_TITLE, 0x118, 0x78, 0) != UI_MSGBOX_ANSWER_YES)
            return 0;
        ui_window_mgr_invalidate_all(&g_windowMgr);
        skin_mgr_set_current_skin_num(&g_skinMgr, 0);
        g_session1.monsterSnapOnNoSkill = session_get_default_option_int(&g_session1, "/snap", 0);
        g_session1.monsterSnapOnSkill = session_get_default_option_int(&g_session1, "/skillsnap", 0);
        g_session1.itemSnap = session_get_default_option_int(&g_session1, "/itemsnap", 0);
        g_session1.fogOn = session_get_default_option_int(&g_session1, "/fog", 1);
        g_session1.auraFull = session_get_default_option_int(&g_session1, "/aura", 1);
        g_session1.auraOff = session_get_default_option_int(&g_session1, "/aura2", 0);
        g_session1.effectOn = session_get_default_option_int(&g_session1, "/effect", 1);
        g_lightmap = session_get_default_option_int(&g_session1, "/lightmap", 1);
        g_session1.noCtrl = session_get_default_option_int(&g_session1, "/noctrl", 1);
        g_session1.noShift = session_get_default_option_int(&g_session1, "/noshift", 0);
        g_sound_settings.bgm_on = session_get_default_option_int(&g_session1, "/bgm", 1);
        mode_send_message(client_current_mode(), 0x5a, 0, 0, 0, 0);
        g_sound_settings.effects_on = session_get_default_option_int(&g_session1, "/sound", 1);
        g_sound_settings.effects_on = value;
        sound_set_bgm_volume(session_get_default_option_int(&g_session1, "Bgm_Volume", 100));
        sound_set_effect_volume(session_get_default_option_int(&g_session1, "Effect_Volume", 100));
        sound_set_effect_volume_3d(session_get_default_option_int(&g_session1, "Effect_Volume", 100));
        ui_graphic_setting_wnd_load_options(self);
        return 0;
    case ID_COMMAND_LIST:
        if (ui_window_mgr_post_quit1(&g_windowMgr, WID_COMMAND_LIST))
            return 0;
        ui_window_open(WID_COMMAND_LIST);
        return 0;
    case ID_DEVICE:
        open_string_list(self, ID_DEVICE, &self->device_names, self->device, W(self->device_box));
        return 0;
    case ID_MODE:
        open_string_list(self, ID_MODE, &self->mode_names, self->mode, W(self->mode_box));
        return 0;
    default:
        return 0;
    }
}

static int scroll_step(int msg, int delta)
{
    if (msg == UI_MSG_PAGE_LEFT)
        return -1;
    if (msg == UI_MSG_PAGE_RIGHT)
        return 1;
    return delta;
}

intptr_t ui_graphic_setting_wnd_send_msg(struct UIWindow *window,
                                           struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3,
                                           intptr_t val4)
{
    struct UIGraphicSettingWnd *self = (struct UIGraphicSettingWnd *)window;
    int old, pos;

    switch (msg) {
    case UI_MSG_COMMAND:
        return on_command(self, sender, val1, val2);
    case UI_MSG_SCROLL_H:
    case UI_MSG_PAGE_LEFT:
    case UI_MSG_PAGE_RIGHT:
        if (sender == W(self->bgm_volume)) {
            old = sound_bgm_volume();
            pos = scroll_step(msg, val1) + sound_bgm_volume();
            pos = pos < 0 ? 0 : pos > 0x7f ? 0x7f : pos;
            ui_scroll_bar_set_pos(self->bgm_volume, pos);
            ui_window_invalidate_base(&self->bgm_volume->base);
            ui_window_invalidate_base(WND(self));
            sound_set_bgm_volume(pos);
            sound_set_bgm_fade_target(sound_bgm_volume());
            return pos - old;
        }
        if (sender == W(self->effect_volume)) {
            old = sound_effect_volume();
            pos = scroll_step(msg, val1) + old;
            pos = pos < 0 ? 0 : pos > 0x7f ? 0x7f : pos;
            ui_scroll_bar_set_pos(self->effect_volume, pos);
            ui_window_invalidate_base(&self->effect_volume->base);
            ui_window_invalidate_base(WND(self));
            sound_set_effect_volume(pos);
            sound_set_effect_volume_3d(pos);
            return pos - old;
        }
        if (sender == W(self->sprite_bar)) {
            old = self->sprite_mode;
            self->sprite_mode = clamp_0_2(old + scroll_step(msg, val1));
            ui_scroll_bar_set_pos(self->sprite_bar, self->sprite_mode);
            ui_window_invalidate_base(&self->sprite_bar->base);
            ui_graphic_setting_wnd_check_changed(self);
            ui_window_invalidate_base(WND(self));
            return self->sprite_mode - old;
        }
        if (sender == W(self->texture_bar)) {
            old = self->texture_detail;
            self->texture_detail = clamp_0_2(old + scroll_step(msg, val1));
            ui_scroll_bar_set_pos(self->texture_bar, self->texture_detail);
            ui_window_invalidate_base(&self->texture_bar->base);
            ui_graphic_setting_wnd_check_changed(self);
            ui_window_invalidate_base(WND(self));
            return self->texture_detail - old;
        }
        return 0;
    case UI_MSG_TAB_CHANGED:
        if (self->tab == val1)
            return 0;
        self->tab = val1;
        ui_graphic_setting_wnd_apply_tab(self);
        return 0;
    case GS_MSG_RELOAD:
        ui_graphic_setting_wnd_load_options(self);
        return 0;
    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!graphic_setting_wnd_info_is_valid(self->info))
            graphic_setting_wnd_info_make_default(self->info);
        ui_frame_wnd_move(WND(self), self->info->x, self->info->y);
        return 0;
    case COMBO_MSG_CHOSEN:
        if (!g_windowMgr.combo_box_wnd)
            return 0;
        switch (g_windowMgr.combo_box_wnd->id) {
        case ID_SKIN:
            if (g_skinMgr.current == val1 - 1)
                return 0;
            ui_window_mgr_invalidate_all(&g_windowMgr);
            skin_mgr_set_current_skin_num(&g_skinMgr, val1 - 1);
            return 0;
        case ID_DEVICE:
            if (self->device == val1)
                return 0;
            self->device = val1;
            ui_graphic_setting_wnd_build_mode_list(self);
            ui_graphic_setting_wnd_check_changed(self);
            break;
        case ID_MODE:
            if (self->mode == val1)
                return 0;
            self->mode = val1;
            ui_graphic_setting_wnd_check_changed(self);
            break;
        default:
            return 0;
        }
        ui_window_invalidate_base(WND(self));
        return 0;
    case GS_MSG_REDRAW:
        ui_window_invalidate_base(WND(self));
        return 0;
    default:
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    }
}
