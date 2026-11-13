#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game/gamemode.h"
#include "game/camera.h"
#include "game/client.h"
#include "game/talk_type.h"
#include "render/math3d.h"
#include "render/renderer.h"
#include "actor/sprite_quad.h"
#include "session/state.h"
#include "ui/window_access.h"
#include "world/world_imports.h"

#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "Camera info check failed at line %d: %s\n", __LINE__, #condition);   \
            exit(2);                                                                               \
        }                                                                                          \
    } while (0)

struct Session g_session1;
struct UIWindowMgr g_windowMgr;
int g_isFixedCamera;
float g_fixedViewDistanceMax = 300.0f;
float g_spriteDepthScale = 1.0f;
float g_spriteRowDepthStep;
float g_depthPerPixelCot;
float g_spriteLayerDepthStep;
float g_spriteScaleDepthBias;
static struct Renderer renderer;
struct Renderer *g_renderer = &renderer;

struct UIWindow *ui_window_find(int id)
{
    (void)id;
    abort();
}

struct UIWindow *ui_window_open(int id)
{
    (void)id;
    abort();
}

void ui_window_show(struct UIWindow *window, int visible)
{
    (void)window;
    (void)visible;
    abort();
}

int ui_window_mgr_send_msg_(struct UIWindowMgr *self, int msg, int val1, int val2, int val3, int val4)
{
    (void)self;
    (void)msg;
    (void)val1;
    (void)val2;
    (void)val3;
    (void)val4;
    abort();
}

#include "camera_info.inc"

static unsigned float_bits(float value)
{
    unsigned bits;

    memcpy(&bits, &value, sizeof bits);
    return bits;
}

static float bits_float(unsigned bits)
{
    float value;

    memcpy(&value, &bits, sizeof value);
    return value;
}

static void send_camera(struct GameMode *mode, unsigned distance, unsigned heading, unsigned pitch)
{
    struct PACKET_ZC_VIEW_CAMERAINFO packet;

    packet.packet_type = 0x8d2;
    packet.action = 0;
    packet.distance = distance;
    packet.heading = heading;
    packet.pitch = pitch;
    game_mode_camera_info(mode, (const unsigned char *)&packet);
}

static void reset_view(struct View *view)
{
    memset(view, 0, sizeof *view);
    view->distance = view->destDistance = 300.0f;
    view->latitude = view->destLatitude = -45.0f;
    view->longitude = view->destLongitude = 45.0f;
}

static int settled(const struct View *view)
{
    return isfinite(view->longitude) && isfinite(view->latitude) && isfinite(view->distance)
        && view->destLongitude >= 0.0f && view->destLongitude <= 360.0f
        && view->longitude >= 0.0f && view->longitude < 360.0f;
}

int main(void)
{
    static const unsigned unusable[] = { 0x7f800000u, 0xff800000u, 0x7fc00000u, 0xffc00000u, 0x7f800001u };
    static const float huge[] = { 8589934592.0f * 2.0f, -8589934592.0f * 2.0f, 8589935616.0f, -8589935616.0f,
                                  3.0e38f, -3.0e38f, 1.0e20f };
    static const float far_apart[][2] = { { -3.0e38f, 3.0e38f }, { 3.0e38f, -3.0e38f },
                                          { -FLT_MAX / 2.0f, FLT_MAX / 2.0f }, { FLT_MAX / 2.0f, -FLT_MAX / 2.0f } };
    struct View view, raw;
    struct GameMode mode;
    unsigned bits, compared = 0;
    size_t i;
    int frame, packet;

    memset(&mode, 0, sizeof mode);
    mode.view = &view;
    renderer.scaleY = 1.0f;

    for (bits = 0; bits < 0xff800000u; bits += 0x10001u) {
        float heading = bits_float(bits);

        if (!isfinite(heading) || fabsf(heading) >= 1048576.0f)
            continue;
        reset_view(&view);
        send_camera(&mode, float_bits(250.0f), bits, float_bits(-40.0f));
        reset_view(&raw);
        raw.destDistance = 250.0f;
        raw.destLongitude = heading;
        raw.destLatitude = -40.0f;
        CHECK(memcmp(&view, &raw, sizeof view) == 0);
        view_process_camera(&view);
        view_process_camera(&raw);
        CHECK(memcmp(&view, &raw, sizeof view) == 0);
        compared++;
    }
    for (i = 0; i < 4; i++) {
        float heading = i < 2 ? (i ? -8589934592.0f : 8589934592.0f) : (i == 2 ? 16777216.0f : 123456789.0f);

        reset_view(&view);
        send_camera(&mode, float_bits(300.0f), float_bits(heading), float_bits(-45.0f));
        CHECK(view.destLongitude == heading);
        view_process_camera(&view);
        CHECK(settled(&view));
    }

    for (i = 0; i < sizeof unusable / sizeof *unusable; i++) {
        reset_view(&view);
        send_camera(&mode, unusable[i], unusable[i], unusable[i]);
        CHECK(view.destDistance == 300.0f && view.destLongitude == 45.0f && view.destLatitude == -45.0f);
        view_process_camera(&view);
        CHECK(settled(&view));
    }
    for (i = 0; i < sizeof huge / sizeof *huge; i++) {
        reset_view(&view);
        send_camera(&mode, float_bits(300.0f), float_bits(huge[i]), float_bits(-45.0f));
        CHECK(view.destLongitude == fmodf(huge[i], 360.0f));
        view_process_camera(&view);
        CHECK(settled(&view));
    }

    reset_view(&view);
    send_camera(&mode, float_bits(FLT_MAX / 2.0f), float_bits(0.0f), float_bits(-FLT_MAX / 2.0f));
    CHECK(view.destDistance == FLT_MAX / 2.0f && view.destLatitude == -FLT_MAX / 2.0f);
    send_camera(&mode, float_bits(nextafterf(FLT_MAX / 2.0f, FLT_MAX)), float_bits(0.0f),
                float_bits(nextafterf(-FLT_MAX / 2.0f, -FLT_MAX)));
    CHECK(view.destDistance == FLT_MAX / 2.0f && view.destLatitude == -FLT_MAX / 2.0f);
    for (i = 0; i < sizeof far_apart / sizeof *far_apart; i++) {
        reset_view(&view);
        for (packet = 0; packet < 2; packet++) {
            send_camera(&mode, float_bits(far_apart[i][packet]), float_bits(0.0f), float_bits(far_apart[i][packet]));
            for (frame = 0; frame < 60; frame++) {
                view_process_camera(&view);
                CHECK(settled(&view));
            }
        }
        send_camera(&mode, float_bits(300.0f), float_bits(0.0f), float_bits(-45.0f));
        for (frame = 0; frame < 2000; frame++)
            view_process_camera(&view);
        CHECK(settled(&view) && fabsf(view.latitude + 45.0f) < 1.0f && fabsf(view.distance - 300.0f) < 1.0f);
    }

    printf("Camera info: %u server headings set the view as the original's subtraction loop does; "
           "infinite, NaN and huge headings, and pitches and distances far apart in two packets, leave a "
           "view that keeps turning and comes back, passed.\n", compared);
    return 0;
}
