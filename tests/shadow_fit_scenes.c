/* Scenes for the shadow fit's tests (tests/shadow_fit_scenes.h). */
#include "shadow_fit_scenes.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

float depth[SCENE_PIXELS];
double hit[SCENE_PIXELS][3];
float front[SCENE_PIXELS];
double frontHit[SCENE_PIXELS][3];
const char *const scene_names[SCENE_KINDS] = {"flat ground", "slope", "wall", "models", "blended"};

/* ---- numbers ---- */

static uint32_t seed = 0x2545f491u;

static uint32_t next(void)
{
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

double uniform(double lo, double hi)
{
    return lo + (hi - lo) * (double)(next() >> 8) / 16777216.0;
}

/* ---- scenes ---- */

double ground_height(const struct Scene *s, double x, double z)
{
    return s->slope[0] * x + s->slope[1] * z + s->slope[2];
}

/* each model's own box is tighter than the leaves widened by their models, which makes the CPU's
   squares no larger than the client's */
size_t scene_boxes(const struct Scene *s, struct ShadowFitBox *out)
{
    size_t n = 0;
    int i, j, k, b;

    if (s->ground)
        for (i = 0; i < LEAVES; i++)
            for (j = 0; j < LEAVES; j++) {
                double x0 = -MAP + 2.0 * MAP * i / LEAVES, x1 = -MAP + 2.0 * MAP * (i + 1) / LEAVES;
                double z0 = -MAP + 2.0 * MAP * j / LEAVES, z1 = -MAP + 2.0 * MAP * (j + 1) / LEAVES;
                double y[4] = {ground_height(s, x0, z0), ground_height(s, x1, z0), ground_height(s, x0, z1),
                               ground_height(s, x1, z1)}, lo = y[0], hi = y[0];
                for (k = 1; k < 4; k++) {
                    lo = y[k] < lo ? y[k] : lo;
                    hi = y[k] > hi ? y[k] : hi;
                }
                out[n].min[0] = (float)x0;
                out[n].max[0] = (float)x1;
                out[n].min[1] = (float)floor(lo);
                out[n].max[1] = (float)ceil(hi);
                out[n].min[2] = (float)z0;
                out[n].max[2] = (float)z1;
                n++;
            }
    for (b = 0; b < s->count; b++, n++)
        for (k = 0; k < 3; k++) {
            out[n].min[k] = (float)s->box[b][0][k];
            out[n].max[k] = (float)s->box[b][1][k];
        }
    return n;
}

/* the view depths of the first surface along the ray o + t d from the near plane to the far one
   that is not blended, and of the first blended one before it, 0 for none; d's view depth is 1, so
   t is the view depth */
static void cast(const struct Scene *s, const double *o, const double *d, double nearZ, double farZ, double *opaque,
                 double *seen)
{
    double best = 0.0, through = 0.0, inv[3];
    int b, k;

    for (k = 0; k < 3; k++)
        inv[k] = d[k] != 0.0 ? 1.0 / d[k] : 0.0;
    if (s->ground) {
        double den = d[1] - s->slope[0] * d[0] - s->slope[1] * d[2];

        if (den != 0.0) {
            double t = (s->slope[0] * o[0] + s->slope[1] * o[2] + s->slope[2] - o[1]) / den;

            if (t >= nearZ && t <= farZ && fabs(o[0] + t * d[0]) <= MAP && fabs(o[2] + t * d[2]) <= MAP)
                best = t;
        }
    }
    for (b = 0; b < s->count; b++) {
        double t0 = -1e300, t1 = 1e300, t;
        int miss = 0;

        for (k = 0; k < 3 && !miss; k++) {
            if (d[k] == 0.0) {
                miss = o[k] < s->box[b][0][k] || o[k] > s->box[b][1][k];
            } else {
                double ta = (s->box[b][0][k] - o[k]) * inv[k], tb = (s->box[b][1][k] - o[k]) * inv[k];
                double enter = ta < tb ? ta : tb, leave = ta < tb ? tb : ta;
                t0 = enter > t0 ? enter : t0;
                t1 = leave < t1 ? leave : t1;
                miss = t0 > t1;
            }
        }
        if (miss)
            continue;
        t = t0 >= nearZ ? t0 : t1 >= nearZ ? t1 : 0.0;
        if (!(t > 0.0 && t <= farZ))
            continue;
        if (s->blended[b])
            through = through == 0.0 || t < through ? t : through;
        else if (best == 0.0 || t < best)
            best = t;
    }
    *opaque = best;
    *seen = through > 0.0 && (best == 0.0 || through < best) ? through : best;
}

struct ShadowFitDepths scene_depths(void)
{
    struct ShadowFitDepths d;

    d.front = front;
    d.back = depth;
    d.width = SCENE_W;
    d.height = SCENE_H;
    return d;
}

/* ---- cameras ---- */

void pose_look(struct Pose *p, const double *focus, double yaw, double pitch, double distance)
{
    double *x = p->axis[0], *y = p->axis[1], *z = p->axis[2];
    int i;

    z[0] = sin(yaw) * cos(pitch);
    z[1] = sin(pitch);
    z[2] = cos(yaw) * cos(pitch);
    x[0] = cos(yaw);
    x[1] = 0.0;
    x[2] = -sin(yaw);
    y[0] = z[1] * x[2] - z[2] * x[1];
    y[1] = z[2] * x[0] - z[0] * x[2];
    y[2] = z[0] * x[1] - z[1] * x[0];
    for (i = 0; i < 3; i++)
        p->eye[i] = focus[i] - distance * z[i];
    p->distance = distance;
}

void pose_camera(const struct Pose *p, int width, int height, struct ShadowFitCamera *cam)
{
    int halfW = width / 2, halfH = height / 2, k;
    float tHalf = (float)tan(0.13089966773986816);

    for (k = 0; k < 3; k++) {
        cam->view[3 * k] = (float)p->axis[0][k];
        cam->view[3 * k + 1] = (float)p->axis[1][k];
        cam->view[3 * k + 2] = (float)p->axis[2][k];
    }
    for (k = 0; k < 3; k++)
        cam->view[9 + k] = (float)-(p->axis[k][0] * p->eye[0] + p->axis[k][1] * p->eye[1] + p->axis[k][2] * p->eye[2]);
    cam->proj[1] = (float)halfH / tHalf;
    cam->proj[0] = (float)halfW / tHalf * ((float)height / (float)width);
    cam->centre[0] = (float)halfW;
    cam->centre[1] = (float)halfH;
    cam->width = width;
    cam->height = height;
    cam->nearZ = 10.0f;   /* RENDER_NEAR_Z and RENDER_FAR_Z (render/render_api.h) */
    cam->farZ = 40000.0f;
    cam->distance = (float)p->distance;
}

/* ---- one frame ---- */

void scene_render(const struct Scene *s, const struct Pose *p, const struct ShadowFitCamera *cam)
{
    int i, j, k;

    for (j = 0; j < SCENE_H; j++)
        for (i = 0; i < SCENE_W; i++) {
            double v[3] = {((double)i - cam->centre[0]) / cam->proj[0], ((double)j - cam->centre[1]) / cam->proj[1], 1.0};
            double d[3], t, seen;
            size_t at = (size_t)j * SCENE_W + (size_t)i;

            for (k = 0; k < 3; k++)
                d[k] = p->axis[0][k] * v[0] + p->axis[1][k] * v[1] + p->axis[2][k] * v[2];
            cast(s, p->eye, d, cam->nearZ, cam->farZ, &t, &seen);
            depth[at] = (float)t;
            front[at] = (float)seen;
            for (k = 0; k < 3; k++) {
                hit[at][k] = p->eye[k] + t * d[k];
                frontHit[at][k] = p->eye[k] + seen * d[k];
            }
        }
}

/* ---- the scenes and the cameras the fuzzing picks ---- */

void scene_make(struct Scene *s, int kind)
{
    int b;

    memset(s, 0, sizeof *s);
    switch (kind) {
    case 0:
        s->ground = 1;
        break;
    case 1:
        s->ground = 1;
        s->slope[0] = 0.25;
        s->slope[1] = -0.15;
        break;
    case 2:
        s->ground = 1;
        s->count = 1;
        s->box[0][0][0] = -800; s->box[0][1][0] = 800;
        s->box[0][0][1] = -500; s->box[0][1][1] = 0;
        s->box[0][0][2] = 100; s->box[0][1][2] = 120;
        break;
    default:
        s->ground = 1;
        s->count = 40;
        for (b = 0; b < s->count; b++) {
            double x = uniform(-900, 900), z = uniform(-900, 900), hx = uniform(10, 120), hz = uniform(10, 120);
            s->box[b][0][0] = floor(x - hx); s->box[b][1][0] = floor(x + hx);
            s->box[b][0][1] = -floor(uniform(20, 400)); s->box[b][1][1] = 0;
            s->box[b][0][2] = floor(z - hz); s->box[b][1][2] = floor(z + hz);
            /* blended ones float over the ground, so that slices part between them and it */
            s->blended[b] = kind == 4 && b % 2;
            if (s->blended[b]) {
                s->box[b][1][1] = -floor(uniform(30, 300));
                s->box[b][0][1] = s->box[b][1][1] - floor(uniform(5, 60));
            }
        }
        break;
    }
}

void random_light(float *light)
{
    double a = uniform(0, 2 * PI), y = uniform(0.2, 1.0), h = sqrt(1.0 - y * y);

    light[0] = (float)(h * cos(a));
    light[1] = (float)y;
    light[2] = (float)(h * sin(a));
}

void random_pose(const struct Scene *s, struct Pose *p)
{
    double focus[3];

    focus[0] = uniform(-600, 600);
    focus[2] = uniform(-600, 600);
    focus[1] = s->ground ? ground_height(s, focus[0], focus[2]) : 0.0;
    pose_look(p, focus, uniform(0, 2 * PI), uniform(5, 85) * PI / 180.0, exp(uniform(log(100.0), log(8000.0))));
}
