#include "core/compiler.h"
#include "game/packet_handlers.h"
#include "game/gamemode.h"
#include "game/quest_marks.h"
#include "contents/town_info.h"
#include "render/rp_pool.h"
#include "render/texture_cache.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include <math.h>
#include <string.h>
#include "platform/win_types.h"
#include "game/minimap.h"
#include "game/cells.h"
#include "actor/game_actor.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "ui/ui_std.h"
#include "platform/clock.h"

enum {
    MSG_MARK_COUNT = 0x1dc,
    MSG_MEMBER_MARK = 0x1db,
    MSG_NAVI_TARGET = 0x1da,
    MSG_PLAYER_CELL = 0x1d9,
};
enum { WID_NAVI = 0xe };
enum { MSG_NAVI_MARK = 6, NAVI_MARK_EMPTY = 0x1bd, NAVI_MARK_ICON = 0x1bf };

int g_lastNaviX, g_lastNaviY;
int g_lastPlayerX, g_lastPlayerY;

struct MemberPing {
    int id;
    int frames_left;
    int x, y;
};

enum { MINIMAP_SIZE = 128, MINIMAP_RIGHT_MARGIN = 144, MINIMAP_TOP = 16 };
#define MARK_Z 0.0999f
#define MAP_Z 0.0998f

RAGE_NOINLINE void game_mode_draw_minimap_triangle(float x, float y, float r, unsigned color)
{
    struct TexDrawPrim *prim = renderer_new_prim(g_renderer);
    struct TexDrawVertex *v = prim->base.verts;
    float z_far, z_near, y_base;

    prim->base.texture = &g_whiteTexture;
    z_far = renderer_project_depth(MARK_Z);
    z_near = renderer_project_depth(MARK_Z);
    v[0].x = x;
    v[0].y = y - (r + r);
    y_base = y + r;
    v[1].x = (float)((double)x - (double)sqrtf(3.0f) * (double)r);
    v[1].y = y_base;
    v[2].x = (float)((double)sqrtf(3.0f) * (double)r + (double)x);
    v[2].y = y_base;
    v[3].x = (float)((double)x - (double)sqrtf(3.0f) * (double)r);
    v[3].y = y_base;
    v[0].z = z_near; v[0].rhw = MARK_Z; v[0].diffuse = color; v[0].u = 0.0f; v[0].v = 0.0f;
    v[1].z = z_near; v[1].rhw = MARK_Z; v[1].diffuse = color; v[1].u = 1.0f; v[1].v = 0.0f;
    v[2].z = z_far;  v[2].rhw = MARK_Z; v[2].diffuse = color; v[2].u = 0.0f; v[2].v = 1.0f;
    v[3].z = z_far;  v[3].rhw = MARK_Z; v[3].diffuse = color; v[3].u = 1.0f; v[3].v = 1.0f;
    renderer_add_prim(g_renderer, &prim->base, 0x201);
}

static void draw_mark(struct GameMode *self, const struct TextureUV *uv, int x, int y, int size,
                      struct String *texture, int kind, unsigned color, char clamp)
{
    const struct C3dAttr *attr = self->world->attr;
    int width = attr->width, height = attr->height;
    double centre_x = (double)(g_renderer->width - MINIMAP_RIGHT_MARGIN) + 64.0;
    float longest = (float)(width >= height ? width : height);
    double box_w = (double)((float)width * 128.0f / longest);
    double box_h = (double)((float)height * 128.0f / longest);
    float fw, fh, uv_w, uv_h, u0, v0;
    double off_x, off_y, px, py, left, cx, cy, half;
    RECT rect, outer, bar;

    if (0.0f > (float)x || 0.0f > (float)y)
        return;

    fw = (float)box_w;
    fh = (float)box_h;
    off_x = (double)((128.0f - fw) * 0.5f);
    uv_w = uv->r - uv->l;
    u0 = uv->l * 128.0f / uv_w;
    off_y = (double)((128.0f - fh) * 0.5f);
    px = ((double)(fw * (float)x / (float)width) + off_x) / (double)uv_w;
    uv_h = uv->b - uv->t;
    v0 = uv->t * 128.0f / uv_h;
    py = (box_h - (double)(fh * (float)y / (float)height) + off_y) / (double)uv_h;
    left = centre_x - 64.0;
    cx = left - (double)u0 + px;
    cy = 16.0 - (double)v0 + py;
    half = (double)size;

    if (clamp) {
        int edge = 0, edge_y = 0;
        double right = centre_x + 64.0;

        if (left > cx - half) {
            edge = 1;
            cx = half + left;
        } else if (half + cx > right) {
            edge = 2;
            cx = right - half;
        }
        if (16.0 > cy - half) {
            cy = half + 16.0;
            edge_y = 3;
        } else if (half + cy > 144.0) {
            cy = 144.0 - half;
            edge_y = 4;
        }
        if (edge_y && !edge)
            edge = edge_y;
        if (kind != MARK_NAVI_TARGET && edge) {
            struct String arrow;
            string_init(&arrow);
            string_format(&arrow, "유저인터페이스\\basic_interface\\quest_arrow%d.bmp",
                              edge);
            string_assign(texture, string_cstr(&arrow), arrow.size);
            string_free(&arrow);
        }
    } else if (kind != MARK_PLAYER_ARROW) {
        if (left > cx - half || half + cx > centre_x + 64.0 || 16.0 > cy - half || half + cy > 144.0)
            return;
    }

    rect.left = (int)(cx - half);
    rect.top = (int)(cy - half);
    rect.right = (int)(half + cx);
    rect.bottom = (int)(half + cy);

    switch (kind) {
    case MARK_TEXTURE:
        break;

    case MARK_TRIANGLE:
        game_mode_draw_minimap_triangle((float)cx, (float)cy, (float)(size + 1), 0xaaffffff);
        game_mode_draw_minimap_triangle((float)cx, (float)cy, (float)size, color);
        return;

    case MARK_SQUARE:
        outer.left = rect.left - 1;
        outer.top = rect.top - 1;
        outer.right = rect.right + 1;
        outer.bottom = rect.bottom + 1;
        screen_texture_rect(g_renderer, &outer, 0, MARK_Z, 0x99ffffff, &g_whiteTexture, g_fullUV);
        screen_texture_rect(g_renderer, &rect, 0, MARK_Z, color, &g_whiteTexture, g_fullUV);
        if (g_windowMgr.mini_map_wnd != 0)
            ui_window_send(g_windowMgr.mini_map_wnd, NULL, MSG_MEMBER_MARK, x, y, 0, 0);
        return;

    case MARK_CROSS: {
        double arm = (double)(size * 4);
        outer.left = (int)(cx - arm);
        outer.top = rect.top;
        outer.right = (int)(arm + cx);
        outer.bottom = rect.bottom;
        bar.left = rect.left;
        bar.top = (int)(cy - arm);
        bar.right = rect.right;
        bar.bottom = (int)(arm + cy);
        screen_texture_rect(g_renderer, &outer, 0, MARK_Z, color, &g_whiteTexture, g_fullUV);
        screen_texture_rect(g_renderer, &bar, 0, MARK_Z, color, &g_whiteTexture, g_fullUV);
        return;
    }

    case MARK_PLAYER_ARROW: {
        int angle = (int)(180.0f - self->world->player->roty);
        outer.left = rect.left - 2;
        outer.top = rect.top - 2;
        outer.right = rect.right + 2;
        outer.bottom = rect.bottom + 4;
        screen_texture_rect(g_renderer, &outer, angle, MARK_Z, 0x77333333,
                          texture_cache_get(&g_textureCache, string_cstr(texture), 0, 0), g_fullUV);
        screen_texture_rect(g_renderer, &rect, angle, MARK_Z, color,
                          texture_cache_get(&g_textureCache, string_cstr(texture), 0, 0), g_fullUV);
        if (g_lastPlayerX == x && g_lastPlayerY == y)
            return;
        g_lastPlayerX = x;
        g_lastPlayerY = y;
        if (g_windowMgr.mini_map_wnd != 0)
            ui_window_send(g_windowMgr.mini_map_wnd, NULL, MSG_PLAYER_CELL, x, y, 0, 0);
        if (g_windowMgr.minimap_zoom_wnd != 0)
            ui_window_refresh(g_windowMgr.minimap_zoom_wnd);
        return;
    }

    case MARK_NAVI_TARGET:
        screen_texture_rect(g_renderer, &rect, 0, MARK_Z, 0xffffffff,
                          texture_cache_get(&g_textureCache, string_cstr(texture), 0, 0), g_fullUV);
        if (g_windowMgr.mini_map_wnd == 0 || (g_lastNaviX == x && g_lastNaviY == y))
            return;
        g_lastNaviX = x;
        g_lastNaviY = y;
        ui_window_send(g_windowMgr.mini_map_wnd, NULL, MSG_NAVI_TARGET, x, y, 0, 0);
        return;

    case MARK_NAVI:
        if (texture->size == 0) {
            ui_window_send(ui_window_find(WID_NAVI), NULL, MSG_NAVI_MARK, NAVI_MARK_EMPTY, (int)color, rect.left, rect.top);
            return;
        }
        ui_window_send(ui_window_find(WID_NAVI), NULL, MSG_NAVI_MARK, NAVI_MARK_ICON, (int)color, rect.left, rect.top);
        break;
    default:
        return;
    }

    screen_texture_rect(g_renderer, &rect, 0, MARK_Z, 0xffffffff,
                      texture_cache_get(&g_textureCache, string_cstr(texture), 0, 0), g_fullUV);
}

RAGE_NOINLINE void game_mode_draw_minimap_mark(struct GameMode *self, const struct TextureUV *uv, int x, int y, int size,
                                         struct String texture, int kind, unsigned color, char clamp)
{
    draw_mark(self, uv, x, y, size, &texture, kind, color, clamp);
    string_free(&texture);
}

RAGE_NOINLINE void game_mode_draw_member_marks(struct GameMode *self, const struct TextureUV *uv)
{
    struct TreeNode *it;
    int index = 0;

    if (g_windowMgr.mini_map_wnd != 0) {
        ui_window_send(g_windowMgr.mini_map_wnd, NULL, MSG_MARK_COUNT, 0, (int)self->square_marks.size, 0, 0);
        ui_window_send(g_windowMgr.mini_map_wnd, NULL, MSG_MARK_COUNT, 1, (int)self->triangle_marks.size, 0, 0);
    }
    for (it = self->square_marks.head->left; it != self->square_marks.head; ) {
        const struct MemberMark *mark = TREE_VALUE(it, const struct MemberMark);
        game_mode_draw_minimap_mark(self, uv, mark->x, mark->y, 2, string_from("", 0), MARK_SQUARE, mark->color, 0);
        if (g_windowMgr.mini_map_wnd != 0)
            ui_window_send(g_windowMgr.mini_map_wnd, NULL, MSG_MEMBER_MARK, 0, index, mark->x, mark->y);
        index++;
        it = tree_next(it);
    }
    for (it = self->triangle_marks.head->left; it != self->triangle_marks.head; ) {
        const struct MemberMark *mark = TREE_VALUE(it, const struct MemberMark);
        if (!tree_find(&self->square_marks, &mark->id, tree_less_uint)) {
            game_mode_draw_minimap_mark(self, uv, mark->x, mark->y, 2, string_from("", 0), MARK_TRIANGLE, mark->color, 0);
            if (g_windowMgr.mini_map_wnd != 0)
                ui_window_send(g_windowMgr.mini_map_wnd, NULL, MSG_MEMBER_MARK, 1, index, mark->x, mark->y);
        }
        it = tree_next(it);
        index++;
    }
    for (it = self->quest_marks.head->left; it != self->quest_marks.head; it = tree_next(it)) {
        const struct QuestMark *mark = TREE_VALUE(it, const struct QuestMark);
        struct String name, copy;
        if (mark->value.icon == 0)
            continue;
        string_init(&name);
        string_format(&name, "유저인터페이스\\basic_interface\\quest_%d.bmp",
                          (int)mark->value.icon);
        copy = string_from(string_cstr(&name), name.size);
        game_mode_draw_minimap_mark(self, uv, mark->value.x, mark->value.y, 4, copy, MARK_TEXTURE, 0, 1);
        string_free(&name);
    }
    if (g_session1.store_search_pos_x > -1 && g_session1.store_search_pos_y > -1)
        game_mode_draw_minimap_mark(self, uv, g_session1.store_search_pos_x, g_session1.store_search_pos_y, 4,
                                 string_from("유저인터페이스\\basic_interface\\search_store.bmp", 0x2f),
                                 MARK_TEXTURE, 0, 1);
}

static void draw_ping_marks(struct GameMode *self, const struct TextureUV *uv)
{
    struct TreeNode *it;

    if (self->ping_marks.size == 0)
        return;
    for (it = tree_first(&self->ping_marks); it != self->ping_marks.head; ) {
        const struct PingMark *mark = TREE_VALUE(it, const struct PingMark);
        unsigned set_at = mark->time;
        if (mark->sticky == 0 && platform_ticks_ms() > set_at + 15000)
            it = tree_erase(&self->ping_marks, it, NULL);
        else
            it = tree_next(it);
    }
    for (it = self->ping_marks.head->left; it != self->ping_marks.head; it = tree_next(it)) {
        const struct PingMark *mark = TREE_VALUE(it, const struct PingMark);
        unsigned set_at = mark->time;
        if ((int)(platform_ticks_ms() - set_at) % 1000 < 500)
            game_mode_draw_minimap_mark(self, uv, mark->x, mark->y, 1, string_from("", 0), MARK_CROSS, mark->color, 0);
    }
}

void game_mode_erase_ping_mark(struct GameMode *self, unsigned id)
{
    tree_erase_key(&self->ping_marks, &id, tree_less_uint, NULL);
}

void game_mode_draw_minimap(struct GameMode *self)
{
    const struct C3dAttr *attr;
    const struct Player *player;
    struct TextureUV uv;
    struct TreeNode *it;
    float centre_x, half, u, v;
    int cell_x, cell_y, width, height, saved_flag, index;
    RECT rect;

    if (g_windowMgr.minimap_zoom_wnd == 0)
        return;

    centre_x = (float)(g_renderer->width - MINIMAP_RIGHT_MARGIN) + 64.0f;
    attr = self->world->attr;
    width = attr->width;
    height = attr->height;
    player = self->world->player;
    game_mode_world_to_cell(self, player->pos.x, player->pos.z, &cell_x, &cell_y);

    half = 1.0f / (g_windowMgr.minimap_zoom * 2.0f);
    u = (float)cell_x / (float)width;
    v = (float)(height - cell_y) / (float)height;
    uv.l = u - half;
    uv.r = u + half;
    uv.t = v - half;
    uv.b = v + half;
    if (0.0f > uv.l) {
        uv.l = 0.0f;
        uv.r = half + half;
        if (uv.r > 1.0f)
            uv.r = 1.0f;
    }
    if (0.0f > uv.t) {
        uv.t = 0.0f;
        uv.b = half + half;
        if (uv.b > 1.0f)
            uv.b = 1.0f;
    }
    if (uv.r > 1.0f) {
        uv.r = 1.0f;
        uv.l = 1.0f - (half + half);
        if (0.0f > uv.l)
            uv.l = 0.0f;
    }
    if (uv.b > 1.0f) {
        uv.b = 1.0f;
        uv.t = 1.0f - (half + half);
        if (0.0f > uv.t)
            uv.t = 0.0f;
    }

    saved_flag = g_textureDivisor;
    g_textureDivisor = 1;
    rect.left = (int)(centre_x - 64.0f);
    rect.top = MINIMAP_TOP;
    rect.right = (int)(centre_x + 64.0f);
    rect.bottom = MINIMAP_TOP + MINIMAP_SIZE;
    screen_texture_rect(g_renderer, &rect, 0, MAP_Z, g_windowMgr.minimap_argb,
                      texture_cache_get(&g_textureCache, self->minimap_texture, 0, 0), uv);
    game_mode_draw_minimap_mark(self, &uv, cell_x, cell_y, 5,
                             string_from("유저인터페이스\\map\\map_arrow.bmp", 0x20),
                             MARK_PLAYER_ARROW, 0xffffffff, 0);

    for (it = tree_first(&g_session1.memberPings); it != g_session1.memberPings.head; ) {
        struct MemberPing *ping = TREE_VALUE(it, struct MemberPing);
        game_mode_draw_minimap_mark(self, &uv, ping->x, ping->y, 2, string_from("", 0), MARK_SQUARE, 0xffff0000, 0);
        if (--ping->frames_left > 0)
            it = tree_next(it);
        else
            it = tree_erase(&g_session1.memberPings, it, NULL);
    }

    if (self->boss_shown)
        game_mode_draw_minimap_mark(self, &uv, self->boss_x, self->boss_y, 6,
                                 string_from("유저인터페이스\\map\\bossmonster.bmp", 0x22),
                                 MARK_NAVI_TARGET, 0, 1);

    if (g_windowMgr.minimap_show_navigation) {
        for (index = 0;; index++) {
            struct TownInfo mark;
            town_info_get(g_townInfo, &mark, string_from(self->map_name, strlen(self->map_name)), index);
            if (mark.icon.size == 0 || mark.kind == 8 || mark.kind == 9) {
                string_free(&mark.icon);
                string_free(&mark.name);
                break;
            }
            game_mode_draw_minimap_mark(self, &uv, mark.x, mark.y, 6, string_from("", 0), MARK_NAVI, (unsigned)index, 1);
            string_free(&mark.icon);
            string_free(&mark.name);
        }
    }

    game_mode_draw_member_marks(self, &uv);
    draw_ping_marks(self, &uv);
    g_textureDivisor = saved_flag;
}

#pragma pack(push, 1)
struct PACKET_ZC_QUEST_NOTIFY_EFFECT {
    unsigned short id;
    unsigned npc_id;
    unsigned short x, y;
    unsigned short effect;
    unsigned short icon;
};
struct PACKET_ZC_SEARCH_STORE_POS {
    unsigned short id;
    short x, y;
};
#pragma pack(pop)

enum { QUEST_EFFECT_NONE = 9999 };
enum { MSG_QUEST_MARK = 0x1dd };

void game_mode_zc_quest_notify_effect(struct GameMode *self, const struct PACKET_ZC_QUEST_NOTIFY_EFFECT *packet)
{
    struct QuestMark *mark;
    int x = packet->x, y = packet->y;

    if (packet->effect == QUEST_EFFECT_NONE) {
        x = -1;
        y = -1;
    }
    mark = tree_get(&self->quest_marks, &packet->npc_id, sizeof packet->npc_id, sizeof *mark, tree_less_uint);
    mark->value.x = (short)x;
    mark->value.y = (short)y;
    mark->value.effect = packet->effect;
    mark->value.icon = packet->icon;
    game_mode_send_quest_marks(self);
}

struct QuestMarkValue *game_mode_get_quest_mark(struct GameMode *self, unsigned npc_id)
{
    struct QuestMark *mark = tree_find(&self->quest_marks, &npc_id, tree_less_uint);

    if (!mark)
        return 0;
    return &mark->value;
}

void game_mode_send_quest_marks(struct GameMode *self)
{
    struct TreeNode *it;

    if (g_windowMgr.mini_map_wnd == 0)
        return;
    for (it = self->quest_marks.head->left; it != self->quest_marks.head; it = tree_next(it)) {
        const struct QuestMark *mark = TREE_VALUE(it, const struct QuestMark);
        if (mark->value.icon != 0)
            ui_window_send(g_windowMgr.mini_map_wnd, NULL, MSG_QUEST_MARK, mark->value.effect, mark->value.icon, mark->value.x, mark->value.y);
    }
}

void game_mode_set_actor_mark_flag(struct GameMode *self, int aid, int flag)
{
    struct GameActor *actor = world_get_game_actor_by_aid(self->world, aid);

    if (actor != 0)
        game_actor_set_mark_listed(actor, flag);
}

void game_mode_set_search_store_pos(const struct PACKET_ZC_SEARCH_STORE_POS *packet)
{
    g_session1.store_search_pos_x = packet->x;
    g_session1.store_search_pos_y = packet->y;
}

#pragma pack(push, 1)
struct PACKET_ZC_BOSS_INFO {
    unsigned short id;
    unsigned char type;
    int x;
    int y;
    unsigned short hours;
    unsigned short minutes;
    unsigned short max_hours;
    unsigned short max_minutes;
    char name[51];
};
#pragma pack(pop)

enum {
    BOSS_INFO_NONE = 0,
    BOSS_INFO_ALIVE = 1,
    BOSS_INFO_ALIVE_SAY = 2,
    BOSS_INFO_DEAD = 3,
    MSG_BOSS_NONE = 0x4c7,
    MSG_BOSS_RESPAWN = 0x4c8,
    MSG_BOSS_ALIVE = 0x4c9,
    MSG_BOSS_RESPAWN_SOON = 0x4ed,
    UIM_BOSS_LINE = 0x13,
};
#define BOSS_COLOR 0xffffff

static void boss_line(const char *text)
{
    ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (intptr_t)text, BOSS_COLOR, 0, 0);
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_BOSS_LINE, (intptr_t)text, BOSS_COLOR, 0, 0);
}

void game_mode_zc_boss_info(struct GameMode *self, const struct PACKET_ZC_BOSS_INFO *packet)
{
    char name[0x34], line[0x100];

    sprintf(name, "%s", packet->name);
    self->boss_x = -1;
    self->boss_y = -1;
    switch (packet->type) {
    case BOSS_INFO_NONE:
        ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (intptr_t)msg_str(MSG_BOSS_NONE), BOSS_COLOR, 0, 0);
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_BOSS_LINE, (intptr_t)msg_str(MSG_BOSS_NONE), BOSS_COLOR, 0, 0);
        return;
    case BOSS_INFO_ALIVE:
    case BOSS_INFO_ALIVE_SAY:
        self->boss_x = packet->x;
        self->boss_y = packet->y;
        if (packet->type != BOSS_INFO_ALIVE_SAY)
            return;
        sprintf(line, msg_str(MSG_BOSS_ALIVE), name);
        break;
    case BOSS_INFO_DEAD:
        if (packet->hours == 0 && packet->minutes == 1)
            sprintf(line, msg_str(MSG_BOSS_RESPAWN_SOON), name);
        else
            sprintf(line, msg_str(MSG_BOSS_RESPAWN), name, packet->hours, packet->minutes);
        break;
    default:
        return;
    }
    boss_line(line);
}

#pragma pack(push, 1)
struct PACKET_ZC_C_MARKERINFO {
    unsigned short id;
    int aid;
    short x;
    short y;
};
#pragma pack(pop)

#define MEMBER_PING_FRAMES 120

void game_mode_zc_c_marker_info(const struct PACKET_ZC_C_MARKERINFO *packet)
{
    struct MemberPing *ping = tree_find(&g_session1.memberPings, &packet->aid, tree_less_int);

    if (ping) {
        ping->x = packet->x;
        ping->y = packet->y;
        ping->frames_left = MEMBER_PING_FRAMES;
    } else {
        struct MemberPing value;

        value.id = packet->aid;
        value.frames_left = MEMBER_PING_FRAMES;
        value.x = packet->x;
        value.y = packet->y;
        tree_add(&g_session1.memberPings, &value, sizeof value, tree_less_int, NULL);
    }
}
#define COMPASS_ERASE           2

#pragma pack(push, 1)
struct PACKET_ZC_COMPASS {
    unsigned short packet_type;
    int npc_id;
    int type;
    int x;
    int y;
    unsigned char id;
    unsigned color;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_COMPASS, 23);

void game_mode_zc_compass(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_COMPASS *packet = (const struct PACKET_ZC_COMPASS *)pkt;
    struct PingMark *mark;
    unsigned id;

    if (packet->type == COMPASS_ERASE) {
        game_mode_erase_ping_mark(self, packet->id);
        return;
    }
    id = packet->id;
    mark = tree_get(&self->ping_marks, &id, sizeof id, sizeof *mark, tree_less_uint);
    mark->x = packet->x;
    mark->y = packet->y;
    mark->color = packet->color | 0xff000000;
    mark->sticky = packet->type;
    mark->time = platform_ticks_ms();
}
