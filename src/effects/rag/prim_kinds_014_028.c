#include "render/projection.h"
#include "render/rp_pool.h"
#include <stdlib.h>
#include <string.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"

static Vec3 vec3_add(Vec3 a, Vec3 b)
{
    Vec3 r;

    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}

#define FLIGHT_TEX_STRIP     0x002
#define FLIGHT_GROUND_STOP   0x004
#define FLIGHT_SET_OFFSET    0x010
#define FLIGHT_SIZE2_FLIP    0x020
#define FLIGHT_FACE_CAMERA   0x040
#define FLIGHT_RAW_VELOCITY  0x100
#define FLIGHT_CAMERA_PITCH  0x200
#define FLIGHT_HOME          0x400
#define FLIGHT_COLOR_HOLD    0x800
#define FLIGHT_QUAD_UPPER_HALF 0x008
#define FLIGHT_QUAD_ABOVE    0x080

#define FLIGHT_STRIP_FRAME(p) ((p)->pattern_arg)
#define FLIGHT_STRIP_FRAMES 10

static unsigned char flight_step(struct EffectPrim *self, Vec3 base)
{
    float *m2 = self->matrix2;
    float heading;
    int hold;

    self->velocity.x += self->velocity_accel.x;
    self->velocity.y += self->velocity_accel.y;
    self->velocity.z += self->velocity_accel.z;
    self->size[1].speed += self->size[1].accel;
    self->speed += self->accel;
    self->size[2].speed += self->size[2].accel;
    self->angle_speed[0] += self->angle_accel[0];
    self->angle_speed[1] += self->angle_accel[1];

    if (self->pattern & FLIGHT_HOME) {
        self->velocity.x = 0.0f;
        self->velocity.y = 0.0f;
        self->velocity.z = 0.0f;
        self->velocity = prim_mat_apply(self->matrix, 0.0f, 0.0f, self->speed);
    } else if (!(self->pattern & FLIGHT_RAW_VELOCITY)) {
        self->velocity = prim_mat_apply(self->matrix, 0.0f, 0.0f, self->speed);
    }
    self->delta.x += self->velocity.x;
    self->delta.y += self->velocity.y;
    self->delta.z += self->velocity.z;

    if (self->pattern & FLIGHT_HOME) {
        float d = render_object_calc_ground_dist(&self->base, self->master->offset.x, self->master->offset.z);
        if (3.0f >= d) {
            self->arrived = 1;
            return 0;
        }
        self->angle[0] = self->base.roty;
    }
    self->base.pos.z = base.z + self->delta.z;
    self->base.pos.y = base.y + self->delta.y;
    self->base.pos.x = base.x + self->delta.x;

    self->size[1].value += self->size[1].speed;
    self->size[2].value += self->size[2].speed;
    self->angle[0] += self->angle_speed[0];
    self->angle[1] += self->angle_speed[1];
    if (self->size[2].value > self->size2_max) {
        self->size[2].accel = 0.0f;
        self->size[2].speed = 0.0f;
    }

    if (self->pattern & FLIGHT_CAMERA_PITCH) {
        mat_set_rotate_z(m2, game_camera_pitch());
        heading = self->base.roty;
    } else {
        if (self->pattern & FLIGHT_FACE_CAMERA) {
            self->angle[0] = game_camera_heading() + 90.0f;
            self->angle[2] = game_camera_pitch();
            mat_set_rotate_x(m2, deg_to_rad((int)self->angle[1]));
            mat_rotate_z(m2, deg_to_rad((int)self->angle[2]));
        } else {
            mat_set_rotate_z(m2, deg_to_rad((int)self->angle[2]));
            mat_rotate_x(m2, deg_to_rad((int)self->angle[1]));
        }
        heading = self->angle[0];
    }
    mat_rotate_y(m2, deg_to_rad((int)heading));

    if (self->pattern & FLIGHT_TEX_STRIP) {
        int frame = FLIGHT_STRIP_FRAME(self);
        float v = (float)frame / 10.0f;
        frame++;
        FLIGHT_STRIP_FRAME(self) = frame == FLIGHT_STRIP_FRAMES ? 0 : frame;
        self->tex_v_top = v;
        self->tex_v_bottom = v + 1.0f;
    }
    if (self->pattern & FLIGHT_SET_OFFSET) {
        self->master->offset.x = self->delta2.x + self->delta.x;
        self->master->offset.y = self->delta2.y + self->delta.y;
        self->master->offset.z = self->delta2.z + self->delta.z;
    }
    if (self->pattern & FLIGHT_GROUND_STOP) {
        struct C3dAttr *ground = game_current_map_attr();
        if (ground && self->base.pos.y > c3d_attr_get_height(ground, self->base.pos.x, self->base.pos.z)) {
            self->pattern &= ~FLIGHT_GROUND_STOP;
            self->frame = self->duration - 25;
            self->speed = 0.0f;
            self->master->id_state = 1;
        }
    }
    if ((self->pattern & FLIGHT_SIZE2_FLIP) && self->frame % self->size2_flip_period == 0)
        self->size[2].speed = self->size[2].speed * -1.0f;

    hold = self->pattern & FLIGHT_COLOR_HOLD;
    if (hold && (self->r == 254 || self->b == 254 || self->g == 254)) {
        if (self->color_hold_frames != 0)
            self->color_hold_count++;
        if (self->color_hold_frames == self->color_hold_count) {
            self->r += self->r_speed;
            self->color_hold_count = 0;
            self->g += self->g_speed;
            self->b += self->b_speed;
        }
    } else {
        self->r += self->r_speed;
        self->g += self->g_speed;
        self->b += self->b_speed;
    }
    if (hold) {
        if (self->r > 254)
            self->r = 0;
        else if (self->r < 0)
            self->r = 254;
        if (self->g > 254)
            self->g = 0;
        else if (self->g < 0)
            self->g = 254;
        if (self->b > 254)
            self->b = 0;
        else if (self->b < 0)
            self->b = 254;
    } else {
        if (self->r > 255)
            self->r = 0;
        else if (self->r < 0)
            self->r = 255;
        if (self->g > 255)
            self->g = 255;
        else if (self->g < 0)
            self->g = 0;
        if (self->b > 255)
            self->b = 255;
        else if (self->b < 0)
            self->b = 0;
    }

    prim_update_alpha(self);
    return prim_step_motion_counted(self);
}

unsigned char prim_flight_process(struct EffectPrim *self)
{
    self->master_pos.x = self->master->base.pos.x + self->delta2.x;
    self->master_pos.y = self->master->base.pos.y + self->delta2.y;
    self->master_pos.z = self->master->base.pos.z + self->delta2.z;
    if (self->change_frame == self->frame) {
        self->change_reached = 1;
        if (self->next_size1_speed != PRIM_UNSET)
            self->size[1].speed = self->next_size1_speed;
        if (self->next_size2_speed != PRIM_UNSET)
            self->size[2].speed = self->next_size2_speed;
        if (self->next_speed != PRIM_UNSET)
            self->speed = self->next_speed;
        if (self->next_size1_accel != PRIM_UNSET)
            self->size[1].accel = self->next_size1_accel;
        if (self->next_size2_accel != PRIM_UNSET)
            self->size[2].accel = self->next_size2_accel;
        if (self->next_accel != PRIM_UNSET)
            self->accel = self->next_accel;
        if (self->next_velocity.x != PRIM_UNSET)
            self->velocity.x = self->next_velocity.x;
        if (self->next_velocity.y != PRIM_UNSET)
            self->velocity.y = self->next_velocity.y;
        if (self->next_velocity.z != PRIM_UNSET)
            self->velocity.z = self->next_velocity.z;
        self->next_size1_speed = PRIM_UNSET;
        self->next_size2_speed = PRIM_UNSET;
        self->next_speed = PRIM_UNSET;
        self->next_size1_accel = PRIM_UNSET;
        self->next_size2_accel = PRIM_UNSET;
        self->next_accel = PRIM_UNSET;
        self->next_velocity.x = PRIM_UNSET;
        self->next_velocity.y = PRIM_UNSET;
        self->next_velocity.z = PRIM_UNSET;
        self->next_velocity_accel.x = PRIM_UNSET;
        self->next_velocity_accel.y = PRIM_UNSET;
        self->next_velocity_accel.z = PRIM_UNSET;
    }
    return flight_step(self, self->master_pos);
}

unsigned char prim_flight_fixed_process(struct EffectPrim *self)
{
    Vec3 base;

    base.x = self->master_pos.x + self->delta2.x;
    base.z = self->master_pos.z + self->delta2.z;
    base.y = self->master_pos.y + self->delta2.y;
    if (self->change_frame == self->frame) {
        if (self->next_size1_speed != PRIM_UNSET)
            self->size[1].speed = self->next_size1_speed;
        if (self->next_size2_speed != PRIM_UNSET)
            self->size[2].speed = self->next_size2_speed;
        if (self->next_speed != PRIM_UNSET)
            self->speed = self->next_speed;
        if (self->next_size1_accel != PRIM_UNSET)
            self->size[1].accel = self->next_size1_accel;
        if (self->next_size2_accel != PRIM_UNSET)
            self->size[2].accel = self->next_size2_accel;
        if (self->next_accel != PRIM_UNSET)
            self->accel = self->next_accel;
        if (self->next_velocity.x != PRIM_UNSET)
            self->velocity.x = self->next_velocity.x;
        if (self->next_velocity.y != PRIM_UNSET)
            self->velocity.y = self->next_velocity.y;
        if (self->next_velocity.z != PRIM_UNSET)
            self->velocity.z = self->next_velocity.z;
        if (self->next_velocity_accel.x != PRIM_UNSET)
            self->velocity_accel.x = self->next_velocity_accel.x;
        if (self->next_velocity_accel.y != PRIM_UNSET)
            self->velocity_accel.y = self->next_velocity_accel.y;
        if (self->next_velocity_accel.z != PRIM_UNSET)
            self->velocity_accel.z = self->next_velocity_accel.z;
    }
    return flight_step(self, base);
}

void prim_upright_quad_render(struct EffectPrim *self, const float *view)
{
    struct PrimTriangle *tri = self->triangles;
    float w = self->size[1].value;
    float h = self->size[2].value;
    float top, bottom = h;
    Vec3 c0, c1, c2, c3;

    if (self->pattern & FLIGHT_QUAD_ABOVE) {
        top = h * -2.0f;
        bottom = 0.0f;
    } else {
        top = -h;
    }
    if (self->pattern & FLIGHT_QUAD_UPPER_HALF)
        bottom = 0.0f;
    c0 = prim_mat_apply(self->matrix2, 0.0f, top, w);
    c1 = prim_mat_apply(self->matrix2, 0.0f, top, -w);
    c2 = prim_mat_apply(self->matrix2, 0.0f, bottom, w);
    c3 = prim_mat_apply(self->matrix2, 0.0f, bottom, -w);

    prim_set_vertex(&tri[0].v[0], view, vec3_add(self->base.pos, c0), self->argb, 0.0f, self->tex_v_top);
    prim_set_vertex(&tri[0].v[1], view, vec3_add(self->base.pos, c1), self->argb, 1.0f, self->tex_v_top);
    prim_set_vertex(&tri[0].v[2], view, vec3_add(self->base.pos, c2), self->argb, 0.0f, self->tex_v_bottom);
    tri[0].texture = self->textures[self->base.motion];
    prim_set_vertex(&tri[1].v[0], view, vec3_add(self->base.pos, c3), self->argb, 1.0f, self->tex_v_bottom);
    prim_set_vertex(&tri[1].v[1], view, vec3_add(self->base.pos, c2), self->argb, 0.0f, self->tex_v_bottom);
    prim_set_vertex(&tri[1].v[2], view, vec3_add(self->base.pos, c1), self->argb, 1.0f, self->tex_v_top);
    tri[1].texture = self->textures[self->base.motion];
    renderer_add_triangle(g_renderer, &tri[0], self->list_flags);
    renderer_add_triangle(g_renderer, &tri[1], self->list_flags);
}

void prim_crossed_quads_render(struct EffectPrim *self, const float *view)
{
    struct PrimTriangle *tri = self->triangles;
    float h = self->size[2].value;
    float w = self->size[1].value;
    Vec3 quad[2][4];
    int i;

    quad[0][0].x = -h; quad[0][0].y = 0.0f; quad[0][0].z = w;
    quad[0][1].x = -h; quad[0][1].y = 0.0f; quad[0][1].z = -w;
    quad[0][2].x = h;  quad[0][2].y = 0.0f; quad[0][2].z = w;
    quad[0][3].x = h;  quad[0][3].y = 0.0f; quad[0][3].z = -w;
    quad[1][0].x = 0.0f; quad[1][0].y = -h; quad[1][0].z = w;
    quad[1][1].x = 0.0f; quad[1][1].y = -h; quad[1][1].z = -w;
    quad[1][2].x = 0.0f; quad[1][2].y = h;  quad[1][2].z = w;
    quad[1][3].x = 0.0f; quad[1][3].y = h;  quad[1][3].z = -w;

    for (i = 0; i < 2; i++) {
        const Vec3 *q = quad[i];
        Vec3 c1 = prim_mat_apply(self->matrix, q[1].x, q[1].y, q[1].z);
        Vec3 c2 = prim_mat_apply(self->matrix, q[2].x, q[2].y, q[2].z);
        Vec3 c3 = prim_mat_apply(self->matrix, q[3].x, q[3].y, q[3].z);
        Vec3 c0 = prim_mat_apply(self->matrix, q[0].x, q[0].y, q[0].z);

        prim_set_vertex(&tri[0].v[0], view, vec3_add(self->base.pos, c0), self->argb, 0.0f, 1.0f);
        prim_set_vertex(&tri[0].v[1], view, vec3_add(self->base.pos, c1), self->argb, 1.0f, 1.0f);
        prim_set_vertex(&tri[0].v[2], view, vec3_add(self->base.pos, c2), self->argb, 0.0f, 0.0f);
        tri[0].texture = self->textures[self->base.motion];
        renderer_add_triangle(g_renderer, &tri[0], 1);
        prim_set_vertex(&tri[1].v[0], view, vec3_add(self->base.pos, c3), self->argb, 1.0f, 0.0f);
        prim_set_vertex(&tri[1].v[1], view, vec3_add(self->base.pos, c2), self->argb, 0.0f, 0.0f);
        prim_set_vertex(&tri[1].v[2], view, vec3_add(self->base.pos, c1), self->argb, 1.0f, 1.0f);
        tri[1].texture = self->textures[self->base.motion];
        renderer_add_triangle(g_renderer, &tri[1], 1);
        tri += 2;
    }
}

void prim_sphere_render(struct EffectPrim *self, const float *view)
{
    float step = self->arc_degrees;
    float du = 1.0f / (360.0f / step);
    float lat_max = 90.0f - step;
    float dv = 1.0f / (180.0f / step);
    float lon_max = 360.0f - step;
    float lat = -90.0f;
    float v_top = 0.0f;
    int index = 0;

    if (!(lat_max >= lat))
        return;
    do {
        float v_bottom = dv + v_top;
        float u = 0.0f;
        float lon = 0.0f;

        if (lon_max >= lon) {
            int ilat = (int)lat;
            struct PrimTriangle *tri = &self->triangles[index];
            do {
                float m1[12], m2[12], m3[12], m4[12];
                float a_lon = deg_to_rad((int)(self->angle[0] + lon));
                float a_lat = deg_to_rad(ilat);
                float a_lon2 = deg_to_rad((int)(lon + self->arc_degrees + 0.25f + self->angle[0]));
                float a_lat2 = deg_to_rad((int)(lat + self->arc_degrees));
                Vec3 p1, p2, p3, p4;
                float u2;

                mat_set_rotate_x(m1, a_lat);
                mat_rotate_y(m1, a_lon);
                p1 = prim_mat_apply(m1, 0.0f, 0.0f, self->size[0].value);
                mat_set_rotate_x(m2, a_lat2);
                mat_rotate_y(m2, a_lon);
                p2 = prim_mat_apply(m2, 0.0f, 0.0f, self->size[0].value);
                mat_set_rotate_x(m3, a_lat);
                mat_rotate_y(m3, a_lon2);
                p3 = prim_mat_apply(m3, 0.0f, 0.0f, self->size[0].value);
                mat_set_rotate_x(m4, a_lat2);
                mat_rotate_y(m4, a_lon2);
                p4 = prim_mat_apply(m4, 0.0f, 0.0f, self->size[0].value);
                p1 = vec3_add(p1, self->base.pos);
                p2 = vec3_add(p2, self->base.pos);
                p3 = vec3_add(p3, self->base.pos);
                p4 = vec3_add(p4, self->base.pos);

                u2 = u + du;
                prim_set_vertex(&tri[0].v[0], view, p1, self->argb, u, v_top);
                prim_set_vertex(&tri[0].v[1], view, p2, self->argb, u, v_bottom);
                prim_set_vertex(&tri[0].v[2], view, p3, self->argb, u2, v_top);
                tri[0].texture = self->textures[0];
                renderer_add_triangle(g_renderer, &tri[0], 1);
                prim_set_vertex(&tri[1].v[0], view, p4, self->argb, u2, v_bottom);
                prim_set_vertex(&tri[1].v[1], view, p3, self->argb, u2, v_top);
                prim_set_vertex(&tri[1].v[2], view, p2, self->argb, u, v_bottom);
                tri[1].texture = self->textures[0];
                renderer_add_triangle(g_renderer, &tri[1], 1);

                tri += 2;
                index += 2;
                u = u2;
                lon = self->arc_degrees + lon;
            } while (lon_max >= lon);
        }
        lat = self->arc_degrees + lat;
        v_top = v_bottom;
    } while (lat_max >= lat);
}

#define PATTERN_BAND_CLOSE_TOP 0x001

unsigned char prim_rising_band_process(struct EffectPrim *self)
{
    int tilt;

    self->master_pos.z = self->master->base.pos.z + self->delta2.z;
    self->master_pos.x = self->master->base.pos.x + self->delta2.x;
    self->master_pos.y = self->master->base.pos.y + self->delta2.y;
    self->speed = self->accel + self->speed;
    tilt = (int)self->angle[1];
    self->radius_speed[1] = self->radius_accel[1] + self->radius_speed[1];
    self->radius_speed[0] = self->radius_accel[0] + self->radius_speed[0];
    self->size[2].speed = self->size[2].accel + self->size[2].speed;

    mat_set_rotate_x(self->matrix, deg_to_rad(tilt));
    mat_rotate_y(self->matrix, deg_to_rad((int)self->angle[0]));
    self->velocity = prim_mat_apply(self->matrix, 0.0f, -self->speed, 0.0f);
    self->delta.x += self->velocity.x;
    self->delta.y += self->velocity.y;
    self->delta.z += self->velocity.z;
    self->base.pos.x = self->delta.x + self->master_pos.x;
    self->base.pos.y = self->master_pos.y + self->delta.y;
    self->base.pos.z = self->master_pos.z + self->delta.z;

    self->angle[0] = self->angle_speed[0] + self->angle[0];
    self->radius[1] = self->radius_speed[1] + self->radius[1];
    self->radius[0] = self->radius_speed[0] + self->radius[0];
    self->size[2].value = self->size[2].value + self->size[2].speed;
    if (self->size[2].value > self->size2_max)
        self->size[2].value = self->size2_max;
    if ((self->pattern & PATTERN_BAND_CLOSE_TOP) && self->frame == self->fade_frame)
        self->radius_speed[1] = -(self->radius[1] / (float)(self->duration - self->frame + 1));

    prim_update_alpha(self);
    return prim_step_motion_counted(self);
}

#define LIGHT_GROUP_COUNT 4
#define LIGHT_GROUP_STYLE_GREEN 2
#define LIGHT_ORBIT_RADIUS 20.0f
#define LIGHT_WANDER_PUSH 2.5f

static const int k_light_slot[4] = {2, 0, 3, 1};

static void wander_step(struct PrimSway *sway)
{
    float angle = sway->angle, target = sway->target;
    int range, pick;

    if (target > angle) {
        angle += 1.0f;
        if (angle > 359.0f)
            angle = 359.0f;
        sway->angle = angle;
        if (angle > target) {
            range = (int)angle;
            if (range == 0) {
                pick = 0;
            } else {
                pick = rand() % range;
                if (pick < 0)
                    pick = 0;
            }
            sway->target = (float)pick;
        }
    } else {
        angle -= 1.0f;
        if (0.0f > angle)
            angle = 0.0f;
        sway->angle = angle;
        if (target >= angle) {
            range = (int)angle;
            if (range == 0) {
                pick = 0;
            } else {
                pick = rand() % range;
                if (pick < 0)
                    pick = 0;
            }
            sway->target = (float)(359 - pick);
        }
    }
}

unsigned char prim_light_groups_process(struct EffectPrim *self)
{
    struct PrimLightGroup *g = self->records.light_groups;
    short pitch;
    int i, k;

    self->camera_yaw = (unsigned short)(360 - (int)game_camera_heading());
    pitch = (short)(450 - (int)game_camera_pitch());
    self->camera_pitch = (unsigned short)pitch;
    if (pitch >= 360)
        self->camera_pitch = (unsigned short)(pitch - 360);
    self->base.pos = self->master->base.pos;

    for (i = 0; i < LIGHT_GROUP_COUNT; i++, g++) {
        float x2, z2, x0, z0, x3, z3, x1, z1;

        if (g->active != 1)
            continue;
        wander_step(&g->sway[0]);
        wander_step(&g->sway[1]);
        for (k = 0; k < 4; k++) {
            float a = g->angle[k] - 1.0f;
            g->angle[k] = a;
            if (0.0f > a)
                g->angle[k] = a + 360.0f;
        }

        x2 = g->point[2].x = g_sinTable[(int)g->angle[0]] * LIGHT_ORBIT_RADIUS;
        z2 = g->point[2].z = g_cosTable[(int)g->angle[0]] * LIGHT_ORBIT_RADIUS;
        x0 = g->point[0].x = g_sinTable[(int)g->angle[1]] * LIGHT_ORBIT_RADIUS;
        z0 = g->point[0].z = g_cosTable[(int)g->angle[1]] * LIGHT_ORBIT_RADIUS;
        x3 = g->point[3].x = g_sinTable[(int)g->angle[2]] * LIGHT_ORBIT_RADIUS;
        z3 = g->point[3].z = g_cosTable[(int)g->angle[2]] * LIGHT_ORBIT_RADIUS;
        x1 = g->point[1].x = g_sinTable[(int)g->angle[3]] * LIGHT_ORBIT_RADIUS;
        z1 = g->point[1].z = g_cosTable[(int)g->angle[3]] * LIGHT_ORBIT_RADIUS;

        if (0.0f > g->point[2].y) {
            g->point[2].x = g_sinTable[(int)g->sway[0].angle] * LIGHT_WANDER_PUSH + x2;
            g->point[2].z = g_sinTable[(int)g->sway[1].angle] * LIGHT_WANDER_PUSH + z2;
        }
        if (0.0f > g->point[0].y) {
            g->point[0].x = x0 - g_sinTable[(int)g->sway[0].angle] * LIGHT_WANDER_PUSH;
            g->point[0].z = z0 - g_sinTable[(int)g->sway[1].angle] * LIGHT_WANDER_PUSH;
        }
        if (0.0f > g->point[3].y) {
            g->point[3].x = g_sinTable[(int)g->sway[0].angle] * LIGHT_WANDER_PUSH + x3;
            g->point[3].z = z3 - g_sinTable[(int)g->sway[1].angle] * LIGHT_WANDER_PUSH;
        }
        if (0.0f > g->point[1].y) {
            g->point[1].x = x1 - g_sinTable[(int)g->sway[0].angle] * LIGHT_WANDER_PUSH;
            g->point[1].z = g_sinTable[(int)g->sway[1].angle] * LIGHT_WANDER_PUSH + z1;
        }
    }
    return 1;
}

void prim_light_groups_render(struct EffectPrim *self, const float *view)
{
    struct PrimLightGroup *g = self->records.light_groups;
    int count = 0;
    int i, k;

    for (i = 0; i < LIGHT_GROUP_COUNT; i++, g++) {
        float s;
        Vec3 q[4];

        if (g->active != 1)
            continue;
        s = g->size;
        q[0].x = g_cosTable[45] * s;
        q[0].y = 0.0f;
        q[0].z = g_sinTable[45] * s;
        q[1].x = g_cosTable[135] * s;
        q[1].y = 0.0f;
        q[1].z = g_sinTable[135] * s;
        q[2].x = g_cosTable[225] * s;
        q[2].y = 0.0f;
        q[2].z = g_sinTable[225] * s;
        q[3].x = g_cosTable[315] * s;
        q[3].y = 0.0f;
        q[3].z = g_sinTable[315] * s;
        prim_quad_turn(&q[0], &q[1], &q[2], &q[3], self->camera_yaw, self->camera_pitch);

        for (k = 0; k < 4; k++) {
            Vec3 c = g->point[k_light_slot[k]];
            Vec3 p0 = vec3_add(c, q[0]);
            Vec3 p1 = vec3_add(c, q[1]);
            Vec3 p2 = vec3_add(c, q[2]);
            Vec3 p3 = vec3_add(c, q[3]);
            int green = g->style == LIGHT_GROUP_STYLE_GREEN;
            int alpha = green ? 150 : 130;

            if (-1.0f > p0.y)
                alpha = (int)((p0.y + 1.0f) * 4.0f + 130.0f);
            if (alpha < 0)
                alpha = 0;
            if (0.0f >= p0.y) {
                if (green)
                    prim_draw_quad(self, &count, (short)alpha, p0, p1, p2, p3, view, PRIM_BLEND_ADDITIVE, 0, 0,
                                   0x9b, 0xff, 0x9b, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);
                else
                    prim_draw_quad(self, &count, (short)alpha, p0, p1, p2, p3, view, PRIM_BLEND_ADDITIVE, 0, 0,
                                   0xff, 0xff, 0xff, 0.0f, 1.0f, 0.0f, 1.0f, PRIM_QUAD_PROJECTED);
            }
        }
    }
}

void prim_sphere_process(struct EffectPrim *self)
{
    self->master_pos.x = self->master->base.pos.x + self->delta2.x;
    self->master_pos.y = self->master->base.pos.y + self->delta2.y;
    self->master_pos.z = self->master->base.pos.z + self->delta2.z;
    self->base.pos = self->master_pos;
    self->size[0].speed += self->size[0].accel;
    self->size[0].value += self->size[0].speed;
    self->angle[0] += self->angle_speed[0];
    prim_update_alpha(self);
}

void prim_streak_render(struct EffectPrim *self, const float *view)
{
    struct PrimTriangle *tri = &self->triangles[0];
    struct TexDrawVertex foot, head;
    Vec3 at = self->base.pos;
    Vec3 above = self->base.pos;

    memset(&foot, 0, sizeof(foot));
    foot.specular = 0xff000000;
    renderer_project_vertex(g_renderer, &at, view, &foot);
    above.y -= 10.0f;
    memset(&head, 0, sizeof(head));
    head.specular = 0xff000000;
    renderer_project_vertex(g_renderer, &above, view, &head);

    tri->v[0].x = foot.x;
    tri->v[0].y = foot.y;
    tri->v[0].z = foot.z;
    tri->v[0].rhw = 0.999999f;
    tri->v[0].diffuse = 0x9cffffff;
    tri->v[0].u = 0.0f;
    tri->v[0].v = 0.0f;
    tri->v[1].x = foot.x + 1.0f;
    tri->v[1].y = foot.y;
    tri->v[1].z = foot.z;
    tri->v[1].rhw = 0.999999f;
    tri->v[1].diffuse = 0x9bffffff;
    tri->v[1].u = 0.0f;
    tri->v[1].v = 0.0f;
    tri->v[2].x = head.x;
    tri->v[2].y = head.y;
    tri->v[2].z = head.z;
    tri->v[2].rhw = 0.999999f;
    tri->v[2].diffuse = 0x9bffffff;
    tri->v[2].u = 0.0f;
    tri->v[2].v = 0.0f;
    tri->texture = NULL;
    renderer_add_triangle(g_renderer, tri, 5);
}

static void card_vertex(struct TexDrawVertex *out, float x, float y, unsigned color, float u, float v)
{
    out->x = x;
    out->y = y;
    out->z = 0.8f;
    out->rhw = 1.0f;
    out->diffuse = color;
    out->u = u;
    out->v = v;
}

void prim_screen_card_render(struct EffectPrim *self, const float *view)
{
    struct PrimTriangle *tri = &self->triangles[0];
    float x, y, rhw, scale, right, top;

    renderer_project_point(g_renderer, &self->base.pos, view, &x, &y, &rhw);
    scale = renderer_screen_scale(g_renderer, rhw);
    right = self->size[1].value * scale + x;
    top = y - self->size[2].value * scale;

    card_vertex(&tri[0].v[0], x, y, self->argb, 0.0f, 0.0f);
    card_vertex(&tri[0].v[1], right, y, self->argb, 1.0f, 0.0f);
    card_vertex(&tri[0].v[2], x, top, self->argb, 0.0f, 1.0f);
    tri[0].texture = self->textures[self->base.motion];
    card_vertex(&tri[1].v[0], right, y, self->argb, 1.0f, 0.0f);
    card_vertex(&tri[1].v[1], x, top, self->argb, 0.0f, 1.0f);
    card_vertex(&tri[1].v[2], right, top, self->argb, 1.0f, 1.0f);
    tri[1].texture = self->textures[self->base.motion];
    renderer_add_triangle(g_renderer, &tri[0], self->list_flags);
    renderer_add_triangle(g_renderer, &tri[1], self->list_flags);
}
