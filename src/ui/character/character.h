#include "game/mode_misc.h"
#include "render/sprite_atlas.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#ifndef UI_CHARACTER_H
#define UI_CHARACTER_H

#include "core/markup_tags.h"
#include "session/hotkey_bindings.h"
#include <string.h>
#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"

static __inline struct UIBitmapButton *ui_bitmap_button(const char *normal, const char *hover,
                                                        const char *pressed)
{
    struct UIBitmapButton *button = mem_alloc(sizeof(struct UIBitmapButton));

    if (button)
        button = ui_bitmap_button_construct(button);
    ui_bitmap_button_set_bitmap(button, normal, UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(button, hover, UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(button, pressed, UI_BTN_DOWN, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    return button;
}

#define UI_POS_HIDDEN (-100)

struct UIBarGraph {
    struct UIWindow base;
    float pos;
};

struct UIBarGraph2 {
    struct UIBarGraph graph;
    int vertical;
    int back_red, back_green, back_blue;
    int bar_red, bar_green, bar_blue;
};
struct UIWindow *ui_bar_graph2_construct(struct UIWindow *self);
void ui_bar_graph_set_pos(struct UIWindow *self, float pos);

struct UIBarGraph3 {
    struct UIWindow base;
    int *current;
    int *maximum;
};
struct UIBarGraph4 {
    struct UIBarGraph3 graph;
    BYTE no_warning;
};
struct UIBarGraph3 *ui_bar_graph3_construct(struct UIBarGraph3 *self);
void ui_bar_graph3_set_values(struct UIBarGraph3 *self, int *current, int *maximum);

struct UIJobListWnd;
void ui_job_list_wnd_show_jobs(struct UIJobListWnd *self);

struct ActRes;
static __inline struct SprImg *spr_res_image(struct SprRes *spr, const struct SprClip *clip)
{
    return clip ? sprite_resource_image(spr, clip->clipType, clip->sprIndex) : NULL;
}

struct UIButtonsOfBasicWnd *ui_buttons_of_basic_wnd_construct(struct UIButtonsOfBasicWnd *self);
struct UIWindow *ui_buttons_of_basic_wnd_new(void);
void ui_buttons_of_basic_wnd_render_buttons(struct UIButtonsOfBasicWnd *self);

struct UIChangeNameWnd *ui_change_name_wnd_construct(struct UIChangeNameWnd *self);
struct UIWindow *ui_change_name_wnd_new(void);

struct UIEscOptionWnd *ui_esc_option_wnd_construct(struct UIEscOptionWnd *self);
struct UIWindow *ui_esc_option_wnd_new(void);

struct UIKeyStrokeWnd *ui_key_stroke_wnd_construct(struct UIKeyStrokeWnd *self);
struct UIWindow *ui_key_stroke_wnd_new(void);
struct UIViewCameraInfoWnd *ui_view_camera_info_wnd_construct(struct UIViewCameraInfoWnd *self);
struct UIWindow *ui_view_camera_info_wnd_new(void);

void job_list_set_wanted_jobs(const short *jobs);
void job_map_init(void);
struct UIJobListWnd *ui_job_list_wnd_construct(struct UIJobListWnd *self);
struct UIWindow *ui_job_list_wnd_new(void);

struct UILoginOTPWnd *ui_login_otp_wnd_construct(struct UILoginOTPWnd *self);
struct UIWindow *ui_login_otp_wnd_new(void);
struct UILoginWnd *ui_login_wnd_construct(struct UILoginWnd *self);
struct UIWindow *ui_login_wnd_new(void);

struct UIMenuIconWnd *ui_menu_icon_wnd_construct(struct UIMenuIconWnd *self);
struct UIWindow *ui_menu_icon_wnd_new(void);

struct UIMonsterDisplayWnd *ui_monster_display_wnd_construct(struct UIMonsterDisplayWnd *self);
struct UIWindow *ui_monster_display_wnd_new(void);

struct UIMonsterInfoWnd *ui_monster_info_wnd_construct(struct UIMonsterInfoWnd *self);
struct UIWindow *ui_monster_info_wnd_new(void);

struct UIEmailAddressWnd *ui_email_address_wnd_new(void);
struct UIPasswordWnd *ui_password_wnd_construct(struct UIPasswordWnd *self);
struct UIWindow *ui_password_wnd_new(void);

struct UIDisconnectedServerMsgWnd *ui_disconnected_server_msg_wnd_construct(
    struct UIDisconnectedServerMsgWnd *self);
struct UIWindow *ui_disconnected_server_msg_wnd_new(void);
struct UILoadingWnd *ui_loading_wnd_construct(struct UILoadingWnd *self);
struct UIWindow *ui_loading_wnd_new(void);
struct UIQuitWnd *ui_quit_wnd_construct(struct UIQuitWnd *self);
struct UIWindow *ui_quit_wnd_new(void);
struct UIRebirthWnd *ui_rebirth_wnd_construct(struct UIRebirthWnd *self);
struct UIWindow *ui_rebirth_wnd_new(void);
struct UIRestartWnd *ui_restart_wnd_construct(struct UIRestartWnd *self);
struct UIWindow *ui_restart_wnd_new(void);
struct UIWaitWnd *ui_wait_wnd_construct(struct UIWaitWnd *self);
struct UIWindow *ui_wait_wnd_new(void);
void ui_wait_wnd_set_text(struct UIWindow *wnd, const char *text);

struct UIShowWarpWnd *ui_show_warp_wnd_construct(struct UIShowWarpWnd *self);
struct UIWindow *ui_show_warp_wnd_new(void);

struct UIWordDisplayWnd *ui_word_display_wnd_new(void);

#endif
