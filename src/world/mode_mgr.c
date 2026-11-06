#include "contents/adventurer_agency.h"
#include "contents/character_settings.h"
#include "contents/account_settings.h"
#include "render/device.h"
#include "render/texture_cache.h"
#include "contents/sound.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_mgr.h"
#include "contents/mouse.h"
#include "world/mode.h"

#include <stdlib.h>
#include "core/app.h"
#include "core/client_path.h"
#include "platform/filesystem.h"
#include "platform/language.h"
#include "core/text_util.h"
#include "render/screen_draw.h"
#include "platform/clock.h"

static int g_screenShotCounter = 0;

intptr_t mode_send_msg_base(struct Mode *self, int msg, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{
    switch (msg) {
    case MODE_MSG_QUIT:
        account_user_data_save(g_accountUserDataMgr);
        if (g_modeMgr.curModeType == MODE_TYPE_GAME && g_modeMgr.curMode) {
            char_user_data_save(g_charUserDataMgr);
            if (g_modeMgr.curModeType == MODE_TYPE_GAME && g_modeMgr.curMode)
                adventurer_agency_cancel_own_booking(g_adventurerAgency);
        }
        if (g_modeMgr.curMode)
            g_modeMgr.curMode->loop_cond = 0;
        g_modeMgr.loopCond = 0;
        return 0;
    case MODE_MSG_SCREENSHOT:
        self->screenshot_pending = 1;
        return 0;
    }
    return 0;
}

void mode_process_fade_in(struct Mode *self)
{
    DWORD elapsed;

    if (self->fade_in_start) {
        g_fadeStart = platform_ticks_ms();
        sound_bgm_fade_in();
        self->fade_in_start = 0;
    }
    elapsed = platform_ticks_ms() - g_fadeStart;
    if (elapsed < 0xff)
        draw_box_screen(0, 0, g_renderer->width, g_renderer->height, (0xff - elapsed) << 24);
}

static void cursor_play_once(struct Mode *self, void *act, unsigned frame, unsigned divisor, int wrapAction)
{
    unsigned step = frame / divisor;

    self->cursor_motion = step % actres_frame_count(act, wrapAction);
    if (step / actres_frame_count(act, self->cursor_action) >= 1)
        self->cursor_motion = actres_frame_count(act, self->cursor_action) - 1;
}

void mode_draw_mouse_cursor(struct Mode *self)
{
    void *act = res_mgr_get(g_resMgr(), "cursors.act");
    void *spr = res_mgr_get(g_resMgr(), "cursors.spr");
    unsigned frame = (platform_ticks_ms() - self->cursor_anim_start) / 24;
    unsigned divisor;
    int action;

    act_res_get_delay(act, self->cursor_action);
    action = self->cursor_action;
    switch (action) {
    case 5:  divisor = 20; break;
    case 7:  divisor = 10; break;
    case 9:  divisor = 1;  break;
    case 10: divisor = 6;  break;
    case 11: divisor = 3;  break;
    case 13: divisor = 2;  break;
    default: divisor = 4;  break;
    }

    if (action == CURSOR_ACT_ONCE) {
        cursor_play_once(self, act, frame, divisor, 1);
    } else if (action == 2 || action == 9) {
        if (g_mouse.button[MOUSE_LEFT]) {
            cursor_play_once(self, act, frame, divisor, self->cursor_action);
        } else {
            self->cursor_anim_start = platform_ticks_ms();
            self->cursor_motion = 0;
        }
    } else {
        self->cursor_motion = frame / divisor % actres_frame_count(act, action);
    }

    ui_window_mgr_send_msg_(&g_windowMgr, WM_MSG_CURSOR_MOTION, self->cursor_motion, 0, 0, 0);
    screen_sprite_frame(g_renderer, (int)((float)g_mouse.x + self->cursor_offset_x), (int)((float)g_mouse.y + self->cursor_offset_y),
               act, spr, self->cursor_action, self->cursor_motion, 1.0f, 0.0f, 0xfaffffff, 0x205);
}

static void file_name_sanitize(struct String *s)
{
    unsigned i;

    for (i = 0; i < s->size; i++) {
        char c = string_cstr(s)[i];
        if (c == '\\' || c == '/' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' ||
            c == '>' || c == '|')
            s->data[i] = '_';
    }
}

static void screenshot_save_error(const char *path)
{
    struct String message;
    string_init(&message);
    string_format(&message, "Could not save screenshot:\n%s", path);
    ui_message_box(string_cstr(&message));
    string_free(&message);
}

void mode_screen_shot(struct Mode *self)
{
    struct String name, prefix, ext, number, path, arg;
    char native[0x800];

    if (!self->screenshot_pending)
        return;
    self->screenshot_pending = 0;
    client_path(native, sizeof native, "ScreenShot");
    if (!platform_path_exists(native) && platform_directory_create(native) != 0) {
        screenshot_save_error(native);
        return;
    }

    string_copy(&name, &g_session1.serverName);
    file_name_sanitize(&name);

    string_init(&prefix);
    string_append_cstr(&prefix, "ScreenShot\\");
    string_append_cstr(&prefix, "screen");
    string_append(&prefix, string_cstr(&name), name.size);
    string_init(&ext);
    string_assign_cstr(&ext, ".jpg");

    for (;;) {
        string_init(&number);
        string_format(&number, "%03d", g_screenShotCounter++);
        client_path(native, sizeof native, "%s%s%s", string_cstr(&prefix), string_cstr(&number),
                    string_cstr(&ext));
        string_free(&number);
        if (!platform_path_exists(native))
            break;
    }
    string_init(&path);
    string_assign_cstr(&path, native);

    string_copy(&arg, &path);
    if (renderer_screenshot(g_renderer, arg)) {
        string_append_cstr(&path, " is Saved.");
        ui_window_mgr_send_msg_(&g_windowMgr, WM_MSG_CHAT_LINE, (intptr_t)string_cstr(&path), 0xff00, 0, 0);
    } else {
        screenshot_save_error(string_cstr(&path));
    }

    string_free(&path);
    string_free(&ext);
    string_free(&prefix);
    string_free(&name);
}

void mode_mgr_quit(struct ModeMgr *self)
{
    if (self->curMode)
        self->curMode->loop_cond = 0;
    self->loopCond = 0;
}

struct Mode *mode_mgr_get_login_mode(struct ModeMgr *self)
{
    if (self->curModeType != MODE_TYPE_LOGIN)
        return NULL;
    return self->curMode;
}
