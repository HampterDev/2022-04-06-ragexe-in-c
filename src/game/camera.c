#include "game/packet_handlers.h"
#include "game/gamemode.h"
#include "render/math3d.h"
#include "session/state.h"
#include "ui/window_access.h"
#include <stdlib.h>
#include "game/camera.h"
#include "game/client.h"
#include "session/session_names.h"
#include "game/mouse_camera.h"

float g_fixedViewDistance = 300.0f;

float g_fixedViewDistanceMax = 300.0f;

float g_fixedViewPitch = -45.0f;

float g_freeViewDistance = 300.0f;

float g_freeViewPitch = -45.0f;

float g_viewDistance = 300.0f;

float g_viewDistanceMin = 230.0f;

float g_viewPitch = -45.0f;

#define VIEW_PITCH_CENTRE   -45.0f
#define VIEW_PITCH_RANGE_FIXED 10.0f
#define VIEW_PITCH_RANGE_FREE  20.0f
#define FIXED_VIEW_HEADING  45.0f

#include "actor/render_ops.h"
#define MOUSE_POINTER_MSG_SHOW_MARKER 1

static short preset_value(const struct Vec *preset, int i)
{
    return (short)(int)atof(string_cstr((const struct String *)preset->first + i));
}

void game_mode_init_view(struct GameMode *self)
{
    struct View *view = self->view;
    const struct Player *player = self->world->player;
    struct Vec *preset;
    float pitch_range, max_distance, pitch, distance;
    int found;

    view->at[0] = player->pos.x;
    view->at[1] = player->pos.y;
    view->at[2] = player->pos.z;
    player = self->world->player;
    view = self->view;
    view->destAt[0] = player->pos.x;
    view->destAt[1] = player->pos.y;
    view->destAt[2] = player->pos.z;

    self->has_camera_preset = 0;
    preset = session_get_map_view_preset(&g_session1, self->map_name, &found);
    if (found) {
        struct CameraPreset *camera = &self->camera_preset;

        self->has_camera_preset = 1;
        camera->distance_min = preset_value(preset, 0);
        camera->distance_range = preset_value(preset, 1);
        camera->distance = preset_value(preset, 2);
        camera->yaw_min = preset_value(preset, 3);
        camera->yaw_max = preset_value(preset, 4);
        camera->yaw = preset_value(preset, 5);
        camera->pitch_max = preset_value(preset, 6);
        camera->pitch_min = preset_value(preset, 7);
        camera->pitch = preset_value(preset, 8);
    }

    if (!session_is_fixed_view_map(&g_session1, self->map_name)) {
        self->camera_free = 1;
        pitch_range = VIEW_PITCH_RANGE_FREE;
        max_distance = g_maxViewDistance;
        pitch = g_freeViewPitch;
        distance = g_freeViewDistance;
    } else {
        self->camera_free = 0;
        pitch_range = VIEW_PITCH_RANGE_FIXED;
        max_distance = g_fixedViewDistanceMax;
        self->yaw_centre = FIXED_VIEW_HEADING;
        self->view->longitude = FIXED_VIEW_HEADING;
        self->view->destLongitude = self->yaw_centre;
        pitch = g_fixedViewPitch;
        distance = g_fixedViewDistance;
    }

    g_viewDistance = distance;
    g_viewPitch = pitch;
    if (VIEW_PITCH_CENTRE - pitch_range > pitch) {
        pitch = VIEW_PITCH_CENTRE - pitch_range;
        g_viewPitch = pitch;
    }
    if (pitch > pitch_range - 45.0f) {
        pitch = pitch_range - 45.0f;
        g_viewPitch = pitch;
    }
    if (g_viewDistanceMin > distance) {
        distance = g_viewDistanceMin;
        g_viewDistance = distance;
    }
    if (distance > max_distance)
        g_viewDistance = max_distance;

    if (self->has_camera_preset) {
        g_viewDistance = (float)self->camera_preset.distance;
        self->view->longitude = (float)self->camera_preset.yaw;
        self->view->destLongitude = (float)self->camera_preset.yaw;
        pitch = (float)self->camera_preset.pitch;
        g_viewPitch = pitch;
    }

    self->view->latitude = pitch;
    self->view->destLatitude = g_viewPitch;
    self->view->distance = g_viewDistance;
    self->view->destDistance = g_viewDistance;
}

float game_mode_get_view_heading(struct GameMode *self)
{
    if (self->view == 0)
        return 0.0f;
    return self->view->destLongitude;
}

void game_mode_point_mouse_at(struct GameMode *self, float x, float z, int red)
{
    float position[2];
    void *pointer = self->mouse_pointer;

    position[0] = x;
    position[1] = z;
    render_object_send(pointer, NULL, MOUSE_POINTER_MSG_SHOW_MARKER, MSG_ARG_PTR(position), red, 0, 0);
}

#define WID_CAMERA_INFO 0x121

#pragma pack(push, 1)
struct PACKET_ZC_VIEW_CAMERAINFO {
    unsigned short packet_type;
    unsigned char action;
    unsigned distance;
    unsigned heading;
    unsigned pitch;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_VIEW_CAMERAINFO, 15);

void game_mode_camera_info(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_VIEW_CAMERAINFO *packet = (const struct PACKET_ZC_VIEW_CAMERAINFO *)pkt;
    struct UIWindow *wnd;

    if (packet->action == 1) {
        wnd = ui_window_find(WID_CAMERA_INFO);
        if (wnd == 0)
            wnd = ui_window_open(WID_CAMERA_INFO);
        g_session1.cameraInfoShown = g_session1.cameraInfoShown == 0;
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_REFRESH, 0, 0, 0, 0);
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE,
                            (int)(g_session1.cameraInfoShown == 1 ? "Camera Info On" : "Camera Info Off"), 0x64ffff, 0, 0);
        ui_window_show(wnd, g_session1.cameraInfoShown);
        return;
    }
    if (self->view != 0) {
        *(unsigned *)&self->view->destDistance = packet->distance;
        *(unsigned *)&self->view->destLongitude = packet->heading;
        *(unsigned *)&self->view->destLatitude = packet->pitch;
    }
}
