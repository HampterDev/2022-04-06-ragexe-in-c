#include "render/texture_cache.h"
#include "session/state.h"
#include "render/render_api.h"
#include "platform/clock.h"

#define BADGE_DEPTH 0.0999f

void renderer_draw_game_grade_badge(struct Renderer *self)
{
    struct PlatformDateTime now;
    struct Texture *tex;
    RECT rc;
    int ramp;
    signed char state;

    platform_date_time_local(&now);
    state = (signed char)g_gradeBadgeState;
    if (now.minute == 0) {
        if (now.second == 0) {
            if (state == 0) {
                g_gradeBadgeState = 1;
                g_gradeBadgeAlpha = 0;
                goto draw;
            }
        } else if (now.second >= 10) {
            if (state == 1) {
                g_gradeBadgeState = 2;
                goto draw;
            }
        }
    }
    if (state < 1)
        return;
draw:
    tex = texture_cache_get(&g_textureCache, "유저인터페이스\\T_GameGrade.tga", 0, 0);
    if (!tex)
        return;
    rc.left = self->width - tex->reqWidth - 0x8c;
    rc.top = 0x2d;
    rc.right = self->width - 0x8c;
    rc.bottom = tex->reqHeight + 0x2d;
    ramp = g_gradeBadgeAlpha;
    if (g_gradeBadgeState == 1) {
        if (ramp < 0xdc)
            g_gradeBadgeAlpha = ++ramp;
    } else if (ramp > 0) {
        g_gradeBadgeAlpha = --ramp;
    } else {
        g_gradeBadgeState = 0;
    }
    screen_texture_rect(self, &rc, 0, BADGE_DEPTH,
                 ((DWORD)ramp << 24) + 0xffffff, tex, g_fullUV);
}

static void draw_points(struct Renderer *self, int points, int y, BYTE wide, void *act, void *spr)
{
    int value = points < 0 ? 0 : points;
    int count = (value > 9) + 1;
    int digits[2];
    int x, i;

    digits[0] = value % 10;
    digits[1] = value / 10 % 10;
    x = wide > 1 ? self->width - 0xaf : self->width - 0xcf;
    for (i = 0; i < count; i++) {
        screen_sprite_frame(self, x, y, act, spr, digits[i], 0, 1.0f, 0, -1,
                         0x201);
        x -= 0x20;
    }
}

void renderer_draw_battle_field_icon(struct Renderer *self)
{
    RECT rc;
    struct Texture *tex;
    void *act, *spr;
    BYTE wide;

    if (g_session1.battlegroundPoints[0] < 0 || g_session1.battlegroundPoints[1] < 0)
        return;
    rc.left = self->width - 0x118;
    rc.top = 0x1a;
    rc.right = self->width - 0xe8;
    rc.bottom = 0x4a;
    tex = texture_cache_get(&g_textureCache, "effect\\사자(A).bmp", 0, 0);
    screen_texture_rect(self, &rc, 0, BADGE_DEPTH, 0xcdffffff, tex, g_fullUV);
    rc.left = self->width - 0x118;
    rc.top = 0x54;
    rc.right = self->width - 0xe8;
    rc.bottom = 0x84;
    tex = texture_cache_get(&g_textureCache, "effect\\독수리(B).bmp", 0, 0);
    screen_texture_rect(self, &rc, 0, BADGE_DEPTH, 0xcdffffff, tex, g_fullUV);

    wide = (g_session1.battlegroundPoints[0] > 9 || g_session1.battlegroundPoints[1] > 9) ? 2 : 1;
    act = res_mgr_get(g_resMgr(), "이팩트\\RankFont.act");
    spr = res_mgr_get(g_resMgr(), "이팩트\\RankFont.spr");
    draw_points(self, g_session1.battlegroundPoints[0], 0x32, wide, act, spr);
    draw_points(self, g_session1.battlegroundPoints[1], 0x6c, wide, act, spr);
}
