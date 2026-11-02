#include "game/gamemode.h"
#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "render/projection.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "actor/sprite_draw.h"
#include "platform/win_types.h"
#include "actor/render_object.h"
#include "resources/attr_queries.h"
#include "contents/item_info_mgr.h"
#include "world/pick_node.h"
#include "game/context.h"
#include <stdlib.h>
#include "render/renderer.h"
#include <string.h>
#include "game/ground_item.h"
#include "game/cells.h"
#include "effects/rag/rag_effect_types.h"
#include "actor/game_actor.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "session/session_names.h"
#include "platform/clock.h"

#define ITEM_FRAME_MS 24.0f
#define ITEM_BLINK_PERIOD 0x5c
#define ITEM_BLINK_FROM 0x5a
#define ITEM_GRAVITY 0.083f
#define ITEM_DROP_VELOCITY -0.6f
#define ITEM_DROP_HEIGHT 15.0f

const struct RenderObjectOps item_ops = {
    .base = {
        .destroy = item_deleting_destroy,
        .on_process = item_on_process,
        .send_msg = item_send_msg,
        .render = item_render,
        .get_direction = game_object_dir_of,
    },
    .render_body = item_add_pick_box,
    .set_action = render_object_set_action_base,
    .update_motion = render_object_update_motion_base,
    .get_effect_state = render_object_return_zero,
};

struct Item *item_create(void)
{
    struct Item *item = malloc(sizeof *item);
    return item ? item_construct(item) : NULL;
}

struct Item *item_construct(struct Item *self)
{
    able_to_make_effect_construct((struct AbleToMakeEffect *)self);
    self->base.ops = &item_ops.base;
    string_init(&self->name);
    self->item_id_short = self->item_id = self->aid = self->falling = self->count = 0;
    self->identified = 0;
    self->velocity = self->base_y = 0.0f;
    self->type = self->drop_effect_mode = 0;
    self->base.has_shadow = 1;
    self->base.pickable = 1;
    self->base.zoom = 1.0f;
    self->base.hit_argb = 0xffffff00u;
    return self;
}

void item_deleting_destroy(struct GameObject *object)
{
    struct Item *self = (struct Item *)object;

    self->base.ops = &item_ops.base;
    string_free(&self->name);
    able_to_make_effect_dc_able_to_make_effect((struct AbleToMakeEffect *)self);
    free(self);
}

unsigned char item_on_process(struct GameObject *object)
{
    struct Item *self = (struct Item *)object;

    float frames = (float)(unsigned)(platform_ticks_ms() - self->base.start_tick) / ITEM_FRAME_MS;

    self->base.hit_flash = (int)frames % ITEM_BLINK_PERIOD >= ITEM_BLINK_FROM;
    if (self->falling) {
        struct GameMode *mode;
        float ground;

        self->base.pos.y = (frames * ITEM_GRAVITY + self->velocity) * frames + self->base_y;
        mode = game_current_mode();
        ground = c3d_attr_get_height(mode->world->attr, self->base.pos.x, self->base.pos.z);
        if (self->base.pos.y > ground) {
            self->base.pos.y = c3d_attr_get_height(mode->world->attr, self->base.pos.x, self->base.pos.z);
            self->falling = 0;
        }
    }
    return 1;
}

#define ITEM_SHADOW_SCALE 0.4f

void item_render(struct GameObject *object, const float *matrix)
{
    struct Item *self = (struct Item *)object;

    if (!self->base.visible)
        return;
    render_object_render_shadow((struct RenderObject *)&self->base, matrix, self->base.zoom * ITEM_SHADOW_SCALE);
    render_object_render_sprite((struct RenderObject *)&self->base, matrix, 0, 0.0f);
}

static __inline int drop_beam_effect(int mode)
{
    switch (mode) {
    case 1: return 0x74d;
    case 2: return 0x4a5;
    case 3: return 0x4a6;
    case 4: return 0x74e;
    case 5: return 0x74f;
    default: return 0x950;
    }
}

static void show_drop_effect(struct Item *self, int aid, int effect_id, Vec3 at)
{
    Vec3 here = { 0.0f, 0.0f, 0.0f };
    struct RagEffect *launched;

    if (effect_mgr_spawn(g_effectMgr, aid, effect_id, at, 0, -1, 0, 0))
        return;
    launched = game_actor_launch_effect((struct GameActor *)self, effect_id, here, 0.0f);
    if (launched)
        launched->base.pos = at;
}

void item_place(struct Item *self, struct GameMode *mode, const struct ItemDropInfo *info)
{
    struct String id_text, spr_name, act_name;

    struct String display = {0};
    struct String *display_name;
    struct ItemInfo item;
    float ground;

    self->aid = info->aid;
    self->count = info->count;
    string_assign_cstr(&self->name, info->item_id_text);
    string_copy(&id_text, &self->name);
    game_mode_sub_cell_to_world(mode, info->x, info->y, info->sub_x, info->sub_y, &self->base.pos.x, &self->base.pos.z);
    ground = c3d_attr_get_height(mode->world->attr, self->base.pos.x, self->base.pos.z);
    self->base.pos.y = ground;
    if (info->falls) {
        ground -= ITEM_DROP_HEIGHT;
        self->falling = 1;
        self->velocity = ITEM_DROP_VELOCITY;
        self->base.pos.y = ground;
    } else {
        self->falling = 0;
    }
    self->base_y = ground;

    string_init(&spr_name);
    {
        const char *res = session_get_item_display_name(string_cstr(&self->name),
                                                     info->is_identified);
        string_assign_cstr(&spr_name, res);
    }
    string_append_cstr(&spr_name, ".spr");
    string_init(&act_name);
    {
        const char *res = session_get_item_display_name(string_cstr(&self->name),
                                                     info->is_identified);
        string_assign_cstr(&act_name, res);
    }
    string_append_cstr(&act_name, ".act");
    render_object_load_spr_res((struct RenderObject *)self, string_cstr(&spr_name));
    render_object_load_act_res((struct RenderObject *)self, string_cstr(&act_name));

    item_info_construct(&item);
    self->item_id = atoi(info->item_id_text);
    self->type = info->type;
    self->item_id_short = (unsigned short)atoi(info->item_id_text);
    self->identified = info->is_identified;
    string_assign(&item.id_text, string_cstr(&id_text), id_text.size);
    item.identified = info->is_identified;
    item.refine = 0;
    display_name = item_info_get_display_name(&item, &display);
    string_format(&self->name, msg_str(0xb7), string_cstr(display_name), info->count);
    string_free(&display);

    render_object_get_act_res((struct RenderObject *)self);
    self->base.start_tick = platform_ticks_ms();

    if (info->show_drop_effect == 1) {
        Vec3 at;
        int effect_id;

        at.x = self->base.pos.x;
        at.y = c3d_attr_get_height(mode->world->attr, self->base.pos.x, self->base.pos.z);
        at.z = self->base.pos.z;
        effect_id = item_info_effect_id(self->item_id);
        if (effect_id >= 0)
            show_drop_effect(self, info->aid, effect_id, at);
        if ((unsigned short)(info->drop_effect_mode - 1) <= 4)
            show_drop_effect(self, info->aid, drop_beam_effect(info->drop_effect_mode), at);
    }

    item_info_destruct(&item);
    string_free(&act_name);
    string_free(&spr_name);
    string_free(&id_text);
}

void item_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct Item *self = (struct Item *)object;

    struct GameMode *mode = game_current_mode();

    if (msg != ITEM_MSG_DROP)
        {
            able_to_make_effect_send_msg(object, source, msg, par1, par2, par3, par4);
            return;
        }
    item_place(self, mode, (const struct ItemDropInfo *)(intptr_t)par1);
}

int g_minPickBoxSide;
struct Once g_minPickBoxSideGuard;

static __inline void widen_to(float *lo, float *hi, int min_side)
{
    int side = (int)(*hi - *lo);

    if (side < min_side) {
        float half = (float)((min_side - side) / 2);

        *lo -= half;
        *hi = half + *hi;
    }
}

void item_add_pick_box(struct RenderObject *object, const struct ScreenBounds *screen, float a, float b)
{
    struct Item *self = (struct Item *)object;
    if (self->base.pickable) {
        struct PickInfo *box;
        float left, right;
        int min_side;

        if (once_begin(&g_minPickBoxSideGuard)) {
            g_minPickBoxSide = (int)(renderer_scale_x(g_renderer) * 34.0f);
            once_complete(&g_minPickBoxSideGuard);
        }
        box = (struct PickInfo *)mem_alloc(sizeof *box);
        if (box) {
            box->left = 0.0f;
            box->top = 0.0f;
            box->near_z = 0.0f;
            box->right = 0.0f;
            box->bottom = 0.0f;
            box->far_z = 0.0f;
        }
        min_side = g_minPickBoxSide;
        left = screen->left < screen->right ? screen->left : screen->right;
        right = screen->left > screen->right ? screen->left : screen->right;
        box->left = left;
        box->top = screen->top;
        box->near_z = screen->z + 300.0f;
        box->right = right;
        box->bottom = screen->bottom;
        box->far_z = screen->z + 300.0f;
        box->aid = self->aid;
        box->job = 0x7d01;
        box->attackable_as_pvp = 0;
        box->kind = 1;
        widen_to(&box->left, &box->right, min_side);
        widen_to(&box->top, &box->bottom, min_side);
        actor_pick_node_insert(&g_actorPickNode, box);
    }
    render_object_set_screen_position(object, a, b);
}
