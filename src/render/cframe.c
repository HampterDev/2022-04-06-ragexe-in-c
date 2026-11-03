#include "render/texture_cache.h"
#include "resources/bitmap_types.h"
#include "session/state.h"
#include "game/context.h"
#define _CRT_SECURE_NO_WARNINGS
#include "render/render_api.h"
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform/language.h"
#include "session/session_names.h"

#pragma comment(lib, "winmm.lib")

int   g_wobbleFramesLeft;
BYTE  g_wobbleMode;
short g_wobbleAmplitude;

void renderer_draw_frame_extras(struct Renderer *self)
{
    if (g_session1.showTimeFont != 0) {
        int unit = self->height / 700;
        void *act = res_mgr_get(g_resMgr(), "이팩트\\TimeFont.act");
        void *spr = res_mgr_get(g_resMgr(), "이팩트\\TimeFont.spr");
        screen_sprite_frame(self, 100, 100, act, spr, 0, 0, (float)unit * 0.5f, 0, -1, 0x201);
        screen_sprite_frame(self, 100, 200, act, spr, 1, 0, (float)unit, 0, -1, 0x201);
    }
    if (session_get_battlefield_state(&g_session1) == 1)
        renderer_draw_battle_field_icon(self);
    if (g_session1.effectOn != 0 && g_wobbleFramesLeft > 0 && g_wobbleMode != 2 && renderer_lock_target(self)) {
        LONG pitch = self->targetPitch;
        BYTE *buf = (BYTE *)malloc(pitch);
        short amp = g_wobbleAmplitude;
        int y;
        if (amp < 2)
            g_wobbleAmplitude = amp = 2;
        for (y = self->height - 1; y >= 0; y--) {
            BYTE *row = (BYTE *)self->targetBits + y * pitch;
            int period = amp <= 10 ? amp : 20 - amp;
            if (y == self->height - 1)
                memcpy(buf, row, pitch);
            if (y % (BYTE)period == 0) {
                memcpy(buf, row, pitch);
            } else {
                int shift = (rand() % 8) << 4;
                if (g_wobbleMode == 0)
                    memcpy(row, buf, pitch);
                else if (g_wobbleMode == 1)
                    memcpy(row + shift, buf, pitch - shift);
            }
            amp = g_wobbleAmplitude;
        }
        amp = (short)(amp + 2);
        g_wobbleAmplitude = amp;
        if (amp < 2) {
            g_wobbleAmplitude = 2;
        } else if (amp > 16) {
            g_wobbleFramesLeft = g_wobbleFramesLeft - 1 < 0 ? 0 : g_wobbleFramesLeft - 1;
            g_wobbleAmplitude = 2;
        }
        free(buf);
        gfx_unlock(g_3dDevice.target);
    }
}

SIZE *renderer_get_text_extent(struct Renderer *self, SIZE *out, const char *str, int len, int a, int b, int c)
{
    struct TextDC t;
    SIZE size;
    (void)self;
    if (!str || (len == 0 && (len = (int)strlen(str)) == 0)) {
        out->cx = 0;
        out->cy = 0;
        return out;
    }
    text_dc_from_surface(&t, g_3dDevice.target);
    text_dc_select_font(&t, a, b, c, 0);
    text_dc_text_extent(&t, str, len, &size);
    *out = size;
    text_dc_destruct(&t);
    return out;
}

int renderer_text_out(struct Renderer *self, int x, int y, const char *str, COLORREF colour)
{
    struct TextDC t;
    (void)self;
    text_dc_from_surface(&t, g_3dDevice.target);
    text_dc_select_font(&t, 0, 12, 0, 0);
    SetTextColor(t.hdc, colour);
    TextOutA(t.hdc, x, y, str, (int)strlen(str));
    text_dc_destruct(&t);
    return 0;
}
