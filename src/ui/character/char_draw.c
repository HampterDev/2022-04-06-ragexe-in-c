#include "render/sprite_atlas.h"
#include "resources/palette.h"
#include "resources/sprite.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "ui/character/character_draw.h"
#include "resources/sprite_pair.h"
#include <string.h>
#include "ui/character/character.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/character/char_info.h"
#include "ui/runtime.h"
#include "ui/ui_std.h"
#include "ui/character/character_std.h"
#include <mmsystem.h>
#include <stdlib.h>
#include "std/std.h"
#include "actor/sprite_render.h"
#include "session/session_jobs.h"
#include "session/session_lua.h"
#include "session/session_names.h"
#include "session/session_talk.h"
#include "std/tree.h"

struct CharacterDraw {
    struct UIWindow *wnd;
    struct CharacterDrawOptions options;
    BYTE is_helm_robe;
    BYTE draw_on_top;
    struct Tree sprites;
    struct String body_palette_name;
    struct String head_palette_name;
    struct Tree layer_ids;
    int head_dx, head_dy;
};

static void character_draw_init_layers(struct CharacterDraw *self);
static BYTE character_draw_query_draw_on_top(struct CharacterDraw *self);
static struct SpriteResPair *character_draw_sprite_at(struct Tree *sprites, const int *layer);
static struct SpriteResPair *character_draw_get_layer(struct CharacterDraw *self,
                                                      struct SpriteResPair *out, int layer);
static int character_draw_clip_index(struct CharacterDraw *self, int layer);
static BYTE character_draw_is_accessory_layer(int layer);
static struct Vec *character_draw_collect_layers(struct CharacterDraw *self,
                                                       struct Vec *out);
static void character_draw_load_layers(struct CharacterDraw *self, const struct Vec *layers);
static int *character_draw_doram_offset(struct CharacterDraw *self, int *out, int layer);
static int *character_draw_head_offset(struct CharacterDraw *self, int *out, void *motion);
static int *character_draw_accessory_offset(struct CharacterDraw *self, int *out, int layer,
                                            void **motion, void *act);
static void *character_draw_robe_motion(struct CharacterDraw *self, int layer);
static DWORD *character_draw_palette(struct CharacterDraw *self, int layer, struct SprRes *spr);
static void character_draw_draw_layer(struct CharacterDraw *self, int layer,
                                      const struct SpriteResPair *pair, void *motion,
                                      DWORD *palette, int dx, int dy);
static void character_draw_draw_layer_scaled(struct CharacterDraw *self, int layer,
                                             const struct SpriteResPair *pair, void *motion,
                                             DWORD *palette, int dx, int dy);

struct CharacterDrawSprite {
    int layer;
    struct SpriteResPair pair;
};

struct LayerId {
    int layer;
    int id;
};

struct LayerOrder {
    int priority;
    int layer;
};

static void character_draw_init(struct CharacterDraw *self, struct UIWindow *wnd,
                                const struct CharacterDrawOptions *options)
{
    self->wnd = wnd;
    self->options = *options;
    tree_init(&self->sprites);
    string_init(&self->body_palette_name);
    string_init(&self->head_palette_name);
    tree_init(&self->layer_ids);
    character_draw_init_layers(self);
}

static struct SpriteResPair *character_draw_sprite_at(struct Tree *sprites, const int *layer)
{
    struct CharacterDrawSprite *sprite = tree_get(sprites, layer, sizeof *layer, sizeof *sprite, tree_less_int);

    return &sprite->pair;
}

static int *character_draw_layer_id(struct CharacterDraw *self, int layer)
{
    struct LayerId *entry = tree_get(&self->layer_ids, &layer, sizeof layer, sizeof *entry, tree_less_int);

    return &entry->id;
}

static BYTE character_draw_query_draw_on_top(struct CharacterDraw *self)
{
    BYTE on_top = 1;

    if (g_session1.new_layer_table_loaded == 1) {
        int job = self->options.job;

        if (self->options.riding == 1)
            job = session_get_mounted_job(job);
        if (client_lua_call(g_session1.luaState, "_New_DrawOnTop", "ddddd>bd", self->options.robe,
                            self->options.sex, job, self->options.action, self->options.motion,
                            &on_top, &job) == 1)
            return on_top;
        return 1;
    }
    if (client_lua_call(g_session1.luaState, "DrawOnTop", "ddddd>b", self->options.robe,
                        self->options.sex, (int)self->options.job, self->options.action,
                        self->options.motion, &on_top) == 1)
        return on_top;
    return 1;
}

static void character_draw_init_layers(struct CharacterDraw *self)
{
    self->head_dx = 0;
    self->head_dy = 0;
    client_lua_call_table(g_session1.luaState, "TB_Layer_Priority", "IsHelmRobe", "d>b",
                          self->options.robe, &self->is_helm_robe);
    self->draw_on_top = character_draw_query_draw_on_top(self);
    *character_draw_layer_id(self, 5) = self->options.accessory;
    *character_draw_layer_id(self, 4) = self->options.accessory2;
    *character_draw_layer_id(self, 3) = self->options.accessory3;
    *character_draw_layer_id(self, 6) = self->options.robe;
}

static struct SpriteResPair *character_draw_get_layer(struct CharacterDraw *self,
                                                      struct SpriteResPair *out, int layer)
{
    const struct CharacterDrawSprite *sprite = tree_find(&self->sprites, &layer, tree_less_int);

    if (!sprite) {
        out->spr = NULL;
        out->act = NULL;
        return out;
    }
    out->spr = sprite->pair.spr;
    out->act = sprite->pair.act;
    return out;
}

static int character_draw_clip_index(struct CharacterDraw *self, int layer)
{
    const struct LayerId *entry = tree_find(&self->layer_ids, &layer, tree_less_int);

    if (entry && session_is_head_replacing_view(&g_session1, entry->id) == 1)
        return 1;
    return layer == 2;
}

static BYTE character_draw_is_accessory_layer(int layer)
{
    return (unsigned)(layer - 3) <= 3;
}

static struct Vec *character_draw_collect_layers(struct CharacterDraw *self,
                                                       struct Vec *out)
{
    struct Tree order;
    int layer, i;

    tree_init(&order);
    for (layer = 0; layer < 8; layer++) {
        int kind = 0;

        if ((unsigned)(layer - 3) > 3)
            continue;
        if (layer == 6) {
            if (!self->is_helm_robe)
                continue;
            kind = 2;
        }
        switch (layer) {
        case 3:
            kind = 4;
            break;
        case 4:
            kind = 3;
            break;
        case 5:
            kind = 2;
            break;
        }
        if (*character_draw_layer_id(self, layer) > 0) {
            struct LayerOrder entry;
            int priority = -1;

            client_lua_call(g_session1.luaState, "GetLayerPriority", "ddd>d", 0, kind,
                            *character_draw_layer_id(self, layer), &priority);
            entry.priority = priority;
            entry.layer = layer;
            tree_add(&order, &entry, sizeof entry, tree_less_int, NULL);
        }
    }

    out->first = out->last = out->end = NULL;
    for (i = 0; i < 8; i++) {
        if (i == 0) {
            if (self->options.robe <= 0 || self->draw_on_top)
                continue;
        } else if (i == 7) {
            int *where;

            if (self->options.robe <= 0 || self->draw_on_top != 1)
                continue;
            if (session_is_top_layer_robe(&g_session1, self->options.robe) != 1) {
                for (where = (int *)out->first; where != (int *)out->last; where++)
                    if (*where == 2)
                        break;
                if (where == (int *)out->last)
                    continue;
                vec_insert(out, where, &i, sizeof i);
                continue;
            }
        } else if ((unsigned)(i - 3) <= 3) {
            struct TreeNode *first;

            if (!order.size)
                continue;
            first = tree_first(&order);
            vec_push(out, &TREE_VALUE(first, struct LayerOrder)->layer, sizeof(int));
            tree_erase(&order, first, NULL);
            continue;
        }
        vec_push(out, &i, sizeof i);
    }
    tree_free(&order, NULL);
    return out;
}

static void *res_by_name(const struct String *name)
{
    struct String copy;

    string_copy(&copy, name);
    return res_mgr_get_string(g_resMgr(), copy);
}

static void character_draw_load_layers(struct CharacterDraw *self, const struct Vec *layers)
{
    const int *it;
    int layer;

    for (it = (const int *)layers->first; it != (const int *)layers->last; it++) {
        struct String spr_name, act_name, tmp;
        void *spr, *act;

        string_init(&spr_name);
        string_init(&act_name);
        switch (*it) {
        case 1: {
            int job = self->options.riding == 1 ? session_get_mounted_job(self->options.job)
                                                : self->options.job;

            session_get_body_spr_name_variant1(&tmp, job, self->options.sex, self->options.body,
                                            self->options.scaled);
            string_assign(&spr_name, string_cstr(&tmp), tmp.size);
            string_free(&tmp);
            session_get_body_spr_name_variant0(&tmp, job, self->options.sex, self->options.body,
                                            self->options.scaled);
            string_assign(&act_name, string_cstr(&tmp), tmp.size);
            string_free(&tmp);
            session_get_body_pal_name(&tmp, job, self->options.sex, self->options.body_palette,
                                    self->options.body, self->options.scaled);
            string_assign(&self->body_palette_name, string_cstr(&tmp), tmp.size);
            string_free(&tmp);
            break;
        }
        case 2: {
            int head = self->options.head;

            session_get_head_spr_name1(&tmp, self->options.job, self->options.sex, &head);
            string_assign(&spr_name, string_cstr(&tmp), tmp.size);
            string_free(&tmp);
            session_get_head_spr_name0(&tmp, self->options.job, self->options.sex, &head);
            string_assign(&act_name, string_cstr(&tmp), tmp.size);
            string_free(&tmp);
            self->options.head = (short)head;
            session_get_head_pal_name(&tmp, (short)head, self->options.job, self->options.sex,
                                    self->options.head_palette);
            string_assign(&self->head_palette_name, string_cstr(&tmp), tmp.size);
            string_free(&tmp);
            break;
        }
        case 3:
        case 4:
        case 5:
        case 6:
            session_get_res_name1(&tmp, self->options.job, self->options.sex, self->options.head,
                                 *character_draw_layer_id(self, *it));
            string_assign(&spr_name, string_cstr(&tmp), tmp.size);
            string_free(&tmp);
            session_get_res_name0(&tmp, self->options.job, self->options.sex, self->options.head,
                                 *character_draw_layer_id(self, *it));
            string_assign(&act_name, string_cstr(&tmp), tmp.size);
            string_free(&tmp);
            break;
        case 0:
        case 7:
            if (self->options.scaled)
                break;
            session_get_robe_spr_name(&tmp, self->options.job, self->options.sex, self->options.robe,
                                   self->options.riding);
            string_assign(&spr_name, string_cstr(&tmp), tmp.size);
            string_free(&tmp);
            session_get_robe_act_name(&tmp, self->options.job, self->options.sex, self->options.robe,
                                   self->options.riding);
            string_assign(&act_name, string_cstr(&tmp), tmp.size);
            string_free(&tmp);
            break;
        }
        spr = res_by_name(&spr_name);
        act = res_by_name(&act_name);
        if (spr && act) {
            struct SpriteResPair *pair = character_draw_sprite_at(&self->sprites, it);

            pair->spr = spr;
            pair->act = act;
        }
        string_free(&act_name);
        string_free(&spr_name);
    }

    for (layer = 3; layer <= 5; layer++) {
        struct CharacterDrawSprite *found = tree_find(&self->sprites, &layer, tree_less_int);
        struct SpriteResPair *head, saved;
        int two = 2;

        if (!found)
            continue;
        if (session_is_head_replacing_view(&g_session1, *character_draw_layer_id(self, layer)) != 1)
            continue;
        head = character_draw_sprite_at(&self->sprites, &two);
        saved = *head;
        head->spr = found->pair.spr;
        head->act = found->pair.act;
        found->pair.spr = saved.spr;
        found->pair.act = saved.act;
        tree_erase(&self->sprites, TREE_NODE_OF(found), NULL);
        return;
    }
}

static int *character_draw_doram_offset(struct CharacterDraw *self, int *out, int layer)
{
    out[0] = 0;
    out[1] = 0;
    if (session_is_doram_job(&g_session1, self->options.job) == 1) {
        int dx = 0, dy = 0, turn = 0, dir;

        if (self->options.motion == 2)
            turn = -1;
        else if (self->options.motion == 1)
            turn = 1;
        dir = self->options.action % 8 + turn;
        if (dir < 0)
            dir = 7;
        if (dir > 7)
            dir = 0;
        client_lua_call(g_session1.luaState, "OffsetItemPos_GetOffsetForDoram", "ddd>dd",
                        *character_draw_layer_id(self, layer), dir,
                        session_get_sex(&g_session1), &dx, &dy);
        out[0] = dx;
        out[1] = dy;
    }
    return out;
}

static int *character_draw_head_offset(struct CharacterDraw *self, int *out, void *motion)
{
    struct SpriteResPair body;
    void *body_motion;

    out[0] = 0;
    out[1] = 0;
    character_draw_get_layer(self, &body, 1);
    body_motion = act_res_get_motion(body.act, self->options.action, self->options.motion);
    if (animation_attachment_count(motion) > 0 && animation_attachment_count(body_motion) > 0) {
        const struct AttachPointInfo *head_at = animation_attachment(motion, 0),
                                      *body_at = animation_attachment(body_motion, 0);

        if (head_at->attr == body_at->attr) {
            if (self->options.scaled) {
                float bx = (float)body_at->x, by;
                const struct SprClip *body_clip = motion_get_clip(body_motion, 0);

                out[0] = (int)((body_clip->zoomX != 0 ? bx / body_clip->zoomX : 0) -
                               (float)head_at->x + (float)out[0]);
                by = (float)body_at->y;
                out[1] = (int)((body_clip->zoomY != 0 ? by / body_clip->zoomY : 0) -
                               (float)animation_attachment(motion, 0)->y + (float)out[1]);
            } else {
                out[0] = body_at->x - head_at->x;
                out[1] = body_at->y - head_at->y;
            }
            self->head_dy = out[1];
            self->head_dx = out[0];
        }
    }
    return out;
}

static int *character_draw_accessory_offset(struct CharacterDraw *self, int *out, int layer,
                                            void **motion, void *act)
{
    struct SpriteResPair head;
    void *head_motion, *stepped, *cur = *motion;
    int frame = 0;

    out[0] = 0;
    out[1] = 0;
    character_draw_get_layer(self, &head, 2);
    head_motion = act_res_get_motion(head.act, self->options.action, self->options.motion);
    stepped = session_get_stepped_motion(head.act, act, self->options.action, self->options.motion,
                                       self->options.start_time, &frame);
    if (stepped && frame != self->options.motion) {
        *motion = stepped;
        cur = stepped;
    }
    if (animation_attachment_count(cur) > 0 && animation_attachment_count(head_motion) > 0) {
        const struct AttachPointInfo *at = animation_attachment(cur, 0),
                                      *head_at = animation_attachment(head_motion, 0);

        if (at->attr == head_at->attr) {
            out[0] = head_at->x - at->x;
            out[1] = head_at->y - at->y;
        }
    }
    out[0] += self->head_dx;
    if (layer_is_ignored_riding_state(
            *character_draw_layer_id(self, layer), self->options.job,
            self->options.action - self->options.action % 8, self->options.riding) == 1)
        out[1] += layer_body_attach_dy(head_motion, self->options.action, self->options.motion,
                                     self->options.job, self->options.sex, self->options.body);
    else
        out[1] += self->head_dy;
    if (layer == 4) {
        switch (self->options.accessory2) {
        case 0xb5:
        case 0xc5:
            out[1] -= 7;
            break;
        case 0xc8:
            if (!session_get_sex(&g_session1))
                out[1] -= 4;
            break;
        }
    }
    return out;
}

static void *character_draw_robe_motion(struct CharacterDraw *self, int layer)
{
    struct SpriteResPair robe, body;
    int frame = self->options.motion, block = self->options.motion;

    character_draw_get_layer(self, &robe, layer);
    if ((self->options.action >= 0 && self->options.action < 8) ||
        (unsigned)(self->options.action - 16) <= 7)
        block = 0;
    character_draw_get_layer(self, &body, 1);
    session_get_stepped_motion(body.act, robe.act, self->options.action, block,
                             self->options.start_time, &frame);
    return act_res_get_motion(robe.act, self->options.action, frame);
}

static DWORD *character_draw_palette(struct CharacterDraw *self, int layer, struct SprRes *spr)
{
    const struct String *name;

    if (layer == 1 && self->options.body_palette > 0)
        name = &self->body_palette_name;
    else if (layer == 2 && self->options.head_palette > 0)
        name = &self->head_palette_name;
    else
        return spr->palette;
    return palette_resource_colors(res_by_name(name), spr->palette);
}

static void *image_texture(struct SprImg *img, DWORD *palette, struct SpriteRegion *place)
{
    void *tex = sprite_atlas_resolve(g_renderer, img, palette, place);
    return tex;
}

static void character_draw_draw_layer(struct CharacterDraw *self, int layer,
                                      const struct SpriteResPair *pair, void *motion,
                                      DWORD *palette, int dx, int dy)
{
    struct SprClip *clip = motion_get_clip(motion, character_draw_clip_index(self, layer));
    struct SpriteRegion place = {0};
    struct SprImg *img;
    void *tex;
    int y, top, visible;

    if (clip->sprIndex == -1)
        return;
    img = spr_res_image(pair->spr, clip);
    if (!img)
        return;
    tex = clip->clipType ? img->texture : image_texture(img, palette, &place);
    if (!tex)
        return;
    y = clip->y + self->options.y + dy;
    top = y < 0 ? y : 0;
    visible = y >= 0 ? y : 0;
    if (top > visible)
        return;
    ui_canvas_draw_texture(self->wnd->canvas, self->options.x + dx + clip->x, visible, tex, place.x,
                         (int)((float)place.y - (float)top), img->width, img->height + top,
                         clip->flags, img->widthHalved + 1, img->heightHalved + 1);
}

static void character_draw_draw_layer_scaled(struct CharacterDraw *self, int layer,
                                             const struct SpriteResPair *pair, void *motion,
                                             DWORD *palette, int dx, int dy)
{
    struct SprClip *clip = motion_get_clip(motion, character_draw_clip_index(self, layer));
    struct SprImg *img;
    float scale;

    if (clip->sprIndex == -1)
        return;
    img = spr_res_image(pair->spr, clip);
    if (!img)
        return;
    scale = session_get_baby_job_scale(&g_session1, self->options.job);
    ui_window_draw_spr_clip(self->wnd, self->options.x + (int)((float)dx * scale),
                         self->options.y + (int)((float)dy * scale), img, clip, scale,
                         (float)clip->angle, ui_window_get_color(self->wnd), palette, 1.0f);
}

static void character_draw_render(struct CharacterDraw *self, BYTE flag)
{
    struct Vec layers;
    const int *it;

    character_draw_collect_layers(self, &layers);
    character_draw_load_layers(self, &layers);
    for (it = (const int *)layers.first; it != (const int *)layers.last; it++) {
        int layer = *it, offset[2], dx, dy;
        struct SpriteResPair pair;
        void *motion;
        DWORD *palette;

        character_draw_get_layer(self, &pair, layer);
        if (!pair.spr || !pair.act)
            continue;
        character_draw_doram_offset(self, offset, layer);
        dx = offset[0];
        dy = offset[1];
        motion = act_res_get_motion(pair.act, self->options.action, self->options.motion);
        if (layer == 2) {
            character_draw_head_offset(self, offset, motion);
            dx += offset[0];
            dy += offset[1];
        } else if (character_draw_is_accessory_layer(layer)) {
            character_draw_accessory_offset(self, offset, layer, &motion, pair.act);
            dx += offset[0];
            dy += offset[1];
        } else if (layer == 0 || layer == 7) {
            motion = character_draw_robe_motion(self, layer);
        }
        palette = character_draw_palette(self, layer, pair.spr);
        if (flag == 1)
            character_draw_draw_layer_scaled(self, layer, &pair, motion, palette, dx, dy);
        else
            character_draw_draw_layer(self, layer, &pair, motion, palette, dx, dy);
    }
    vec_free(&layers);
}

static void character_draw_destroy(struct CharacterDraw *self)
{
    tree_free(&self->layer_ids, NULL);
    string_free(&self->head_palette_name);
    string_free(&self->body_palette_name);
    tree_free(&self->sprites, NULL);
}

void ui_draw_character(struct UIWindow *window, const struct CharacterDrawOptions *options,
                       unsigned char flag)
{
    struct CharacterDraw draw;

    character_draw_init(&draw, window, options);
    character_draw_render(&draw, flag);
    character_draw_destroy(&draw);
}
