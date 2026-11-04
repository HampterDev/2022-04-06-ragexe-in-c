#include "actor/render_ops.h"
#include "render/renderer.h"
#include "resources/palette.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include <string.h>
#include "contents/body_layer.h"
#include "std/std.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_state.h"
#include "ui/ui_std.h"
#include "session/session_skills.h"

struct FoundSprites {
    char found;
    struct SpriteResPair pair;
};

struct MotionPick {
    char found;
    struct Motion *motion;
};

struct EventName {
    char found;
    struct String name;
};

struct ScreenOffset {
    int x, y;
};

static int action_count(const struct ActRes *act)
{
    return (int)animation_action_count(act);
}

static void *clip_image(const struct SprRes *spr, const struct SprClip *clip)
{
    return clip ? sprite_resource_image(spr, clip->clipType, clip->sprIndex) : NULL;
}

int body_layer_order_layer(struct BodyLayerDraw *self, int order, char *head_seen, char *body_moved)
{
    struct Pc *pc = self->pc;
    int layer = imf_layer_at(self->imf, order, pc->base_action, pc->action, pc->motion);
    char seen = *head_seen;

    if (!seen) {
        seen = layer == BODY_LAYER_HEAD;
        *head_seen = seen;
    }
    if (layer == 0) {
        if (seen != 1)
            return 0;
        *body_moved = seen;
        return 4;
    }
    if (layer == 2)
        return *body_moved == 1 ? 0 : 4;
    if (layer == 4)
        return 2;
    return layer;
}

char body_layer_is_visible(struct BodyLayerDraw *self, int layer)
{
    if (layer == 5 || layer == 6 || layer == 7) {
        if (!action_is_ready_or_attack(self->pc->action))
            return 0;
    }
    if ((layer == 5 || layer == 6) && self->pc->mounted == 1)
        return 0;
    if ((layer == 2 || layer == 3 || layer == 4) && !self->head_drawn)
        return 0;
    if (layer == 8 && !self->pc->helm_robe)
        return 0;
    return 1;
}

struct MotionPick *body_layer_motion_for(struct BodyLayerDraw *self, struct MotionPick *out, int layer,
                                                    struct ActRes *act)
{
    if ((layer >= 2 && layer <= 4) || layer == 8) {
        struct Pc *pc = self->pc;
        int head = actres_frame_count(pc_layer_act(pc, BODY_LAYER_HEAD), pc->action);
        int own = actres_frame_count(pc_layer_act(self->pc, layer), self->pc->action);

        if (head > 0 && own > 0 && own > head && own % head == 0) {
            out->motion = act_res_get_motion(act, self->pc->action, self->pc->motion * (own / head));
            out->found = 1;
            return out;
        }
    }
    out->found = 0;
    out->motion = 0;
    return out;
}

struct FoundSprites *body_layer_find_sprites(struct BodyLayerDraw *self, struct FoundSprites *out,
                                                        int layer)
{
    struct LayerSprite *sprite = tree_find(&self->sprites, &layer, tree_less_int);

    if (!sprite) {
        out->found = 0;
        out->pair.spr = 0;
        out->pair.act = 0;
        return out;
    }
    out->found = 1;
    out->pair.spr = sprite->pair.spr;
    out->pair.act = sprite->pair.act;
    return out;
}

struct EventName *body_layer_event_name(struct BodyLayerDraw *self, struct EventName *out, int layer,
                                                   struct ActRes *act, int event)
{
    struct String name;
    const char *text, *p, *end;

    (void)self;
    if (layer != 6 || event == -1) {
        out->found = 0;
        string_init(&out->name);
        return out;
    }
    text = act_res_get_event_name(act, event);
    string_init(&name);
    string_assign_cstr(&name, text);
    if (name.size >= 4) {
        p = string_cstr(&name);
        end = p + name.size;
        while ((p = (const char *)memchr(p, '.', end - 3 - p)) != 0) {
            if (memcmp(p, ".wav", 4) == 0) {
                string_clear(&name);
                break;
            }
            p++;
        }
    }
    out->found = 1;
    string_copy(&out->name, &name);
    string_free(&name);
    return out;
}

void *body_layer_palette(struct BodyLayerDraw *self, int layer, struct SpriteResPair pair)
{
    struct Pc *pc = self->pc;
    struct String name;

    if (layer == BODY_LAYER_BODY && pc->body_palette > 0) {
        string_copy(&name, &pc->body_pal_name);
        return palette_resource_colors(res_mgr_get_string(g_resMgr(), name), pair.spr->palette);
    }
    if (layer == BODY_LAYER_HEAD && pc->head_palette > 0) {
        string_copy(&name, &pc->head_pal_name);
        return palette_resource_colors(res_mgr_get_string(g_resMgr(), name), pair.spr->palette);
    }
    return pair.spr->palette;
}

int body_layer_flags(struct BodyLayerDraw *self, int layer, unsigned color)
{
    switch (layer) {
    case 2: case 3: case 8:
        return 1;
    case 4:
        return self->pc->accessory[2] == 0xb9 ? 0x11 : 1;
    }
    if (color >= 0xff000000u && self->opaque_override == 1) {
        if (self->opaque_override_kind == 1) {
            if (layer == 5)
                return 5;
        } else if (self->opaque_override_kind == 2) {
            if (layer == 0 || layer == 1)
                return 0x11;
        }
    }
    return 0x101;
}

void body_layer_grow_bounds(struct BodyLayerDraw *self, int layer, const struct SqQuad *quad)
{
    if ((layer == 0 || layer == 1) && self->pc->pickable == 1)
        bounds_add_quad(&self->bounds, quad);
}

#define ORCFACE "이팩트\\orcface"

void body_layer_load_sprites(struct BodyLayerDraw *self)
{
    int layer;

    for (layer = 0; layer < BODY_LAYER_COUNT; layer++) {
        struct SprRes *spr = pc_layer_spr(self->pc, layer);
        struct ActRes *act = pc_layer_act(self->pc, layer);
        struct LayerSprite *node;

        if (!spr || !act)
            continue;
        if (layer == BODY_LAYER_HEAD && effect_state_bit11(self->pc->effect_state) == 1) {
            spr = (struct SprRes *)res_mgr_get(g_resMgr(), ORCFACE ".spr");
            act = (struct ActRes *)res_mgr_get(g_resMgr(), ORCFACE ".act");
        }
        node = body_layer_sprite_slot(self, layer);
        node->pair.spr = spr;
        node->pair.act = act;
    }

    for (layer = 2; layer <= 4; layer++) {
        struct LayerSprite *worn = tree_find(&self->sprites, &layer, tree_less_int);
        struct LayerSprite *head;
        struct SprRes *spr;
        struct ActRes *act;

        if (!worn)
            continue;
        if (session_is_head_replacing_view(&g_session1, *body_layer_view_slot(self, layer)) != 1)
            continue;
        head = body_layer_sprite_slot(self, BODY_LAYER_HEAD);
        spr = head->pair.spr;
        act = head->pair.act;
        head->pair.spr = worn->pair.spr;
        head->pair.act = worn->pair.act;
        worn->pair.spr = spr;
        worn->pair.act = act;
        tree_erase(&self->sprites, TREE_NODE_OF(worn), NULL);
        return;
    }
}

void body_layer_reset(struct BodyLayerDraw *self)
{
    self->layers_drawn = 0;
    self->offset_x = 0;
    self->offset_y = 0;
    self->head_drawn = 0;
    self->any_drawn = 0;
    *body_layer_view_slot(self, 2) = self->pc->accessory[0];
    *body_layer_view_slot(self, 3) = self->pc->accessory[1];
    *body_layer_view_slot(self, 4) = self->pc->accessory[2];
    *body_layer_view_slot(self, 8) = self->pc->robe;
    body_layer_load_sprites(self);
}

static void queue_layer(struct BodyLayerDraw *self, int layer, const struct SqQuad *quad, void *palette,
                        int flags)
{
    struct Pc *pc = self->pc;
    int dir = pc->action - pc->base_action;
    int priority, view;
    struct QueuedLayer entry;

    if (*body_layer_view_slot(self, layer) <= 0)
        return;
    priority = -1;
    view = *body_layer_view_slot(self, layer);
    client_lua_call(g_session1.luaState, "GetLayerPriority", "ddd>d", dir, layer, view, &priority);
    if (priority == -1)
        return;
    entry.priority = priority;
    entry.quad = *quad;
    entry.palette = palette;
    entry.flags = flags;
    tree_add(&self->queued, &entry, sizeof entry, tree_less_int, NULL);
}

void body_layer_flush(struct BodyLayerDraw *self)
{
    struct Vec depth_a = { 0, 0, 0 }, depth_b = { 0, 0, 0 };
    struct TreeNode *it;
    float max_a = 0.0f, max_b = 0.0f, step = 0.0f;
    const float *f;

    for (it = tree_first(&self->queued); it != self->queued.head; it = tree_next(it)) {
        struct QueuedLayer entry = *TREE_VALUE(it, struct QueuedLayer);

        vec_push(&depth_a, &entry.quad.depth_a, sizeof(float));
        vec_push(&depth_b, &entry.quad.depth_b, sizeof(float));
    }
    if (depth_a.first != depth_a.last) {
        const float *best = (const float *)depth_a.first;
        for (f = best + 1; f != (const float *)depth_a.last; f++)
            if (*f > *best)
                best = f;
        max_a = *best;
    }
    if (depth_b.first != depth_b.last) {
        const float *best = (const float *)depth_b.first;
        for (f = best + 1; f != (const float *)depth_b.last; f++)
            if (*f > *best)
                best = f;
        max_b = *best;
    }

    for (it = tree_first(&self->queued); it != self->queued.head; it = tree_next(it)) {
        struct QueuedLayer *q = TREE_VALUE(it, struct QueuedLayer);

        q->quad.depth_a = step + max_a;
        q->quad.depth_b = step + max_b;
        if (session_is_screen_flipped(&g_session1) == 1)
            render_object_submit_flipped_quad((struct RenderObject *)self->pc, &q->quad, q->palette, q->flags, self->screen_x, self->screen_y);
        else
            QUAD_BUILDER(self->pc, &q->quad, q->palette, q->flags, 0.0f, 0.0f);
        render_object_draw_flashes((struct RenderObject *)self->pc, q->quad, q->palette);
        step += 1.0e-6f;
    }
    vec_free(&depth_b);
    vec_free(&depth_a);
}

struct LayerClip *body_layer_clip_offset(struct BodyLayerDraw *self, struct LayerClip *out, int layer,
                                                    struct SprClip *clip, int clip_index, struct SpriteResPair pair,
                                                    struct Motion *motion, void *image, int pick_action,
                                                    int pick_motion, char use_pick)
{
    struct Pc *pc = self->pc;
    int dx = 0, dy = 0;

    if (layer == BODY_LAYER_HEAD) {
        struct Motion *body = act_res_get_motion(pc_layer_act(pc, BODY_LAYER_BODY), pc->action,
                                                 pc->motion);
        if (motion->attachment_count > 0 && body->attachment_count > 0) {
            const struct AttachPointInfo *a = (const struct AttachPointInfo *)body->attachments;
            const struct AttachPointInfo *b = (const struct AttachPointInfo *)motion->attachments;
            if (b->attr == a->attr) {
                dx = a->x - b->x;
                dy = a->y - b->y;
                self->offset_x = dx;
                self->offset_y = dy;
            }
        }
    } else if ((layer >= 2 && layer <= 4) || layer == 8) {
        struct Motion *head, *step;
        int step_index = 0, ex = 0, ey = 0, x, view;
        char mounted;
        int base_action;

        step = session_get_stepped_motion(pc_layer_act(pc, BODY_LAYER_HEAD), pc_layer_act(pc, layer),
                          pc->action, pc->motion, pc->start_tick, &step_index);
        if (step && step_index != self->pc->motion) {
            motion = step;
            clip = motion_get_clip(step, clip_index);
            image = clip_image(pair.spr, clip);
        }
        pc = self->pc;
        if (use_pick == 1)
            head = act_res_get_motion(pc_layer_act(pc, BODY_LAYER_HEAD), pick_action, pick_motion);
        else
            head = act_res_get_motion(pc_layer_act(pc, BODY_LAYER_HEAD), pc->action, pc->motion);
        if (motion->attachment_count > 0 && head->attachment_count > 0) {
            const struct AttachPointInfo *a = (const struct AttachPointInfo *)head->attachments;
            const struct AttachPointInfo *b = (const struct AttachPointInfo *)motion->attachments;
            if (b->attr == a->attr) {
                ex = a->x - b->x;
                ey = a->y - b->y;
            }
        }
        pc = self->pc;
        x = ex + self->offset_x;
        mounted = pc->mounted;
        base_action = pc->base_action;
        view = *body_layer_view_slot(self, layer);
        if (layer_is_ignored_riding_state(view, game_actor_get_job((struct GameActor *)pc), base_action, mounted) == 1) {
            pc = self->pc;
            ey += layer_body_attach_dy(head, pc->action, pc->motion, game_actor_get_job((struct GameActor *)pc), pc->sex,
                             pc->body_style);
        } else {
            ey += self->offset_y;
        }
        dx = clip->zoomX != 1.0f ? (int)((float)x / clip->zoomX) : x;
        dy = clip->zoomY != 1.0f ? (int)((float)ey / clip->zoomY) : ey;
    }
    out->image = image;
    out->clip = clip;
    out->dy = dy;
    out->dx = dx;
    return out;
}

void body_layer_draw_layer(struct BodyLayerDraw *self, int order, int layer, struct Motion *motion,
                                      struct SpriteResPair pair, int pick_action, int pick_motion, char use_pick)
{
    int drawn_clip = layer_clip_index(layer);
    int i;

    for (i = 0; i < motion->clip_count; i++) {
        struct SprClip *clip;
        struct LayerClip lc;
        struct SqQuad quad;
        int rect[4];
        unsigned colour;
        float qa, qb;
        void *palette;
        int x, y, flags;
        struct Pc *pc;

        if (layer == 7)
            drawn_clip = i;
        else if (drawn_clip != i)
            continue;
        clip = motion_get_clip(motion, drawn_clip);
        if (clip->sprIndex == -1)
            continue;
        if (layer == BODY_LAYER_HEAD)
            self->head_drawn = 1;

        x = 0;
        y = 0;
        body_layer_clip_offset(self, &lc, layer, clip, drawn_clip, pair, motion, clip_image(pair.spr, clip),
                               pick_action, pick_motion, use_pick);
        if (!lc.image)
            continue;
        x += lc.dx;
        y += lc.dy;
        render_object_swing_point((struct RenderObject *)self->pc, layer, &x, &y, &self->offset_x, &self->offset_y);

        qa = 0.0f;
        qb = 0.0f;
        pc = self->pc;
        if (!render_object_clip_to_quad(
                (struct RenderObject *)pc, order, self->layers_drawn, self->screen_scale, lc.clip, self->screen_x, self->screen_y,
                self->pos, self->screen_shift, &qa, &qb, lc.image, &quad, rect, &colour, x, y, 0))
            continue;

        pc = self->pc;
        if (pc->half_alpha == 1) {
            unsigned alpha = quad.color >> 24;
            quad.color = ((unsigned)(unsigned char)(int)((float)alpha * 0.27450982f) << 24) | (quad.color & 0xffffff);
        }
        if (layer == BODY_LAYER_HEAD) {
            pc->head_clip[3] = (float)(lc.clip->y + self->offset_y);
            pc->head_clip[2] = (float)(lc.clip->x + self->offset_x);
            pc_layer_depth_bias_by_view(self->pc, layer, &quad, qa, qb);
        } else if ((layer >= 2 && layer <= 4) || layer == 8) {
            pc_layer_depth_bias(layer, &quad, qa, qb);
        } else {
            pc_layer_depth_bias_by_view(pc, layer, &quad, qa, qb);
        }
        quad.image = lc.image;

        palette = body_layer_palette(self, layer, pair);
        {
            int msg_arg = 0xa6;
            render_object_send(self->pc, NULL, 0x78, (intptr_t)&msg_arg, 0, 0, 0);
        }
        flags = body_layer_flags(self, layer, quad.color);
        body_layer_grow_bounds(self, layer, &quad);

        self->layers_drawn++;
        self->any_drawn = 1;
        pc_draw_layer_overlays(self->pc, &quad, lc.clip, layer, rect, self->screen_shift, self->screen_z, qa, qb,
                   self->screen_x, self->screen_y, palette, lc.image, &flags, x, y);

        if ((layer >= 2 && layer <= 4) || layer == 8) {
            queue_layer(self, layer, &quad, palette, flags);
            continue;
        }
        if (session_is_screen_flipped(&g_session1) == 1)
            render_object_submit_flipped_quad((struct RenderObject *)self->pc, &quad, palette, flags, self->screen_x, self->screen_y);
        else
            QUAD_BUILDER(self->pc, &quad, palette, flags, 0.0f, 0.0f);
        render_object_draw_flashes((struct RenderObject *)self->pc, quad, palette);
        if (layer == BODY_LAYER_BODY) {
            self->pc->body_flags = flags;
            self->pc->body_clip_x = lc.clip->x;
            self->pc->body_clip_y = lc.clip->y;
            pc_render_robe(self->pc, 1, &self->layers_drawn, self->pos, self->screen_x,
                       self->screen_y, self->screen_scale, self->screen_shift);
        }
    }
}

static void render_layer(struct BodyLayerDraw *self, int order, int layer)
{
    struct FoundSprites found;
    struct MotionPick pick;
    struct EventName event;
    struct ActRes *act;
    struct Motion *motion;
    struct Pc *pc;
    int pick_action = 0, pick_motion = 0;
    char use_pick = 0;

    body_layer_find_sprites(self, &found, layer);
    if (!found.found)
        return;
    act = found.pair.act;
    if (!(self->pc->action < action_count(act)))
        return;
    pc = self->pc;
    if (!(pc->motion < actres_frame_count(act, pc->action)))
        return;
    motion = pc_get_layer_motion(pc, layer, act, &pick_action, &pick_motion, &use_pick);
    body_layer_event_name(self, &event, layer, act, motion->eventId);
    if (event.found == 1) {
        struct String text;

        self->pc->wave_pending = 1;
        string_copy(&text, &event.name);
        pc = self->pc;
        snprintf((char *)pc->wave_name, sizeof pc->wave_name, "%s",
                    text.size ? string_cstr(&text) : (const char *)pc->wave_name);
        string_free(&text);
    }
    body_layer_motion_for(self, &pick, layer, act);
    body_layer_draw_layer(self, order, layer, pick.found == 1 ? pick.motion : motion, found.pair, pick_action,
                          pick_motion, use_pick);
    string_free(&event.name);
}

void body_layer_render(struct BodyLayerDraw *self, const float *matrix, int opaque_override,
                                  char opaque_override_kind)
{
    const Vec3 *p;
    struct Pc *pc = self->pc;
    struct String imf_name;
    char head_seen = 0, body_moved = 0;
    float depth;
    int order;

    if (!pc)
        return;
    memcpy(self->matrix, matrix, sizeof self->matrix);
    self->opaque_override = opaque_override;
    self->opaque_override_kind = opaque_override_kind;
    body_layer_reset(self);

    p = &self->pc->pos;
    self->pos[0] = matrix[3] * p->y + matrix[0] * p->x + matrix[6] * p->z + matrix[9];
    self->pos[1] = matrix[1] * p->x + matrix[4] * p->y + matrix[7] * p->z + matrix[10];
    self->pos[2] = matrix[2] * p->x + matrix[5] * p->y + matrix[8] * p->z + matrix[11];
    renderer_project_point(g_renderer, &self->pc->pos, matrix, &self->screen_x, &self->screen_y, &self->screen_z);
    self->screen_scale = (float)renderer_screen_scale(g_renderer, self->screen_z);
    self->pc->scale = self->screen_scale / g_spriteScaleDivisor;
    render_object_apply_pulse_scale((struct RenderObject *)self->pc, &self->screen_scale);
    self->screen_scale = (float)(session_get_baby_job_scale(&g_session1, game_actor_get_job((struct GameActor *)self->pc)) * self->screen_scale);
    pc = self->pc;
    depth = (float)pc->shift_x * pc->zoom;
    depth = (float)(renderer_scale_x(g_renderer) * depth);
    self->screen_shift = pc->scale * depth;

    if (game_actor_get_transform_job((struct GameActor *)pc) > 0) {
        pc_render_body_alone(self->pc, 0, self->pos, self->screen_x, self->screen_y, self->screen_scale,
                   self->screen_shift, self->screen_z);
        return;
    }
    pc_render_robe(self->pc, 0, &self->layers_drawn, self->pos, self->screen_x, self->screen_y,
               self->screen_scale, self->screen_shift);
    string_copy(&imf_name, &self->pc->imf_name);
    self->imf = res_mgr_get_string(g_resMgr(), imf_name);
    bounds_set(&self->bounds, (float)g_renderer->width, (float)g_renderer->height, 0.0f, 0.0f, self->screen_z);

    for (order = 8; order >= 0; order--) {
        int layer = body_layer_order_layer(self, order, &head_seen, &body_moved);

        if ((unsigned)layer > 8 || !body_layer_is_visible(self, layer))
            continue;
        render_layer(self, order, layer);
    }

    body_layer_flush(self);
    pc = self->pc;
    if (self->any_drawn == 1)
        game_actor_set_render_info(
            (struct RenderObject *)pc, &self->bounds, self->screen_x, self->screen_y);
    else
        render_object_set_screen_position(
            (struct RenderObject *)pc, self->screen_x, self->screen_y);
    self->pc->drawn = 1;
}

void body_layer_destroy(struct BodyLayerDraw *self)
{
    tree_free(&self->queued, NULL);
    tree_free(&self->views, NULL);
    tree_free(&self->sprites, NULL);
}
