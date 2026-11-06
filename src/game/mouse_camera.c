#include "world/mode_actions.h"
#include "game/gamemode.h"
#include "session/option_pairs.h"
#include "skills/skill_bundle.h"
#include "contents/sound.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "contents/mouse.h"
#include <windows.h>
#include <mmsystem.h>
#include "game/mouse_camera.h"
#include "skills/skill_info.h"
#include "game/client.h"
#include "game/camera.h"
#include "actor/actor_common.h"
#include "session/session_jobs.h"
#include "session/session_options.h"
#include "session/session_state.h"
#include "session/hotkey_types.h"
#include "ui/hotkeys/hotkeys.h"
#include "world/camera.h"

float g_maxViewDistance = 1224.0f;

enum { JOB_STAR_GLADIATOR = 0xfcf, JOB_BABY_STAR_GLADIATOR = 0x1082, JOB_STAR_EMPEROR = 0x108f, JOB_BABY_STAR_EMPEROR = 0x1091 };

enum { SKILL_SG_DEVIL = 0x1b9, SKILL_DA_DREAM = 0x27f };

struct GameActor;
enum { EFFECT_BLIND = 0x14e };

enum { BUTTON_PRESSED = 1, BUTTON_HELD = 2, BUTTON_DOUBLE = 4 };

enum { SKILL_GC_ROLLINGCUTTER = 0x7f4, BINDMSG_RUN = 0x29 };
enum { BIND_SLOT_WHEEL_DOWN = 6, BIND_SLOT_WHEEL_UP = 7, BIND_SLOT_RIGHT_CLICK = 8 };

enum { WID_RELEASE_CLOSE = 0x12 };
enum { CURSOR_TURN = 4 };

enum { GMMSG_CANCEL_DRAG = 0x12, GMMSG_CANCEL_SKILL_AIM = 0x47 };

static int key_down(int vk)
{
    return (unsigned char)((unsigned)GetAsyncKeyState(vk) >> 8) != 0;
}

static void set_pitch(struct GameMode *self, float pitch, float range)
{
    g_viewPitch = pitch;
    if (self->has_camera_preset) {
        float lo = (float)self->camera_preset.pitch_min, hi;
        if (lo > pitch) {
            pitch = lo;
            g_viewPitch = pitch;
        }
        hi = (float)self->camera_preset.pitch_max;
        if (pitch > hi) {
            pitch = hi;
            g_viewPitch = pitch;
        }
    } else {
        float lo = -45.0f - range, hi;
        if (lo > pitch)
            g_viewPitch = lo;
        if (key_down(VK_CONTROL) && g_maxViewDistance >= self->view->destDistance)
            hi = range - 45.0f;
        else
            hi = range - 70.0f;
        pitch = g_viewPitch;
        if (pitch > hi) {
            pitch = hi;
            g_viewPitch = pitch;
        }
    }
    self->view->destLatitude = pitch;
}

static float bound_distance(struct GameMode *self)
{
    float d = g_viewDistance, lo = (float)self->camera_preset.distance_min, hi;

    if (lo > d) {
        d = lo;
        g_viewDistance = d;
    }
    hi = (float)(self->camera_preset.distance_min + self->camera_preset.distance_range);
    if (d > hi) {
        d = hi;
        g_viewDistance = d;
    }
    return d;
}

static float limit_distance(float far_limit)
{
    float d = g_viewDistance;

    if (g_viewDistanceMin > d) {
        d = g_viewDistanceMin;
        g_viewDistance = d;
    }
    if (d > far_limit) {
        d = far_limit;
        g_viewDistance = d;
    }
    return d;
}

static void run_binding(struct UIWindow *bar, int slot)
{
    const struct HotkeySlot *b = ui_short_cut_wnd_slot(bar, slot);

    if (b->is_skill && b->id == SKILL_GC_ROLLINGCUTTER) {
        int count = session_get_instant_var(&g_session1, 4) + 1;
        session_set_pair(&g_session1, 4, count);
    } else {
        session_reset_option2(&g_session1);
    }
    ui_window_send(bar, NULL, BINDMSG_RUN, slot, 0, 0, 0);
}

static void right_button_pressed(struct GameMode *self)
{
    struct UIWindow *w;

    if (g_windowMgr.menu_wnd)
        ui_window_mgr_post_quit1(&g_windowMgr, WID_RELEASE_CLOSE);
    mode_send_msg(self, GMMSG_CANCEL_DRAG, 0, 0, 0, 0);
    mode_send_msg(self, GMMSG_CANCEL_SKILL_AIM, 0, 0, 0, 0);
    self->right_press_x = g_mouse.x;
    self->right_press_y = g_mouse.y;
    if (g_session1.quickSpellRightClick != 1 || (w = g_windowMgr.short_cut_wnd) == 0)
        return;
    run_binding(w, BIND_SLOT_RIGHT_CLICK);
}

static void right_button_held(struct GameMode *self)
{
    if (!key_down(VK_MENU) && !key_down(VK_SHIFT)) {
        mode_set_cursor_action(&self->base, CURSOR_TURN);
        if (self->has_camera_preset) {
            struct View *view = self->view;
            float heading = ((float)g_mouse.x + view->destLongitude) - (float)self->last_mouse_x;
            float lo = (float)self->camera_preset.yaw_min, hi = (float)self->camera_preset.yaw_max;
            heading = lo > heading ? lo : heading;
            heading = hi < heading ? hi : heading;
            view->destLongitude = heading;
        } else if (!self->camera_free && !session_is_admin_option(&g_session1, 6)) {
            struct View *view = self->view;
            float centre = self->yaw_centre;
            float heading = ((float)g_mouse.x + view->destLongitude) - (float)self->last_mouse_x;
            float lo = centre - 20.0f, hi = centre + 20.0f;
            heading = lo > heading ? lo : heading;
            heading = hi < heading ? hi : heading;
            view->destLongitude = heading;
        } else {
            view_add_longitude(self->view, (float)(g_mouse.x - self->last_mouse_x));
        }
        sound_set_listener_angle(self->view->destLongitude);
    }

    if (!key_down(VK_MENU) && key_down(VK_SHIFT)) {
        int dy = (g_mouse.y - self->last_mouse_y) / 2;
        float range = 10.0f;
        if (self->camera_free)
            range = 42.0f;
        set_pitch(self, self->view->destLatitude - (float)dy, range);
    }

    if (key_down(VK_CONTROL)) {
        int dy = (g_mouse.y - self->last_mouse_y) / 2;
        float far_limit, d;
        g_viewDistance = self->view->destDistance - (float)dy;
        far_limit = self->camera_free ? g_maxViewDistance : g_fixedViewDistanceMax;
        if (self->has_camera_preset)
            d = bound_distance(self);
        else if (!session_is_admin_option(&g_session1, 5))
            d = limit_distance(far_limit);
        else
            d = g_viewDistance;
        self->view->destDistance = d;
    }
}

static void right_button_double(struct GameMode *self)
{
    if (self->has_camera_preset)
        return;
    if (key_down(VK_CONTROL))
        self->view->destDistance = g_fixedViewDistanceMax;
    if (key_down(VK_SHIFT))
        self->view->destLatitude = -50.0f;
    if (g_session1.quickSpellRightClick != 1 && self->camera_free && !key_down(VK_SHIFT) && !key_down(VK_CONTROL))
        self->view->destLongitude = 0.0f;
}

__declspec(noinline) void game_mode_process_right_button(struct GameMode *self)
{
    switch (g_mouse.button[MOUSE_RIGHT]) {
    case BUTTON_PRESSED:
        right_button_pressed(self);
        break;
    case BUTTON_HELD:
        right_button_held(self);
        break;
    case BUTTON_DOUBLE:
        right_button_double(self);
        break;
    }
}

static void wheel_zoom(struct GameMode *self)
{
    struct SkillInfo skill;
    struct Player *player = NULL;
    float step = g_mouse.wheel > 0 ? -24.3f : 24.3f, far_limit, d;
    int job_zoom;

    g_viewDistance = self->view->destDistance - step;
    far_limit = self->camera_free ? g_maxViewDistance : g_fixedViewDistanceMax;
    skill_table_find(&g_session1.skills, &skill, SKILL_SG_DEVIL);
    job_zoom = skill.level <= 0
               && (session_get_drawn_job(&g_session1) == JOB_STAR_GLADIATOR || session_get_drawn_job(&g_session1) == JOB_BABY_STAR_GLADIATOR
                   || session_get_drawn_job(&g_session1) == JOB_STAR_EMPEROR || session_get_drawn_job(&g_session1) == JOB_BABY_STAR_EMPEROR)
               && (player = self->world->player)->state == 6;
    if (job_zoom && !self->has_camera_preset
            && !able_to_make_effect_is_effect_exist((struct GameActor *)player, EFFECT_BLIND)) {
        float far_x3;
        d = g_viewDistance;
        if (g_viewDistanceMin > d) {
            d = g_viewDistanceMin;
            g_viewDistance = d;
        }
        far_x3 = far_limit * 3.0f;
        if (d > far_x3) {
            d = far_x3;
            g_viewDistance = d;
        }
        if (d > far_limit && self->view->destLatitude > -50.0f) {
            self->view->destLatitude = -50.0f;
            d = g_viewDistance;
        }
    } else if (self->has_camera_preset) {
        d = bound_distance(self);
    } else {
        d = limit_distance(far_limit);
    }
    self->view->destDistance = d;
    skill_info_destruct(&skill);
}

__declspec(noinline) void game_mode_process_wheel(struct GameMode *self, int verdict)
{
    struct SkillInfo skill;

    if (timeGetTime() - self->throttle_time <= 15)
        return;
    self->throttle_time = timeGetTime();
    if (g_mouse.wheel == 0 || self->skill_use.type != 0)
        return;

    skill_table_find(&g_session1.skills, &skill, SKILL_DA_DREAM);
    if (skill.level > 0) {
        int n = self->wheel_count;
        if (n > self->wheel_count_max)
            self->wheel_count_max = n;
        self->wheel_count = n + 4;
    }

    if (key_down(VK_SHIFT)) {
        float step = g_mouse.wheel > 0 ? -5.0f : 5.0f;
        float range = self->camera_free ? 20.0f : 10.0f;
        set_pitch(self, self->view->destLatitude - step, range);
    } else if (!verdict || key_down(VK_CONTROL)) {
        if (g_session1.quickSpellWheel == 1) {
            struct UIWindow *w = g_windowMgr.short_cut_wnd;
            g_session1.wheelConsumed = 1;
            if (g_mouse.wheel > 0) {
                if (w)
                    run_binding(w, BIND_SLOT_WHEEL_UP);
            } else if (w) {
                run_binding(w, BIND_SLOT_WHEEL_DOWN);
            }
        } else {
            wheel_zoom(self);
        }
    }
    skill_info_destruct(&skill);
}
