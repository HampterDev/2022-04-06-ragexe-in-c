#include "game/gamemode.h"
#include "actor/actor_spawns.h"
#include "actor/actor_ops.h"
#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "render/math3d.h"
#include "render/rp_pool.h"
#include "render/texture_cache.h"
#include "render/texture.h"
#include <stdlib.h>
#include "resources/emblem_types.h"
#include "resources/granny_sdk.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include <string.h>
#include "actor/actor.h"
#include "actor/granny_actor.h"
#include "actor/game_actor.h"
#include "actor/weather.h"
#include "actor/sprite_render.h"
#include "world/camera.h"

const struct GameActorOps c3d_granny_game_actor_ops = {
    .base = {
        .base = {
            .destroy = c3d_granny_game_actor_delete,
            .on_process = c3d_granny_game_actor_on_process,
            .send_msg = game_actor_send_msg,
            .render = c3d_granny_game_actor_render,
            .get_direction = game_actor_dir_of,
        },
        .render_body = game_actor_set_render_info,
        .set_action = c3d_granny_game_actor_set_action,
        .update_motion = c3d_granny_game_actor_update_motion,
        .get_effect_state = game_actor_get_effect_state,
    },
    .process_state = game_actor_process_state,
    .set_state = game_actor_set_state,
    .set_unit_cell = game_actor_set_unit_cell,
    .clear_unit_cell = game_actor_clear_unit_cell,
    .set_job_model = c3d_granny_game_actor_set_job_model,
    .set_look = actor_default_look,
    .set_look_with_state = actor_default_look_state,
    .set_spr_job = actor_default_int,
    .set_head = actor_default_int,
    .set_weapon = actor_default_weapon,
    .set_weapon_look = actor_default_weapon,
    .set_accessory = actor_default_int,
    .set_accessory2 = actor_default_int,
    .set_accessory3 = actor_default_int,
    .set_shield = actor_default_int,
    .set_appearance_value = actor_default_int,
    .set_imf_name = actor_default_noop,
    .set_head_pal_name = actor_default_noop,
    .set_body_pal_name = actor_default_noop,
    .set_body_palette = actor_default_int,
    .set_head_palette = actor_default_int,
    .get_weapon = actor_default_no_weapon,
    .walk_motion = actor_default_noop,
    .get_attack_motion = actor_default_zero,
    .make_corpse = c3d_granny_game_actor_make_corpse,
    .set_attack_speed = game_actor_set_attack_speed,
    .set_honor = actor_default_int,
    .set_honor_virtue = actor_default_int_flags,
    .get_char_id = actor_default_zero,
    .set_robe = actor_default_int_bool,
    .set_body_style = actor_default_int_bool,
    .set_mounted = game_actor_set_mounted,
    .get_sprite_variant = actor_default_zero,
    .set_sprite_variant = actor_default_int,
    .get_skip_arrow = actor_default_false,
    .set_skip_arrow = actor_default_bool,
    .refresh_accessory3 = actor_default_noop,
    .get_head_extent = game_actor_get_head_extent,
};

const struct GameObjectOps granny_corpse3d_ops = {
    granny_corpse3d_delete,
    granny_corpse3d_on_process,
    game_object_send_msg,
    granny_corpse3d_render,
    game_object_dir_of,
};

#define ACTOR(self) ((struct GameActor *)(self))

static __inline float light_term(float d)
{
    float t = 1.0f < d ? 1.0f : d;

    return 0.0f > t ? 0.0f : t;
}

struct C3dGrannyGameActor *c3d_granny_game_actor_construct(struct C3dGrannyGameActor *self,
                                                                   const char *name)
{
    game_actor_construct(ACTOR(self));
    self->ops = &c3d_granny_game_actor_ops.base.base;
    self->shadow_points.first = NULL;
    self->shadow_points.last = NULL;
    self->shadow_points.end = NULL;
    self->model = NULL;
    self->first_frame = 1;
    self->anim = 0;
    self->alpha = 0xff;
    strcpy(self->name, name);
    self->hidden_state = 0;
    self->hidden_since = 0;
    self->emblem_tex = NULL;
    self->shadow_tex = texture_create(0x23, 0x23, 1);
    vec_resize(&self->shadow_points, 0x1324, sizeof(Vec3));
    return self;
}

void c3d_granny_game_actor_destruct(struct C3dGrannyGameActor *self)
{
    self->ops = &c3d_granny_game_actor_ops.base.base;
    granny_model_state_destroy(self->model);
    self->model = NULL;
    if (self->emblem_tex)
        texture_destroy(self->emblem_tex);
    if (self->shadow_tex)
        texture_destroy(self->shadow_tex);
    vec_free(&self->shadow_points);
    game_actor_dc_game_actor(ACTOR(self));
}

void c3d_granny_game_actor_delete(struct GameObject *object)
{
    struct C3dGrannyGameActor *self = (struct C3dGrannyGameActor *)object;

    c3d_granny_game_actor_destruct(self);
    mem_free(self);
}

struct GrannyCorpse3d *granny_corpse3d_construct(struct GrannyCorpse3d *self,
                                                             const char *name)
{
    game_object_construct((struct GameObject *)self);
    self->ops = &granny_corpse3d_ops;
    self->pos.x = 0.0f;
    self->pos.y = 0.0f;
    self->pos.z = 0.0f;
    self->alpha = 0xff;
    self->model = NULL;
    self->frames = 0;
    self->anim_time = 0.0f;
    strcpy(self->name, name);
    granny_corpse3d_load_model(self);
    return self;
}

void granny_corpse3d_destruct(struct GrannyCorpse3d *self)
{
    self->ops = &granny_corpse3d_ops;
    granny_corpse3d_free_model(self);
    game_object_destruct((struct GameObject *)self);
}

void granny_corpse3d_delete(struct GameObject *object)
{
    struct GrannyCorpse3d *self = (struct GrannyCorpse3d *)object;

    granny_corpse3d_destruct(self);
    mem_free(self);
}

static void actor_anim_step(struct C3dGrannyGameActor *self)
{
    float elapsed;

    if (!self->model)
        return;
    *(GrannyClock *)self->clock_now = _GrannyGetSystemSeconds();
    elapsed = _GrannyGetSecondsElapsed((const GrannyClock *)self->clock_last, (const GrannyClock *)self->clock_now);
    self->anim_time = self->anim_time + (elapsed + elapsed);
    granny_playback_pose(self->model->playback, self->anim_time);
    *(GrannyClock *)self->clock_last = *(GrannyClock *)self->clock_now;
    if (self->anim == 2 && granny_playback_finished(self->model->playback, 2)) {
        self->anim = 0;
        granny_playback_play(self->model->playback, 0, 0);
    }
}

static void corpse_anim_step(struct GrannyCorpse3d *self)
{
    float elapsed;

    *(GrannyClock *)self->clock_now = _GrannyGetSystemSeconds();
    elapsed = _GrannyGetSecondsElapsed((const GrannyClock *)self->clock_last, (const GrannyClock *)self->clock_now);
    self->anim_time = self->anim_time + (elapsed + elapsed);
    granny_playback_pose(self->model->playback, self->anim_time);
    *(GrannyClock *)self->clock_last = *(GrannyClock *)self->clock_now;
}

unsigned char c3d_granny_game_actor_on_process(struct GameObject *object)
{
    struct C3dGrannyGameActor *self = (struct C3dGrannyGameActor *)object;

    unsigned char result;
    float roty, facing;
    int target, diff, ahead;

    c3d_granny_game_actor_update_motion((struct RenderObject *)self);
    game_actor_process_effect_state(ACTOR(self));
    game_actor_process_will_be_attacked(ACTOR(self));
    game_actor_place_hp_gage(ACTOR(self));
    result = game_actor_process_state((struct GameActor *)self);
    roty = self->roty;
    if (roty > 360.0f) {
        roty = (float)((int)roty % 360);
        self->roty = roty;
    }
    if (self->first_frame == 1) {
        self->first_frame = 0;
        self->facing = 180.0f - roty;
        return result;
    }
    facing = self->facing;
    target = (int)(180.0f - (float)(int)roty);
    if (facing > 360.0f) {
        facing = (float)((int)facing % 360);
        self->facing = facing;
    }
    if (0.0f > facing) {
        facing = facing + 360.0f;
        self->facing = facing;
    }
    diff = (int)(facing - (float)target);
    if (diff > 360 || diff < -360)
        diff %= 360;
    ahead = diff < 0 ? diff + 360 : diff;
    if (ahead <= 12)
        return result;
    self->facing = ahead < 180 ? facing - 6.0f : facing + 6.0f;
    return result;
}

unsigned char granny_corpse3d_on_process(struct GameObject *object)
{
    struct GrannyCorpse3d *self = (struct GrannyCorpse3d *)object;

    int frames = self->frames;

    if (frames > 0x163)
        self->alpha = (unsigned char)(0x58 - (unsigned char)frames);
    self->frames = frames + 1;
    return frames <= 600;
}

void c3d_granny_game_actor_make_corpse(struct GameActor * base_actor)
{
    struct C3dGrannyGameActor * self = (struct C3dGrannyGameActor *)base_actor;

    actor_spawn_granny_corpse(game_current_mode(), self->pos, self->facing, self->name);
}

void *c3d_granny_game_actor_get_emblem_texture(struct C3dGrannyGameActor *self)
{
    struct String name, arg;
    struct EmblemRes *res;
    void *tex;

    guild_mgr_emblem_file_name(g_guildMgr, &name, game_actor_get_guild_id((struct GameActor *)self));
    string_copy(&arg, &name);
    res = res_mgr_get_string(g_resMgr(), arg);
    if (res) {
        if (!self->emblem_tex)
            self->emblem_tex = texture_from_pixels(0x18, 0x18, res->picture.bmp.pixels, 0, 0, 0);
        else
            texture_upload(self->emblem_tex, 0, 0, 0x18, 0x18, res->picture.bmp.pixels, 0, 0);
    }
    tex = self->emblem_tex;
    string_free(&name);
    return tex;
}

static void model_matrix(float *r, const char *name, float facing, const Vec3 *at)
{
    mat_set_rotate_x(r, strstr(name, "90") ? 1.570796f : 3.141592f);
    mat_rotate_y(r, (facing + 180.0f) / 180.0f * 3.141592f);
    mat_translate(r, at);
}

void granny_corpse3d_render(struct GameObject *object, const float *view)
{
    struct GrannyCorpse3d *self = (struct GrannyCorpse3d *)object;

    float mesh_m[12], r[12], m1[12], m2[12], inv[12];
    float lx, ly, lz, k, sx, sy, rhw;
    struct Texture *tex = NULL;
    unsigned short *indices = NULL;
    int tris;
    int i, t, j, n;

    if (!self->model)
        return;
    corpse_anim_step(self);
    memcpy(mesh_m, g_identityMatrix.m, sizeof mesh_m);
    model_matrix(r, self->name, self->facing, &self->pos);
    mat_invert(inv, view);
    lx = ((inv[0] * 0.0f - inv[3] * 300.0f) + inv[6] * 0.0f) + inv[9];
    ly = ((inv[1] * 0.0f - inv[4] * 300.0f) + inv[7] * 0.0f) + inv[10];
    lz = ((inv[2] * 0.0f - inv[5] * 300.0f) + inv[8] * 0.0f) + inv[11];
    k = 1.0f / float_sqrt((ly * ly + lx * lx) + lz * lz);
    lx = lx * k;
    ly = ly * k;
    lz = lz * k;
    renderer_project_point(g_renderer, &self->pos, view, &sx, &sy, &rhw);
    for (i = 0; i < self->model->mesh_count; i++) {
        const struct GrannyMeshVertex *v;
        struct TexDrawVertex *projected;

        v = granny_model_prepare_mesh(self->model, i, NULL, NULL);
        tex = self->model->meshes[i].texture;
        indices = self->model->meshes[i].indices;
        tris = self->model->meshes[i].triangles;
        memcpy(mesh_m, self->model->meshes[i].matrix, sizeof mesh_m);
        projected = granny_projection_vertices(self->model->meshes[i].projection, tris);
        if (!v || !tex || !projected)
            continue;
        mat_multiply(m1, mesh_m, r);
        mat_multiply(m2, m1, view);
        n = 0;
        for (t = 0; t < tris; t++) {
            for (j = 0; j < 3; j++) {
                struct TexDrawVertex *dst = projected + n;
                const struct GrannyMeshVertex *src = &v[indices[n]];
                Vec3 pt;
                unsigned b;

                n++;
                pt = src->pos;
                renderer_project_vertex(g_renderer, &pt, m2, dst);
                memcpy(&dst->u, &src->u, sizeof dst->u);
                memcpy(&dst->v, &src->v, sizeof dst->v);
                b = (unsigned)(light_term((src->nx * lx + src->ny * ly) + src->nz * lz) * 100.0f + 155.0f);
                self->color = b;
                dst->diffuse = (unsigned char)b * 0x10101u + ((unsigned)self->alpha << 24);
            }
        }
        granny_projection_submit(g_renderer, self->model->meshes[i].projection, tex, tris);
    }
}

void c3d_granny_game_actor_render_emblem_model(struct C3dGrannyGameActor *self, const float *view)
{
    float eye[3];
    struct GameMode *mode;
    float mesh_m[12], r[12], z[12], m[12];
    Vec3 light, pt;
    float lx, ly, lz, k, sx, sy, rhw;
    struct ScreenBounds box;
    struct Texture *tex = NULL;
    unsigned short *indices = NULL;
    int tris;
    int i, t, j, n;

    if (self->aid == -1)
        return;
    self->color = ((unsigned)self->alpha << 24) + 0xffffff;
    mode = game_current_mode();
    memcpy(mesh_m, g_identityMatrix.m, sizeof mesh_m);
    model_matrix(r, self->name, self->facing, &self->pos);
    mat_set_rotate_z(z, self->facing / 180.0f * 3.141592f);
    mat_multiply(m, r, view);
    pt.x = 0.0f;
    pt.y = 0.0f;
    pt.z = 0.0f;
    light.x = 0.0f;
    light.y = 0.0f;
    light.z = 0.0f;
    view_get_view_dir(mode->view, &light);
    lx = ((view[0] * light.x + view[3] * light.y) + view[6] * light.z) + view[9];
    ly = ((view[1] * light.x + view[4] * light.y) + view[7] * light.z) + view[10];
    lz = ((view[2] * light.x + view[5] * light.y) + view[8] * light.z) + view[11];
    k = 1.0f / float_sqrt((ly * ly + lx * lx) + lz * lz);
    lx = lx * k;
    ly = ly * k;
    lz = lz * k;
    renderer_project_point(g_renderer, &self->pos, view, &sx, &sy, &rhw);
    box.left = (float)g_renderer->width;
    box.top = (float)g_renderer->height;
    box.right = 0.0f;
    box.bottom = 0.0f;
    box.z = rhw;
    for (i = 0; i < self->model->mesh_count; i++) {
        const struct GrannyMeshVertex *v;
        struct TexDrawVertex *projected;
        struct Texture *drawn;
        float x, y, w;

        v = granny_model_prepare_mesh(self->model, i, NULL, NULL);
        tex = self->model->meshes[i].texture;
        indices = self->model->meshes[i].indices;
        tris = self->model->meshes[i].triangles;
        memcpy(mesh_m, self->model->meshes[i].matrix, sizeof mesh_m);
        projected = granny_projection_vertices(self->model->meshes[i].projection, tris);
        if (!v || !projected)
            continue;
        if (self->job == 0x49d && i == 28) {
            Vec3 at;

            if (granny_mesh_vertex_position(self->model->resource, i, v, 64, &at))
                game_actor_launch_effect(ACTOR(self), 0x175, at, -self->facing);
        }
        n = 0;
        for (t = 0; t < tris; t++) {
            for (j = 0; j < 3; j++) {
                struct TexDrawVertex *dst = projected + n;
                const struct GrannyMeshVertex *src = &v[indices[n]];
                unsigned b;

                n++;
                pt = src->pos;
                renderer_project_vertex(g_renderer, &pt, m, dst);
                memcpy(&dst->u, &src->u, sizeof dst->u);
                memcpy(&dst->v, &src->v, sizeof dst->v);
                b = (unsigned)(light_term((src->nx * lx + src->ny * ly) + src->nz * lz) * 100.0f + 155.0f);
                self->color = b;
                dst->diffuse = (unsigned char)b * 0x10101u + ((unsigned)self->alpha << 24);
            }
        }
        x = pt.x;
        y = pt.y;
        w = pt.z;
        pt.y = ((z[4] * y + z[1] * x) + z[7] * w) + z[10];
        pt.x = (((z[0] * x + z[3] * y) + z[6] * w) + z[9]) + (float)((int)self->pos.x % 5);
        pt.z = (((z[2] * x + z[5] * y) + z[8] * w) + z[11]) + (float)((int)self->pos.z % 5);
        vec_push(&self->shadow_points, &pt, sizeof pt);
        drawn = i == 1 ? c3d_granny_game_actor_get_emblem_texture(self) : tex;
        granny_projection_submit(g_renderer, self->model->meshes[i].projection, drawn, tris);
    }
    if (game_copy_camera_position(eye) && granny_bounds_screen_box(&self->model->bounds, &box, view, r, eye))
        game_actor_set_render_info((struct RenderObject *)self, &box, sx, sy);
    else
        render_object_set_screen_position((struct RenderObject *)self, sx, sy);
}

void c3d_granny_game_actor_render_lit_model(struct C3dGrannyGameActor *self, const float *view,
                           const struct GrannyLight *light)
{
    float eye[3];
    float mesh_m[12], r[12], m1[12], m2[12], inv[12];
    Vec3 pt;
    float lx, ly, lz, k, sx, sy, rhw;
    struct ScreenBounds box;
    struct Texture *tex = NULL;
    unsigned short *indices = NULL;
    int tris;
    int i, t, j, n;

    if (self->aid == -1)
        return;
    memcpy(mesh_m, g_identityMatrix.m, sizeof mesh_m);
    mat_set_rotate_x(r, strstr(self->name, "90") ? 1.570796f : 3.141592f);
    mat_rotate_y(r, (self->facing + 180.0f) / 180.0f * 3.141592f);
    pt.x = self->pos.x - 2.0f;
    pt.y = self->pos.y;
    pt.z = self->pos.z - 2.0f;
    mat_translate(r, &pt);
    renderer_project_point(g_renderer, &self->pos, view, &sx, &sy, &rhw);
    mat_invert(inv, view);
    lx = ((light->dir.x * inv[0] + light->dir.y * inv[3]) + light->dir.z * inv[6]) + inv[9];
    ly = ((light->dir.x * inv[1] + inv[4] * light->dir.y) + light->dir.z * inv[7]) + inv[10];
    lz = ((inv[2] * light->dir.x + inv[5] * light->dir.y) + light->dir.z * inv[8]) + inv[11];
    k = 1.0f / float_sqrt((ly * ly + lx * lx) + lz * lz);
    lx = k * lx;
    ly = k * ly;
    lz = k * lz;
    box.left = (float)g_renderer->width;
    box.top = (float)g_renderer->height;
    box.right = 0.0f;
    box.bottom = 0.0f;
    box.z = rhw;
    for (i = 0; i < self->model->mesh_count; i++) {
        const struct GrannyMeshVertex *v;
        struct TexDrawVertex *projected;

        v = granny_model_prepare_mesh(self->model, i, NULL, NULL);
        tex = self->model->meshes[i].texture;
        indices = self->model->meshes[i].indices;
        tris = self->model->meshes[i].triangles;
        memcpy(mesh_m, self->model->meshes[i].matrix, sizeof mesh_m);
        projected = granny_projection_vertices(self->model->meshes[i].projection, tris);
        if (!v || !tex || !projected)
            continue;
        mat_multiply(m1, mesh_m, r);
        mat_multiply(m2, m1, view);
        if (self->job == 0x49d && i == 28) {
            Vec3 at;

            if (granny_mesh_vertex_position(self->model->resource, i, v, 64, &at))
                game_actor_launch_effect(ACTOR(self), 0x175, at, -self->facing);
        }
        n = 0;
        for (t = 0; t < tris; t++) {
            for (j = 0; j < 3; j++) {
                struct TexDrawVertex *dst = projected + n;
                const struct GrannyMeshVertex *src = &v[indices[n]];
                int a = light->intensity;
                unsigned b;

                n++;
                pt = src->pos;
                renderer_project_vertex(g_renderer, &pt, m2, dst);
                memcpy(&dst->u, &src->u, sizeof dst->u);
                memcpy(&dst->v, &src->v, sizeof dst->v);
                b = (unsigned)(light_term((src->nx * lx + src->ny * ly) + src->nz * lz) * (float)a + (float)(0xff - a));
                self->color = b;
                dst->diffuse = (((light->g + 0x38 + b) << 8) & 0xff00) + (((light->r + 0x38 + b) << 16) & 0xff0000) +
                             (unsigned char)(light->b + 0x38 + b) + ((unsigned)self->alpha << 24);
            }
            vec_push(&self->shadow_points, &pt, sizeof pt);
        }
        granny_projection_submit(g_renderer, self->model->meshes[i].projection, tex, tris);
    }
    if (game_copy_camera_position(eye) && granny_bounds_screen_box(&self->model->bounds, &box, view, r, eye))
        game_actor_set_render_info((struct RenderObject *)self, &box, sx, sy);
    else
        render_object_set_screen_position((struct RenderObject *)self, sx, sy);
}

void c3d_granny_game_actor_render(struct GameObject *object, const float *view)
{
    struct C3dGrannyGameActor *self = (struct C3dGrannyGameActor *)object;

    Vec3 tmp, *sun;
    struct GrannyLight light;

    if (!self->model)
        return;
    self->shadow_points.last = self->shadow_points.first;
    actor_anim_step(self);
    sun = weather_sun_position(&g_weather, &tmp);
    light.r = 200;
    light.g = 200;
    light.b = 200;
    light.dir = *sun;
    light.intensity = 0x80;
    if (self->hidden_state == 1 || self->hidden_state == 2)
        return;
    if (self->model_kind == 2)
        c3d_granny_game_actor_render_emblem_model(self, view);
    else
        c3d_granny_game_actor_render_lit_model(self, view, &light);
}
