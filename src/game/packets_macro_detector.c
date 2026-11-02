#include "game/objects.h"
#include "ui/chat/captcha.h"
#include "ui/chat/captcha_data.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include "game/packets_macro_detector.h"
#include "core/msg_str.h"

enum {
    WID_MACRO_DETECTOR = 0x11d,
    WID_CAPTCHA_PREVIEW = 0x120,
};
void game_mode_zc_ack_preview_macro_detector(struct GameMode *self, int result, int image_size,
                                                       const char *key)
{
    struct UIWindow *wnd;
    int created;

    (void)self;
    switch (result) {
    case 0:
        if (!key || image_size <= 0 || image_size > CAPTCHA_PACKED_LIMIT) return;
        wnd = ui_window_find(WID_CAPTCHA_PREVIEW);
        created = wnd == NULL;
        if (created) {
            wnd = ui_window_open(WID_CAPTCHA_PREVIEW);
            if (wnd == 0)
                return;
        }
        if (!captcha_preview_begin(wnd, key, (size_t)image_size) && created)
            ui_window_mgr_post_quit1(&g_windowMgr, WID_CAPTCHA_PREVIEW);
        break;
    case 1:
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)"wrong captcha ID.", 0xff, 0, 0);
        break;
    }
}

void game_mode_zc_apply_macro_detector(struct GameMode *self, int image_size, const char *key)
{
    struct UIWindow *wnd;

    (void)self;
    if (!key || image_size <= 0 || image_size > CAPTCHA_PACKED_LIMIT) return;
    if (ui_window_find(WID_MACRO_DETECTOR) != 0)
        return;
    wnd = ui_window_open(WID_MACRO_DETECTOR);
    if (wnd == 0)
        return;
    if (!captcha_detector_begin(wnd, key, (size_t)image_size))
        ui_window_mgr_post_quit1(&g_windowMgr, WID_MACRO_DETECTOR);
}

void game_mode_zc_apply_macro_detector_captcha(struct GameMode *self, const char *key, const void *data,
                                                         int size)
{
    struct UIWindow *wnd;

    (void)self;
    wnd = ui_window_find(WID_MACRO_DETECTOR);
    if (size > 0) captcha_detector_append(wnd, key, data, (size_t)size);
}

void game_mode_zc_close_macro_detector(struct GameMode *self, int result)
{
    int msg;

    (void)self;
    switch (result) {
    case 0: msg = 0xb35; break;
    case 1: msg = 0xb36; break;
    case 2:
        ui_window_mgr_post_quit1(&g_windowMgr, WID_MACRO_DETECTOR);
        ui_window_mgr_message_box(&g_windowMgr, msg_str(0xb37), 0, 1, 0, 0, "메시지", 0x118, 0x78, 0);
        return;
    default:
        return;
    }
    ui_window_mgr_message_box(&g_windowMgr, msg_str(msg), 0, 1, 0, 0, "메시지", 0x118, 0x78, 0);
    connection_disconnect(rag_connection_transport(rag_connection_instance()));
    mode_send_message(client_current_mode(), 2, 0, 0, 0, 0);
}

void game_mode_zc_ack_apply_macro_detector(struct GameMode *self, int result)
{
    int msg;

    (void)self;
    switch (result) {
    case 0: msg = 0xb3d; break;
    case 1: msg = 0xb3e; break;
    case 2: msg = 0xb3f; break;
    default: return;
    }
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)msg_str(msg), 0xff, 0, 0);
}
