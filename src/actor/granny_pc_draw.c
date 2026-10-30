#include "render/texture.h"
#include "actor/object_ops.h"
#include "render/rp_pool.h"
#include "game/context.h"
#include <string.h>
#include "actor/actor.h"
#include "actor/pc.h"
#include "std/std.h"
#include "actor/weather.h"
#include "actor/sprite_render.h"
#include "actor/granny_pc.h"

void granny_pc_render(struct GameObject *object, const float *matrix)
{
    struct GrannyPc *self = (struct GrannyPc *)object;

    struct GrannyLight light;
    Vec3 sun, shadow;
    int i;

    if (!self->model)
        return;
    light.dir = *weather_sun_position(&g_weather, &sun);
    light.intensity = 0x80;
    light.r = light.g = light.b = 0xc8;
    shadow.x = 1800.0f;
    shadow.y = -1000.0f;
    shadow.z = 0.0f;
    if (self->draw_phase == 0) {
        granny_pc_render_part(self, matrix, &light, 0, 0, 0);
        for (i = 1; i < 10; i++) {
            struct GrannyModelState *model = self->parts[i];

            if (model)
                granny_pc_render_part(self, matrix, &light, model, i, i == 1 ? model : 0);
        }
        granny_pc_render_shadow(self, matrix, 0);
    } else {
        granny_pc_render_part_kept(self, matrix, 0, 0);
        for (i = 1; i < 10; i++)
            if (self->parts[i])
                granny_pc_render_part_kept(self, matrix, i, self->parts[i]);
        granny_pc_render_shadow(self, matrix, 1);
    }
    if (++self->draw_phase == 3)
        self->draw_phase = 0;
}

static unsigned granny_pc_shadow_image[35 * 35];

static __inline float granny_pc_light_term(float d)
{
    float t = 1.0f < d ? 1.0f : d;

    return 0.0f > t ? 0.0f : t;
}

static void granny_pc_model_matrix(struct GrannyPc *self, float *r)
{
    Vec3 at;

    at.x = 0.0f;
    at.y = 0.0f;
    at.z = 0.0f;
    mat_set_rotate_x(r, 1.5707964f);
    mat_rotate_y(r, (self->facing + 180.0f) / 180.0f * 3.141592f);
    mat_scale(r, 0.3f, 0.3f, 0.3f);
    at.y = self->pos.y;
    at.x = self->pos.x - 2.0f;
    at.z = self->pos.z - 2.0f;
    mat_translate(r, &at);
}

static void granny_pc_through(Vec3 *pt, const float *m)
{
    float x = pt->x, y = pt->y, z = pt->z;

    pt->x = ((m[3] * y + m[0] * x) + m[6] * z) + m[9];
    pt->y = ((m[4] * y + m[1] * x) + m[7] * z) + m[10];
    pt->z = ((m[5] * y + m[2] * x) + m[8] * z) + m[11];
}

void granny_pc_render_part(struct GrannyPc *self, const float *matrix,
                           const struct GrannyLight *light, struct GrannyModelState *model, int part, struct GrannyModelState *pose_model)
{
    float eye[3];
    granny_world_pose *instance;
    float mesh_m[12], r[12], m1[12], m2[12], inv[12];
    Vec3 pt;
    float lx, ly, lz, k, sx, sy, rhw;
    struct ScreenBounds box;
    struct Texture *tex = NULL;
    unsigned short *indices = NULL;
    int tris;
    unsigned char kept = 0;
    int i, t, j, n;

    if (!self->visible || self->aid == -1 || part < 0 || part >= 10 || (!model && !self->model))
        return;
    self->color = ((unsigned)self->alpha << 24) + 0xffffff;
    instance = NULL;
    if (!model) {
        model = self->model;
        self->body_pose = granny_pc_pose(self, NULL);
    } else {
        instance = pose_model ? granny_pc_pose(self, pose_model) : self->body_pose;
    }
    granny_pc_model_matrix(self, r);
    renderer_project_point(g_renderer, &self->pos, matrix, &sx, &sy, &rhw);
    mat_invert(inv, matrix);
    lx = ((light->dir.x * inv[0] + light->dir.y * inv[3]) + light->dir.z * inv[6]) + inv[9];
    ly = ((light->dir.x * inv[1] + light->dir.y * inv[4]) + light->dir.z * inv[7]) + inv[10];
    lz = ((light->dir.x * inv[2] + light->dir.y * inv[5]) + inv[8] * light->dir.z) + inv[11];
    k = 1.0f / float_sqrt((ly * ly + lx * lx) + lz * lz);
    lx = k * lx;
    ly = k * ly;
    lz = k * lz;
    box.left = (float)g_renderer->width;
    box.top = (float)g_renderer->height;
    box.right = 0.0f;
    box.bottom = 0.0f;
    box.z = rhw;
    for (i = 0; i < model->mesh_count; i++) {
        struct TexDrawVertex *projected;
        const struct GrannyMeshVertex *v;

        v = granny_model_prepare_mesh(model, i, instance, part == 0 ? &self->hair_pivot : NULL);
        tex = model->meshes[i].texture;
        indices = model->meshes[i].indices;
        tris = model->meshes[i].triangles;
        memcpy(mesh_m, model->meshes[i].matrix, sizeof mesh_m);
        projected = granny_projection_vertices(model->meshes[i].projection, tris);
        if (!v || !tex || !projected)
            continue;
        if (part == self->hair_pivot.part) {
            kept = 1;
            mat_multiply(m1, self->hair_pivot.matrix, r);
        } else {
            mat_multiply(m1, mesh_m, r);
        }
        mat_multiply(m2, m1, matrix);
        n = 0;
        for (t = 0; t < tris; t++) {
            for (j = 0; j < 3; j++) {
                const struct GrannyMeshVertex *src = &v[indices[n]];
                struct TexDrawVertex *dst = projected + n;
                int a;
                unsigned b;

                n++;
                pt = src->pos;
                if (kept == 1 && i > 0)
                    granny_pc_through(&pt, mesh_m);
                renderer_project_vertex(g_renderer, &pt, m2, dst);
                memcpy(&dst->u, &src->u, sizeof dst->u);
                memcpy(&dst->v, &src->v, sizeof dst->v);
                a = light->intensity;
                b = (unsigned)(granny_pc_light_term(1.0f - ((src->nx * lx + src->ny * ly) + src->nz * lz)) * (float)a
                               + (float)(0xff - a));
                self->color = b;
                dst->diffuse = (((light->g + 0x38 + b) << 8) & 0xff00) + (((light->r + 0x38 + b) << 16) & 0xff0000)
                             + (unsigned char)(light->b + 0x38 + b) + ((unsigned)self->alpha << 24);
            }
            if (part != 1) {
                struct Vec *sp = &self->shadow_points;

                vec_push(sp, &pt, sizeof(Vec3));
            }
        }
        granny_projection_submit(g_renderer, model->meshes[i].projection, tex, tris);
    }
    if (model == self->model) {
        if (game_copy_camera_position(eye) && granny_bounds_screen_box(&self->model->bounds, &box, matrix, r,
                       eye))
            game_actor_set_render_info((struct RenderObject *)self, &box, sx, sy);
        else
            render_object_set_screen_position((struct RenderObject *)self, sx, sy);
    }
}

void granny_pc_render_part_kept(struct GrannyPc *self, const float *matrix, int part, struct GrannyModelState *model)
{
    float eye[3];
    float r[12], m1[12], m2[12];
    Vec3 pt;
    float sx, sy, rhw;
    struct ScreenBounds box;
    unsigned char kept = 0;
    int i, t, j, n;

    if (!self->visible || self->aid == -1 || part < 0 || part >= 10 || (!model && !self->model))
        return;
    self->color = ((unsigned)self->alpha << 24) + 0xffffff;
    granny_pc_model_matrix(self, r);
    renderer_project_point(g_renderer, &self->pos, matrix, &sx, &sy, &rhw);
    box.left = (float)g_renderer->width;
    box.top = (float)g_renderer->height;
    box.right = 0.0f;
    box.bottom = 0.0f;
    box.z = rhw;
    if (!model) {
        model = self->model;
    }
    for (i = 0; i < model->mesh_count; i++) {
        struct TexDrawVertex *projected;
        const unsigned short *indices;
        const struct GrannyMeshVertex *v = granny_model_cached_vertices(model, i);

        if (!v)
            continue;
        if (!model->meshes[i].texture)
            continue;
        if (part == self->hair_pivot.part)
            kept = 1;
        mat_multiply(m1, part == self->hair_pivot.part ? self->hair_pivot.matrix : model->meshes[i].matrix, r);
        mat_multiply(m2, m1, matrix);
        indices = model->meshes[i].indices;
        projected = granny_projection_vertices(model->meshes[i].projection, model->meshes[i].triangles);
        if (!projected)
            continue;
        n = 0;
        for (t = model->meshes[i].triangles; t > 0; t--) {
            for (j = 0; j < 3; j++) {
                struct TexDrawVertex *dst = projected + n;
                const struct GrannyMeshVertex *src = &v[indices[n]];

                n++;
                pt = src->pos;
                if (kept == 1 && i > 0) {
                    const float *m = model->meshes[i].matrix;
                    float x = pt.x, y = pt.y, z = pt.z;

                    pt.x = ((m[0] * x + m[3] * y) + z * m[6]) + m[9];
                    pt.y = ((m[4] * y + m[1] * x) + m[7] * z) + m[10];
                    pt.z = ((m[5] * y + m[2] * x) + m[8] * z) + m[11];
                }
                renderer_project_vertex(g_renderer, &pt, m2, dst);
            }
        }
        granny_projection_submit(g_renderer, model->meshes[i].projection, model->meshes[i].texture, model->meshes[i].triangles);
    }
    if (model == self->model) {
        if (game_copy_camera_position(eye) && granny_bounds_screen_box(&self->model->bounds, &box, matrix, r, eye))
            game_actor_set_render_info((struct RenderObject *)self, &box, sx, sy);
        else
            render_object_set_screen_position((struct RenderObject *)self, sx, sy);
    }
}

void granny_pc_render_shadow(struct GrannyPc *self, const float *matrix, int reuse)
{
    struct GameMode *mode = game_current_mode();
    float s = get_sin_deg((int)self->facing + 90);
    float c = get_cos_deg((int)self->facing + 90);
    int cell_x, cell_y, sub_x, sub_y;
    struct C3dGround15 *ground;
    float sn, cs;
    int j, k;

    if (!(unsigned char)reuse) {
        float wx, wz;
        int ox, oz;
        const Vec3 *p;

        game_mode_world_to_sub_cell(mode, self->pos.x, self->pos.z, &cell_x, &cell_y, &sub_x, &sub_y);
        game_mode_cell_to_world(mode, cell_x, cell_y, &wx, &wz);
        ox = (int)(self->pos.x - wx);
        oz = (int)(self->pos.z - wz);
        memset(granny_pc_shadow_image, 0, 0x1324);
        for (p = (const Vec3 *)self->shadow_points.first; p != (const Vec3 *)self->shadow_points.last; p++) {
            float x = p->x * 0.35f;
            float y = p->y * 0.35f;
            unsigned gx = (unsigned)((int)((x * s + y * c) + 17.0f) + ox);
            unsigned gy = (unsigned)((int)((x * c - y * s) + 17.0f) + oz);

            if (gx <= 0x21 && gy <= 0x21) {
                unsigned at = gy * 35 + gx;

                granny_pc_shadow_image[at + 1] |= 0x2f000000;
                granny_pc_shadow_image[at + 35] |= 0x2f000000;
                granny_pc_shadow_image[at] = 0x5f000000;
            }
        }
        self->shadow_points.last = self->shadow_points.first;
        texture_upload(self->shadow_tex, 0, 0, 0x23, 0x23, granny_pc_shadow_image, 0, 0);
    }
    game_mode_world_to_sub_cell(mode, self->pos.x, self->pos.z, &cell_x, &cell_y, &sub_x, &sub_y);
    ground = game_current_ground();
    cell_x -= 2;
    cell_y -= 2;
    sn = float_sin(0.0f);
    cs = float_cos(0.0f);
    for (j = 0; j < 5; j++) {
        float j0 = (float)j - 2.5f;
        float j1 = (float)(j + 1) - 2.5f;

        for (k = 0; k < 5; k++) {
            float k0 = (float)k - 2.5f;
            float k1 = (float)(k + 1) - 2.5f;
            float uv[8];

            uv[0] = ((j0 * sn + k0 * cs) + 2.5f) / 5.0f;
            uv[1] = ((j0 * cs - k0 * sn) + 2.5f) / 5.0f;
            uv[2] = ((k1 * cs + j0 * sn) + 2.5f) / 5.0f;
            uv[3] = ((j0 * cs - k1 * sn) + 2.5f) / 5.0f;
            uv[4] = ((j1 * sn + k0 * cs) + 2.5f) / 5.0f;
            uv[5] = ((j1 * cs - k0 * sn) + 2.5f) / 5.0f;
            uv[6] = ((j1 * sn + k1 * cs) + 2.5f) / 5.0f;
            uv[7] = ((j1 * cs - k1 * sn) + 2.5f) / 5.0f;
            c3d_ground15_draw_attr_quad(ground, matrix, cell_x + k, cell_y + j, 0x70ffffff, self->shadow_tex, uv);
        }
    }
}
