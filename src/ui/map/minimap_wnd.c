#include "game/cells.h"
#include "game/mode_misc.h"
#include "game/quest_marks.h"
#include "contents/town_info.h"
#include "ui/controls/buttons.h"
#include "render/rp_pool.h"
#include "resources/sprite.h"
#include "std/vec.h"
#include "std/tree.h"
#include "navi/navi_manager.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "core/object_type.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "ui/character/character.h"
#include "game/context.h"
#include "ui/ui_std.h"
#include "ui/map/map_std.h"
#include <math.h>
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_names.h"
#include "session/session_state.h"
#include "ui/social/guild_member_sort.h"
#include "ui/wndmgr/wndmgr.h"

struct UIMiniMapWnd;
void ui_mini_map_wnd_destroy(struct UIWindow *window);
void ui_mini_map_wnd_store_info(struct UIWindow *self);
void ui_mini_map_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_mini_map_wnd_on_draw(struct UIWindow *self);
void ui_mini_map_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
int ui_mini_map_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_mini_map_wnd_ops = {
    .destroy = ui_mini_map_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_mini_map_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_mini_map_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_mini_map_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_mini_map_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_mini_map_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};
#define MINIMAP_WND 0x111
#define TOWN_ID_FIRST 1000
#define QUEST_ID_FIRST 0x7d1
#define OUT_OF_SIGHT -100

#define CMD_SHOW_TOWNS 0xba
#define CMD_SHOW_QUESTS 0x161
#define CMD_SHOW_MEMBERS 0x16d

#define MSG_PLAYER_CELL 0x1d9
#define MSG_BOSS_CELL 0x1da
#define MSG_MEMBER_MARK 0x1db
#define MSG_MARK_COUNT 0x1dc
#define MSG_QUEST_MARK 0x1dd
#define MSG_CLEAR_QUESTS 0x1de

enum { MARK_PARTY, MARK_GUILD, MARK_WARP, MARK_KINDS };

#define MINIMAP_SKIN "유저인터페이스\\"

struct UIMiniMapWnd {
    struct UIFrameWnd frame;
    struct MiniMapWndInfo *info;
    float scale;
    float origin_x, origin_y;
    struct UIWindow *show_quests;
    struct UIWindow *show_towns;
    struct UIWindow *show_members;
    struct UIWindow *player;
    struct UIBitmapButton *boss;
    struct Tree town_icons;
    struct Tree quest_icons;
    struct Tree quest_cells;
    struct Vec marks[MARK_KINDS];
};
OFFSET_ASSERT(UIMiniMapWnd, show_quests, 0xa4);
OFFSET_ASSERT(UIMiniMapWnd, town_icons, 0xb8);
OFFSET_ASSERT(UIMiniMapWnd, marks, 0xd0);
SIZE_ASSERT(UIMiniMapWnd, 0xf4);

struct MapIcon {
    int id;
    struct UIWindow *button;
};

struct QuestCell {
    int id;
    POINT cell;
};

static int checked(const struct UIWindow *check_box)
{
    return check_box->state != 0;
}

static int cell_to_x(const struct UIMiniMapWnd *self, float x, int off)
{
    return (int)(self->scale * x - (float)off + self->origin_x);
}
static int cell_to_y(const struct UIMiniMapWnd *self, float y, int off)
{
    return (int)(self->origin_y - (self->scale * y + (float)off));
}

struct UIMiniMapWnd *ui_mini_map_wnd_construct(struct UIMiniMapWnd *self)
{
    int i;

    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_mini_map_wnd_ops;
    tree_init(&self->town_icons);
    tree_init(&self->quest_icons);
    tree_init(&self->quest_cells);
    for (i = 0; i < MARK_KINDS; i++)
        self->marks[i].first = self->marks[i].last = self->marks[i].end = NULL;
    self->show_quests = NULL;
    self->show_towns = NULL;
    return self;
}

struct UIWindow *ui_mini_map_wnd_new(void)
{
    return (struct UIWindow *)ui_mini_map_wnd_construct(mem_alloc(sizeof(struct UIMiniMapWnd)));
}

void ui_mini_map_wnd_destroy(struct UIWindow *window)
{
    struct UIMiniMapWnd *self = (struct UIMiniMapWnd *)window;
    int i;

    self->frame.base.ops = &ui_mini_map_wnd_ops;
    self->show_quests = NULL;
    self->show_towns = NULL;
    self->show_members = NULL;
    for (i = MARK_KINDS - 1; i >= 0; i--)
        vec_free(&self->marks[i]);
    tree_free(&self->quest_cells, NULL);
    tree_free(&self->quest_icons, NULL);
    tree_free(&self->town_icons, NULL);
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

static void minimap_fit(struct UIMiniMapWnd *self)
{
    float w = (float)game_mode_get_map_width(game_current_mode());
    float h = (float)game_mode_get_map_height(game_current_mode());
    float margin;

    self->origin_x = 2.0f;
    self->origin_y = 531.0f;
    self->scale = 512.0f / (w > h ? w : h);
    margin = (float)fabs(w - h) * 0.5f * self->scale;
    if (h > w)
        self->origin_x = margin + 2.0f;
    else if (w > h)
        self->origin_y = 531.0f - margin;
}

static struct UIBitmapButton *icon_button(const char *bitmap)
{
    struct UIBitmapButton *button = mem_alloc(sizeof *button);

    if (button != NULL)
        button = ui_bitmap_button_construct(button);
    ui_bitmap_button_set_bitmap(button, bitmap, 0, 0);
    ui_bitmap_button_set_bitmap(button, bitmap, 1, 0);
    ui_bitmap_button_set_bitmap(button, bitmap, 2, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    return button;
}

static void minimap_load_towns(struct UIMiniMapWnd *self)
{
    const char *map_name = game_current_map_name();
    struct String map, arg;
    struct TownInfo info;
    struct UIBitmapButton *button;
    struct MapIcon icon;
    POINT cell;
    int i;

    string_init(&map);
    string_assign_cstr(&map, map_name);
    self->marks[MARK_WARP].last = self->marks[MARK_WARP].first;
    for (i = 0;; i++) {
        string_copy(&arg, &map);
        town_info_get(g_townInfo, &info, arg, i);
        if (info.icon.size == 0)
            break;
        if (info.kind == 8) {
            POINT *last = self->marks[MARK_WARP].last;

            cell.x = cell_to_x(self, (float)info.x, 6);
            cell.y = cell_to_y(self, (float)info.y, 6);
            if (last != self->marks[MARK_WARP].end) {
                *last = cell;
                self->marks[MARK_WARP].last = last + 1;
            } else {
                vec_push(&self->marks[MARK_WARP], &cell, sizeof cell);
            }
        } else {
            button = icon_button(string_cstr(&info.icon));
            button->button.base.id = i + TOWN_ID_FIRST;
            ui_bitmap_button_set_tool_tip(button, string_cstr(&info.name));
            ui_window_move_base(&button->button.base, cell_to_x(self, (float)info.x, 6), cell_to_y(self, (float)info.y, 6));
            ui_window_add_child(&self->frame.base, &button->button.base);
            icon.id = i;
            icon.button = &button->button.base;
            tree_add(&self->town_icons, &icon, sizeof icon, tree_less_int, NULL);
        }
        string_free(&info.icon);
        string_free(&info.name);
    }
    string_free(&info.icon);
    string_free(&info.name);
    string_free(&map);
}

static void minimap_dress_player(struct UIMiniMapWnd *self)
{
    struct String path;
    struct SprRes *spr;

    if (self->player == NULL)
        return;
    session_get_own_head_spr_name0(&g_session1, &path);
    ui_sprite_button_set_act(self->player, string_cstr(&path));
    string_free(&path);
    if (g_session1.hairColor != 0) {
        session_get_own_head_pal_name(&g_session1, &path);
        ui_sprite_button_set_palette(self->player, string_cstr(&path));
        string_free(&path);
    }
    session_get_own_head_spr_name1(&g_session1, &path);
    ui_sprite_button_set_sprite(self->player, string_cstr(&path));
    spr = object_as_type(res_mgr_get(g_resMgr(), string_cstr(&path)), OBJECT_SPRITE_RESOURCE);
    if (spr != NULL) {
        const struct SprImg *image = sprite_resource_image(spr, 0, 0);

        ui_window_create(self->player, image ? image->width : 2, image ? image->height : 2);
        if (!image) ui_window_move(self->player, OUT_OF_SIGHT, OUT_OF_SIGHT);
    } else {
        ui_window_create(self->player, 2, 2);
        ui_window_move(self->player, OUT_OF_SIGHT, OUT_OF_SIGHT);
    }
    string_free(&path);
}

void ui_mini_map_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIMiniMapWnd *self = (struct UIMiniMapWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UISpriteButton *player;
    int height = wnd->height, row;

    minimap_fit(self);
    row = height - 0x13;
    self->show_quests = ui_window_add_check_box_plain(wnd, CMD_SHOW_QUESTS, 0x64, 0xc, msg_str(0xab6), 0x14, row, 1,
                                                  msg_str(0xab7));
    self->show_towns = ui_window_add_check_box_plain(wnd, CMD_SHOW_TOWNS, 0x64, 0xc, msg_str(0xab8), 0x78, row, 1,
                                                 msg_str(0xab9));
    self->show_members = ui_window_add_check_box_plain(wnd, CMD_SHOW_MEMBERS, 0x64, 0xc, msg_str(0xaba), 0xdc,
                                                   row, 1, msg_str(0xabb));
    ui_window_add_close_button(wnd, msg_str(0xab5), 0);
    minimap_load_towns(self);
    self->boss = ui_window_add_numbered_button(wnd, 0xc5, OUT_OF_SIGHT, OUT_OF_SIGHT, "minimap\\boss",
                                            msg_str(0xabd), 1);
    game_mode_send_quest_marks(game_current_mode());

    self->player = mem_alloc(sizeof(struct UISpriteButton));
    if (self->player != NULL)
        self->player = ui_headspr_button_construct(self->player);
    minimap_dress_player(self);
    ui_window_move(self->player, OUT_OF_SIGHT, OUT_OF_SIGHT);
    player = (struct UISpriteButton *)self->player;
    player->action = 0;
    player->motion = 0;
    player->animating = 0;
    ui_bitmap_button_set_tool_tip((struct UIBitmapButton *)self->player, msg_str(0xabe));
    self->player->id = 0xc0;
    ui_window_add_child(wnd, self->player);
}

static void draw_marks(struct UIMiniMapWnd *self, int kind, const char *bitmap, int placed_only)
{
    struct BitmapRes *res = ui_skin_bitmap(bitmap);
    const POINT *marks;
    size_t i;

    for (i = 0; i < VEC_COUNT(&self->marks[kind], POINT); i++) {
        marks = self->marks[kind].first;
        if (placed_only && (marks[i].x <= 0 || marks[i].y <= 0))
            continue;
        if (res != NULL)
            ui_window_draw_bitmap(&self->frame.base, marks[i].x, marks[i].y, res, 1);
    }
}

void ui_mini_map_wnd_on_draw(struct UIWindow *window)
{
    struct UIMiniMapWnd *self = (struct UIMiniMapWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct String title, path;
    struct BitmapRes *picture;
    int x = 0, y = 0;

    game_mode_get_player_cell(game_current_mode(), &x, &y);
    string_init(&title);
    string_format(&title, "%s ( %s ) %d %d", session_get_map_display_name(&g_session1), g_session1.mapName, x, y);
    ui_window_draw_body(wnd, ui_window_draw_title_bar(wnd, 1, string_cstr(&title), 0), 0, "");
    ui_window_draw_button_bar(wnd, 1, 0, 1);
    ui_window_fill_rect(wnd, 2, 0x13, 0x200, 0x200, 0xffaaaaaa);
    string_init(&path);
    string_format(&path, MINIMAP_SKIN "map\\%s.bmp", g_session1.mapName);
    picture = ui_skin_bitmap(string_cstr(&path));
    if (picture != NULL) {
        ui_window_draw_bitmap(wnd, 2, 0x13, picture, 1);
        dc_bitmap_brighten(wnd->canvas, 2, 0x13, 0x200, 0x200, -0x32);
    }
    draw_marks(self, MARK_WARP, MINIMAP_SKIN "minimap\\warp.bmp", 0);
    if (checked(self->show_members)) {
        draw_marks(self, MARK_GUILD, MINIMAP_SKIN "minimap\\guild.bmp", 1);
        draw_marks(self, MARK_PARTY, MINIMAP_SKIN "minimap\\Party.bmp", 1);
    }
    string_free(&path);
    string_free(&title);
}

void ui_mini_map_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    if (y <= 0x11)
        ui_frame_wnd_on_l_btn_down(window, x, y);
}

void ui_mini_map_wnd_store_info(struct UIWindow *window)
{
    struct UIMiniMapWnd *self = (struct UIMiniMapWnd *)window;
    struct MiniMapWndInfo *info = self->info;

    if (info != NULL) {
        info->x = self->frame.base.x;
        self->info->y = self->frame.base.y;
        self->info->width = self->frame.base.width;
        self->info->height = self->frame.base.height;
        info = self->info;
    }
    info->show_quest = checked(self->show_quests);
    self->info->show_npc = checked(self->show_towns);
    self->info->show_member = checked(self->show_members);
}

static void show_icons(struct UIMiniMapWnd *self, struct Tree *icons, BYTE show)
{
    struct TreeNode *node;

    for (node = tree_first(icons); node != icons->head; node = tree_next(node))
        ui_button_show((struct UIButton *)(TREE_VALUE(node, struct MapIcon)->button), show);
    ui_window_invalidate_base(&self->frame.base);
}

static void show_town_icons(struct UIMiniMapWnd *self)
{
    show_icons(self, &self->town_icons, (BYTE)checked(self->show_towns));
}
static void show_quest_icons(struct UIMiniMapWnd *self)
{
    show_icons(self, &self->quest_icons, (BYTE)checked(self->show_quests));
}

static void navigate_to(int x, int y)
{
    struct String map;
    size_t dot;

    game_mode_get_map_name(game_current_mode(), &map);
    dot = string_find_first_of(&map, ".", 0);
    if (dot != (size_t)-1) {
        string_truncate(&map, dot);
    }
    navi_navigate(&g_navi, string_cstr(&map), 0, 5, 1, x, y, 0x3ea);
    string_free(&map);
}

static int minimap_command(struct UIMiniMapWnd *self, int id)
{
    struct UIWindow *wnd = &self->frame.base;

    switch (id) {
    case CMD_SHOW_TOWNS:
        show_town_icons(self);
        return 0;
    case CMD_SHOW_QUESTS:
        show_quest_icons(self);
        return 0;
    case CMD_SHOW_MEMBERS:
        ui_window_invalidate_base(wnd);
        return 0;
    case 0xc0:
        return 0;
    case UI_ID_CLOSE:
        ui_window_mgr_post_quit1(&g_windowMgr, MINIMAP_WND);
        return 0;
    }

    if (id >= TOWN_ID_FIRST + 1 && id < TOWN_ID_FIRST + 1000) {
        struct String map, arg;
        struct TownInfo info;

        game_mode_get_map_name(game_current_mode(), &map);
        string_copy(&arg, &map);
        town_info_get(g_townInfo, &info, arg, id - TOWN_ID_FIRST);
        if (info.icon.size != 0) {
            size_t dot = string_find_first_of(&map, ".", 0);

            if (dot != (size_t)-1) {
                string_truncate(&map, dot);
            }
            navi_navigate(&g_navi, string_cstr(&map), 0, 5, 1, info.x, info.y, 0x3ea);
        }
        string_free(&info.icon);
        string_free(&info.name);
        string_free(&map);
        return 0;
    }

    if (id > QUEST_ID_FIRST - 1) {
        struct QuestCell *mark = tree_find(&self->quest_cells, &id, tree_less_int);

        if (mark)
            navigate_to(mark->cell.x, mark->cell.y);
        return 0;
    }
    return 0;
}

int ui_mini_map_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                    int val3, int val4)
{
    struct UIMiniMapWnd *self = (struct UIMiniMapWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIBitmapButton *button;
    struct String path;
    POINT cell;
    int off, id;
    size_t i;

    switch (msg) {
    case UI_MSG_SET_INFO:
        self->info = (struct MiniMapWndInfo *)val1;
        if (!mini_map_info_is_on_screen(self->info))
            mini_map_info_set_default(self->info);
        ui_frame_wnd_move(wnd, self->info->x, self->info->y);
        ui_check_box_set_check(self->show_quests, self->info->show_quest);
        ui_check_box_set_check(self->show_towns, self->info->show_npc);
        ui_check_box_set_check(self->show_members, self->info->show_member);
        return 0;
    case UI_MSG_COMMAND:
        return minimap_command(self, val1);
    case MSG_PLAYER_CELL:
        ui_window_move(self->player, cell_to_x(self, (float)val1, 13), cell_to_y(self, (float)val2, 13));
        ui_window_invalidate_base(wnd);
        return 0;
    case MSG_BOSS_CELL:
        ui_window_move_base(&self->boss->button.base, cell_to_x(self, (float)val1, 12), cell_to_y(self, (float)val2, 12));
        ui_window_invalidate_base(wnd);
        return 0;
    case MSG_MEMBER_MARK:
        if ((unsigned)val1 > MARK_WARP || val2 < 0
            || (unsigned)val2 >= VEC_COUNT(&self->marks[val1], POINT))
            return 0;
        off = val1 == MARK_WARP ? 8 : 6;
        ((POINT *)self->marks[val1].first)[val2].x = cell_to_x(self, (float)val3, off);
        ((POINT *)self->marks[val1].first)[val2].y = cell_to_y(self, (float)val4, off);
        ui_window_invalidate_base(wnd);
        return 0;
    case MSG_MARK_COUNT:
        if (val2 < 0 || (unsigned)val2 == VEC_COUNT(&self->marks[val1], POINT))
            return 0;
        cell.x = OUT_OF_SIGHT;
        cell.y = OUT_OF_SIGHT;
        i = VEC_COUNT(&self->marks[val1], POINT);
        vec_resize(&self->marks[val1], val2, sizeof cell);
        for (; i < (size_t)val2; i++)
            ((POINT *)self->marks[val1].first)[i] = cell;
        ui_window_invalidate_base(wnd);
        return 0;
    case MSG_QUEST_MARK: {
        struct MapIcon icon;
        struct QuestCell mark;

        id = (int)self->quest_icons.size + QUEST_ID_FIRST;
        string_init(&path);
        string_format(&path, MINIMAP_SKIN "minimap\\Quest_%d_%d.bmp", val1, val2);
        button = icon_button(string_cstr(&path));
        button->button.base.id = id;
        ui_bitmap_button_set_tool_tip(button, msg_str(0xac0));
        ui_window_move_base(&button->button.base, cell_to_x(self, (float)val3, 6), cell_to_y(self, (float)val4, 6));
        ui_window_add_child(wnd, &button->button.base);
        icon.id = id;
        icon.button = &button->button.base;
        tree_add(&self->quest_icons, &icon, sizeof icon, tree_less_int, NULL);
        mark.id = id;
        mark.cell.x = val3;
        mark.cell.y = val4;
        tree_add(&self->quest_cells, &mark, sizeof mark, tree_less_int, NULL);
        ui_window_invalidate_base(wnd);
        string_free(&path);
        return 0;
    }
    case MSG_CLEAR_QUESTS: {
        struct TreeNode *node;

        for (node = tree_first(&self->town_icons); node != self->town_icons.head; node = tree_next(node))
            ui_window_remove_child(wnd, TREE_VALUE(node, struct MapIcon)->button, 1);
        tree_clear(&self->quest_icons, NULL);
        tree_clear(&self->quest_cells, NULL);
        ui_window_invalidate_base(wnd);
        return 0;
    }
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
