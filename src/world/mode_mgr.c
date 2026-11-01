#include "render/device.h"
#include "render/texture_cache.h"
#include "web/service_managers.h"
#include "contents/sound.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_mgr.h"
#include "contents/mouse.h"
#include "world/mode.h"

#include <string.h>
#include <stdlib.h>
#include <io.h>
#include <direct.h>
#include <shellapi.h>
#include <mmsystem.h>
#include "core/app.h"
#include "platform/language.h"
#include "core/text_util.h"
#include "render/screen_draw.h"

static int g_screenShotCounter = 0;

void mode_nothing(struct Mode *self)
{
}

int mode_send_msg_base(struct Mode *self, int msg, int a, int b, int c, int d)
{
    const char *name;

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
        if (g_serviceType == 0xb)
            ShellExecuteA(g_hMainWnd, "open", "http://www.ragnarok.co.in/index.php", NULL, NULL, SW_SHOWNORMAL);
        g_modeMgr.loopCond = 0;
        return 0;
    case MODE_MSG_SCREENSHOT:
        name = (const char *)a;
        self->screenshot_pending = 1;
        if (name)
            string_assign_cstr(&self->screenshot_name, name);
        return 0;
    }
    return 0;
}

void mode_process_fade_in(struct Mode *self)
{
    DWORD elapsed;

    if (self->fade_in_start) {
        g_fadeStart = timeGetTime();
        sound_bgm_fade_in();
        self->fade_in_start = 0;
    }
    elapsed = timeGetTime() - g_fadeStart;
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
    unsigned frame = (timeGetTime() - self->cursor_anim_start) / 24;
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
            self->cursor_anim_start = timeGetTime();
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

static void watermark_code(int id, char code[12])
{
    int divisor = 100000000;
    int pos = 0;

    memset(code, 0, 12);
    do {
        int digit = 0;
        while (id >= divisor) {
            id -= divisor;
            digit++;
        }
        divisor /= 10;
        if (pos > 0)
            pos++;
        else if (digit > 0)
            pos = 1;
        switch (digit) {
        case 0: if (pos > 0) code[pos - 1] = 'R'; break;
        case 1: code[pos - 1] = 'O'; break;
        case 2: code[pos - 1] = 'H'; break;
        case 3: code[pos - 1] = 'U'; break;
        case 4: code[pos - 1] = 'T'; break;
        case 5: code[pos - 1] = 'N'; break;
        case 6: code[pos - 1] = 'A'; break;
        case 7: code[pos - 1] = 'S'; break;
        case 8: code[pos - 1] = 'E'; break;
        case 9: code[pos - 1] = 'W'; break;
        }
    } while (divisor > 0);
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
    static const char *const weekdays[7] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
    };
    SYSTEMTIME st;
    char stamp[0x80];
    struct String dir, name, prefix, ext, number, path, arg;
    int h;

    if (!self->screenshot_pending)
        return;
    GetLocalTime(&st);
    memset(stamp, 0, sizeof stamp);
    if (g_serviceType != 0) {
        sprintf(stamp, "%d/%02d/%02d/%s  %02d: %02d: %02d", st.wYear, st.wMonth, st.wDay,
                    weekdays[st.wDayOfWeek], st.wHour, st.wMinute, st.wSecond);
    } else if (g_session1.ownGid != 0) {
        char code[12];
        watermark_code(g_session1.ownGid, code);
        sprintf(stamp, "%d/%02d/%02d/%s  %02d: %02d: %02d     %s", st.wYear, st.wMonth, st.wDay,
                    weekdays[st.wDayOfWeek], st.wHour, st.wMinute, st.wSecond, code);
    }
    h = g_renderer->height;
    renderer_text_out(g_renderer, 5, h - 0x10, stamp, 0);
    renderer_text_out(g_renderer, 3, g_renderer->height - 0x10, stamp, 0);
    renderer_text_out(g_renderer, 4, g_renderer->height - 0xf, stamp, 0);
    renderer_text_out(g_renderer, 4, g_renderer->height - 0x11, stamp, 0);
    renderer_text_out(g_renderer, 4, g_renderer->height - 0x10, stamp, 0xffffff);

    self->screenshot_pending = 0;
    string_init(&dir);
    string_format(&dir, "%s\\ScreenShot", g_clientDir);
    if (_access(string_cstr(&dir), 0) != 0 && _mkdir(string_cstr(&dir)) != 0) {
        screenshot_save_error(string_cstr(&dir));
        string_clear(&self->screenshot_name);
        string_free(&dir);
        return;
    }

    string_copy(&name, &g_session1.serverName);
    file_name_sanitize(&name);
    file_name_sanitize(&self->screenshot_name);

    string_init(&prefix);
    string_append(&prefix, string_cstr(&dir), dir.size);
    string_append_cstr(&prefix, "\\");
    string_append(&prefix, string_cstr(&self->screenshot_name), self->screenshot_name.size);
    string_append_cstr(&prefix, "screen");
    string_append(&prefix, string_cstr(&name), name.size);
    string_init(&ext);
    string_assign_cstr(&ext, ".jpg");

    for (;;) {
        string_init(&number);
        string_format(&number, "%03d", g_screenShotCounter++);
        string_init(&path);
        string_append(&path, string_cstr(&prefix), prefix.size);
        string_append(&path, string_cstr(&number), number.size);
        string_append(&path, string_cstr(&ext), ext.size);
        string_free(&number);
        if (_access(string_cstr(&path), 0) != 0)
            break;
        string_free(&path);
    }

    string_copy(&arg, &path);
    if (renderer_screenshot(g_renderer, arg)) {
        string_append_cstr(&path, " is Saved.");
        ui_window_mgr_send_msg_(&g_windowMgr, WM_MSG_CHAT_LINE, (int)string_cstr(&path), 0xff00, 0, 0);
    } else {
        screenshot_save_error(string_cstr(&path));
    }
    string_clear(&self->screenshot_name);

    string_free(&path);
    string_free(&ext);
    string_free(&prefix);
    string_free(&name);
    string_free(&dir);
}

void mode_mgr_quit(struct ModeMgr *self)
{
    if (self->curMode)
        self->curMode->loop_cond = 0;
    if (g_serviceType == 0xb)
        ShellExecuteA(g_hMainWnd, "open", "http://www.ragnarok.co.in/index.php", NULL, NULL, SW_SHOWNORMAL);
    self->loopCond = 0;
}

struct Mode *mode_mgr_get_login_mode(struct ModeMgr *self)
{
    if (self->curModeType != MODE_TYPE_LOGIN)
        return NULL;
    return self->curMode;
}
