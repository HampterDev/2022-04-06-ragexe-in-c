#include "render/rp_pool.h"
#include "render/sprite_atlas.h"
#include "resources/palette.h"
#include "render/texture_types.h"
#include "resources/sprite.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include <string.h>
#include "ui/render/character_render.h"
#include "game/context.h"
#include "world/world_imports.h"
#include "actor/sprite_render.h"
#include "game/chat_room.h"
#include "session/session_jobs.h"
#include "session/session_lua.h"
#include "session/session_names.h"
#include "session/session_state.h"
#include "actor/sprite_quad.h"
#include "game/camera.h"

int g_chrHeadDx;
int g_chrHeadDy;
int g_chrWorldHeadDx;
int g_chrWorldHeadDy;

#define PI_F 3.141592f

static void *res_by_path(struct String path)
{
    return res_mgr_get_string(g_resMgr(), path);
}

static void *res_by_cstr(struct String *path)
{
    void *res = res_mgr_get(g_resMgr(), string_cstr(path));

    string_free(path);
    return res;
}

static int action_dir(int action)
{
    return action % 8;
}

static int riding_job(const struct CharacterRender *self)
{
    return self->riding == 1 ? session_get_mounted_job(self->job) : self->job;
}

static void transform(const float *m, const float *p, float *out)
{
    out[0] = m[0] * p[0] + m[3] * p[1] + m[6] * p[2] + m[9];
    out[1] = m[1] * p[0] + m[4] * p[1] + m[7] * p[2] + m[10];
    out[2] = m[2] * p[0] + m[5] * p[1] + m[8] * p[2] + m[11];
}

static void character_render_init(struct CharacterRender *self, int view_width,
                                                               int view_height)
{
    self->pos[0] = self->pos[1] = self->pos[2] = 0.0f;
    self->view_width = view_width;
    self->job2 = 0;
    self->accessory_top = 0;
    self->accessory_mid = 0;
    self->accessory_bottom = 0;
    self->head_low_location = 0;
    self->head_top_location = 0;
    self->head_mid_location = 0;
    self->riding = 0;
    self->body_style = 0;
    self->centre_dx = 0.0f;
    self->centre_dy = 80.0f;
    self->body_flags = -1;
    self->body_x = -1;
    self->body_y = -1;
    self->view_height = view_height;
    self->distance = 400.0f;
    self->pitch = -45.0f;
    self->yaw = 0.0f;
    self->up[0] = 0.0f;
    self->up[1] = 1.0f;
    self->up[2] = 0.0f;
    self->screen_x = 0.0f;
    self->screen_y = 0.0f;
    tree_init(&self->layers);
    self->robe = 0;
    self->is_helm_robe = 0;
    self->pos[0] = self->pos[1] = self->pos[2] = 0.0f;
    self->action = 0;
    self->motion = 0;
    self->shadow = 1;
}

struct CharacterRender *character_render_create(int view_width, int view_height)
{
    struct CharacterRender *self = mem_alloc(sizeof *self);

    if (self)
        character_render_init(self, view_width, view_height);
    return self;
}

void character_render_destroy(struct CharacterRender *self)
{
    tree_free(&self->layers, NULL);
    mem_free(self);
}

void character_render_set_robe(struct CharacterRender *self, int robe)
{
    self->robe = robe;
    client_lua_call_table(g_session1.luaState, "TB_Layer_Priority", "IsHelmRobe", "d>b", robe,
                            &self->is_helm_robe);
}

static int place_sprite(int index, float scale, const struct SprClip *clip, float ox, float oy,
                        const float *viewpos, float xoff, float *rhw_bottom, float *rhw_top,
                        const struct SprImg *img, struct ChrSpriteRect *rec, int dx, int dy)
{
    double sx = (double)(clip->zoomX * scale);
    double sy = (double)(clip->zoomY * scale);
    float bias = scale / g_spriteScaleDivisor * g_spriteScaleDepthBias;
    float top = (float)((double)(clip->y + dy) * sy + (double)oy);
    float bottom = (float)((double)((img->heightHalved + 1) * img->height + clip->y + dy) * sy + (double)oy + 1.0);
    float near_x = (float)((double)(clip->x + dx) * sx + (double)ox);
    float far_x = (float)((double)((img->widthHalved + 1) * img->width + clip->x + dx) * sx + (double)ox);
    float left, right, step;
    const BYTE *rgba = clip->color;
    BYTE colour[4];

    if (clip->flags & 1) {
        left = far_x;
        right = (float)((double)(clip->x + dx) * sx + (double)ox + 1.0);
        if (top >= (float)g_renderer->height || 0.0f >= bottom || right >= (float)g_renderer->width
            || 0.0f >= left)
            return 0;
    } else {
        left = near_x;
        right = (float)((double)((img->widthHalved + 1) * img->width + clip->x + dx) * sx + (double)ox + 1.0);
        if (top >= (float)g_renderer->height || 0.0f >= bottom || left >= (float)g_renderer->width
            || 0.0f >= right)
            return 0;
    }
    step = (float)(index + 3);
    *rhw_bottom = sprite_row_rhw(viewpos[2], (bottom - oy) * g_spriteRowDepthStep) + step * g_spriteLayerDepthStep + bias;
    *rhw_top = sprite_row_rhw(viewpos[2], (top - oy) * g_spriteRowDepthStep) + step * g_spriteLayerDepthStep + bias;
    rec->left = left + xoff;
    rec->top = top;
    rec->bottom = bottom;
    rec->right = right + xoff;
    rec->rhw_bottom = *rhw_bottom;
    rec->rhw_top = *rhw_top;
    rec->angle = (float)clip->angle;
    colour[3] = (BYTE)(int)((float)rgba[3] * 255.0f / 255.0f);
    colour[2] = (BYTE)(int)((float)rgba[0] * 255.0f / 255.0f);
    colour[1] = (BYTE)(int)((float)rgba[1] * 255.0f / 255.0f);
    colour[0] = (BYTE)(int)((float)rgba[2] * 255.0f / 255.0f);
    memcpy(&rec->colour, colour, 4);
    return 1;
}

static struct TexDrawPrim *make_quad(const struct ChrSpriteRect *rec, DWORD *palette, int flags, float dx, float dy)
{
    struct SpriteRegion place = {0};
    struct SprImg *img = rec->img;
    void *texture = img->texture;
    float z_bottom, z_top;
    struct TexDrawPrim *quad;
    struct TexDrawVertex *v;

    memset(&place, 0, sizeof place);
    if (!texture) {
        texture = sprite_atlas_resolve(g_renderer, img, palette, &place);
    } else {
        struct Texture *tex = texture;

        place.u = 0.0f;
        place.v = 0.0f;
        place.u1 = (float)((unsigned)tex->reqWidth) / (float)(unsigned)tex->base.width;
        place.v1 = (float)((unsigned)tex->reqHeight) / (float)(unsigned)tex->base.height;
    }
    if (!texture)
        return NULL;
    z_bottom = renderer_project_depth(rec->rhw_bottom);
    z_top = renderer_project_depth(rec->rhw_top);
    quad = renderer_new_prim(g_renderer);
    quad->base.texture = texture;
    v = quad->base.verts;
    if (rec->angle > 0.0f) {
        float c = get_cos_deg((int)rec->angle), s = get_sin_deg((int)rec->angle);
        float hh = (rec->bottom - rec->top) * 0.5f;
        float hw = (rec->right - rec->left) * 0.5f;
        float cy = rec->top + hh + dy, cx = rec->left + hw + dx;
        float hh_s = hh * s, hw_c = hw * c, hw_s = hw * s, hh_c = hh * c;

        v[0].x = (hh_s - hw_c) + cx + 0.5f;
        v[0].y = (-hh_c - hw_s) + cy + 0.5f;
        v[1].x = (hw_c + hh_s) + cx + 0.5f;
        v[1].y = (hw_s - hh_c) + cy + 0.5f;
        v[2].x = (-hw_c - hh_s) + cx + 0.5f;
        v[2].y = (hh_c - hw_s) + cy + 0.5f;
        v[3].x = (hw_c - hh_s) + cx + 0.5f;
        v[3].y = (hh_c + hw_s) + cy + 0.5f;
    } else {
        v[0].x = rec->left + 0.5f;
        v[0].y = rec->top + 0.5f;
        v[1].x = rec->right + 0.5f;
        v[1].y = rec->top + 0.5f;
        v[2].x = rec->left + 0.5f;
        v[2].y = rec->bottom + 0.5f;
        v[3].x = rec->right + 0.5f;
        v[3].y = rec->bottom + 0.5f;
    }
    /* custom: the rows keep their own depth (z) for the depth test, but all four corners share
       the top row's rhw. The GPU interpolates the texture by rhw, so a top and bottom rhw that
       differ (more so the steeper the camera tilts) warp the flat sprite. The top row's rhw is
       what the original's first corner carried, and the draw queues sort by it, so a shadow still
       draws before the figure standing on it. */
    v[0].z = z_top;
    v[0].rhw = rec->rhw_top;
    v[0].u = place.u;
    v[0].v = place.v;
    v[0].diffuse = rec->colour;
    v[1].z = z_top;
    v[1].rhw = rec->rhw_top;
    v[1].u = place.u1;
    v[1].v = place.v;
    v[1].diffuse = rec->colour;
    v[2].z = z_bottom;
    v[2].rhw = rec->rhw_top;
    v[2].u = place.u;
    v[2].v = place.v1;
    v[2].diffuse = rec->colour;
    v[3].z = z_bottom;
    v[3].rhw = rec->rhw_top;
    v[3].u = place.u1;
    v[3].v = place.v1;
    v[3].diffuse = rec->colour;
    quad->base.srcBlend = 5;
    quad->base.destBlend = flags != 5 ? 6 : 2;
    return quad;
}

static void headgear_depth(int layer, struct ChrSpriteRect *rec, float b, float t)
{
    int near_ = 320.0f > g_viewDistance;

    switch (layer) {
    case CHR_LAYER_TOP:
        if (near_) {
            b += 2.7e-5f;
            t += 2.7e-5f;
        }
        break;
    case CHR_LAYER_MID:
        if (near_) {
            b += 4.0e-5f;
            t += 0.0001f;
        } else {
            t += 4.5e-5f;
        }
        break;
    case CHR_LAYER_BOTTOM:
        if (near_) {
            b += 3.0e-5f;
            t += 3.0e-5f;
        }
        break;
    default:
        return;
    }
    rec->rhw_bottom = b;
    rec->rhw_top = t;
}

static void layer_depth(struct CharacterRender *self, int layer, struct ChrSpriteRect *rec, float rhw_b,
                        float rhw_t)
{
    int near_ = 320.0f > g_viewDistance;
    float b = rhw_b, t = rhw_t;
    BYTE found = 0;
    double up_near = 0.0, up_far = 0.0;

    switch (layer) {
    case CHR_LAYER_HEAD:
        if (self->accessory_mid == 0x447) {
            if (32.0f > g_viewDistance) {
                b -= 8.0e-6f;
                t -= 8.0e-6f;
            } else {
                b -= 5.0e-7f;
                t -= 5.0e-7f;
            }
        } else if (near_) {
            b += 2.5e-5f;
            t += 2.5e-5f;
        }
        break;
    case CHR_LAYER_TOP:
        switch (self->accessory_top) {
        case 0x411:
        case 0x54e:
        case 0x5c6:
        case 0x5d0:
        case 0x5db:
            if (near_) {
                b += 0.0008f;
                t += 0.0008f;
            } else {
                b += 0.00019f;
                t += 0.00019f;
            }
            break;
        case 0x4d3:
            if (near_) {
                b += 0.0008f;
                t += 0.0008f;
            } else {
                b += 0.00013f;
                t += 0.00013f;
            }
            break;
        default:
            if (near_) {
                b += 2.7e-5f;
                t += 2.7e-5f;
            }
        }
        break;
    case CHR_LAYER_MID:
        if (near_) {
            b += 4.0e-5f;
            t += 0.0001f;
        } else {
            t += 4.5e-5f;
        }
        break;
    case CHR_LAYER_BOTTOM:
        if (self->accessory_mid == 0x5cd) {
            if (near_) {
                b += 0.00035f;
                t += 0.0006f;
            } else {
                b += 1.0e-5f;
                t += 0.00015f;
            }
            break;
        }
        switch (self->accessory_bottom) {
        case 0x54a:
            if (near_) {
                b += 9.0e-5f;
                t -= 5.0e-5f;
            }
            break;
        case 0x526:
            if (near_) {
                b += 0.00025f;
                t += 0.0001f;
            } else {
                b += 5.0e-5f;
                t += 5.0e-5f;
            }
            break;
        case 0x553:
            if (near_) {
                b = b + 2.7e-5f + 0.0004f;
                t = t + 2.7e-5f + 0.0007f;
            } else {
                b += 7.0e-5f;
                t += 0.00017f;
            }
            break;
        case 0x62e:
            if (near_) {
                b += 0.0002f;
                t += 0.0002f;
            } else {
                b += 4.0e-5f;
                t += 6.0e-5f;
            }
            break;
        default:
            if (near_) {
                b += 3.0e-5f;
                t += 3.0e-5f;
            }
        }
        break;
    }
    rec->rhw_bottom = b;
    rec->rhw_top = t;
    client_lua_call(g_session1.luaState, "OffsetItemPos_GetUppingBottomItem", "d>bll", self->accessory_top, &found,
            &up_near, &up_far);
    if (layer == CHR_LAYER_TOP && found == 1) {
        if (up_near == 0.0)
            up_near = 0.00025;
        if (up_far == 0.0)
            up_far = 0.0001;
        if (near_) {
            rec->rhw_bottom = (float)((double)rhw_b + up_near);
            rec->rhw_top = (float)((double)rhw_t + up_near);
        } else {
            rec->rhw_bottom = (float)((double)rhw_b + up_far);
            rec->rhw_top = (float)((double)rhw_t + up_far);
        }
    }
}

static BYTE robe_on_top(const struct CharacterRender *self)
{
    BYTE on_top = 1;

    if (g_session1.new_layer_table_loaded == 1) {
        int returned_job = 1;

        client_lua_call(g_session1.luaState, "_New_DrawOnTop", "ddddd>bd", self->robe, self->sex, self->job,
                self->action, self->motion, &on_top, &returned_job);
    } else {
        client_lua_call(g_session1.luaState, "DrawOnTop", "ddddd>b", self->robe, self->sex, self->job, self->action,
                self->motion, &on_top);
    }
    return on_top;
}

static int robe_resources(const struct CharacterRender *self, struct SprRes **spr, struct ActRes **act)
{
    struct String spr_name, act_name;
    int ok = 0;

    session_get_robe_spr_name(&spr_name, self->job, self->sex, self->robe, self->riding != 0);
    session_get_robe_act_name(&act_name, self->job, self->sex, self->robe, self->riding != 0);
    if (res_mgr_exists(g_resMgr(), string_from(string_cstr(&spr_name), spr_name.size))
        && res_mgr_exists(g_resMgr(), string_from(string_cstr(&act_name), act_name.size))) {
        *spr = res_by_path(string_from(string_cstr(&spr_name), spr_name.size));
        *act = res_by_path(string_from(string_cstr(&act_name), act_name.size));
        ok = 1;
    }
    string_free(&act_name);
    string_free(&spr_name);
    return ok;
}

static int robe_wanted(const struct CharacterRender *self)
{
    if (self->robe <= 0 || self->is_helm_robe == 1)
        return 0;
    if (self->action >= 0x40 && self->action <= 0x47)
        return 0;
    if (self->body_flags == -1 && self->body_x == -1 && self->body_y == -1)
        return 0;
    return 1;
}

static void robe_depth(const struct CharacterRender *self, BYTE on_top, struct ChrSpriteRect *rec, float rhw_b,
                       float rhw_t)
{
    if (!on_top) {
        if (game_camera_pitch() >= -40.0f) {
            rec->rhw_bottom = rhw_b - 6.5e-6f;
            rec->rhw_top = rhw_t - 6.5e-6f;
        }
    }
    if ((unsigned)(self->action - 0x3a) <= 3) {
        rec->rhw_bottom += 6.5e-6f;
        rec->rhw_top += 6.5e-6f;
    }
}

static void robe_pass(struct CharacterRender *self, struct List *out, BYTE on_top, int *index,
                      const float *viewpos, float ox, float oy, float scale, float xoff, struct ActRes *body_act)
{
    struct SprRes *spr;
    struct ActRes *act;
    struct Motion *m;
    int motion, i;

    if (!robe_wanted(self))
        return;
    if (robe_on_top(self) != on_top) {
        ++*index;
        return;
    }
    if (!robe_resources(self, &spr, &act))
        return;
    motion = self->motion;
    if (body_act) {
        int block = (self->action >= 0 && self->action < 8) || (unsigned)(self->action - 0x10) <= 6 ? 0
                                                                                                     : self->motion;

        session_get_stepped_motion(body_act, act, self->action, block, 0, &motion);
    }
    m = act_res_get_motion(act, self->action, motion);
    for (i = 0; i < m->clip_count; i++) {
        struct SprClip adjusted = *motion_get_clip(m, i), *clip = &adjusted;
        struct ChrSpriteRect rec;
        struct SprImg *img;
        float rhw_b, rhw_t;

        if (clip->sprIndex == -1)
            continue;
        clip->color[3] = session_get_transparent_item_alpha(&g_session1, self->robe);
        img = spr_res_image(spr, clip);
        if (!img)
            continue;
        memset(&rec, 0, sizeof rec);
        place_sprite(*index, scale, clip, ox, oy, viewpos, xoff, &rhw_b, &rhw_t, img, &rec, 0, 0);
        robe_depth(self, on_top, &rec, rhw_b, rhw_t);
        rec.img = img;
        {
            struct TexDrawPrim *quad = make_quad(&rec, spr->palette, self->body_flags, 0.0f, 0.0f);
            if (quad)
                list_push_back(out, &quad, sizeof quad);
        }
    }
    ++*index;
}

static void robe_pass_world(struct CharacterRender *self, struct List *out, BYTE on_top, int *index,
                            const float *viewpos, float ox, float oy, float scale, float xoff)
{
    struct SprRes *spr;
    struct ActRes *act;
    struct Motion *m;
    int i;

    if (!robe_wanted(self))
        return;
    if (robe_on_top(self) != on_top) {
        ++*index;
        return;
    }
    if (!robe_resources(self, &spr, &act))
        return;
    m = act_res_get_motion(act, self->action, self->motion);
    for (i = 0; i < m->clip_count; i++) {
        struct SprClip adjusted = *motion_get_clip(m, i), *clip = &adjusted;
        struct ChrSpriteRect rec;
        struct SprImg *img;
        float rhw_b, rhw_t;

        if (clip->sprIndex == -1)
            continue;
        clip->color[3] = session_get_transparent_item_alpha(&g_session1, self->robe);
        img = spr_res_image(spr, clip);
        if (!img)
            continue;
        memset(&rec, 0, sizeof rec);
        place_sprite(*index, scale, clip, ox, oy, viewpos, xoff, &rhw_b, &rhw_t, img, &rec, 0, 0);
        robe_depth(self, on_top, &rec, rhw_b, rhw_t);
        if (on_top && session_is_top_layer_robe(&g_session1, self->robe)) {
            float up = 320.0f > g_viewDistance ? 0.000251f : 0.00011f;

            rec.rhw_bottom = rhw_b + up;
            rec.rhw_top = rhw_t + up;
        }
        rec.img = img;
        {
            struct ChrQuadPair pair = {1, make_quad(&rec, spr->palette, 1, 0.0f, 0.0f)};
            if (pair.quad)
                list_push_back(out, &pair, sizeof pair);
        }
    }
    ++*index;
}

struct TexDrawPrim *character_render_shadow_quad(struct CharacterRender *self, const float *m, float scale)
{
    struct C3dAttr *attr = game_current_map_attr();
    Vec3 ground;
    float depth, sx, sy, rhw, ppu, s, top, bottom, left, right;
    double dppu, dys, dxs, ds;
    struct SprRes *spr;
    struct ActRes *act;
    struct SprClip *clip;
    struct SprImg *img;
    struct ChrSpriteRect rec;
    const BYTE *rgba;

    if (scale == 0.0f || !attr)
        return NULL;
    ground.x = self->pos[0];
    ground.y = c3d_attr_get_height(attr, self->pos[0], self->pos[2]);
    ground.z = self->pos[2];
    depth = m[2] * ground.x + m[5] * ground.y + m[8] * ground.z + m[11];
    if (self->view_width == 0 && self->view_height == 0)
        renderer_project_point(g_renderer, &ground, m, &sx, &sy, &rhw);
    else
        renderer_project_centred(g_renderer, &ground, m, &sx, &sy, &rhw,
                                (float)self->view_width + self->centre_dx,
                                (float)self->view_height + self->centre_dy);
    ppu = renderer_screen_scale(g_renderer, rhw);
    spr = res_mgr_get(g_resMgr(), "shadow.spr");
    act = res_mgr_get(g_resMgr(), "shadow.act");
    clip = motion_get_clip(act_res_get_motion(act, 0, 0), 0);
    if (clip->sprIndex == -1)
        return NULL;
    img = sprite_resource_image(spr, clip->clipType, 0);
    if (!img) return NULL;
    s = session_get_npc_location_radius(&g_session1, self->job, self->riding) * scale;
    dppu = ppu;
    ds = s;
    dys = clip->zoomY;
    dxs = clip->zoomX;
    top = (float)((double)clip->y * dppu * dys * ds + (double)sy);
    bottom = (float)((double)((img->heightHalved + 1) * img->height + clip->y) * dppu * dys * ds + (double)sy + 1.0);
    left = (float)((double)clip->x * dppu * dxs * ds + (double)sx);
    right = (float)((double)((img->widthHalved + 1) * img->width + clip->x) * dppu * dxs * ds + (double)sx + 1.0);
    if (top >= (float)g_renderer->height || 0.0f >= bottom || left >= (float)g_renderer->width
        || 0.0f >= right)
        return NULL;
    rgba = clip->color;
    memset(&rec, 0, sizeof rec);
    rec.top = top;
    rec.left = left;
    rec.bottom = bottom;
    rec.right = right;
    {
        float anchor[3];

        anchor[0] = m[0] * ground.x + m[3] * ground.y + m[6] * ground.z + m[9];
        anchor[1] = m[1] * ground.x + m[4] * ground.y + m[7] * ground.z + m[10];
        anchor[2] = depth;
        rec.rhw_bottom = sprite_flat_row_rhw(m + 3, anchor, bottom - sy) + 1.0e-5f;
        rec.rhw_top = sprite_flat_row_rhw(m + 3, anchor, top - sy) + 1.0e-5f;
    }
    rec.colour = (((((DWORD)rgba[3] << 8) + rgba[0]) << 8) + rgba[1]) << 8 | rgba[2];
    rec.angle = (float)clip->angle;
    rec.img = img;
    return make_quad(&rec, spr->palette, 0x101, 0.0f, 0.0f);
}

struct ChrPass {
    int world;
    struct List *out;
    const float *matrix;
    float viewpos[3];
    float sx, sy;
    float rhw;
    float scale;
    int index;
    BYTE use_bbox;
    BYTE *drawn;
    struct ActRes *acts[CHR_LAYERS];
    struct SprRes *sprs[CHR_LAYERS];
};

struct ChrSortedLayer {
    int priority;
    struct ChrSpriteRect rec;
    DWORD *palette;
    int flags;
};

static void pass_push(struct ChrPass *pass, int flags, struct TexDrawPrim *quad)
{
    if (!quad)
        return;
    if (pass->world) {
        struct ChrQuadPair pair = {flags, quad};

        list_push_back(pass->out, &pair, sizeof pair);
    } else {
        list_push_back(pass->out, &quad, sizeof quad);
    }
}

static void pass_robe(struct CharacterRender *self, struct ChrPass *pass, BYTE on_top, struct ActRes *body_act)
{
    if (pass->world)
        robe_pass_world(self, pass->out, on_top, &pass->index, pass->viewpos, self->screen_x, self->screen_y,
                        pass->scale, 0.0f);
    else
        robe_pass(self, pass->out, on_top, &pass->index, pass->viewpos, pass->sx, pass->sy, pass->scale, 0.0f,
                  body_act);
}

static int *layer_view(struct CharacterRender *self, int layer)
{
    struct IntIntPair *pair = tree_get(&self->layers, &layer, sizeof layer, sizeof *pair, tree_less_int);

    return &pair->value;
}

static void load_layers(struct CharacterRender *self, struct ChrPass *pass)
{
    static const int accessories[3] = { CHR_LAYER_TOP, CHR_LAYER_MID, CHR_LAYER_BOTTOM };
    int head_job = pass->world ? self->job : self->job2;
    struct String path;
    int i, key;

    memset(pass->acts, 0, sizeof pass->acts);
    memset(pass->sprs, 0, sizeof pass->sprs);
    if (pass->world || self->job != CHR_JOB_WEDDING) {
        pass->acts[CHR_LAYER_BODY] = res_by_path(
            *session_get_body_spr_name_variant0(&path, riding_job(self), self->sex, self->body_style, 0));
        pass->sprs[CHR_LAYER_BODY] = res_by_path(
            *session_get_body_spr_name_variant1(&path, riding_job(self), self->sex, self->body_style, 0));
    } else {
        session_get_body_spr_name(&path, 0, self->job2, CHR_JOB_WEDDING, self->sex, 0);
        pass->acts[CHR_LAYER_BODY] = res_by_cstr(&path);
        session_get_body_spr_name(&path, 1, self->job2, self->job, self->sex, 0);
        pass->sprs[CHR_LAYER_BODY] = res_by_cstr(&path);
    }
    pass->acts[CHR_LAYER_HEAD] = res_by_path(*session_get_head_spr_name0(&path, head_job, self->sex, &self->head));
    pass->sprs[CHR_LAYER_HEAD] = res_by_path(*session_get_head_spr_name1(&path, head_job, self->sex, &self->head));
    for (i = 0; i < 3; i++) {
        key = accessories[i];
        pass->acts[key] = res_by_path(*session_get_res_name0(&path, self->job, self->sex,
                                                                self->head, *layer_view(self, key)));
        pass->sprs[key] = res_by_path(*session_get_res_name1(&path, self->job, self->sex,
                                                                self->head, *layer_view(self, key)));
    }
    if (self->is_helm_robe == 1) {
        key = CHR_LAYER_ROBE;
        pass->acts[key] = res_by_path(*session_get_res_name0(&path, self->job, self->sex,
                                                                self->head, *layer_view(self, key)));
        pass->sprs[key] = res_by_path(*session_get_res_name1(&path, self->job, self->sex,
                                                                self->head, *layer_view(self, key)));
    }
}

static void replace_head(struct CharacterRender *self, struct ChrPass *pass)
{
    int last = pass->world ? CHR_LAYER_MID : CHR_LAYER_BOTTOM;
    int i;

    for (i = CHR_LAYER_TOP; i <= last; i++) {
        if (!pass->acts[i] || !pass->sprs[i])
            continue;
        if (session_is_head_replacing_view(&g_session1, *layer_view(self, i)) == 1) {
            pass->acts[CHR_LAYER_HEAD] = pass->acts[i];
            pass->acts[i] = NULL;
            pass->sprs[CHR_LAYER_HEAD] = pass->sprs[i];
            pass->sprs[i] = NULL;
            return;
        }
    }
}

static int is_headgear(int layer)
{
    return layer >= CHR_LAYER_TOP && (layer <= CHR_LAYER_BOTTOM || layer == CHR_LAYER_ROBE);
}

static int attach_offset(struct Motion *m, struct Motion *ref, int *dx, int *dy)
{
    const struct AttachPointInfo *a, *b;

    if (animation_attachment_count(m) <= 0 || animation_attachment_count(ref) <= 0)
        return 0;
    a = animation_attachment(m, 0);
    b = animation_attachment(ref, 0);
    if (a->attr != b->attr)
        return 0;
    *dx += b->x - a->x;
    *dy += b->y - a->y;
    return 1;
}

static void render_layers(struct CharacterRender *self, struct ChrPass *pass, const struct ImfRes *imf)
{
    struct ChrSortedLayer sorted[CHR_LAYERS];
    int nsorted = 0, layer, i;
    BYTE head_seen = 0, head_drawn = 0;
    int extra_bodies = 0;
    int *head_dx = pass->world ? &g_chrWorldHeadDx : &g_chrHeadDx;
    int *head_dy = pass->world ? &g_chrWorldHeadDy : &g_chrHeadDy;

    for (layer = CHR_LAYERS - 1; layer >= 0; layer--) {
        int action = self->action, base = action - action % 8;
        int slot = imf_layer_at(imf, layer, base, action, self->motion);
        int ly = slot, clip_index, dx, dy, flags;
        struct Motion *m;
        struct SprClip *clip;
        struct SprImg *img;
        struct ChrSpriteRect rec;
        float rhw_b, rhw_t;
        DWORD *palette;

        if (!head_seen)
            head_seen = slot == CHR_LAYER_HEAD;
        if (head_seen == 1 && ly == 0) {
            extra_bodies++;
            ly = 2;
        }
        if (extra_bodies == 1 && slot == 2) {
            extra_bodies = 2;
            ly = 0;
        } else if (ly == 2) {
            ly = 4;
        } else if (ly == 4) {
            ly = 2;
        }
        if (!pass->acts[ly] || !pass->sprs[ly])
            continue;
        if (ly == 5 || ly == 6 || ly == 7)
            continue;
        if ((ly == 2 || ly == 3 || ly == 4) && !head_drawn)
            continue;
        clip_index = ly == CHR_LAYER_HEAD;
        if (self->motion >= (int)actres_frame_count(pass->acts[ly], self->action))
            continue;
        m = act_res_get_motion(pass->acts[ly], self->action, self->motion);
        if (is_headgear(ly)) {
            int nhead = (int)actres_frame_count(pass->acts[CHR_LAYER_HEAD], self->action);
            int n = (int)actres_frame_count(pass->acts[ly], self->action);

            if (nhead > 0 && n > 0 && nhead < n && n % nhead == 0)
                m = act_res_get_motion(pass->acts[ly], self->action, self->motion * (n / nhead));
        }
        clip = motion_get_clip(m, clip_index);
        if (clip->sprIndex == -1)
            continue;
        if (ly == CHR_LAYER_HEAD)
            head_drawn = 1;
        dx = 0;
        dy = 0;
        img = spr_res_image(pass->sprs[ly], clip);
        if (ly == CHR_LAYER_HEAD) {
            struct ActRes *body;
            struct String path;

            if (pass->world || self->job != CHR_JOB_WEDDING) {
                body = res_by_path(
                    *session_get_body_spr_name_variant0(&path, riding_job(self), self->sex, self->body_style, 0));
            } else {
                session_get_body_spr_name(&path, 0, self->job2, CHR_JOB_WEDDING, self->sex, 0);
                body = res_by_cstr(&path);
            }
            if (pass->world || body) {
                if (attach_offset(m, act_res_get_motion(body, self->action, self->motion), &dx, &dy)) {
                    *head_dx = dx;
                    *head_dy = dy;
                }
            }
        } else if (is_headgear(ly)) {
            int frame = 0;
            struct Motion *sm = session_get_stepped_motion(pass->acts[CHR_LAYER_HEAD],
                                                          pass->acts[ly], self->action, self->motion, 0, &frame);
            struct Motion *hm;

            if (sm && frame != self->motion) {
                m = sm;
                clip = motion_get_clip(sm, clip_index);
                img = spr_res_image(pass->sprs[ly], clip);
            }
            hm = act_res_get_motion(pass->acts[CHR_LAYER_HEAD], self->action, self->motion);
            attach_offset(m, hm, &dx, &dy);
            dx += *head_dx;
            if (layer_is_ignored_riding_state(*layer_view(self, ly), self->job, base, self->riding) == 1)
                dy += layer_body_attach_dy(hm, self->action, self->motion, self->job, self->sex, self->body_style);
            else
                dy += *head_dy;
            if (clip->zoomX != 1.0f)
                dx = clip->zoomX != 0 ? (int)((float)dx / clip->zoomX) : 0;
            if (clip->zoomY != 1.0f)
                dy = clip->zoomY != 0 ? (int)((float)dy / clip->zoomY) : 0;
        }

        memset(&rec, 0, sizeof rec);
        rhw_b = 0.0f;
        rhw_t = 0.0f;
        if (!img || !place_sprite(pass->index, pass->scale, clip, pass->world ? self->screen_x : pass->sx,
                          pass->world ? self->screen_y : pass->sy, pass->viewpos, 0.0f, &rhw_b, &rhw_t, img, &rec,
                          dx, dy))
            continue;
        pass->index++;
        if (pass->world)
            *pass->drawn = 1;
        if (is_headgear(ly))
            headgear_depth(ly, &rec, rhw_b, rhw_t);
        else
            layer_depth(self, ly, &rec, rhw_b, rhw_t);
        rec.img = img;
        if (pass->world && pass->use_bbox == 1 && (ly == CHR_LAYER_BODY || ly == CHR_LAYER_HEAD)) {
            struct ScreenBounds *box = &self->bbox;
            float lo = rec.left < rec.right ? rec.left : rec.right;
            float hi = rec.left > rec.right ? rec.left : rec.right;

            if (box->left > lo)
                box->left = lo;
            if (box->top > rec.top)
                box->top = rec.top;
            if (hi > box->right)
                box->right = hi;
            if (rec.bottom > box->bottom)
                box->bottom = rec.bottom;
        }

        palette = pass->sprs[ly]->palette;
        if (ly == CHR_LAYER_BODY && self->body_palette > 0) {
            struct String path;

            if (pass->world)
                session_get_body_pal_name(&path, self->job, self->sex, self->body_palette,
                                        self->body_style, 0);
            else
                session_get_body_pal_name(&path, riding_job(self), self->sex, self->body_palette,
                                        self->body_style, 0);
            palette = palette_resource_colors(res_by_cstr(&path), pass->sprs[ly]->palette);
        } else if (ly == CHR_LAYER_HEAD && self->head_palette > 0) {
            struct String path;

            session_get_head_pal_name(&path, self->head, self->job, self->sex, self->head_palette);
            palette = palette_resource_colors(res_by_cstr(&path), pass->sprs[ly]->palette);
        }

        if (ly == CHR_LAYER_TOP || ly == CHR_LAYER_MID || ly == CHR_LAYER_ROBE)
            flags = 1;
        else if (ly == CHR_LAYER_BOTTOM)
            flags = self->accessory_bottom != 0xb9;
        else
            flags = 0x101;

        if (is_headgear(ly)) {
            int dir = action_dir(self->action);

            if (*layer_view(self, ly) > 0) {
                int priority = -1;

                client_lua_call(g_session1.luaState, "GetLayerPriority", "ddd>d", dir, ly,
                        *layer_view(self, ly), &priority);
                if (priority != -1) {
                    for (i = nsorted; i > 0 && sorted[i - 1].priority > priority; i--)
                        sorted[i] = sorted[i - 1];
                    sorted[i].priority = priority;
                    sorted[i].rec = rec;
                    sorted[i].palette = palette;
                    sorted[i].flags = flags;
                    nsorted++;
                }
            }
        } else {
            pass_push(pass, flags, make_quad(&rec, palette, flags, 0.0f, 0.0f));
        }
        if (ly == CHR_LAYER_BODY) {
            self->body_flags = flags;
            self->body_x = clip->x;
            self->body_y = clip->y;
            pass_robe(self, pass, 1, pass->acts[CHR_LAYER_BODY]);
        }
    }

    if (nsorted) {
        float max_b = sorted[0].rec.rhw_bottom, max_t = sorted[0].rec.rhw_top, bias = 0.0f;

        for (i = 1; i < nsorted; i++) {
            max_b = sorted[i].rec.rhw_bottom > max_b ? sorted[i].rec.rhw_bottom : max_b;
            max_t = sorted[i].rec.rhw_top > max_t ? sorted[i].rec.rhw_top : max_t;
        }
        for (i = 0; i < nsorted; i++) {
            sorted[i].rec.rhw_bottom = bias + max_b;
            sorted[i].rec.rhw_top = bias + max_t;
            pass_push(pass, sorted[i].flags, make_quad(&sorted[i].rec, sorted[i].palette, sorted[i].flags, 0.0f,
                                                       0.0f));
            bias += 1.0e-6f;
        }
    }
}

static void render_pass(struct CharacterRender *self, struct ChrPass *pass)
{
    struct String path;
    const struct ImfRes *imf;

    if (self->shadow == 1) {
        struct TexDrawPrim *shadow = character_render_shadow_quad(self, pass->matrix, 1.0f);

        if (shadow)
            pass_push(pass, 0x101, shadow);
    }
    session_get_imf_name(&path, pass->world ? self->job : self->job2, self->sex);
    imf = res_by_cstr(&path);
    if (pass->world)
        pass_robe(self, pass, 0, NULL);
    load_layers(self, pass);
    if (!pass->world)
        pass_robe(self, pass, 0, pass->acts[CHR_LAYER_BODY]);
    replace_head(self, pass);
    render_layers(self, pass, imf);
}

struct List *character_render_collect_world_quads(struct CharacterRender *self, struct List *out,
                                                        BYTE use_bbox, BYTE *drawn)
{
    struct ChrPass pass;
    float m[12];

    list_init(out);
    *drawn = 0;
    if (!game_copy_view_matrix(m))
        return out;
    memset(&pass, 0, sizeof pass);
    pass.world = 1;
    transform(m, self->pos, pass.viewpos);
    self->screen_x = 0.0f;
    self->screen_y = 0.0f;
    {
        Vec3 world_point = {self->pos[0], self->pos[1], self->pos[2]};
        renderer_project_point(g_renderer, &world_point, m, &self->screen_x, &self->screen_y, &pass.rhw);
    }
    pass.scale = session_get_baby_job_scale(&g_session1, self->job) * renderer_screen_scale(g_renderer, pass.rhw);
    self->bbox.left = (float)g_renderer->width;
    self->bbox.top = (float)g_renderer->height;
    self->bbox.right = 0.0f;
    self->bbox.bottom = 0.0f;
    self->bbox.z = pass.rhw;
    pass.out = out;
    pass.matrix = m;
    pass.use_bbox = use_bbox;
    pass.drawn = drawn;
    render_pass(self, &pass);
    return out;
}
