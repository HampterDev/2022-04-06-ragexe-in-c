#include "render/rp_pool.h"
#include "render/sprite_atlas.h"
#include "render/texture_cache.h"
#include "render/texture_types.h"
#include "resources/sprite.h"
#include "core/object_type.h"
#include <math.h>
#include <string.h>
#include "ui/worldmap/romap.h"
#include "std/std.h"
#include "std/tree.h"
#include "ui/character/character.h"
#include "render/screen_draw.h"

#define KAFRA_SPRITE "npc\\4_f_kafra6.spr"
#define KAFRA_ACT "npc\\4_f_kafra6.act"
#define KAFRA_CYCLE_MS 4000
#define SHIP_SPEED 200.0f
#define ROUTE_TWO_PI 6.283184f
#define ROUTE_HALF_PI 1.570796f
#define ROUTE_Z 0.8f
#define ROUTE_LABEL_ARGB 0x66000000
#define ROUTE_LABEL_TEXT 0xd2ff
#define WORLDMAP_DIR UI_SKIN "worldview_interface\\"

static void route_quad(float ax, float ay, float bx, float by, float cx, float cy, float dx, float dy,
                       struct Texture *tex, DWORD argb)
{
    struct TexDrawPrim *prim = (struct TexDrawPrim *)renderer_new_prim(g_renderer);
    struct TexDrawVertex *v = prim->base.verts;
    int i;

    v[0].x = ax;
    v[0].y = ay;
    v[1].x = bx;
    v[1].y = by;
    v[2].x = cx;
    v[2].y = cy;
    v[3].x = dx;
    v[3].y = dy;
    for (i = 0; i < 4; i++) {
        v[i].z = ROUTE_Z;
        v[i].rhw = 1.0f;
        v[i].diffuse = argb;
        v[i].u = (float)(i & 1);
        v[i].v = (float)(i >> 1);
    }
    prim->base.texture = tex;
    prim->base.srcBlend = 2;
    prim->base.destBlend = 4;
    renderer_add_prim(g_renderer, &prim->base, ROUTE_PRIM_LIST);
}

static void route_axes(float x0, float y0, float x1, float y1, float *ux, float *uy, float *nx, float *ny)
{
    float dy = y1 - y0, dx = x1 - x0;
    float inv = 1.0f / float_sqrt(dy * dy + dx * dx);
    float angle, c, s, n;

    *uy = inv * dy;
    *ux = inv * dx;
    angle = (float)acos((double)*ux);
    if (0.0f > *uy)
        angle = ROUTE_TWO_PI - angle;
    angle += ROUTE_HALF_PI;
    c = float_cos(angle);
    s = float_sin(angle);
    n = 1.0f / float_sqrt(c * c + s * s);
    *nx = c * n;
    *ny = s * n;
}

static void route_leave_box(float *x, float *y, float dx, float dy, float left, float top, float right,
                            float bottom)
{
    while (right > *x && *y > top && bottom > *y) {
        *x += dx;
        *y += dy;
        if (!(*x > left))
            break;
    }
}

static void route_arrow(float x0, float y0, float x1, float y1, float a_left, float a_top, float a_right,
                        float a_bottom, float b_left, float b_top, float b_right, float b_bottom, int width,
                        struct Texture *body, int head_size, struct Texture *head, float progress,
                        DWORD argb)
{
    float ux, uy, nx, ny, ox, oy;
    float sx, sy, ex = x1, ey = y1;
    float psx, psy, pex, pey;
    float tx, ty, tex_, tey;

    route_axes(x0, y0, x1, y1, &ux, &uy, &nx, &ny);
    ox = (float)width * nx;
    oy = (float)width * ny;
    psx = ox + x0;
    psy = oy + y0;
    pex = ox + x1;
    pey = oy + y1;
    sx = x0;
    sy = y0;
    if (x0 > a_left)
        route_leave_box(&sx, &sy, ux, uy, a_left, a_top, a_right, a_bottom);
    if (x1 > b_left)
        route_leave_box(&ex, &ey, -ux, -uy, b_left, b_top, b_right, b_bottom);
    if (psx > a_left)
        route_leave_box(&psx, &psy, ux, uy, a_left, a_top, a_right, a_bottom);
    if (pex > b_left)
        route_leave_box(&pex, &pey, -ux, -uy, b_left, b_top, b_right, b_bottom);

    tx = (ex - sx) * progress + sx;
    ty = (ey - sy) * progress + sy;
    if (progress == 1.0f) {
        tex_ = (pex - psx) + psx;
        tey = (pey - psy) + psy;
    } else {
        tex_ = tx + ox;
        tey = ty + oy;
    }
    route_quad(sx, sy, psx, psy, tx, ty, tex_, tey, body, argb);

    if (1.0f > progress) {
        float w = (float)width, hs = (float)head_size;
        float hx = ox * 0.5f + tx - w * ux;
        float hy = oy * 0.5f + ty - w * uy;
        float hnx = hs * nx * 0.5f, hny = hs * ny * 0.5f;

        route_quad(hx, hy, hx - hnx, hy - hny, hnx + hx, hny + hy, hs * ux + hx, hs * uy + hy, head, argb);
    }
}

static void route_dashes(float x0, float y0, float x1, float y1, int dash, int band, struct Texture *dash_tex,
                         int ship_size, struct Texture *ship_tex, float t, DWORD ship_argb, int offset)
{
    float travelled = float_sqrt((y1 - y0) * (y1 - y0) + (x1 - x0) * (x1 - x0)) * t;
    float ux, uy, nx, ny, wx, wy, sx, sy, dx, dy, ex, ey, px, py;
    DWORD argb = 0x33ffffff;

    route_axes(x0, y0, x1, y1, &ux, &uy, &nx, &ny);
    wx = (float)band * nx;
    wy = (float)band * ny;
    sx = x0 - (float)offset * nx;
    sy = y0 - (float)offset * ny;
    dx = (float)dash * ux;
    dy = (float)dash * uy;
    ex = dx + sx;
    ey = dy + sy;
    px = ux * travelled + x0;
    py = uy * travelled + y0;
    if (travelled > float_sqrt((ey - y0) * (ey - y0) + (ex - x0) * (ex - x0))) {
        float half_dx = dx * 0.5f, half_dy = dy * 0.5f;

        do {
            if (100.0f > float_sqrt((py - sy) * (py - sy) + (px - sx) * (px - sx))) {
                route_quad(sx, sy, sx + wx, sy + wy, ex, ey, ex + wx, ey + wy, dash_tex, argb);
                argb += 0x33000000;
            }
            sx = half_dx + ex;
            sy = half_dy + ey;
            ey = sy + dy;
            ex = sx + dx;
        } while (travelled > float_sqrt((ey - y0) * (ey - y0) + (ex - x0) * (ex - x0)));
    }
    if (1.0f > t) {
        float bx = wx * 0.5f + ex - (float)band * ux;
        float by = wy * 0.5f + ey - (float)band * uy;
        float s = (float)ship_size;
        float snx = s * nx, sny = s * ny, sux = s * ux + s * ux, suy = s * uy + s * uy;

        route_quad(bx + snx + sux, by + sny + suy, sux + (bx - snx), suy + (by - sny), bx + snx, by + sny,
                   bx - snx, by - sny, ship_tex, ship_argb);
    }
}

static void route_stop_label(struct UIRoMapWnd *wnd, const struct RouteStop *stop, BYTE skip_spot)
{
    struct UIWindow *w = &wnd->frame.base;
    struct String name;
    const char *shown;

    if (skip_spot == 1 && stop->rect.left == wnd->spot->rect.left && stop->rect.right == wnd->spot->rect.right
        && stop->rect.top == wnd->spot->rect.top && stop->rect.bottom == wnd->spot->rect.bottom)
        return;
    ui_window_draw_round_frame(w, stop->rect.left, stop->rect.top, (int)stop->width, (int)stop->height,
                            0xffffff);
    draw_box_screen(stop->rect.left, stop->rect.top, (int)stop->width, (int)stop->height, ROUTE_LABEL_ARGB);
    shown = session_map_display_name(string_cstr(&stop->name));
    string_init(&name);
    string_assign_cstr(&name, shown);
    ui_window_text_out_outlined(w, stop->rect.left + 2, stop->rect.top + 5, string_cstr(&name), 0,
                             ROUTE_LABEL_TEXT, 0, 0, 0xc, 1, 0);
    string_free(&name);
}

static void route_stop_set(const struct String *name, const struct WorldViewRect *rect, struct RouteStop *out)
{
    int width = rect->right - rect->left, height = rect->bottom - rect->top;
    float cx = (float)width * 0.5f + (float)rect->left;
    float cy = (float)height * 0.5f + (float)rect->top;

    string_assign(&out->name, string_cstr(name), name->size);
    out->rect = *rect;
    out->cx = cx;
    out->cy = cy;
    out->width = (float)width;
    out->height = (float)height;
}

static void route_stop_copy(struct RouteStop *dst, const struct RouteStop *src)
{
    string_copy(&dst->name, &src->name);
    dst->rect = src->rect;
    dst->cx = src->cx;
    dst->cy = src->cy;
    dst->width = src->width;
    dst->height = src->height;
}

static void route_append_stop(struct Vec *stops, const char *name)
{
    const struct WorldViewWorld *world = world_view_world_at(&g_worldView, 0);
    const struct WorldViewMap *spot = world_view_find_map(world, name);
    struct RouteStop stop;
    if (!spot) return;
    string_init(&stop.name);
    string_assign_cstr(&stop.name, spot->name);
    route_stop_set(&stop.name, &spot->rect, &stop);
    vec_push(stops, &stop, sizeof stop);
}

static void route_load_stops(struct Vec *stops, const struct Vec *names)
{
    const struct String *name;
    route_stops_clear(stops);
    for (name = names->first; name != (const struct String *)names->last; ++name)
        route_append_stop(stops, string_cstr(name));
}

struct RouteLabel {
    struct String name;
    struct RouteStop stop;
};

static int name_less(const void *a, const void *b)
{
    return string_compare(a, b) < 0;
}

static void destroy_label(void *value)
{
    struct RouteLabel *label = value;

    string_free(&label->stop.name);
    string_free(&label->name);
}

static void route_labels_add(struct Tree *labels, const struct RouteStop *stop)
{
    struct RouteLabel label;
    int added;

    string_copy(&label.name, &stop->name);
    route_stop_copy(&label.stop, stop);
    tree_add(labels, &label, sizeof label, name_less, &added);
    if (!added)
        destroy_label(&label);
}

static void route_labels_draw_and_free(struct Tree *labels, struct UIRoMapWnd *wnd)
{
    struct TreeNode *node;

    for (node = tree_first(labels); node != labels->head; node = tree_next(node))
        route_stop_label(wnd, &TREE_VALUE(node, struct RouteLabel)->stop, 1);
    tree_free(labels, destroy_label);
}

static void route_draw_kafra(struct UIRoMapWnd *wnd, float x, float y)
{
    float scale = (float)g_worldView.resolution * 0.2f + 0.5f;
    struct SprRes *spr = object_as_type(res_mgr_get(g_resMgr(), KAFRA_SPRITE), OBJECT_SPRITE_RESOURCE);
    void *act = object_as_type(res_mgr_get(g_resMgr(), KAFRA_ACT), OBJECT_ACTION_RESOURCE);
    struct SpriteAction *motions;
    struct SprClip *clip;
    struct SprImg *img;
    struct SpriteRegion place = {0};
    struct TexDrawPrim *prim;
    struct TexDrawVertex *v;
    struct Texture *tex;
    int count, ms, frame, lift, top, height, half_width;

    if (!spr || !act)
        return;
    motions = act_res_get_action(act, 0);
    count = (int)motions->motion_count;
    if (count == 0)
        return;
    ms = (int)(timeGetTime() % KAFRA_CYCLE_MS);
    frame = ms == 0 ? 0 : (count <= KAFRA_CYCLE_MS ? ms / (KAFRA_CYCLE_MS / count) : (int)((unsigned long long)ms * (unsigned)count / KAFRA_CYCLE_MS));
    frame = count > frame ? frame : count - 1;
    if (frame < 0)
        frame = 0;

    clip = motion_get_clip(motions->motions, 0);
    if (!clip || clip->sprIndex == -1)
        return;
    img = spr_res_image(spr, clip);
    if (!img)
        return;
    top = (int)(y - ((float)img->height * scale + 23.0f));
    lift = top <= 0 ? top : 0;
    if (frame > 0) {
        clip = motion_get_clip(&motions->motions[frame], 0);
        if (!clip || clip->sprIndex == -1)
            return;
        img = spr_res_image(spr, clip);
        if (!img)
            return;
    }

    memset(&place, 0, sizeof place);
    if (clip->clipType == 0) {
        tex = sprite_atlas_resolve(g_renderer, img, spr->palette, &place);
    } else {
        tex = img->texture;
    }
    if (!tex) return;
    height = (int)((float)img->height * scale);
    half_width = (int)((float)img->width * scale * 0.5f);

    prim = (struct TexDrawPrim *)renderer_new_prim(g_renderer);
    v = prim->base.verts;
    v[0].x = x - (float)half_width;
    v[0].y = y - (float)height - (float)lift;
    v[1].x = (float)half_width + x;
    v[1].y = v[0].y;
    v[2].x = v[0].x;
    v[2].y = y - (float)lift;
    v[3].x = v[1].x;
    v[3].y = v[2].y;
    v[0].z = v[1].z = v[2].z = v[3].z = ROUTE_Z;
    v[0].rhw = v[1].rhw = v[2].rhw = v[3].rhw = 1.0f;
    v[0].diffuse = v[1].diffuse = v[2].diffuse = v[3].diffuse = 0xffffffff;
    v[0].u = place.u;
    v[0].v = place.v;
    v[1].u = place.u1;
    v[1].v = place.v;
    v[2].u = place.u;
    v[2].v = place.v1;
    v[3].u = place.u1;
    v[3].v = place.v1;
    prim->base.texture = tex;
    prim->base.srcBlend = 2;
    prim->base.destBlend = 4;
    renderer_add_prim(g_renderer, &prim->base, ROUTE_PRIM_LIST);
    wnd->kafra_rect.top = (int)v[0].y;
    wnd->kafra_rect.bottom = (int)v[3].y;
    wnd->kafra_rect.left = (int)v[0].x;
    wnd->kafra_rect.right = (int)v[3].x;
}

static void kafra_load_stops(struct KafraMoveMapServiceInfo *self, const struct String *map)
{
    const char *name = string_cstr(map);
    size_t i, count = kafra_destination_count(name);
    route_stops_clear(&self->stops);
    for (i = 0; i < count; ++i)
        route_append_stop(&self->stops, kafra_destination(name, i));
}

void kafra_move_map_service_info_update(struct KafraMoveMapServiceInfo *self, struct UIRoMapWnd *wnd, float seconds)
{
    struct RoMapEntry *spot = wnd->spot;
    float progress;

    if (!spot || !kafra_destination_count(string_cstr(&spot->name)))
        goto none;
    if (!string_equal(&self->map, &spot->name)) {
        string_assign(&self->map, string_cstr(&spot->name), spot->name.size);
        kafra_load_stops(self, &spot->name);
        route_stop_set(&spot->name, &spot->rect, &self->origin);
        self->progress = 0.0f;
        progress = 0.0f;
    } else {
        progress = self->progress;
    }
    progress += seconds;
    self->progress = progress;
    if (progress > 1.0f)
        self->progress = 1.0f;
    return;
none:
    string_clear(&self->map);
    route_stops_clear(&self->stops);
}

void kafra_move_map_service_info_draw(struct KafraMoveMapServiceInfo *self, struct UIRoMapWnd *wnd)
{
    struct Tree labels;
    int count, i;

    if (!wnd)
        return;
    wnd->kafra_rect.top = 0;
    wnd->kafra_rect.bottom = 0;
    wnd->kafra_rect.left = 0;
    wnd->kafra_rect.right = 0;
    if (self->stops.first == self->stops.last)
        return;
    tree_init(&labels);
    count = (int)VEC_COUNT(&self->stops, struct RouteStop);
    for (i = 0; i < count; i++) {
        struct RouteStop from, to;

        struct Texture *head, *body;

        route_stop_copy(&from, &self->origin);
        route_stop_copy(&to, &((struct RouteStop *)self->stops.first)[i]);
        route_labels_add(&labels, &to);
        if (string_equal_cstr(&to.name, "izlude.rsw")) {
            from.cx = (float)from.rect.right;
            to.cx = to.width * 0.8f + (float)to.rect.left;
            to.cy = (float)to.rect.top;
        }
        head = texture_cache_get(&g_textureCache, "mapArrow_head.bmp", 0, 1);
        body = texture_cache_get(&g_textureCache, "mapArrow_body.bmp", 1, 1);
        route_arrow(from.cx, from.cy, to.cx, to.cy, (float)from.rect.left, (float)from.rect.top,
                    (float)from.rect.right, (float)from.rect.bottom, (float)to.rect.left, (float)to.rect.top,
                    (float)to.rect.right, (float)to.rect.bottom, 10, body, 0x14, head, self->progress, 0xaaffffff);
        string_free(&to.name);
        string_free(&from.name);
    }
    route_draw_kafra(wnd, (float)self->origin.rect.right, (float)self->origin.rect.top);
    route_labels_draw_and_free(&labels, wnd);
}

#define EINBROCH "einbroch.rsw"
#define LIGHTHALZEN "lighthalzen.rsw"
#define HUGEL "hugel.rsw"
#define YUNO "yuno.rsw"
#define IZLUDE "izlude.rsw"
#define RA_FILD12 "ra_fild12.rsw"
static const char *const einbroch_routes[4][7] = {
    { EINBROCH, LIGHTHALZEN, EINBROCH, YUNO, HUGEL, YUNO, EINBROCH },
    { LIGHTHALZEN, EINBROCH, YUNO, HUGEL, YUNO, EINBROCH, LIGHTHALZEN },
    { HUGEL, YUNO, EINBROCH, LIGHTHALZEN, EINBROCH, YUNO, HUGEL },
    { YUNO, EINBROCH, LIGHTHALZEN, EINBROCH, YUNO, HUGEL, YUNO },
};
static const char *const izlude_routes[3][4] = {
    { IZLUDE, YUNO, RA_FILD12, IZLUDE },
    { RA_FILD12, IZLUDE, YUNO, RA_FILD12 },
    { YUNO, RA_FILD12, IZLUDE, YUNO },
};

static const char *const *find_route(const struct String *name, const char *const *routes, int rows, int count)
{
    int i;

    for (i = 0; i < rows; i++)
        if (string_equal_cstr(name, routes[i * count]))
            return routes + i * count;
    return NULL;
}

void flying_ship_route_info_stop(struct FlyingShipRouteInfo *self)
{
    if (self->active) {
        self->at_start = 1;
        self->leg = 0;
        self->travelled = 0.0f;
        self->active = 0;
        self->loaded = 0;
    }
}

void flying_ship_route_info_advance(struct FlyingShipRouteInfo *self, struct UIRoMapWnd *wnd, float seconds)
{
    int last_leg = (int)VEC_COUNT(&self->stops, struct RouteStop) - 1;
    int leg;
    struct RouteStop *stops;
    float length, travelled;

    if (last_leg <= 0)
        return;
    self->active = 1;
    leg = self->leg;
    if (self->travelled > self->leg_length) {
        if (leg >= last_leg - 1) {
            ui_window_invalidate_base(&wnd->frame.base);
            leg = -1;
        }
        leg++;
        self->travelled = 0.0f;
        self->leg = leg;
    }
    stops = self->stops.first;
    length = float_sqrt((stops[leg].cx - stops[leg + 1].cx) * (stops[leg].cx - stops[leg + 1].cx)
                        + (stops[leg].cy - stops[leg + 1].cy) * (stops[leg].cy - stops[leg + 1].cy));
    travelled = self->travelled;
    if (travelled == 0.0f)
        self->leg_length = length;
    travelled = seconds * SHIP_SPEED + travelled;
    self->travelled = travelled;
    self->leg_t = travelled / self->leg_length;
}

static void ship_load_stops(struct FlyingShipRouteInfo *self, const struct Vec *names)
{
    route_load_stops(&self->stops, names);
    self->loaded = 1;
}

static BYTE ship_prepare(struct FlyingShipRouteInfo *self, const struct RoMapEntry *spot)
{
    const char *const *route;
    struct Vec names;
    int i, count;

    if (!spot)
        goto off;
    switch (self->route) {
    case FLYING_ROUTE_IZLUDE:
        if (!string_equal_cstr(&spot->name, IZLUDE) && !string_equal_cstr(&spot->name, RA_FILD12) && !string_equal_cstr(&spot->name, YUNO))
            goto off;
        break;
    case FLYING_ROUTE_EINBROCH:
        if (!string_equal_cstr(&spot->name, EINBROCH) && !string_equal_cstr(&spot->name, LIGHTHALZEN) && !string_equal_cstr(&spot->name, HUGEL)
            && !string_equal_cstr(&spot->name, YUNO))
            goto off;
        break;
    default:
        goto off;
    }
    if (self->loaded == 1)
        return 1;
    if (self->route == FLYING_ROUTE_IZLUDE) {
        route = find_route(&spot->name, &izlude_routes[0][0], 3, 4);
        count = 4;
    } else {
        route = find_route(&spot->name, &einbroch_routes[0][0], 4, 7);
        count = 7;
    }
    if (!route)
        return 0;
    memset(&names, 0, sizeof names);
    for (i = 0; i < count; i++) {
        struct String name;

        string_init(&name);
        string_assign_cstr(&name, route[i]);
        string_vec_push_move(&names, &name);
        string_free(&name);
    }
    ship_load_stops(self, &names);
    string_vec_free(&names);
    return 1;
off:
    flying_ship_route_info_stop(self);
    return 0;
}

static void ship_draw(struct FlyingShipRouteInfo *self, struct UIRoMapWnd *wnd)
{
    struct RouteStop *stops;
    struct Tree labels;
    struct Texture *ship, *dash;
    unsigned i;

    if (self->bitmap.size == 0)
        return;
    tree_init(&labels);
    for (i = 0; i < VEC_COUNT(&self->stops, struct RouteStop) - 1; i++) {
        stops = self->stops.first;
        route_labels_add(&labels, &stops[i + 1]);
        screen_thick_line(stops[i].cx, stops[i].cy, stops[i + 1].cx, stops[i + 1].cy, 2,
                          texture_cache_get(&g_textureCache, string_cstr(&self->bitmap), 0, 1),
                          self->colour);
    }
    stops = self->stops.first;
    ship = texture_cache_get(&g_textureCache, WORLDMAP_DIR "wv_airplen32.bmp", 0, 1);
    dash = texture_cache_get(&g_textureCache, WORLDMAP_DIR "wv_Airf.bmp", 0, 1);
    route_dashes(stops[self->leg].cx, stops[self->leg].cy, stops[self->leg + 1].cx, stops[self->leg + 1].cy, 0x10,
                 0x10, dash, 0x14, ship, self->leg_t, 0xffffffff, 6);
    route_labels_draw_and_free(&labels, wnd);
}

void ui_ro_map_wnd_process_ship(struct UIRoMapWnd *self, struct FlyingShipRouteInfo *ship, float seconds)
{
    if (!self || !ship_prepare(ship, self->spot))
        return;
    flying_ship_route_info_advance(ship, self, seconds);
    ship_draw(ship, self);
}
