#include "actor/render_ops.h"
#include "render/renderer.h"
#include "render/math3d.h"
#include "resources/palette.h"
#include "resources/sprite.h"
#include "contents/client_lua_calls.h"
#include "game/own_states.h"
#include "session/state.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "contents/body_layer.h"

#include "actor/sprite_render.h"
#include "std/std.h"
#include "world/world_imports.h"
#include "session/session_jobs.h"
#include "session/session_lua.h"
#include "session/session_names.h"
#include "session/session_state.h"
#include "ui/ui_std.h"
#include "game/camera.h"

void pc_layer_depth_bias(int layer, struct SqQuad *quad,
                           float a, float b)
{
    switch (layer) {
    case 2:
        if (320.0f > g_viewDistance) {
            quad->depth_a = a + 2.7e-5f;
            quad->depth_b = b + 2.7e-5f;
        } else {
            quad->depth_a = a;
            quad->depth_b = b;
        }
        break;
    case 3:
        if (320.0f > g_viewDistance) {
            quad->depth_a = a + 4.0e-5f;
            quad->depth_b = 0.0001f + b;
        } else {
            quad->depth_a = a;
            quad->depth_b = 4.5e-5f + b;
        }
        break;
    case 4:
        if (320.0f > g_viewDistance) {
            b += 3.0e-5f;
            a += 3.0e-5f;
        }
        quad->depth_a = a;
        quad->depth_b = b;
        break;
    }
}

static int job_has_layer_motions(int job)
{
    switch (job) {
    case 0xfce: case 0xfcf: case 0xfd0:
    case 0x1081: case 0x1082: case 0x108e:
        return 1;
    }
    return 0;
}

struct Motion *pc_get_layer_motion(struct Pc *pc, int layer, struct ActRes *act,
                                      int *action_out, int *motion_out, char *use_out)
{
    struct Motion *motion = act_res_get_motion(act, pc->action, pc->motion);
    int action, m;

    if (pc->mounted != 0)
        return motion;
    if ((unsigned)(layer - 1) > 3)
        return motion;
    if (pc->action < 0x58)
        return motion;
    if (!job_has_layer_motions(game_actor_get_job((struct GameActor *)pc)))
        return motion;

    if ((unsigned)(pc->action - 0x58) <= 7 && (unsigned)(pc->motion - 3) <= 2) {
        m = pc->motion;
        if (pc->sex == 0) {
            if (m != 3) {
                *use_out = 1;
                action = pc->action;
                if (action <= 0x59) {
                    if (pc->motion == 4)
                        *action_out = 6;
                    else if (pc->motion == 5)
                        *action_out = 5;
                } else if (action <= 0x5b) {
                    if (pc->motion == 4)
                        *action_out = 6;
                    else if (pc->motion == 5)
                        *action_out = 7;
                } else if (action <= 0x5d) {
                    if (pc->motion == 4)
                        *action_out = 2;
                    else if (pc->motion == 5)
                        *action_out = 1;
                } else if (action <= 0x5f) {
                    if (pc->motion == 4)
                        *action_out = 2;
                    else if (pc->motion == 5)
                        *action_out = 3;
                }
            }
        } else if (pc->sex == 1) {
            if (m != 5) {
                *use_out = 1;
                action = pc->action;
                if (action <= 0x59) {
                    if (pc->motion == 3)
                        *action_out = 6;
                    else if (pc->motion == 4)
                        *action_out = 5;
                } else if (action <= 0x5b) {
                    if (pc->motion == 3)
                        *action_out = 6;
                    else if (pc->motion == 4)
                        *action_out = 7;
                } else if (action <= 0x5d) {
                    if (pc->motion == 3)
                        *action_out = 2;
                    else if (pc->motion == 4)
                        *action_out = 1;
                } else if (action <= 0x5f) {
                    if (pc->motion == 3)
                        *action_out = 2;
                    else if (pc->motion == 4)
                        *action_out = 3;
                }
            }
        }
    }
    if (pc->action >= 0x60 && pc->motion == 1) {
        *use_out = 1;
        action = pc->action;
        if ((unsigned)(action - 0x60) <= 1)
            *action_out = 0;
        else
            *action_out = (unsigned)(action - 0x62) > 1 ? 2 : 6;
    }
    if (*use_out == 1)
        return act_res_get_motion(act, *action_out, *motion_out);
    return motion;
}

#define CAMERA_CLOSE() (320.0f > g_viewDistance)

void pc_layer_depth_bias_by_view(struct Pc *pc, int layer, struct SqQuad *quad,
                           float a, float b)
{
    unsigned char upping;
    double near_off, far_off;
    float da = a, db = b;
    int view;

    switch (layer) {
    case 1:
        if (CAMERA_CLOSE()) {
            da = a + 6.0e-6f;
            db = b + 6.0e-6f;
        }
        break;
    case 2:
        view = pc->accessory[0];
        if (view == 0x4d3) {
            if (CAMERA_CLOSE()) {
                da = a + 0.0008f;
                db = b + 0.0008f;
            } else {
                da = a + 0.00013f;
                db = b + 0.00013f;
            }
        } else if (view == 0x411 || view == 0x54e || view == 0x5c6 || view == 0x5d0 || view == 0x5db) {
            if (CAMERA_CLOSE()) {
                da = a + 0.0008f;
                db = b + 0.0008f;
            } else {
                da = a + 0.00019f;
                db = b + 0.00019f;
            }
        } else if (CAMERA_CLOSE()) {
            da = a + 2.7e-5f;
            db = b + 2.7e-5f;
        }
        break;
    case 3:
        if (CAMERA_CLOSE()) {
            da = a + 4.0e-5f;
            db = b + 0.0001f;
        } else {
            db = b + 4.5e-5f;
        }
        break;
    case 4:
        if (pc->accessory[1] == 0x5cd) {
            if (CAMERA_CLOSE()) {
                da = a + 0.00035f;
                db = b + 0.0006f;
            } else {
                da = a + 1.0e-5f;
                db = b + 0.00015f;
            }
            break;
        }
        switch (pc->accessory[2]) {
        case 0x526:
            if (CAMERA_CLOSE()) {
                da = a + 0.00025f;
                db = b + 0.0001f;
            } else {
                da = a + 5.0e-5f;
                db = b + 5.0e-5f;
            }
            break;
        case 0x54a:
            if (CAMERA_CLOSE()) {
                da = a + 9.0e-5f;
                db = b - 5.0e-5f;
            }
            break;
        case 0x553:
            if (CAMERA_CLOSE()) {
                da = a + 2.7e-5f + 0.0004f;
                db = b + 2.7e-5f + 0.0007f;
            } else {
                da = a + 7.0e-5f;
                db = b + 0.00017f;
            }
            break;
        case 0x62e:
            if (CAMERA_CLOSE()) {
                da = a + 0.0002f;
                db = b + 0.0002f;
            } else {
                da = a + 4.0e-5f;
                db = b + 6.0e-5f;
            }
            break;
        default:
            if (CAMERA_CLOSE()) {
                da = a + 3.0e-5f;
                db = b + 3.0e-5f;
            }
            break;
        }
        break;
    case 5:
        da = a + 6.0e-5f;
        db = b + 6.0e-5f;
        break;
    }
    quad->depth_a = da;
    quad->depth_b = db;

    upping = 0;
    near_off = 0.0;
    far_off = 0.0;

    client_lua_call(g_session1.luaState, "OffsetItemPos_GetUppingBottomItem", "d>bll", pc->accessory[0], &upping, &near_off, &far_off);
    if (layer == 2 && upping == 1) {
        if (near_off == 0.0)
            near_off = 0.00025;
        if (far_off == 0.0)
            far_off = 0.0001;
        if (CAMERA_CLOSE()) {
            quad->depth_a = (float)((double)a + near_off);
            quad->depth_b = (float)((double)b + near_off);
        } else {
            quad->depth_a = (float)((double)a + far_off);
            quad->depth_b = (float)((double)b + far_off);
        }
    }
}

struct SprRes *pc_layer_spr(struct Pc *pc, int layer)
{
    return ((struct SprRes **)pc->layer_sprs.first)[layer];
}

struct ActRes *pc_layer_act(struct Pc *pc, int layer)
{
    if ((unsigned)layer >= VEC_COUNT(&pc->layer_acts, struct ActRes *))
        return NULL;
    return ((struct ActRes **)pc->layer_acts.first)[layer];
}

const char *pc_layer_spr_name(struct Pc *pc, int layer)
{
    void *res = ((void **)pc->layer_sprs.first)[layer];

    return res ? res_get_name(res) : "";
}

const char *pc_layer_act_name(struct Pc *pc, int layer)
{
    void *res = ((void **)pc->layer_acts.first)[layer];

    return res ? res_get_name(res) : "";
}

int layer_clip_index(int layer)
{
    return layer == 1;
}

char action_is_ready_or_attack(int action)
{
    return action >= 0x20 && (unsigned)(action - 0x28) > 0x27 && action <= 0x5f;
}

static struct Vec *layer_names(struct Vec *out, const struct Vec *layers)
{
    struct String *names;
    const char *name;
    int i;

    vec_init(out);
    names = vec_add(out, 9, sizeof *names);
    for (i = 0; i < 9; i++) {
        void *res = ((void **)layers->first)[i];

        if (res) {
            name = res_get_name(res);
            string_assign_cstr(&names[i], name);
        } else {
            string_clear(&names[i]);
        }
    }
    return out;
}

struct Vec *pc_layer_spr_names(struct Pc *pc, struct Vec *out)
{
    return layer_names(out, &pc->layer_sprs);
}

struct Vec *pc_layer_act_names(struct Pc *pc, struct Vec *out)
{
    return layer_names(out, &pc->layer_acts);
}

char layer_is_ignored_riding_state(int view, int job, int base_action, char mounted)
{
    char ignored;

    if (!mounted && job == session_get_job_without_mount(&g_session1, job))
        return 0;
    if (base_action != 0 && base_action != 8 && base_action != 16 && base_action != 64)
        return 0;
    ignored = 0;

    client_lua_call_table(g_session1.luaState, "TB_Layer_Priority", "IsIgnoredRidingState", "d>b", view, &ignored);
    return ignored;
}

int layer_body_attach_dy(struct Motion *head_motion, int action, int motion, int job, int sex, int body_style)
{
    struct String path, arg;
    struct ActRes *act;
    struct Motion *body;
    const struct AttachPointInfo *body_at, *head_at;
    int dy;

    if (!head_motion || !motion_get_clip(head_motion, 1))
        return 0;
    session_get_body_spr_name_variant0(&path, session_get_job_without_mount(&g_session1, job), sex, body_style, 0);
    string_copy(&arg, &path);
    act = (struct ActRes *)res_mgr_get_string(g_resMgr(), arg);
    dy = 0;
    if (act) {
        body = act_res_get_motion(act, action, motion);
        if (body && body->attachment_count
            && head_motion->attachment_count) {
            body_at = body->attachments;
            head_at = head_motion->attachments;
            if (body_at->attr == head_at->attr)
                dy = body_at->y - head_at->y;
        }
    }
    string_free(&path);
    return dy;
}

char pc_draw_on_top(struct Pc *pc)
{
    char top = 1;
    int second, job;

    if (g_session1.new_layer_table_loaded) {
        job = game_actor_get_job((struct GameActor *)pc);
        if (pc->mounted)
            job = session_get_mounted_job(job);
        second = 1;

        client_lua_call(g_session1.luaState, "_New_DrawOnTop", "ddddd>bd", pc->robe, pc->sex, job, pc->action,
                pc->motion, &top, &second);
    } else {
        client_lua_call(g_session1.luaState, "DrawOnTop", "ddddd>b", pc->robe, pc->sex, pc->job, pc->action,
                pc->motion, &top);
    }
    return top;
}

void pc_place_head(struct Pc *pc)
{
    struct String path, arg;
    struct ActRes *head_act, *body_act;
    struct Motion *head, *body;
    const struct SprClip *clip;
    const struct AttachPointInfo *head_at, *body_at;
    float *out = pc->head_clip;

    if (VEC_COUNT(&pc->layer_acts, struct ActRes *) <= 1)
        return;
    head_act = ((struct ActRes **)pc->layer_acts.first)[1];
    if (!head_act)
        return;
    head = act_res_get_motion(head_act, 0, 0);
    if (!head)
        return;
    clip = motion_get_clip(head, 1);
    if (!clip)
        return;
    session_get_body_spr_name_variant0(&path, session_get_job_without_mount(&g_session1, game_actor_get_job((struct GameActor *)pc)), pc->sex,
               pc->body_style, 0);
    string_copy(&arg, &path);
    body_act = (struct ActRes *)res_mgr_get_string(g_resMgr(), arg);
    if (body_act) {
        body = act_res_get_motion(body_act, 0, 0);
        if (body && body->attachment_count && head->attachment_count) {
            head_at = head->attachments;
            body_at = body->attachments;
            out[0] = (float)(clip->x - head_at->x + body_at->x);
            out[1] = (float)(body_at->y - head_at->y + clip->y);
        }
    }
    string_free(&path);
}

void pc_render_body_alone(struct Pc *pc, int layer,
                           const float *pos, float x, float y, float screen_scale, float screen_shift, float z)
{
    struct ScreenBounds bounds;
    struct SqQuad quad;
    struct SprRes *spr;
    struct ActRes *act;
    struct Motion *motion;
    struct SprClip *clip;
    void *image, *palette;
    float depth_a, depth_b;
    int rect[4], flags;
    unsigned colour;
    int motion_index, i;
    int kind = pc->state;

    if (game_actor_get_transform_job((struct GameActor *)pc) < 0)
        return;
    bounds.left = (float)g_renderer->width;
    bounds.top = (float)g_renderer->height;
    bounds.right = 0.0f;
    bounds.bottom = 0.0f;
    bounds.z = z;
    spr = ((struct SprRes **)pc->layer_sprs.first)[0];
    act = VEC_COUNT(&pc->layer_acts, struct ActRes *) ? ((struct ActRes **)pc->layer_acts.first)[0] : NULL;
    if (kind == 3 || pc->state_locked != 0) {
        pc->action = game_actor_transform_action_of(kind);
        motion_index = pc->motion = actres_frame_count(act, pc->action) - 1;
    } else {
        motion_index = pc->motion;
    }
    motion = act_res_get_motion(act, pc->action, motion_index);
    if (motion->clip_count < 1)
        motion = act_res_get_motion(act, 0, pc->motion);

    for (i = 0; i < motion->clip_count; i++) {
        clip = motion_get_clip(motion, i);
        if (clip->sprIndex == -1)
            continue;
        image = sprite_resource_image(spr, clip->clipType, clip->sprIndex);
        if (!image) continue;
        if (game_actor_get_transform_job((struct GameActor *)pc) == 0x6ac) {
            render_object_clip_to_quad((struct RenderObject *)pc, i, i, screen_scale, clip, x, y, pos, screen_shift, &depth_a, &depth_b, image,
                                        &quad, rect, &colour, 0, 0, 0);
            if (pc->state == 0 && i == 1) {
                quad.depth_a = depth_a + 8.0e-6f;
                quad.depth_b = depth_b + 8.0e-5f;
            }
        } else {
            render_object_clip_to_quad((struct RenderObject *)pc, layer, layer, screen_scale, clip, x, y, pos, screen_shift, &depth_a,
                                        &depth_b, image, &quad, rect, &colour, 0, 0, 0);
            if (320.0f >= g_viewDistance) {
                quad.depth_a = depth_a - 2.5e-6f;
                quad.depth_b = depth_b - 2.5e-6f;
            }
        }
        game_actor_raise_body_quarter_height((struct GameActor *)pc, (rect[3] - rect[1]) / 4);
        if (pc->half_alpha == 1)
            quad.color = (quad.color & 0xffffff) | 0x46000000;
        quad.image = (struct SprImg *)image;
        if (pc->pickable)
            bounds_add_quad(&bounds, &quad);
        flags = pc->body_flags;
        palette = spr->palette;
        pc_draw_layer_overlays(pc, &quad, clip, 0, rect, screen_shift, depth_a, depth_a, depth_b, x, y, palette, image,
                   &flags, 0, 0);
        if (session_is_screen_flipped(&g_session1))
            render_object_submit_flipped_quad((struct RenderObject *)pc, &quad, palette, 0x101, x, y);
        else
            QUAD_BUILDER(pc, &quad, palette, 0x101, 0.0f, 0.0f);
        render_object_draw_flashes((struct RenderObject *)pc, quad, palette);
    }
    game_actor_set_render_info((struct RenderObject *)pc, &bounds, x, y);
    pc->drawn = 1;
}

void pc_render_robe(struct Pc *pc, int on_top_pass,
                           int *layers_drawn, const float *pos, float x, float y, float screen_scale, float screen_shift)
{
    struct String spr_name, act_name;
    struct SqQuad quad;
    struct SprRes *spr;
    struct ActRes *act;
    struct Motion *motion;
    struct SprClip *clip;
    void *image, *palette;
    float depth_a, depth_b;
    int rect[4], flags;
    unsigned colour;
    int job, motion_index, step_motion, i;
    char mode = (char)on_top_pass;

    if (pc->robe <= 0 || pc->helm_robe == 1)
        return;
    if (pc->action >= 0x40 && pc->action <= 0x47)
        return;
    if (pc->body_flags == -1 && pc->body_clip_x == -1 && pc->body_clip_y == -1)
        return;
    if (pc_draw_on_top(pc) != mode) {
        ++*layers_drawn;
        return;
    }
    session_get_robe_spr_name(&spr_name, game_actor_get_job((struct GameActor *)pc), pc->sex,
                           pc->robe, pc->mounted);
    session_get_robe_act_name(&act_name, game_actor_get_job((struct GameActor *)pc), pc->sex,
                           pc->robe, pc->mounted);
    if (!res_mgr_is_available(g_resMgr(), string_cstr(&spr_name))
        || !res_mgr_is_available(g_resMgr(), string_cstr(&act_name))) {
        string_free(&act_name);
        string_free(&spr_name);
        return;
    }
    spr = (struct SprRes *)res_mgr_get(g_resMgr(), string_cstr(&spr_name));
    act = (struct ActRes *)res_mgr_get(g_resMgr(), string_cstr(&act_name));

    step_motion = pc->motion;
    motion_index = pc->motion;
    if ((pc->action >= 0 && pc->action < 8) || (unsigned)(pc->action - 0x10) <= 6)
        motion_index = 0;
    session_get_stepped_motion(pc_layer_act(pc, BODY_LAYER_HEAD), act, pc->action,
                             motion_index, pc->start_tick, &step_motion);
    motion = act_res_get_motion(act, pc->action, step_motion);

    for (i = 0; i < motion->clip_count; i++) {
        clip = motion_get_clip(motion, i);
        if (clip->sprIndex == -1)
            continue;
        clip->color[3] = (unsigned char)session_get_transparent_item_alpha(&g_session1, pc->robe);
        image = sprite_resource_image(spr, clip->clipType, clip->sprIndex);
        if (!image) continue;
        render_object_clip_to_quad((struct RenderObject *)pc, *layers_drawn, *layers_drawn, screen_scale, clip, x, y, pos, screen_shift, &depth_a,
                                    &depth_b, image, &quad, rect, &colour, 0, 0, mode ^ 1);
        if (!mode && game_camera_pitch() >= -40.0f) {
            quad.depth_a = depth_a - 6.5e-6f;
            quad.depth_b = depth_b - 6.5e-6f;
        }
        if ((unsigned)(pc->action - 0x3a) <= 3) {
            quad.depth_a += 6.5e-6f;
            quad.depth_b += 6.5e-6f;
        }
        if (mode && session_is_top_layer_robe(&g_session1, pc->robe)) {
            if (320.0f > g_viewDistance) {
                quad.depth_a = depth_a + 0.000251f;
                quad.depth_b = depth_b + 0.000251f;
            } else {
                quad.depth_a = depth_a + 0.00011f;
                quad.depth_b = depth_b + 0.00011f;
            }
        }
        if (pc->half_alpha == 1)
            quad.color = ((unsigned)(unsigned char)(int)((float)(quad.color >> 24) * 0.27450982f) << 24)
                         | (quad.color & 0xffffff);
        quad.image = image;
        flags = pc->body_flags;
        palette = spr->palette;
        pc_draw_layer_overlays(pc, &quad, clip, 0, rect, screen_shift, depth_a, depth_a, depth_b, x, y, palette, image,
                   &flags, 0, 0);
        if (session_is_screen_flipped(&g_session1))
            render_object_submit_flipped_quad((struct RenderObject *)pc, &quad, palette, flags, x, y);
        else
            QUAD_BUILDER(pc, &quad, palette, flags, 0.0f, 0.0f);
    }
    string_free(&act_name);
    string_free(&spr_name);
    ++*layers_drawn;
}

void pc_render_body_layers(struct Pc *pc, const float *matrix, int mode, char flag)
{
    struct BodyLayerDraw draw;

    draw.pc = pc;
    draw.pos[0] = 0.0f;
    draw.pos[1] = 0.0f;
    draw.pos[2] = 0.0f;
    tree_init(&draw.sprites);
    tree_init(&draw.views);
    tree_init(&draw.queued);
    body_layer_render(&draw, matrix, mode, flag);
    body_layer_destroy(&draw);
}

void render_object_render_shadow(struct RenderObject *obj, const float *matrix, float zoom)
{
    struct C3dAttr *attr = game_current_map_attr();
    const float *m = matrix;
    struct SqQuad quad;
    struct SprRes *spr;
    struct SprClip *clip;
    struct SprImg *image;
    void *palette;
    float v[3], x, y, rhw, s, k, depth, top, bottom, left, right, top_d, bottom_d, shift;
    double t;

    if (obj->monster_job == 0x235 || !obj->has_shadow)
        return;
    if (obj->shadow_scale == 0.0f || zoom == 0.0f || !attr)
        return;
    v[0] = obj->pos.x;
    v[1] = c3d_attr_get_height(attr, obj->pos.x, obj->pos.z);
    v[2] = obj->pos.z;
    depth = m[2] * v[0] + m[5] * v[1] + m[8] * v[2] + m[11];
    {
        Vec3 world_point = {v[0], v[1], v[2]};
        renderer_project_point(g_renderer, &world_point, matrix, &x, &y, &rhw);
    }
    s = (float)renderer_screen_scale(g_renderer, rhw);
    spr = (struct SprRes *)res_mgr_get(g_resMgr(), "shadow.spr");
    clip = motion_get_clip(act_res_get_motion((struct ActRes *)res_mgr_get(g_resMgr(), "shadow.act"), 0, 0), 0);
    if (clip->sprIndex == -1)
        return;
    image = sprite_resource_image(spr, clip->clipType, 0);
    if (!image) return;
    k = 1.0f;
    render_object_apply_pulse_scale(obj, &k);
    t = (double)(k * zoom * obj->shadow_scale);
    top = (float)((double)clip->y * (double)s * (double)clip->zoomY * t + (double)y);
    bottom = (float)((double)((image->heightHalved + 1) * image->height + clip->y) * (double)s * (double)clip->zoomY * t + (double)y
                     + 1.0);
    left = (float)((double)clip->x * (double)s * (double)clip->zoomX * t + (double)x);
    right = (float)((double)((image->widthHalved + 1) * image->width + clip->x) * (double)s * (double)clip->zoomX * t + (double)x
                    + 1.0);
    if (top >= (float)g_renderer->height || 0.0f >= bottom
        || left >= (float)g_renderer->width || 0.0f >= right)
        return;
    shift = (float)obj->shift_x;
    quad.top = top;
    quad.x0 = shift + left;
    quad.bottom = bottom;
    quad.x1 = shift + right;
    top_d = (top - y) * g_depthPerPixelCot;
    bottom_d = (bottom - y) * g_depthPerPixelCot;
    quad.depth_a = 1.0f / (depth - bottom_d) + 1.0e-5f;
    quad.depth_b = 1.0f / (depth - top_d) + 1.0e-5f;
    quad.color = ((((unsigned)clip->color[3] << 8) + clip->color[0]) << 16) + ((unsigned)clip->color[1] << 8)
                 + clip->color[2];
    quad.angle = (float)clip->angle;
    quad.image = (struct SprImg *)image;
    palette = spr->palette;
    if (session_is_screen_flipped(&g_session1))
        render_object_submit_flipped_quad(obj, &quad, palette, 0x101, x, y);
    else
        QUAD_BUILDER(obj, &quad, palette, 0x101, 0.0f, 0.0f);
}
