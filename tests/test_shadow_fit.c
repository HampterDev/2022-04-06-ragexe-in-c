/* render/shadow_fit.c: the shadow cascades fitted to a frame's depths, against scenes whose depths
   are cast exactly (flat ground, a slope, a wall, models, sky, a single pixel, nothing) from
   fuzzed cameras and lights, and against the CPU's fit to boxes (render/rp_project.c,
   renderer_shadow_frame) that it is to replace. With --report it prints the texel sizes of both
   fits and how often their squares change size while the camera moves. */
#include "shadow_fit_scenes.h"
#include "render/renderer.h"
#include "render/render_api.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* what renderer_shadow_frame reads besides the renderer */
DWORD g_fogColor;
float g_fogNear, g_fogFar;
struct Renderer *g_renderer;
int g_lightmap = 1;
int gfx_shadows_supported(void) { return 1; }

static int report;


/* a point times a row-vector 4x4 */
static void apply(const float *m, const double *p, double *out)
{
    int i, j;

    for (j = 0; j < 4; j++) {
        out[j] = m[12 + j];
        for (i = 0; i < 3; i++)
            out[j] += p[i] * m[i * 4 + j];
    }
}

static int all_finite(const float *v, int n)
{
    int i;

    for (i = 0; i < n; i++)
        if (!(v[i] - v[i] == 0.0f))
            return 0;
    return 1;
}

struct Fit {
    struct ShadowFitFrame frame;
    struct ShadowFitRange range;
    float split[SHADOW_FIT_CASCADES + 1];
    struct ShadowFitExtent extents[SHADOW_FIT_CASCADES];
    struct ShadowFitCascades cascades;
};

static int fit_depths(struct Fit *fit, const struct ShadowFitCamera *cam, const float *light,
                      const struct ShadowFitBox *within, size_t count, const float *buffer, int width, int height)
{
    memset(fit, 0, sizeof *fit);
    if (!shadow_fit_frame(&fit->frame, cam, light, within, count))
        return 0;
    shadow_fit_range(&fit->range, &fit->frame, buffer, width, height);
    shadow_fit_split(fit->split, &fit->range);
    shadow_fit_extents(fit->extents, &fit->frame, fit->split, buffer, width, height);
    shadow_fit_cascades(&fit->cascades, &fit->frame, fit->split, fit->extents);
    return 1;
}

static float radius_of(const struct ShadowFitCascades *c, int k)
{
    return (c->hi[k][0] - c->lo[k][0]) * 0.5f;
}

static int cascade_of(const float *farDepth, int cascades, double d)
{
    int c;

    for (c = 0; c < cascades && !(d < farDepth[c]); c++)
        ;
    return c;
}

/* today's fit for the same frame */
static int cpu_fit(struct Renderer *r, const struct ShadowFitCamera *cam, const float *light,
                   const struct ShadowBox *boxes, size_t count)
{
    memset(r, 0, sizeof *r);
    r->width = SCENE_W;
    r->height = SCENE_H;
    r->centreX = SCENE_W / 2;
    r->centreY = SCENE_H / 2;
    r->proj[0] = cam->proj[0];
    r->proj[1] = cam->proj[1];
    memcpy(r->lightDir, light, sizeof r->lightDir);
    r->lightSet = 1;
    r->shadowDarkness = 0.5f;
    renderer_shadow_frame(r, cam->view, cam->distance, boxes, count);
    return r->shadowPending;
}

/* ---- what is measured ---- */

#define BINS 800  /* log2 of a texel ratio from -4 to 4 in steps of 0.01 */

struct Stats {
    double pixels, logSum, worse, maxRatio;
    double hist[BINS];
    double radiusNew[SHADOW_FIT_CASCADES], radiusOld[SHADOW_FIT_CASCADES], used[SHADOW_FIT_CASCADES];
    double firstNew, firstOld, lastNew, lastOld, frames;  /* far depths of the first and last cascade */
    double slack;  /* the least room a looked-up pixel had inside its map, world units */
    double boundary;  /* pixels whose own view depth picks another cascade than their stored depth */
};

static double ratio_percentile(const struct Stats *st, double share)
{
    double seen = 0.0;
    int b;

    for (b = 0; b < BINS; b++) {
        seen += st->hist[b];
        if (seen >= share * st->pixels)
            return pow(2.0, (b - BINS / 2) * 0.01);
    }
    return pow(2.0, 4.0);
}

/* Every pixel that receives lies in the cascade the shaders pick for its depth, inside the map
   by the soft edge and the lookup's offset, and between the light's depth bounds; each square is
   no wider than its pixels need, before rounding up to 8 units. */
static void check_cover(const struct Fit *fit, const struct ShadowFitCamera *cam, struct Stats *st)
{
    const struct ShadowFitCascades *cs = &fit->cascades;
    double lo[SHADOW_FIT_CASCADES][2], hi[SHADOW_FIT_CASCADES][2];
    int p, c, a, k;

    for (c = 0; c < SHADOW_FIT_CASCADES; c++)
        for (a = 0; a < 2; a++) {
            lo[c][a] = 1e300;
            hi[c][a] = -1e300;
        }
    for (p = 0; p < SCENE_PIXELS; p++) {
        double d = depth[p], l[4], v[3], lv[4], radius, texel, margin, room;
        int c2;

        if (!(d >= cam->nearZ && d <= fit->frame.reach))
            continue;
        c = cascade_of(cs->farDepth, SHADOW_FIT_CASCADES, d);
        assert(cs->cascades == SHADOW_FIT_CASCADES && c < SHADOW_FIT_CASCADES);
        radius = radius_of(cs, c);
        texel = 2.0 * radius / SHADOW_FIT_SIZE;
        margin = (0.5 + 1.5 * texel) / radius;
        apply(cs->world[c], hit[p], l);
        assert(fabs(l[0]) <= 1.0 - margin && fabs(l[1]) <= 1.0 - margin);
        assert(l[2] > 0.0 && l[2] < 1.0);
        room = (1.0 - fmax(fabs(l[0]), fabs(l[1]))) * radius - (0.5 + 1.5 * texel);
        st->slack = room < st->slack ? room : st->slack;
        /* as the receiving shader has it: view space, then fromView */
        for (k = 0; k < 3; k++)
            v[k] = cam->view[k] * hit[p][0] + cam->view[3 + k] * hit[p][1] + cam->view[6 + k] * hit[p][2] + cam->view[9 + k];
        apply(cs->fromView[c], v, lv);
        assert(fabs(lv[0]) <= 1.0 - margin + 1e-5 && fabs(lv[1]) <= 1.0 - margin + 1e-5);
        assert(lv[2] > 0.0 && lv[2] < 1.0);
        /* the receiver's own view depth rounds differently from the buffer's: where it picks the
           neighbouring cascade, that one holds it too */
        c2 = cascade_of(cs->farDepth, SHADOW_FIT_CASCADES, v[2]);
        if (c2 != c) {
            st->boundary++;
            assert(c2 < SHADOW_FIT_CASCADES);
            apply(cs->fromView[c2], v, lv);
            assert(fabs(lv[0]) <= 1.0 && fabs(lv[1]) <= 1.0 && lv[2] > 0.0 && lv[2] < 1.0);
        }
        for (a = 0; a < 2; a++) {
            const float *axis = fit->frame.axis[a];
            double x = hit[p][0] * axis[0] + hit[p][1] * axis[1] + hit[p][2] * axis[2];
            lo[c][a] = x < lo[c][a] ? x : lo[c][a];
            hi[c][a] = x > hi[c][a] ? x : hi[c][a];
        }
    }
    for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
        double radius = radius_of(cs, c), need, texel;

        if (lo[c][0] > hi[c][0]) {
            assert(cs->lo[c][0] > cs->hi[c][0] || cs->cascades == 0);
            continue;
        }
        need = fmax(hi[c][0] - lo[c][0], hi[c][1] - lo[c][1]) * 0.5 * 1.05 + 4.5;
        texel = 2.0 * radius / SHADOW_FIT_SIZE;
        assert(radius >= need - 0.02 && radius <= need + 8.0 + 0.02);
        assert(fmod(radius, 8.0) == 0.0);
        for (a = 0; a < 2; a++) {
            double square = (cs->lo[c][a] + cs->hi[c][a]) * 0.5;
            assert(fabs(square - (lo[c][a] + hi[c][a]) * 0.5) <= texel * 0.5 + 0.02);
        }
    }
}

/* the texel each receiving pixel is looked up in, against today's */
static void compare(const struct Fit *fit, const struct Renderer *r, struct Stats *st)
{
    const struct ShadowFitCascades *cs = &fit->cascades;
    int p, c;

    for (p = 0; p < SCENE_PIXELS; p++) {
        double d = depth[p], ratio;
        int cn, co, bin;

        if (!(d >= RENDER_NEAR_Z && d <= fit->frame.reach))
            continue;
        cn = cascade_of(cs->farDepth, cs->cascades, d);
        co = cascade_of(r->shadow.farDepth, r->shadow.cascades, d);
        assert(cn < cs->cascades && co < r->shadow.cascades);
        ratio = (double)radius_of(cs, cn) / ((r->shadowReach.hi[co][0] - r->shadowReach.lo[co][0]) * 0.5);
        st->pixels++;
        st->logSum += log(ratio);
        st->worse += ratio > 1.000001;
        st->maxRatio = ratio > st->maxRatio ? ratio : st->maxRatio;
        bin = (int)floor(log2(ratio) * 100.0 + 0.5) + BINS / 2;
        bin = bin < 0 ? 0 : bin >= BINS ? BINS - 1 : bin;
        st->hist[bin]++;
    }
    for (c = 0; c < SHADOW_FIT_CASCADES; c++)
        if (cs->lo[c][0] <= cs->hi[c][0] && r->shadowReach.lo[c][0] <= r->shadowReach.hi[c][0]) {
            st->radiusNew[c] += radius_of(cs, c);
            st->radiusOld[c] += (r->shadowReach.hi[c][0] - r->shadowReach.lo[c][0]) * 0.5;
            st->used[c]++;
        }
    st->firstNew += cs->farDepth[0];
    st->lastNew += cs->farDepth[SHADOW_FIT_CASCADES - 1];
    st->firstOld += r->shadow.farDepth[0];
    st->lastOld += r->shadow.farDepth[SHADOW_FIT_CASCADES - 1];
    st->frames++;
}

/* ---- the tests ---- */

static struct ShadowBox boxes[SCENE_BOXES];
static struct ShadowFitBox fitBoxes[SCENE_BOXES];

/* the scene's boxes for both fits */
static size_t scene_both(const struct Scene *s)
{
    size_t count = scene_boxes(s, fitBoxes), b;

    for (b = 0; b < count; b++) {
        memcpy(boxes[b].min, fitBoxes[b].min, sizeof boxes[b].min);
        memcpy(boxes[b].max, fitBoxes[b].max, sizeof boxes[b].max);
    }
    return count;
}

/* fuzzed cameras and lights over each scene: coverage, tightness, the same bits from the same
   frame, and the texels against today's fit */
static void test_fuzz(struct Stats *stats)
{
    int kind, n;

    for (kind = 0; kind < SCENE_KINDS; kind++) {
        struct Scene scene;
        size_t count;

        scene_make(&scene, kind);
        count = scene_both(&scene);
        stats[kind].slack = 1e300;
        for (n = 0; n < (report ? 48 : 12); n++) {
            struct Pose pose;
            struct ShadowFitCamera cam;
            struct Fit fit, again;
            struct Renderer r;
            float light[3];

            random_pose(&scene, &pose);
            random_light(light);
            if (n == 0) {
                light[0] = 0.5f; light[1] = 0.707f; light[2] = 0.5f;  /* Prontera's */
            }
            pose_camera(&pose, SCENE_W, SCENE_H, &cam);
            scene_render(&scene, &pose, &cam);
            assert(fit_depths(&fit, &cam, light, fitBoxes, count, depth, SCENE_W, SCENE_H));
            assert(fit_depths(&again, &cam, light, fitBoxes, count, depth, SCENE_W, SCENE_H));
            assert(!memcmp(&fit, &again, sizeof fit));
            if (fit.cascades.cascades == 0)
                continue;
            check_cover(&fit, &cam, &stats[kind]);
            if (cpu_fit(&r, &cam, light, boxes, count))
                compare(&fit, &r, &stats[kind]);
        }
    }
}

/* Fed the squares today's fit chose, the cascades come out as today's to the bit: the same
   matrices, lookups and reach, so the shaders read them as they read today's. */
static void test_conventions(void)
{
    struct Scene scene;
    size_t count;
    int n, c, a;

    scene_make(&scene, 3);
    count = scene_both(&scene);
    for (n = 0; n < 24; n++) {
        struct Pose pose;
        struct ShadowFitCamera cam;
        struct ShadowFitFrame frame;
        struct ShadowFitExtent ext[SHADOW_FIT_CASCADES];
        struct ShadowFitCascades out;
        struct Renderer r;
        float light[3], split[SHADOW_FIT_CASCADES + 1];

        random_pose(&scene, &pose);
        random_light(light);
        pose_camera(&pose, SCENE_W, SCENE_H, &cam);
        if (!cpu_fit(&r, &cam, light, boxes, count))
            continue;
        assert(shadow_fit_frame(&frame, &cam, light, fitBoxes, count));
        assert(!memcmp(frame.axis, r.shadowReach.axis, sizeof frame.axis));
        assert(frame.lightNear == r.shadowReach.depth[0]);
        assert(frame.lightNear + frame.lightRange == r.shadowReach.depth[1]);
        assert(frame.bias == r.shadow.bias);
        assert(!memcmp(frame.lightView, r.shadow.lightView, sizeof frame.lightView));
        split[0] = 0.0f;
        for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
            float radius = (r.shadowReach.hi[c][0] - r.shadowReach.lo[c][0]) * 0.5f, texel, half = 0.0f;
            int q;

            split[c + 1] = r.shadow.farDepth[c];
            if (!(r.shadowReach.lo[c][0] <= r.shadowReach.hi[c][0])) {
                for (a = 0; a < 2; a++) {
                    ext[c].lo[a] = HUGE_VALF;
                    ext[c].hi[a] = -HUGE_VALF;
                }
                continue;
            }
            /* extents of whole texels about today's square that round up to today's size */
            texel = 2.0f * radius / SHADOW_FIT_SIZE;
            for (q = SHADOW_FIT_SIZE / 2; q > 0; q--) {
                half = (float)q * texel;
                if (ceilf((half * 1.05f + 4.0f + 0.5f) / 8.0f) * 8.0f == radius)
                    break;
            }
            assert(q > 0);
            for (a = 0; a < 2; a++) {
                float square = (r.shadowReach.lo[c][a] + r.shadowReach.hi[c][a]) * 0.5f;
                ext[c].lo[a] = square - half;
                ext[c].hi[a] = square + half;
            }
        }
        shadow_fit_cascades(&out, &frame, split, ext);
        assert(out.cascades == r.shadow.cascades);
        for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
            assert(!memcmp(out.fromView[c], r.shadow.fromView[c], sizeof out.fromView[c]));
            assert(out.farDepth[c] == r.shadow.farDepth[c] && out.offset[c] == r.shadow.offset[c]);
            assert(out.blur[c] == r.shadow.blur[c] && out.slant[c] == r.shadow.slant[c]);
            assert(!memcmp(out.lo[c], r.shadowReach.lo[c], sizeof out.lo[c]));
            assert(!memcmp(out.hi[c], r.shadowReach.hi[c], sizeof out.hi[c]));
        }
    }
}

/* Moving the camera by less than a texel moves a square that keeps its size by whole texels,
   with the rest of its matrix the same bits; counted, how often a square changes size instead,
   for this fit and today's */
static void test_steady(double *changed, double *changedOld, double *compared)
{
    int kind, n, c, k;

    for (kind = 0; kind < SCENE_KINDS; kind++) {
        struct Scene scene;
        size_t count;

        scene_make(&scene, kind);
        count = scene_both(&scene);
        for (n = 0; n < (report ? 24 : 6); n++) {
            struct Pose pose, moved;
            struct ShadowFitCamera cam, cam2;
            struct Fit fit, fit2;
            struct Renderer r, r2;
            float light[3], smallest = 1e30f;
            double angle = uniform(0, 2 * PI), step;
            int old;

            random_pose(&scene, &pose);
            random_light(light);
            pose_camera(&pose, SCENE_W, SCENE_H, &cam);
            scene_render(&scene, &pose, &cam);
            assert(fit_depths(&fit, &cam, light, fitBoxes, count, depth, SCENE_W, SCENE_H));
            if (!fit.cascades.cascades)
                continue;
            for (c = 0; c < SHADOW_FIT_CASCADES; c++)
                if (fit.cascades.lo[c][0] <= fit.cascades.hi[c][0])
                    smallest = fminf(smallest, 2.0f * radius_of(&fit.cascades, c) / SHADOW_FIT_SIZE);
            step = 0.9 * smallest;
            moved = pose;
            moved.eye[0] += step * cos(angle);
            moved.eye[2] += step * sin(angle);
            pose_camera(&moved, SCENE_W, SCENE_H, &cam2);
            scene_render(&scene, &moved, &cam2);
            assert(fit_depths(&fit2, &cam2, light, fitBoxes, count, depth, SCENE_W, SCENE_H));
            old = cpu_fit(&r, &cam, light, boxes, count) && cpu_fit(&r2, &cam2, light, boxes, count);
            for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
                const float *a = fit.cascades.world[c], *b = fit2.cascades.world[c];
                int both = fit.cascades.lo[c][0] <= fit.cascades.hi[c][0] && fit2.cascades.lo[c][0] <= fit2.cascades.hi[c][0];

                if (!both)
                    continue;
                compared[kind]++;
                if (radius_of(&fit.cascades, c) != radius_of(&fit2.cascades, c)) {
                    changed[kind]++;
                } else {
                    assert(!memcmp(a, b, 12 * sizeof *a) && a[14] == b[14] && a[15] == b[15]);
                    for (k = 12; k < 14; k++) {
                        double texels = ((double)b[k] - (double)a[k]) * (SHADOW_FIT_SIZE / 2);
                        assert(texels == floor(texels));
                    }
                }
                if (old && r.shadowReach.hi[c][0] - r.shadowReach.lo[c][0] != r2.shadowReach.hi[c][0] - r2.shadowReach.lo[c][0])
                    changedOld[kind]++;
            }
        }
    }
}

/* walking: the camera's focus moves a unit a frame along the ground; how many frames change a
   square's size, for this fit and today's */
static void test_walk(double *changes, double *changesOld, double *frames)
{
    int kind, n, f, c;

    for (kind = 0; kind < SCENE_KINDS; kind++) {
        struct Scene scene;
        size_t count;

        scene_make(&scene, kind);
        count = scene_both(&scene);
        for (n = 0; n < (report ? 3 : 1); n++) {
            float light[3], last[SHADOW_FIT_CASCADES], lastOld[SHADOW_FIT_CASCADES];
            double focus[3], yaw = uniform(0, 2 * PI), pitch = uniform(20, 60) * PI / 180.0;
            double distance = n == 0 ? 300.0 : n == 1 ? 1000.0 : 3000.0, heading = uniform(0, 2 * PI);

            random_light(light);
            focus[0] = uniform(-300, 300);
            focus[2] = uniform(-300, 300);
            for (f = 0; f < (report ? 120 : 24); f++) {
                struct Pose pose;
                struct ShadowFitCamera cam;
                struct Fit fit;
                struct Renderer r;
                double fx = focus[0] + f * cos(heading), fz = focus[2] + f * sin(heading);
                double at[3] = {fx, scene.ground ? ground_height(&scene, fx, fz) : 0.0, fz};

                pose_look(&pose, at, yaw, pitch, distance);
                pose_camera(&pose, SCENE_W, SCENE_H, &cam);
                scene_render(&scene, &pose, &cam);
                assert(fit_depths(&fit, &cam, light, fitBoxes, count, depth, SCENE_W, SCENE_H));
                assert(cpu_fit(&r, &cam, light, boxes, count));
                for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
                    float now = radius_of(&fit.cascades, c);
                    float nowOld = (r.shadowReach.hi[c][0] - r.shadowReach.lo[c][0]) * 0.5f;

                    if (f > 0) {
                        changes[kind] += now != last[c];
                        changesOld[kind] += nowOld != lastOld[c];
                    }
                    last[c] = now;
                    lastOld[c] = nowOld;
                }
                frames[kind] += f > 0;
            }
        }
    }
}

static void check_finite(const struct ShadowFitCascades *cs)
{
    int c;

    for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
        assert(all_finite(cs->world[c], 16) && all_finite(cs->fromView[c], 16));
        assert(all_finite(&cs->farDepth[c], 1) && all_finite(&cs->offset[c], 1));
        assert(all_finite(&cs->blur[c], 1) && all_finite(&cs->slant[c], 1));
    }
}

/* the camera straight down, the camera along the light, the light straight down and level, a
   view of nothing but sky, a single pixel, nothing at all, depths out of reach or not numbers,
   and frames that cannot be fitted */
static void test_degenerate(void)
{
    static const float down[3] = {0, 1, 0}, level[3] = {1, 0, 0}, prontera[3] = {0.5f, 0.707f, 0.5f};
    static const float none[3] = {0, 0, 0};
    const float *lights[3] = {prontera, down, level};
    struct Scene scene;
    struct Pose pose;
    struct ShadowFitCamera cam;
    struct Fit fit;
    struct Stats st;
    double focus[3] = {50, 0, -80};
    size_t count;
    int l, c, p;

    memset(&st, 0, sizeof st);
    st.slack = 1e300;
    scene_make(&scene, 0);
    count = scene_both(&scene);
    for (l = 0; l < 3; l++) {
        float len = sqrtf(lights[l][0] * lights[l][0] + lights[l][1] * lights[l][1] + lights[l][2] * lights[l][2]);

        /* straight down */
        pose_look(&pose, focus, 0.3, PI / 2, 500.0);
        pose_camera(&pose, SCENE_W, SCENE_H, &cam);
        scene_render(&scene, &pose, &cam);
        assert(fit_depths(&fit, &cam, lights[l], fitBoxes, count, depth, SCENE_W, SCENE_H) && fit.cascades.cascades);
        check_finite(&fit.cascades);
        check_cover(&fit, &cam, &st);
        /* along the light */
        pose_look(&pose, focus, atan2(lights[l][0], lights[l][2]), asin(lights[l][1] / len), 500.0);
        pose_camera(&pose, SCENE_W, SCENE_H, &cam);
        scene_render(&scene, &pose, &cam);
        assert(fit_depths(&fit, &cam, lights[l], fitBoxes, count, depth, SCENE_W, SCENE_H));
        check_finite(&fit.cascades);
        if (fit.cascades.cascades)
            check_cover(&fit, &cam, &st);
    }

    /* looking up from high over the ground: all sky */
    {
        double above[3] = {50, -1000, -80};
        pose_look(&pose, above, 0.0, -0.3, 500.0);
    }
    pose_camera(&pose, SCENE_W, SCENE_H, &cam);
    scene_render(&scene, &pose, &cam);
    assert(fit_depths(&fit, &cam, prontera, fitBoxes, count, depth, SCENE_W, SCENE_H));
    assert(fit.cascades.cascades == 0 && fit.range.nearest > fit.range.farthest);
    check_finite(&fit.cascades);
    for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
        static const float centre[3] = {0, 0, 0}, half[3] = {5000, 5000, 5000};
        assert(fit.cascades.farDepth[c] == 0.0f && fit.cascades.lo[c][0] > fit.cascades.hi[c][0]);
        assert(!shadow_fit_reaches(&fit.frame, &fit.cascades, c, centre, half, NULL));
    }

    /* a single pixel: its slice gets the smallest square around it, the others none */
    pose_look(&pose, focus, 0.0, 0.8, 500.0);
    pose_camera(&pose, SCENE_W, SCENE_H, &cam);
    scene_render(&scene, &pose, &cam);
    for (p = 0; p < SCENE_PIXELS; p++)
        if (p != 77 * SCENE_W + 130)
            depth[p] = 0.0f;
    assert(depth[77 * SCENE_W + 130] > 0.0f);
    assert(fit_depths(&fit, &cam, prontera, fitBoxes, count, depth, SCENE_W, SCENE_H) && fit.cascades.cascades);
    assert(fit.range.nearest == fit.range.farthest && fit.split[SHADOW_FIT_CASCADES] == fit.range.farthest + 1.0f);
    check_finite(&fit.cascades);
    check_cover(&fit, &cam, &st);
    for (c = 0; c < SHADOW_FIT_CASCADES; c++)
        assert(radius_of(&fit.cascades, c) == 8.0f || fit.cascades.lo[c][0] > fit.cascades.hi[c][0]);

    /* nothing: no pixels, or none that receives */
    assert(fit_depths(&fit, &cam, prontera, fitBoxes, count, depth, 0, 0) && fit.cascades.cascades == 0);
    for (p = 0; p < SCENE_PIXELS; p++)
        depth[p] = p % 3 == 0 ? fit.frame.reach * 1.5f : p % 3 == 1 ? 5.0f : NAN;
    assert(fit_depths(&fit, &cam, prontera, fitBoxes, count, depth, SCENE_W, SCENE_H) && fit.cascades.cascades == 0);
    check_finite(&fit.cascades);

    /* frames that cannot be fitted: no light, a light that is not a number, no boxes, boxes that
       are empty or not finite, a view that cannot be inverted */
    assert(!fit_depths(&fit, &cam, none, fitBoxes, count, depth, SCENE_W, SCENE_H));
    {
        float bad[3] = {NAN, 1.0f, 0.0f}, huge[3] = {INFINITY, 1.0f, 0.0f};
        struct ShadowFitBox empty[2] = {{{1, 1, 1}, {0, 0, 0}}, {{-INFINITY, 0, 0}, {0, 0, 0}}};
        struct ShadowFitCamera flat = cam;

        assert(!fit_depths(&fit, &cam, bad, fitBoxes, count, depth, SCENE_W, SCENE_H));
        assert(!fit_depths(&fit, &cam, huge, fitBoxes, count, depth, SCENE_W, SCENE_H));
        assert(!fit_depths(&fit, &cam, prontera, fitBoxes, 0, depth, SCENE_W, SCENE_H));
        assert(!fit_depths(&fit, &cam, prontera, empty, 2, depth, SCENE_W, SCENE_H));
        memset(flat.view, 0, sizeof flat.view);
        assert(!fit_depths(&fit, &flat, prontera, fitBoxes, count, depth, SCENE_W, SCENE_H));
    }
}

/* the reach: a caster within a square and the light's depth reaches its cascade, one beside the
   square, beyond the depth or in a cascade no pixel lies in does not; turned boxes by their axes */
static void test_reaches(void)
{
    static const float light[3] = {0.5f, 0.707f, 0.5f};
    struct Scene scene;
    struct Pose pose;
    struct ShadowFitCamera cam;
    struct Fit fit;
    double focus[3] = {0, 0, 0};
    size_t count;
    int c, a, i;

    scene_make(&scene, 0);
    count = scene_both(&scene);
    pose_look(&pose, focus, 0.4, 0.7, 400.0);
    pose_camera(&pose, SCENE_W, SCENE_H, &cam);
    scene_render(&scene, &pose, &cam);
    assert(fit_depths(&fit, &cam, light, fitBoxes, count, depth, SCENE_W, SCENE_H) && fit.cascades.cascades);
    for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
        const struct ShadowFitFrame *fr = &fit.frame;
        float centre[3], half[3] = {1, 1, 1}, mid[2], radius = radius_of(&fit.cascades, c), along;
        static const float turned[3][3] = {{0.6f, 0.8f, 0}, {-0.8f, 0.6f, 0}, {0, 0, 1}};

        if (fit.cascades.lo[c][0] > fit.cascades.hi[c][0])
            continue;
        for (a = 0; a < 2; a++)
            mid[a] = (fit.cascades.lo[c][a] + fit.cascades.hi[c][a]) * 0.5f;
        along = fr->lightNear + fr->lightRange * 0.5f;
        for (i = 0; i < 3; i++)
            centre[i] = mid[0] * fr->axis[0][i] + mid[1] * fr->axis[1][i] + along * fr->axis[2][i];
        assert(shadow_fit_reaches(fr, &fit.cascades, c, centre, half, NULL));
        assert(shadow_fit_reaches(fr, &fit.cascades, c, centre, half, turned));
        /* just beside the square, across the light */
        for (i = 0; i < 3; i++)
            centre[i] = (mid[0] + radius + 2.0f) * fr->axis[0][i] + mid[1] * fr->axis[1][i] + along * fr->axis[2][i];
        assert(!shadow_fit_reaches(fr, &fit.cascades, c, centre, half, NULL));
        half[0] = half[1] = half[2] = 3.0f;  /* now overlapping it */
        assert(shadow_fit_reaches(fr, &fit.cascades, c, centre, half, NULL));
        half[0] = half[1] = half[2] = 1.0f;
        /* beyond the light's depth, in the square */
        for (i = 0; i < 3; i++)
            centre[i] = mid[0] * fr->axis[0][i] + mid[1] * fr->axis[1][i] + (fr->lightNear - 5.0f) * fr->axis[2][i];
        assert(!shadow_fit_reaches(fr, &fit.cascades, c, centre, half, NULL));
        assert(!shadow_fit_reaches(fr, &fit.cascades, -1, centre, half, NULL));
        assert(!shadow_fit_reaches(fr, &fit.cascades, SHADOW_FIT_CASCADES, centre, half, NULL));
    }
}

static void print_report(const struct Stats *stats, const double *changed, const double *changedOld,
                         const double *compared, const double *walkNew, const double *walkOld, const double *walkFrames)
{
    int kind, c;

    printf("texel size, world units per texel = 2 * radius / %d (this fit / today's CPU fit)\n", SHADOW_FIT_SIZE);
    for (kind = 0; kind < SCENE_KINDS; kind++) {
        const struct Stats *st = &stats[kind];

        printf("  %-12s %4.0f frames, %8.0f pixels: per pixel ratio geomean %.3f, p50 %.3f, p90 %.3f, p99 %.3f,"
               " max %.3f, larger %.2f%%; least room %.2f units; boundary pixels %.0f\n",
               scene_names[kind], st->frames, st->pixels, exp(st->logSum / st->pixels), ratio_percentile(st, 0.5),
               ratio_percentile(st, 0.9), ratio_percentile(st, 0.99), st->maxRatio, 100.0 * st->worse / st->pixels,
               st->slack, st->boundary);
        printf("  %-12s far depth of cascade 0 %.0f / %.0f, of the last %.0f / %.0f (mean)\n", "",
               st->firstNew / st->frames, st->firstOld / st->frames, st->lastNew / st->frames, st->lastOld / st->frames);
        for (c = 0; c < SHADOW_FIT_CASCADES; c++)
            printf("  %-12s cascade %d: texel %.4f / %.4f (mean over %.0f frames)\n", "", c,
                   2.0 * st->radiusNew[c] / st->used[c] / SHADOW_FIT_SIZE, 2.0 * st->radiusOld[c] / st->used[c] / SHADOW_FIT_SIZE,
                   st->used[c]);
    }
    printf("squares that change size when the camera moves 0.9 of the smallest texel (this fit / today's)\n");
    for (kind = 0; kind < SCENE_KINDS; kind++)
        printf("  %-12s %.0f / %.0f of %.0f cascades\n", scene_names[kind], changed[kind], changedOld[kind], compared[kind]);
    printf("walking a unit a frame: cascade size changes per 100 frames (this fit / today's)\n");
    for (kind = 0; kind < SCENE_KINDS; kind++)
        printf("  %-12s %.1f / %.1f over %.0f frames\n", scene_names[kind], 100.0 * walkNew[kind] / walkFrames[kind],
               100.0 * walkOld[kind] / walkFrames[kind], walkFrames[kind]);
}

int main(int argc, char **argv)
{
    static struct Stats stats[SCENE_KINDS];
    double changed[SCENE_KINDS] = {0}, changedOld[SCENE_KINDS] = {0}, compared[SCENE_KINDS] = {0},
           walkNew[SCENE_KINDS] = {0}, walkOld[SCENE_KINDS] = {0}, walkFrames[SCENE_KINDS] = {0};

    report = argc > 1 && !strcmp(argv[1], "--report");
    test_degenerate();
    test_reaches();
    test_conventions();
    test_fuzz(stats);
    test_steady(changed, changedOld, compared);
    test_walk(walkNew, walkOld, walkFrames);
    if (report)
        print_report(stats, changed, changedOld, compared, walkNew, walkOld, walkFrames);
    puts("shadow fit: every receiving pixel in its cascade, squares no wider than their pixels need, today's "
         "conventions to the bit, whole texels while the camera moves, sky, single pixels and unfittable frames passed");
    return 0;
}
