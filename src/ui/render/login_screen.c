#include "render/texture_cache.h"
#include "render/texture_types.h"
#include "platform/client_config.h"
#include <stdio.h>
#include "ui/wndmgr/wndmgr.h"
#include "ui/render/render.h"
#include "contents/mouse.h"

#include "render/texture_quad.h"
#include "core/text_util.h"

#define LOGIN_Z 0.0999f
#define LOGIN_WHITE 0xffffffffu

enum {
    LOGIN_PIECE_NONE = 0,
    LOGIN_PIECE_LOGO = 1,
    LOGIN_PIECE_RATING = 2,
    LOGIN_PIECE_GRADE = 3,
    LOGIN_PIECE_TAPE = 4,
    LOGIN_PIECE_SERVICE = 5,
    LOGIN_PIECE_UNPICKED = 6
};

#define GRADE_TGA UI_SKIN "T_GameGrade.tga"
#define TILE_FMT UI_SKIN "T_배경%d-%d.bmp"
#define TAPE_FMT UI_SKIN "T_테입%d.tga"
#define RATING_TGA UI_SKIN "T_중력성인.tga"
#define LOGO_FMT UI_SKIN "T_R%d.tga"

static void draw(RECT *rc, struct Texture *tex)
{
    screen_texture_rect(g_renderer, rc, 0, LOGIN_Z, LOGIN_WHITE,
                        (struct Texture *)tex, g_fullUV);
}

static int between(float v, float lo, float hi)
{
    return v > lo && hi > v;
}

static void drag_pieces(struct UIWindowMgr *self, int tw, int th)
{
    int mx, my;
    float ftw = (float)tw, fth = (float)th, fx, fy;

    if (g_mouse.button[MOUSE_LEFT] == MOUSE_BUTTON_PRESSED) {
        if (self->capture)
            return;
        self->login_drag_piece = LOGIN_PIECE_NONE;
        mx = g_mouse.x;
        my = g_mouse.y;
        if (mx > self->grade_x && mx < self->grade_x + self->grade_width
            && my > self->grade_y && my < self->grade_y + self->grade_height)
            self->login_drag_piece = LOGIN_PIECE_GRADE;
        fx = (float)mx;
        fy = (float)my;
        if (between(fx, ftw * 3.3f + self->rating_x, ftw * 3.9f + self->rating_x)
            && between(fy, fth * 2.4f + self->rating_y, fth * 2.8f + self->rating_y))
            self->login_drag_piece = LOGIN_PIECE_RATING;
        if (between(fx, ftw * 0.1f + self->logo_x, ftw * 1.8f + self->logo_x)
            && between(fy, fth * 0.3f + self->logo_y, fth * 1.2f + self->logo_y))
            self->login_drag_piece = LOGIN_PIECE_LOGO;
        if (g_serviceType != 0 && g_startupSecondMod4 > 0
            && between(fx, ftw * 2.2f + self->service_piece_x, ftw * 3.5f + self->service_piece_x)
            && my > self->service_piece_y && my < self->service_piece_y + th * 3)
            self->login_drag_piece = LOGIN_PIECE_SERVICE;
        if (between(fy, fth * 2.7f + self->tape_y, fth * 3.0f + self->tape_y))
            self->login_drag_piece = LOGIN_PIECE_TAPE;
    }

    if (g_mouse.button[MOUSE_LEFT] != MOUSE_BUTTON_HELD || self->capture)
        return;
    switch (self->login_drag_piece) {
    case LOGIN_PIECE_LOGO:
        self->logo_x += (short)g_mouse.dx;
        self->logo_y += (short)g_mouse.dy;
        break;
    case LOGIN_PIECE_RATING:
        self->rating_x += (short)g_mouse.dx;
        self->rating_y += (short)g_mouse.dy;
        break;
    case LOGIN_PIECE_GRADE:
        self->grade_x += g_mouse.dx;
        self->grade_y += g_mouse.dy;
        break;
    case LOGIN_PIECE_UNPICKED:
        self->unpicked_piece_x += (short)g_mouse.dx;
        self->unpicked_piece_y += (short)g_mouse.dy;
        break;
    case LOGIN_PIECE_TAPE:
        if (self->tape_y + g_mouse.dy >= 0)
            self->tape_y += (short)g_mouse.dy;
        break;
    case LOGIN_PIECE_SERVICE:
        if (g_serviceType != 0 || self->service_piece_x + g_mouse.dx >= 0)
            self->service_piece_x += (short)g_mouse.dx;
        break;
    }
}

static void draw_tiles(int tw, int th)
{
    struct String name;
    RECT rc;
    int x, y;
    signed char row, col = 0;

    for (x = 0; x < tw * 4; x += tw) {
        col++;
        row = 0;
        for (y = 0; y < th * 3;) {
            struct Texture *tex;

            row++;
            rc.top = y;
            y += th;
            rc.left = x;
            rc.right = x + tw;
            rc.bottom = y;
            string_init(&name);
            string_format(&name, TILE_FMT, row, col);
            tex = texture_cache_get(&g_textureCache, string_cstr(&name), 0, 0);
            string_free(&name);
            if (tex)
                draw(&rc, tex);
        }
    }
}

static void draw_tape(struct UIWindowMgr *self, int tw, float fth)
{
    char name[256];
    RECT rc;
    int x;
    signed char n = 0;

    for (x = 0; x < tw * 4;) {
        n++;
        rc.left = x;
        x += tw;
        rc.right = x;
        rc.top = (int)(fth * 2.5f + (float)self->tape_y);
        self->tape_top = (short)rc.top;
        rc.bottom = (int)(fth * 3.0f + (float)self->tape_y);
        sprintf(name, TAPE_FMT, n);
        draw(&rc, texture_cache_get(&g_textureCache, name, 0, 0));
    }
}

static void draw_logo(struct UIWindowMgr *self, int tw, int th, float fth)
{
    char name[256];
    RECT rc;
    float top_offset = fth * 0.1f, half_row = fth * 0.5f, depth = fth + fth;
    int x, y, left, right;
    signed char column = -2, n;

    for (x = self->logo_x; x < self->logo_x + tw * 2; x += tw) {
        column++;
        y = (int)(top_offset + (float)self->logo_y);
        if (!((float)self->logo_y + depth > (float)y))
            continue;
        left = (int)((float)tw * 0.05f + (float)x);
        right = left + tw;
        n = column;
        do {
            n += 2;
            rc.top = y;
            y += th;
            rc.left = left;
            rc.right = right;
            rc.bottom = n <= 2 ? y : (int)(half_row + (float)rc.top);
            sprintf(name, LOGO_FMT, n);
            draw(&rc, texture_cache_get(&g_textureCache, name, 0, 0));
        } while ((float)self->logo_y + depth > (float)y);
    }
}

void ui_window_mgr_draw_login_screen(struct UIWindowMgr *self)
{
    RECT rc = {0, 0, 0xff, 0xff};
    int tw = (short)(g_renderer->width / 4);
    int th = (short)(g_renderer->height / 3);
    float fth;
    struct Texture *grade = texture_cache_get(&g_textureCache, GRADE_TGA, 0, 0);

    if (grade) {
        self->grade_width = ((unsigned)grade->reqWidth);
        self->grade_height = ((unsigned)grade->reqHeight);
    }
    if (self->grade_x == 0)
        self->grade_x = g_renderer->width - self->grade_width - 10;
    if (self->grade_y == 0)
        self->grade_y = 10;

    drag_pieces(self, tw, th);

    fth = (float)th;
    draw_tiles(tw, th);
    draw_tape(self, tw, fth);

    rc.left = self->grade_x;
    rc.top = self->grade_y;
    rc.right = self->grade_x + self->grade_width;
    rc.bottom = self->grade_y + self->grade_height;
    draw(&rc, texture_cache_get(&g_textureCache, GRADE_TGA, 0, 0));

    rc.left = (int)((float)self->rating_x + (float)tw * 3.1f);
    rc.top = (int)((float)self->rating_y + fth * 2.4f);
    rc.right = rc.left + tw;
    rc.bottom = (int)((float)rc.top + fth * 0.5f);
    draw(&rc, texture_cache_get(&g_textureCache, RATING_TGA, 0, 0));

    draw_logo(self, tw, th, fth);
}
