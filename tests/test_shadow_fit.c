/* render/shadow_fit.c: the shadow cascades fitted to a frame's depths, against scenes whose depths
   are cast exactly (flat ground, a slope, a wall, models, blended models before the ground, sky, a
   single pixel, nothing) from fuzzed cameras and lights; the range and the squares kept from frame
   to frame; the split; the snap to whole texels on the half-texel boundary; the view's reach for
   casters; and
   against the CPU's fit to boxes that it replaced (tests/fixtures/shadow_box_fit.c). With --motions
   it runs only the camera's motions over the scenes, in which the fit must not resize more often
   than that fit while the camera walks, passes a post, turns, zooms or quakes: half a minute, so the
   suite runs them as a test of their own (shadow_fit_motions, labelled long). With --report it runs
   everything and prints the texel sizes of both fits and how often their squares change size. */
#include "shadow_fit_scenes.h"
#include "fixtures/shadow_box_fit.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    struct ShadowFitRange range, kept;
    float split[SHADOW_FIT_CASCADES + 1];
    struct ShadowFitExtent extents[SHADOW_FIT_CASCADES];
    struct ShadowFitCascades cascades;
};

/* what a frame keeps for the next: its squares and its range */
struct Kept {
    float squares[SHADOW_FIT_CASCADES];
    struct ShadowFitRange range;
};

static const struct Kept none = {{0}, {0.0f, 0.0f}};

static void keep(struct Kept *k, const struct Fit *fit)
{
    memcpy(k->squares, fit->cascades.kept, sizeof k->squares);
    k->range = fit->kept;
}

/* the fit of the depth buffers, from what the frame before kept */
static int fit_depths(struct Fit *fit, const struct ShadowFitCamera *cam, const float *light,
                      const struct ShadowFitBox *within, size_t count, const struct ShadowFitDepths *depths,
                      const struct Kept *before)
{
    memset(fit, 0, sizeof *fit);
    if (!shadow_fit_frame(&fit->frame, cam, light) || !shadow_fit_bounds(&fit->frame, within, count))
        return 0;
    shadow_fit_range(&fit->range, &fit->frame, depths);
    shadow_fit_keep(&fit->kept, &fit->range, &before->range, &fit->frame);
    shadow_fit_split(fit->split, &fit->kept);
    shadow_fit_extents(fit->extents, &fit->frame, fit->split, depths);
    shadow_fit_cascades(&fit->cascades, &fit->frame, fit->split, fit->extents, before->squares);
    return 1;
}

static int fit_scene(struct Fit *fit, const struct ShadowFitCamera *cam, const float *light,
                     const struct ShadowFitBox *within, size_t count, const struct Kept *before)
{
    struct ShadowFitDepths d = scene_depths();

    return fit_depths(fit, cam, light, within, count, &d, before);
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
static int cpu_fit(struct BoxFit *r, const struct ShadowFitCamera *cam, const float *light,
                   const struct ShadowFitBox *boxes, size_t count)
{
    return box_fit(r, cam, SCENE_W, SCENE_H, light, boxes, count);
}

/* the half size a square needs for an extent `wide` across, a multiple of 8 units, in doubles */
static double rounded_need(double wide)
{
    return ceil((wide * 0.5 * 1.05 + 4.5) / 8.0) * 8.0;
}

/* the most a slice's square can need: its slice of the view, the screen's corners from its near
   depth (the near plane for the first) to its far one (render/shadow_fit.h), in doubles */
static double slice_most(const struct Fit *fit, int c)
{
    const struct ShadowFitFrame *f = &fit->frame;
    double lo[2] = {1e300, 1e300}, hi[2] = {-1e300, -1e300};
    int k, a;

    for (k = 0; k < 8; k++) {
        double i = k & 1 ? f->size[0] - 1.0 : 0.0, j = k & 2 ? f->size[1] - 1.0 : 0.0;
        double d = k & 4 ? fit->split[c + 1] : c ? fit->split[c] : f->nearZ;

        for (a = 0; a < 2; a++) {
            const float *p = f->pixel[a];
            double x = p[3] + d * ((i - f->centre[0]) * p[0] + (j - f->centre[1]) * p[1] + p[2]);

            lo[a] = x < lo[a] ? x : lo[a];
            hi[a] = x > hi[a] ? x : hi[a];
        }
    }
    return rounded_need(fmax(hi[0] - lo[0], hi[1] - lo[1]));
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
    double layered;   /* pixels that show a blended receiver before another */
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

/* One receiving point at view depth d and world point p: it lies in the cascade the shaders pick
   for its depth, inside the map by the soft edge and the lookup's offset, and between the light's
   depth bounds; from view space too, as the receiving shader has it. Answers its cascade. */
static int check_point(const struct Fit *fit, const struct ShadowFitCamera *cam, struct Stats *st, double d,
                       const double *p)
{
    const struct ShadowFitCascades *cs = &fit->cascades;
    double l[4], v[3], lv[4], radius, texel, margin, room;
    int c, c2, k;

    c = cascade_of(cs->farDepth, SHADOW_FIT_CASCADES, d);
    assert(cs->cascades == SHADOW_FIT_CASCADES && c < SHADOW_FIT_CASCADES);
    radius = radius_of(cs, c);
    texel = 2.0 * radius / SHADOW_FIT_SIZE;
    margin = (0.5 + 1.5 * texel) / radius;
    apply(cs->world[c], p, l);
    assert(fabs(l[0]) <= 1.0 - margin && fabs(l[1]) <= 1.0 - margin);
    assert(l[2] > 0.0 && l[2] < 1.0);
    room = (1.0 - fmax(fabs(l[0]), fabs(l[1]))) * radius - (0.5 + 1.5 * texel);
    st->slack = room < st->slack ? room : st->slack;
    for (k = 0; k < 3; k++)
        v[k] = cam->view[k] * p[0] + cam->view[3 + k] * p[1] + cam->view[6 + k] * p[2] + cam->view[9 + k];
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
    return c;
}

static int receiving(const struct Fit *fit, const struct ShadowFitCamera *cam, double d)
{
    return d >= cam->nearZ && d <= fit->frame.reach;
}

/* Every point that receives, the front and the back of each pixel, lies in its cascade (check_point)
   and every square holds them. Where no pixel shows one receiver before another, each square is
   centred on its points to the texel and at least as wide as they need, and no wider than the
   widest of: a fifth more than that rounded up to 8 units, a fifth more and 8 units, and its slice
   of the view (with 8 units for that slice measured in doubles). */
static void check_cover(const struct Fit *fit, const struct ShadowFitCamera *cam, struct Stats *st)
{
    const struct ShadowFitCascades *cs = &fit->cascades;
    double lo[SHADOW_FIT_CASCADES][2], hi[SHADOW_FIT_CASCADES][2];
    int p, c, a, side, layered = 0;

    for (c = 0; c < SHADOW_FIT_CASCADES; c++)
        for (a = 0; a < 2; a++) {
            lo[c][a] = 1e300;
            hi[c][a] = -1e300;
        }
    for (p = 0; p < SCENE_PIXELS; p++) {
        if (front[p] > 0.0f && front[p] < depth[p] && receiving(fit, cam, front[p]))
            layered++;
        for (side = 0; side < 2; side++) {
            double d = side ? depth[p] : front[p];
            const double *at = side ? hit[p] : frontHit[p];

            if (!receiving(fit, cam, d) || (side && !(depth[p] > front[p])))
                continue;
            c = check_point(fit, cam, st, d, at);
            for (a = 0; a < 2; a++) {
                const float *axis = fit->frame.axis[a];
                double x = at[0] * axis[0] + at[1] * axis[1] + at[2] * axis[2];
                lo[c][a] = x < lo[c][a] ? x : lo[c][a];
                hi[c][a] = x > hi[c][a] ? x : hi[c][a];
            }
        }
    }
    st->layered += layered;
    for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
        double radius = radius_of(cs, c), need, texel;

        if (cs->lo[c][0] <= cs->hi[c][0])
            assert(cs->kept[c] == (float)radius);
        if (lo[c][0] > hi[c][0] || layered)
            continue;
        need = fmax(hi[c][0] - lo[c][0], hi[c][1] - lo[c][1]) * 0.5 * 1.05 + 4.5;
        texel = 2.0 * radius / SHADOW_FIT_SIZE;
        assert(radius >= need - 0.02);
        assert(fmod(radius, 8.0) == 0.0);
        assert(radius <= fmax(fmax(ceil(need / 8.0) * 8.0 * 1.2, need * 1.2 + 8.0), slice_most(fit, c) + 8.0) + 0.02);
        for (a = 0; a < 2; a++) {
            double square = (cs->lo[c][a] + cs->hi[c][a]) * 0.5;
            assert(fabs(square - (lo[c][a] + hi[c][a]) * 0.5) <= texel * 0.5 + 0.02);
        }
    }
}

/* the texel each receiving pixel is looked up in, against today's */
static void compare(const struct Fit *fit, const struct BoxFit *r, struct Stats *st)
{
    const struct ShadowFitCascades *cs = &fit->cascades;
    int p, c;

    for (p = 0; p < SCENE_PIXELS; p++) {
        double d = depth[p], ratio;
        int cn, co, bin;

        if (!(d >= 10.0 && d <= fit->frame.reach))
            continue;
        cn = cascade_of(cs->farDepth, cs->cascades, d);
        co = cascade_of(r->farDepth, r->cascades, d);
        assert(cn < cs->cascades && co < r->cascades);
        ratio = (double)radius_of(cs, cn) / ((r->hi[co][0] - r->lo[co][0]) * 0.5);
        st->pixels++;
        st->logSum += log(ratio);
        st->worse += ratio > 1.000001;
        st->maxRatio = ratio > st->maxRatio ? ratio : st->maxRatio;
        bin = (int)floor(log2(ratio) * 100.0 + 0.5) + BINS / 2;
        bin = bin < 0 ? 0 : bin >= BINS ? BINS - 1 : bin;
        st->hist[bin]++;
    }
    for (c = 0; c < SHADOW_FIT_CASCADES; c++)
        if (cs->lo[c][0] <= cs->hi[c][0] && r->lo[c][0] <= r->hi[c][0]) {
            st->radiusNew[c] += radius_of(cs, c);
            st->radiusOld[c] += (r->hi[c][0] - r->lo[c][0]) * 0.5;
            st->used[c]++;
        }
    st->firstNew += cs->farDepth[0];
    st->lastNew += cs->farDepth[SHADOW_FIT_CASCADES - 1];
    st->firstOld += r->farDepth[0];
    st->lastOld += r->farDepth[SHADOW_FIT_CASCADES - 1];
    st->frames++;
}

/* ---- the tests ---- */

static struct ShadowFitBox fitBoxes[SCENE_BOXES];

/* fuzzed cameras and lights over each scene: coverage, tightness, the same bits from the same
   frame, and the texels against today's fit; each frame from nothing kept and from what the frame
   before it kept, which a camera far off does not keep */
static void test_fuzz(struct Stats *stats)
{
    int kind, n;

    for (kind = 0; kind < SCENE_KINDS; kind++) {
        struct Scene scene;
        size_t count;
        struct Kept last = none;

        scene_make(&scene, kind);
        count = scene_boxes(&scene, fitBoxes);
        stats[kind].slack = 1e300;
        for (n = 0; n < (report ? 48 : 12); n++) {
            struct Pose pose;
            struct ShadowFitCamera cam;
            struct Fit fit, again;
            struct BoxFit r;
            float light[3];
            int from;

            random_pose(&scene, &pose);
            random_light(light);
            if (n == 0) {
                light[0] = 0.5f; light[1] = 0.707f; light[2] = 0.5f;  /* Prontera's */
            }
            pose_camera(&pose, SCENE_W, SCENE_H, &cam);
            scene_render(&scene, &pose, &cam);
            for (from = 0; from < 2; from++) {
                const struct Kept *before = from ? &last : &none;

                assert(fit_scene(&fit, &cam, light, fitBoxes, count, before));
                assert(fit_scene(&again, &cam, light, fitBoxes, count, before));
                assert(!memcmp(&fit, &again, sizeof fit));
                if (fit.cascades.cascades == 0)
                    continue;
                check_cover(&fit, &cam, &stats[kind]);
            }
            keep(&last, &fit);
            if (fit.cascades.cascades && cpu_fit(&r, &cam, light, fitBoxes, count))
                compare(&fit, &r, &stats[kind]);
        }
    }
}

/* Fed the squares today's fit chose, and those as the frame before's, the cascades come out as
   today's to the bit: the same matrices, lookups and reach, so the shaders read them as they read
   today's. */
static void test_conventions(void)
{
    struct Scene scene;
    size_t count;
    int n, c, a;

    scene_make(&scene, 3);
    count = scene_boxes(&scene, fitBoxes);
    for (n = 0; n < 24; n++) {
        struct Pose pose;
        struct ShadowFitCamera cam;
        struct ShadowFitFrame frame;
        struct ShadowFitExtent ext[SHADOW_FIT_CASCADES];
        struct ShadowFitCascades out;
        struct BoxFit r;
        float light[3], split[SHADOW_FIT_CASCADES + 1], previous[SHADOW_FIT_CASCADES];

        random_pose(&scene, &pose);
        random_light(light);
        pose_camera(&pose, SCENE_W, SCENE_H, &cam);
        if (!cpu_fit(&r, &cam, light, fitBoxes, count))
            continue;
        assert(shadow_fit_frame(&frame, &cam, light) && shadow_fit_bounds(&frame, fitBoxes, count));
        assert(!memcmp(frame.axis, r.axis, sizeof frame.axis));
        assert(frame.lightNear == r.depth[0]);
        assert(frame.lightNear + frame.lightRange == r.depth[1]);
        assert(frame.bias == r.bias);
        assert(!memcmp(frame.lightView, r.lightView, sizeof frame.lightView));
        split[0] = 0.0f;
        for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
            float radius = (r.hi[c][0] - r.lo[c][0]) * 0.5f, texel, half = 0.0f;
            int q;

            split[c + 1] = r.farDepth[c];
            previous[c] = radius;
            if (!(r.lo[c][0] <= r.hi[c][0])) {
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
                float square = (r.lo[c][a] + r.hi[c][a]) * 0.5f;
                ext[c].lo[a] = square - half;
                ext[c].hi[a] = square + half;
            }
        }
        shadow_fit_cascades(&out, &frame, split, ext, previous);
        assert(out.cascades == r.cascades);
        for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
            assert(!memcmp(out.fromView[c], r.fromView[c], sizeof out.fromView[c]));
            assert(out.farDepth[c] == r.farDepth[c] && out.offset[c] == r.offset[c]);
            assert(out.blur[c] == r.blur[c] && out.slant[c] == r.slant[c]);
            assert(!memcmp(out.lo[c], r.lo[c], sizeof out.lo[c]));
            assert(!memcmp(out.hi[c], r.hi[c], sizeof out.hi[c]));
            assert(out.kept[c] == previous[c]);
        }
    }
}

/* A frame whose slices' views are `wide` units across the light at their far depths: the pixel
   (i, j) at view depth d lies at d * i along axis 0 on a screen two pixels wide, so that the
   slice from d0 to d1 spans d1 (render/shadow_fit.c, view_wide). */
static void slice_frame(struct ShadowFitFrame *frame)
{
    memset(frame, 0, sizeof *frame);
    frame->axis[0][0] = frame->axis[1][2] = frame->axis[2][1] = 1.0f;
    frame->inverse[0] = frame->inverse[4] = frame->inverse[8] = 1.0f;
    frame->pixel[0][0] = 1.0f;
    frame->size[0] = frame->size[1] = 2.0f;
    frame->nearZ = 10.0f;
    frame->lightNear = -100.0f;
    frame->lightRange = 200.0f;
}

/* The squares' sizes from the frame before's, where the slice of the view is little larger than
   what the receivers need: one that is at least what a slice needs and at most a fifth larger is
   kept; further above, or with none, the square takes what it needs grown 5 percent and rounded up
   to 8; below what it needs, a fifth more than it needs, no more than the slice of the view asks
   for; a slice with nothing in it keeps the frame before's size for the next frame and takes the 8
   units of an empty slice itself. */
static void test_hysteresis(void)
{
    struct ShadowFitFrame frame;
    struct ShadowFitExtent ext[SHADOW_FIT_CASCADES];
    struct ShadowFitCascades out;
    float split[SHADOW_FIT_CASCADES + 1] = {10, 220, 300, 1050, 1100};
    static const float half[SHADOW_FIT_CASCADES] = {100.0f, 137.25f, 500.0f, 0.0f};
    static const float most[SHADOW_FIT_CASCADES] = {120.0f, 168.0f, 560.0f, 0.0f};
    int c, a, k;

    /* slices of the view 220, 300 and 1050 across, which ask for at most 120, 168 and 560: no less
       than what the receivers need grown 5 percent, and less than a fifth more than they need */
    slice_frame(&frame);
    for (c = 0; c < SHADOW_FIT_CASCADES; c++)
        for (a = 0; a < 2; a++) {
            ext[c].lo[a] = c == 3 ? HUGE_VALF : 17.0f - half[c] * (a ? 0.5f : 1.0f);
            ext[c].hi[a] = c == 3 ? -HUGE_VALF : 17.0f + half[c] * (a ? 0.5f : 1.0f);
        }
    for (k = 0; k < 7; k++) {
        float previous[SHADOW_FIT_CASCADES];

        for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
            float need = half[c] * 1.05f + 4.5f, base = ceilf(need / 8.0f) * 8.0f;
            float grown = ceilf(need * 1.05f / 8.0f) * 8.0f;
            /* none, the base, a fifth above it, just over, just under, the grown one, far over */
            const float ways[7] = {0.0f, base, base * 1.2f, base * 1.2f + 8.0f, base - 8.0f, grown, base * 3.0f};

            previous[c] = ways[k];
        }
        shadow_fit_cascades(&out, &frame, split, ext, previous);
        assert(out.cascades == SHADOW_FIT_CASCADES);
        for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
            float need = half[c] * 1.05f + 4.5f, base = ceilf(need / 8.0f) * 8.0f;
            float grown = ceilf(need * 1.05f / 8.0f) * 8.0f, radius = radius_of(&out, c);
            float outgrown = fminf(ceilf(need * 1.2f / 8.0f) * 8.0f, most[c]);
            int kept = previous[c] >= base && previous[c] <= base * 1.2f;

            if (c == 3) {
                assert(out.lo[c][0] > out.hi[c][0] && out.kept[c] == previous[c]);
                assert(out.blur[c] == 0.5f / 16.0f);
                continue;
            }
            assert(grown >= base && grown > need * 1.05f - 0.001f && grown <= most[c]);
            assert(radius == (kept ? previous[c] : k == 4 ? outgrown : grown) && out.kept[c] == radius);
            assert(kept == (k == 1 || k == 2 || (k == 5 && grown <= base * 1.2f)));
            assert(out.offset[c] == 1.5f * (2.0f * radius / SHADOW_FIT_SIZE));
        }
    }
}

/* The squares' sizes where the slice of the view is larger than the receivers need, as when the
   receivers fill only part of it: one up to what the slice of the view asks for is kept, however
   much larger than the receivers need, so that receivers coming and going within a slice change
   nothing; above that it is taken anew, and what it takes anew is no larger than the slice of the
   view asks for, nor smaller than the receivers need. */
static void test_slice_keeps(void)
{
    struct ShadowFitFrame frame;
    struct ShadowFitExtent ext[SHADOW_FIT_CASCADES];
    struct ShadowFitCascades out;
    /* the slices' views 600, 900, 1200 and 1500 across: at most 320, 480, 640 and 792 */
    float split[SHADOW_FIT_CASCADES + 1] = {10, 600, 900, 1200, 1500};
    static const float half[SHADOW_FIT_CASCADES] = {40.0f, 200.0f, 590.0f, 1.0f};
    static const float most[SHADOW_FIT_CASCADES] = {320.0f, 480.0f, 640.0f, 792.0f};
    int c, a, k;

    slice_frame(&frame);
    for (c = 0; c < SHADOW_FIT_CASCADES; c++)
        for (a = 0; a < 2; a++) {
            ext[c].lo[a] = -half[c];
            ext[c].hi[a] = half[c];
        }
    for (k = 0; k < 5; k++) {
        float previous[SHADOW_FIT_CASCADES];

        for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
            float base = ceilf((half[c] * 1.05f + 4.5f) / 8.0f) * 8.0f;
            /* none, far above the receivers' need up to the slice's, just over that, under the need */
            const float ways[5] = {0.0f, most[c], most[c] - 8.0f, most[c] + 8.0f, base - 8.0f};

            previous[c] = ways[k];
        }
        shadow_fit_cascades(&out, &frame, split, ext, previous);
        for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
            float need = half[c] * 1.05f + 4.5f, base = ceilf(need / 8.0f) * 8.0f;
            float grown = ceilf(need * (k == 4 ? 1.2f : 1.05f) / 8.0f) * 8.0f, radius = radius_of(&out, c);
            float top = base * 1.2f > most[c] ? base * 1.2f : most[c];
            int kept = previous[c] >= base && previous[c] <= top;

            assert(radius == (kept ? previous[c] : fmaxf(base, fminf(grown, most[c]))) && out.kept[c] == radius);
            assert(kept == (k == 1 || k == 2 || (k == 3 && base * 1.2f >= most[c] + 8.0f)));
        }
        /* the third slice's receivers need less than 5 percent below its slice of the view: grown
           only up to it, and kept up to a fifth over what they need */
        assert(radius_of(&out, 2) == (k >= 1 && k <= 3 ? previous[2] : 640.0f));
    }
}

/* The range kept from frame to frame: the far end grows at once to hold every receiver, a
   hundredth ahead, or to the reach where the reach lies within a tenth beyond the farthest, and is
   kept while it is at most a tenth beyond the farthest and within the reach; the near end is kept
   while the nearest lies from it to a quarter beyond it, so never beyond the far end; nothing kept
   or a range not finite take the measured one, and where nothing receives the frame before's
   stays. */
static void test_keep(void)
{
    struct ShadowFitFrame frame;
    struct ShadowFitRange kept, previous, measured;
    static const struct ShadowFitRange empty = {HUGE_VALF, 0.0f};

    memset(&frame, 0, sizeof frame);
    frame.reach = 1000.0f;
    /* nothing kept: the measured range, the far end a hundredth ahead */
    previous.nearest = previous.farthest = 0.0f;
    measured.nearest = 200.0f;
    measured.farthest = 500.0f;
    shadow_fit_keep(&kept, &measured, &previous, &frame);
    assert(kept.nearest == 200.0f && kept.farthest == 500.0f * 1.01f);
    /* kept while it serves */
    previous = kept;
    measured.nearest = 249.0f;
    measured.farthest = 460.0f;
    shadow_fit_keep(&kept, &measured, &previous, &frame);
    assert(!memcmp(&kept, &previous, sizeof kept));
    measured.nearest = 200.0f;
    measured.farthest = 500.0f * 1.01f;
    shadow_fit_keep(&kept, &measured, &previous, &frame);
    assert(!memcmp(&kept, &previous, sizeof kept));
    /* the far end: grown at once past the farthest, to the reach where that lies within a tenth
       beyond it; taken anew more than a tenth beyond it */
    measured.farthest = 506.0f;
    shadow_fit_keep(&kept, &measured, &previous, &frame);
    assert(kept.nearest == 200.0f && kept.farthest == 506.0f * 1.01f);
    measured.farthest = 900.0f;
    shadow_fit_keep(&kept, &measured, &previous, &frame);
    assert(kept.farthest == 900.0f * 1.01f);
    measured.farthest = 910.0f;
    shadow_fit_keep(&kept, &measured, &previous, &frame);
    assert(kept.farthest == 1000.0f);
    measured.farthest = 999.0f;
    shadow_fit_keep(&kept, &measured, &previous, &frame);
    assert(kept.farthest == 1000.0f);
    measured.farthest = 450.0f;
    shadow_fit_keep(&kept, &measured, &previous, &frame);
    assert(kept.farthest == 450.0f * 1.01f);
    /* a reach that drew in below it */
    frame.reach = 480.0f;
    measured.farthest = 476.0f;
    shadow_fit_keep(&kept, &measured, &previous, &frame);
    assert(kept.farthest == 480.0f);
    frame.reach = 1000.0f;
    /* the near end: taken anew nearer than it or a quarter beyond it, so kept only before the far
       end */
    measured.farthest = 480.0f;
    measured.nearest = 199.0f;
    shadow_fit_keep(&kept, &measured, &previous, &frame);
    assert(kept.nearest == 199.0f && kept.farthest == previous.farthest);
    measured.nearest = 251.0f;
    shadow_fit_keep(&kept, &measured, &previous, &frame);
    assert(kept.nearest == 251.0f);
    previous.nearest = 390.0f;
    measured.nearest = 400.0f;
    measured.farthest = 400.0f;
    shadow_fit_keep(&kept, &measured, &previous, &frame);
    assert(kept.nearest == 390.0f && kept.farthest == 400.0f * 1.01f);
    /* nothing receives: the frame before's stays */
    previous.nearest = 200.0f;
    previous.farthest = 505.0f;
    shadow_fit_keep(&kept, &empty, &previous, &frame);
    assert(!memcmp(&kept, &previous, sizeof kept));
    /* a range kept that is not finite is taken anew */
    previous.nearest = NAN;
    previous.farthest = INFINITY;
    measured.nearest = 200.0f;
    measured.farthest = 500.0f;
    shadow_fit_keep(&kept, &measured, &previous, &frame);
    assert(kept.nearest == 200.0f && kept.farthest == 500.0f * 1.01f);
}

/* The split: from the nearest to a unit beyond the farthest, each slice's far depth the same
   multiple of its near one, within the rounding of a division and two square roots; one depth
   (nearest and farthest the same) still gets slices that grow; nothing receives, all 0. */
static void test_split(void)
{
    static const struct ShadowFitRange ranges[] = {
        {40.0f, 448.0f}, {10.0f, 5000.0f}, {345.0f, 2100.0f}, {600.0f, 600.0f}, {0.25f, 1.0e6f}, {1.0f, 1.0f},
    };
    static const struct ShadowFitRange empty[] = {{HUGE_VALF, 0.0f}, {0.0f, 100.0f}, {-5.0f, 100.0f}, {200.0f, 100.0f}};
    float split[SHADOW_FIT_CASCADES + 1];
    size_t i;
    int c;

    for (i = 0; i < sizeof ranges / sizeof *ranges; i++) {
        double last = (double)ranges[i].farthest + 1.0, ratio = pow(last / ranges[i].nearest, 0.25);

        shadow_fit_split(split, &ranges[i]);
        assert(split[0] == ranges[i].nearest && split[SHADOW_FIT_CASCADES] == ranges[i].farthest + 1.0f);
        for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
            assert(split[c + 1] > split[c]);
            assert(fabs(split[c + 1] / (double)split[c] / ratio - 1.0) < 1e-6);
        }
    }
    for (i = 0; i < sizeof empty / sizeof *empty; i++) {
        shadow_fit_split(split, &empty[i]);
        for (c = 0; c <= SHADOW_FIT_CASCADES; c++)
            assert(split[c] == 0.0f);
    }
}

/* A square's centre on whole texels is the nearest multiple of a texel to the receivers' centre,
   ties to the greater, whatever the radius (texels that are not powers of two among them) and
   wherever the centre lies: on the half-texel boundary and a few units in the last place either
   side of it, where a division that rounds differently, as a GPU's may, would pick the other. */
static void test_snap(void)
{
    static const float radii[] = {8.0f, 24.0f, 200.0f, 1000.0f, 1048.0f, 4096.0f};
    struct ShadowFitFrame frame;
    struct ShadowFitExtent ext[SHADOW_FIT_CASCADES];
    struct ShadowFitCascades out;
    float split[SHADOW_FIT_CASCADES + 1] = {10, 100, 200, 300, 400};
    int r, s, u, c, a, checked = 0;

    slice_frame(&frame);
    frame.pixel[0][0] = 0.0f;
    for (r = 0; r < (int)(sizeof radii / sizeof radii[0]); r++) {
        float radius = radii[r], texel = 2.0f * radius / SHADOW_FIT_SIZE, previous[SHADOW_FIT_CASCADES];
        /* receivers that need half a unit less than this radius */
        float half = (radius - 5.0f) / 1.05f;

        for (c = 0; c < SHADOW_FIT_CASCADES; c++)
            previous[c] = radius;
        for (s = -3; s <= 3; s++)
            for (u = -4; u <= 4; u++) {
                /* (n + 1/2) texels, n from a range of places, moved u units in the last place */
                float n = (float)(s * 1237 + 610), centre = (n + 0.5f) * texel;

                for (a = 0; a < abs(u); a++)
                    centre = nextafterf(centre, u < 0 ? -INFINITY : INFINITY);
                for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
                    ext[c].lo[0] = centre - half;
                    ext[c].hi[0] = centre + half;
                    ext[c].lo[1] = -half;
                    ext[c].hi[1] = half;
                }
                shadow_fit_cascades(&out, &frame, split, ext, previous);
                for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
                    float actual = (ext[c].lo[0] + ext[c].hi[0]) * 0.5f;
                    double m, got;

                    assert(radius_of(&out, c) == radius);
                    got = ((double)out.lo[c][0] + out.hi[c][0]) * 0.5;
                    /* the nearest multiple of the texel to the float centre the C takes, ties up */
                    m = floor((double)actual / texel + 0.5);
                    if ((m - 0.5) * texel > actual)
                        m -= 1.0;
                    else if ((m + 0.5) * texel <= actual)
                        m += 1.0;
                    assert(got == m * texel && out.lo[c][0] == (float)(m * texel - radius));
                    assert(out.world[c][12] == (float)(-m / 1024.0));
                    checked++;
                }
            }
    }
    assert(checked == 6 * 7 * 9 * SHADOW_FIT_CASCADES);
}

/* Moving the camera by less than a texel moves a square that keeps its size by whole texels, with
   the rest of its matrix the same bits; counted, how often a square changes size instead, for this
   fit and today's */
static void test_steady(double *changed, double *changedOld, double *compared)
{
    int kind, n, c, k;

    for (kind = 0; kind < SCENE_KINDS; kind++) {
        struct Scene scene;
        size_t count;

        scene_make(&scene, kind);
        count = scene_boxes(&scene, fitBoxes);
        for (n = 0; n < (report ? 24 : 6); n++) {
            struct Pose pose, moved;
            struct ShadowFitCamera cam, cam2;
            struct Fit fit, fit2;
            struct BoxFit r, r2;
            struct Kept kept;
            float light[3], smallest = 1e30f;
            double angle = uniform(0, 2 * PI), step;
            int old;

            random_pose(&scene, &pose);
            random_light(light);
            pose_camera(&pose, SCENE_W, SCENE_H, &cam);
            scene_render(&scene, &pose, &cam);
            assert(fit_scene(&fit, &cam, light, fitBoxes, count, &none));
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
            keep(&kept, &fit);
            assert(fit_scene(&fit2, &cam2, light, fitBoxes, count, &kept));
            old = cpu_fit(&r, &cam, light, fitBoxes, count) && cpu_fit(&r2, &cam2, light, fitBoxes, count);
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
                if (old && r.hi[c][0] - r.lo[c][0] != r2.hi[c][0] - r2.lo[c][0])
                    changedOld[kind]++;
            }
        }
    }
}

/* Moving the camera along a path, what each frame keeps handed to the next: how many frames change
   a square's size, for this fit and today's. Each motion over each scene from three distances, 120
   frames each: walking a unit a frame along the ground; walking towards a post that comes into the
   bottom of the view and leaves it, and back; turning a quarter of a degree a frame; zooming 0.3
   percent a frame; and quaking, the camera tilted up and down by a degree that dies away, as a
   skill's earthquake shakes it (world/view.c). Over its three paths, this fit must not change sizes
   more often than today's on any motion over any scene. A single path may: this fit follows what
   the frame shows, today's what the boxes hold, so where the camera walks into a wall that comes to
   fill the view, this fit's squares shrink with the view and today's, which do not see the wall hide
   what lies behind it, stay; and a square that grows past a multiple of 8 units is not always
   today's that grows past one. --report lists those paths. A square that keeps its size moves by
   whole texels. */
#define MOTIONS 5
static const char *const motion_names[MOTIONS] = {"walking 1 unit", "passing a post", "turning 0.25 degrees",
                                                  "zooming 0.3%", "quaking 1 degree"};

/* the post: where the bottom of the view meets the ground halfway along the walk */
static void place_post(struct Scene *scene, const double *focus, double yaw, double pitch, double distance, double mid)
{
    struct Pose pose;
    struct ShadowFitCamera cam;
    double at[3], ray[3], v1, t;
    int k;

    at[0] = focus[0] + mid * sin(yaw);
    at[2] = focus[2] + mid * cos(yaw);
    at[1] = ground_height(scene, at[0], at[2]);
    pose_look(&pose, at, yaw, pitch, distance);
    pose_camera(&pose, SCENE_W, SCENE_H, &cam);
    v1 = (SCENE_H - 1 - cam.centre[1]) / cam.proj[1];
    for (k = 0; k < 3; k++)
        ray[k] = pose.axis[1][k] * v1 + pose.axis[2][k];
    t = (at[1] - pose.eye[1]) / ray[1];
    k = scene->count < MAX_BOXES ? scene->count++ : MAX_BOXES - 1;
    scene->box[k][0][0] = floor(pose.eye[0] + t * ray[0] - 12.0);
    scene->box[k][1][0] = scene->box[k][0][0] + 24.0;
    scene->box[k][0][2] = floor(pose.eye[2] + t * ray[2] - 12.0);
    scene->box[k][1][2] = scene->box[k][0][2] + 24.0;
    scene->box[k][1][1] = 0.0;
    scene->box[k][0][1] = -floor(distance * 0.3);
    scene->blended[k] = 0;
}

static void test_motion(double (*changes)[SCENE_KINDS], double (*changesOld)[SCENE_KINDS],
                        double (*frames)[SCENE_KINDS], double (*growth)[SCENE_KINDS], double (*growthOld)[SCENE_KINDS],
                        double (*squares)[SCENE_KINDS])
{
    int kind, m, n, f, c, k, length = 120;

    for (m = 0; m < MOTIONS; m++)
        for (kind = 0; kind < SCENE_KINDS; kind++) {
            for (n = 0; n < 3; n++) {
                struct Scene scene;
                size_t count;
                struct Kept kept = none;
                float light[3], last[SHADOW_FIT_CASCADES], lastOld[SHADOW_FIT_CASCADES];
                float lastWorld[SHADOW_FIT_CASCADES][16];
                double focus[3], yaw = uniform(0, 2 * PI), pitch = uniform(20, 60) * PI / 180.0;
                double distance = n == 0 ? 300.0 : n == 1 ? 1000.0 : 3000.0, heading = uniform(0, 2 * PI);
                double changed = 0.0, changedOld = 0.0;

                random_light(light);
                focus[0] = uniform(-300, 300);
                focus[2] = uniform(-300, 300);
                scene_make(&scene, kind);
                if (m == 1) {
                    /* walking towards the post along the view's heading, and back */
                    heading = yaw;
                    place_post(&scene, focus, yaw, pitch, distance, length * 0.25);
                }
                count = scene_boxes(&scene, fitBoxes);
                for (f = 0; f < length; f++) {
                    struct Pose pose;
                    struct ShadowFitCamera cam;
                    struct Fit fit;
                    struct BoxFit r;
                    double walk = m == 0 ? f : m == 1 ? (f < length / 2 ? f : length - f) * 0.5 : 0.0;
                    double turn = m == 2 ? f * 0.25 * PI / 180.0 : 0.0, zoom = m == 3 ? pow(1.003, f) : 1.0;
                    double quake = m == 4 ? (f & 1 ? 1.0 : -1.0) * (1.0 - (double)f / length) * PI / 180.0 : 0.0;
                    double fx = focus[0] + walk * sin(heading), fz = focus[2] + walk * cos(heading);
                    double at[3] = {fx, scene.ground ? ground_height(&scene, fx, fz) : 0.0, fz};

                    pose_look(&pose, at, yaw + turn, pitch + quake, distance * zoom);
                    pose_camera(&pose, SCENE_W, SCENE_H, &cam);
                    scene_render(&scene, &pose, &cam);
                    assert(fit_scene(&fit, &cam, light, fitBoxes, count, &kept));
                    assert(cpu_fit(&r, &cam, light, fitBoxes, count));
                    for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
                        float now = radius_of(&fit.cascades, c);
                        float nowOld = (r.hi[c][0] - r.lo[c][0]) * 0.5f;

                        /* a size change of a square that holds receivers in both frames: a slice
                           that empties or fills draws no edge that could move */
                        if (f > 0) {
                            changed += now > 0.0f && last[c] > 0.0f && now != last[c];
                            changedOld += nowOld > 0.0f && lastOld[c] > 0.0f && nowOld != lastOld[c];
                            if (now == last[c] && fit.cascades.lo[c][0] <= fit.cascades.hi[c][0]) {
                                assert(!memcmp(fit.cascades.world[c], lastWorld[c], 12 * sizeof(float)));
                                for (k = 12; k < 14; k++) {
                                    double texels = ((double)fit.cascades.world[c][k] - (double)lastWorld[c][k])
                                                    * (SHADOW_FIT_SIZE / 2);
                                    assert(texels == floor(texels));
                                }
                            }
                        }
                        if (fit.cascades.lo[c][0] <= fit.cascades.hi[c][0] && r.lo[c][0] <= r.hi[c][0]) {
                            double base = rounded_need(fmax(fit.extents[c].hi[0] - fit.extents[c].lo[0],
                                                            fit.extents[c].hi[1] - fit.extents[c].lo[1]));

                            growth[m][kind] += now / base;
                            growthOld[m][kind] += nowOld / base;
                            squares[m][kind]++;
                        }
                        last[c] = now;
                        lastOld[c] = nowOld;
                        memcpy(lastWorld[c], fit.cascades.world[c], sizeof lastWorld[c]);
                    }
                    keep(&kept, &fit);
                    frames[m][kind] += f > 0;
                }
                if (report && changed > changedOld)
                    printf("%s over %s from %.0f: %.0f size changes, today's %.0f\n", motion_names[m],
                           scene_names[kind], distance, changed, changedOld);
                changes[m][kind] += changed;
                changesOld[m][kind] += changedOld;
            }
            fflush(stdout);
            assert(changes[m][kind] <= changesOld[m][kind]);
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
   view of nothing but sky, a single pixel, a blended pixel alone, nothing at all, depths out of
   reach or not numbers, and frames that cannot be fitted */
static void test_degenerate(void)
{
    static const float down[3] = {0, 1, 0}, level[3] = {1, 0, 0}, prontera[3] = {0.5f, 0.707f, 0.5f};
    static const float noLight[3] = {0, 0, 0};
    const float *lights[3] = {prontera, down, level};
    struct Scene scene;
    struct Pose pose;
    struct ShadowFitCamera cam;
    struct ShadowFitDepths depths = scene_depths();
    struct Fit fit;
    struct Stats st;
    double focus[3] = {50, 0, -80};
    size_t count;
    int l, c, p;

    memset(&st, 0, sizeof st);
    st.slack = 1e300;
    scene_make(&scene, 0);
    count = scene_boxes(&scene, fitBoxes);
    for (l = 0; l < 3; l++) {
        float len = sqrtf(lights[l][0] * lights[l][0] + lights[l][1] * lights[l][1] + lights[l][2] * lights[l][2]);

        /* straight down */
        pose_look(&pose, focus, 0.3, PI / 2, 500.0);
        pose_camera(&pose, SCENE_W, SCENE_H, &cam);
        scene_render(&scene, &pose, &cam);
        assert(fit_scene(&fit, &cam, lights[l], fitBoxes, count, &none) && fit.cascades.cascades);
        check_finite(&fit.cascades);
        check_cover(&fit, &cam, &st);
        /* along the light */
        pose_look(&pose, focus, atan2(lights[l][0], lights[l][2]), asin(lights[l][1] / len), 500.0);
        pose_camera(&pose, SCENE_W, SCENE_H, &cam);
        scene_render(&scene, &pose, &cam);
        assert(fit_scene(&fit, &cam, lights[l], fitBoxes, count, &none));
        check_finite(&fit.cascades);
        if (fit.cascades.cascades)
            check_cover(&fit, &cam, &st);
    }

    /* looking up from high over the ground: all sky; the squares and the range of the frame before
       are kept */
    {
        double above[3] = {50, -1000, -80};
        static const struct Kept before = {{16, 24, 512, 4096}, {120.0f, 900.0f}};
        pose_look(&pose, above, 0.0, -0.3, 500.0);
        pose_camera(&pose, SCENE_W, SCENE_H, &cam);
        scene_render(&scene, &pose, &cam);
        assert(fit_scene(&fit, &cam, prontera, fitBoxes, count, &before));
        assert(fit.cascades.cascades == 0 && fit.range.nearest > fit.range.farthest);
        assert(!memcmp(fit.cascades.kept, before.squares, sizeof before.squares));
        assert(!memcmp(&fit.kept, &before.range, sizeof before.range));
        check_finite(&fit.cascades);
        /* and from nothing kept, nothing at all */
        assert(fit_scene(&fit, &cam, prontera, fitBoxes, count, &none));
        assert(fit.cascades.cascades == 0);
    }
    check_finite(&fit.cascades);
    for (c = 0; c < SHADOW_FIT_CASCADES; c++)
        assert(fit.cascades.farDepth[c] == 0.0f && fit.cascades.lo[c][0] > fit.cascades.hi[c][0]);

    /* a single pixel: its slice gets the smallest square around it, the others none */
    pose_look(&pose, focus, 0.0, 0.8, 500.0);
    pose_camera(&pose, SCENE_W, SCENE_H, &cam);
    scene_render(&scene, &pose, &cam);
    for (p = 0; p < SCENE_PIXELS; p++)
        if (p != 77 * SCENE_W + 130)
            depth[p] = front[p] = 0.0f;
    assert(depth[77 * SCENE_W + 130] > 0.0f);
    assert(fit_scene(&fit, &cam, prontera, fitBoxes, count, &none) && fit.cascades.cascades);
    assert(fit.range.nearest == fit.range.farthest && fit.kept.farthest == fit.range.farthest * 1.01f);
    assert(fit.split[SHADOW_FIT_CASCADES] == fit.kept.farthest + 1.0f);
    check_finite(&fit.cascades);
    check_cover(&fit, &cam, &st);
    for (c = 0; c < SHADOW_FIT_CASCADES; c++)
        assert(radius_of(&fit.cascades, c) == 8.0f || fit.cascades.lo[c][0] > fit.cascades.hi[c][0]);
    /* the same pixel blended before the sky (front only) fits the same */
    {
        float d = depth[77 * SCENE_W + 130];
        struct Fit alone;

        depth[77 * SCENE_W + 130] = 0.0f;
        assert(fit_scene(&alone, &cam, prontera, fitBoxes, count, &none));
        assert(!memcmp(&alone.cascades, &fit.cascades, sizeof fit.cascades));
        /* and before a back half as deep again: the range holds both */
        depth[77 * SCENE_W + 130] = d * 1.5f;
        assert(fit_scene(&alone, &cam, prontera, fitBoxes, count, &none));
        assert(alone.range.nearest == d && alone.range.farthest == d * 1.5f);
        depth[77 * SCENE_W + 130] = d;
    }

    /* nothing: no pixels, or none that receives */
    depths.width = depths.height = 0;
    assert(fit_depths(&fit, &cam, prontera, fitBoxes, count, &depths, &none) && fit.cascades.cascades == 0);
    for (p = 0; p < SCENE_PIXELS; p++)
        depth[p] = front[p] = p % 3 == 0 ? fit.frame.reach * 1.5f : p % 3 == 1 ? 5.0f : NAN;
    assert(fit_scene(&fit, &cam, prontera, fitBoxes, count, &none) && fit.cascades.cascades == 0);
    check_finite(&fit.cascades);
    /* a back that is not a number behind a front that receives takes the front alone, and a back
       beyond the reach the part up to it */
    front[2] = 50.0f;
    assert(fit_scene(&fit, &cam, prontera, fitBoxes, count, &none) && fit.cascades.cascades);
    assert(fit.range.nearest == 50.0f && fit.range.farthest == 50.0f);
    front[0] = 60.0f;
    assert(fit_scene(&fit, &cam, prontera, fitBoxes, count, &none) && fit.cascades.cascades);
    assert(fit.range.nearest == 50.0f && fit.range.farthest == fit.frame.reach);
    assert(fit.kept.farthest == fit.frame.reach);

    /* frames that cannot be fitted: no light, a light that is not a number, no boxes, boxes that
       are empty or not finite, a view that cannot be inverted, a screen of no pixels */
    assert(!fit_scene(&fit, &cam, noLight, fitBoxes, count, &none));
    {
        float bad[3] = {NAN, 1.0f, 0.0f}, huge[3] = {INFINITY, 1.0f, 0.0f};
        struct ShadowFitBox empty[2] = {{{1, 1, 1}, {0, 0, 0}}, {{-INFINITY, 0, 0}, {0, 0, 0}}};
        struct ShadowFitCamera flat = cam;

        assert(!fit_scene(&fit, &cam, bad, fitBoxes, count, &none));
        assert(!fit_scene(&fit, &cam, huge, fitBoxes, count, &none));
        assert(!fit_scene(&fit, &cam, prontera, fitBoxes, 0, &none));
        assert(!fit_scene(&fit, &cam, prontera, empty, 2, &none));
        memset(flat.view, 0, sizeof flat.view);
        assert(!fit_scene(&fit, &flat, prontera, fitBoxes, count, &none));
        flat = cam;
        flat.width = 0;
        assert(!fit_scene(&fit, &flat, prontera, fitBoxes, count, &none));
    }
}

/* The view's reach for casters: every receiving point of fuzzed frames, with the disc its lookup
   reads around it in its cascade, lies inside it across the light and no farther than its
   farthest point along the light. */
static void test_reach(void)
{
    int kind, n, p, c, a, side;

    for (kind = 0; kind < SCENE_KINDS; kind++) {
        struct Scene scene;
        size_t count;

        scene_make(&scene, kind);
        count = scene_boxes(&scene, fitBoxes);
        for (n = 0; n < 8; n++) {
            struct Pose pose;
            struct ShadowFitCamera cam;
            struct ShadowFitReach reach;
            struct Fit fit;
            float light[3];
            const struct ShadowFitFrame *fr = &fit.frame;

            random_pose(&scene, &pose);
            random_light(light);
            pose_camera(&pose, SCENE_W, SCENE_H, &cam);
            scene_render(&scene, &pose, &cam);
            assert(fit_scene(&fit, &cam, light, fitBoxes, count, &none));
            shadow_fit_reach(&reach, fr);
            for (p = 0; p < SCENE_PIXELS && fit.cascades.cascades; p++)
                for (side = 0; side < 2; side++) {
                    double d = side ? depth[p] : front[p], x[3], disc;
                    const double *at = side ? hit[p] : frontHit[p];

                    if (!receiving(&fit, &cam, d))
                        continue;
                    c = cascade_of(fit.cascades.farDepth, SHADOW_FIT_CASCADES, d);
                    disc = fit.cascades.offset[c] + fit.cascades.blur[c] * 2.0 * radius_of(&fit.cascades, c);
                    for (a = 0; a < 3; a++)
                        x[a] = at[0] * fr->axis[a][0] + at[1] * fr->axis[a][1] + at[2] * fr->axis[a][2];
                    for (a = 0; a < 2; a++)
                        assert(x[a] - disc >= reach.lo[a] && x[a] + disc <= reach.hi[a]);
                    assert(x[2] <= reach.farthest + 0.01);
                }
        }
    }
}

static void print_report(const struct Stats *stats, const double *changed, const double *changedOld,
                         const double *compared, double (*mNew)[SCENE_KINDS], double (*mOld)[SCENE_KINDS],
                         double (*mFrames)[SCENE_KINDS], double (*mGrowth)[SCENE_KINDS],
                         double (*mGrowthOld)[SCENE_KINDS], double (*mSquares)[SCENE_KINDS])
{
    int kind, c, m;

    printf("texel size, world units per texel = 2 * radius / %d (this fit / today's CPU fit)\n",
           SHADOW_FIT_SIZE);
    for (kind = 0; kind < SCENE_KINDS; kind++) {
        const struct Stats *st = &stats[kind];

        printf("  %-12s %4.0f frames, %8.0f pixels: per pixel ratio geomean %.3f, p50 %.3f, p90 %.3f, p99 %.3f,"
               " max %.3f, larger %.2f%%; least room %.2f units; boundary pixels %.0f; layered pixels %.0f\n",
               scene_names[kind], st->frames, st->pixels, exp(st->logSum / st->pixels), ratio_percentile(st, 0.5),
               ratio_percentile(st, 0.9), ratio_percentile(st, 0.99), st->maxRatio, 100.0 * st->worse / st->pixels,
               st->slack, st->boundary, st->layered);
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
    for (m = 0; m < MOTIONS; m++) {
        printf("%s a frame: cascade size changes per 100 frames (this fit / today's), and the squares' mean half "
               "size over what the pixels need rounded up to 8 units (this fit / today's)\n", motion_names[m]);
        for (kind = 0; kind < SCENE_KINDS; kind++)
            printf("  %-12s %.1f / %.1f over %.0f frames; %.3f / %.3f\n", scene_names[kind],
                   100.0 * mNew[m][kind] / mFrames[m][kind], 100.0 * mOld[m][kind] / mFrames[m][kind], mFrames[m][kind],
                   mGrowth[m][kind] / mSquares[m][kind], mGrowthOld[m][kind] / mSquares[m][kind]);
    }
}

int main(int argc, char **argv)
{
    static struct Stats stats[SCENE_KINDS];
    double changed[SCENE_KINDS] = {0}, changedOld[SCENE_KINDS] = {0}, compared[SCENE_KINDS] = {0};
    static double mNew[MOTIONS][SCENE_KINDS], mOld[MOTIONS][SCENE_KINDS], mFrames[MOTIONS][SCENE_KINDS],
        mGrowth[MOTIONS][SCENE_KINDS], mGrowthOld[MOTIONS][SCENE_KINDS], mSquares[MOTIONS][SCENE_KINDS];

    report = argc > 1 && !strcmp(argv[1], "--report");
    if (argc > 1 && !strcmp(argv[1], "--motions")) {
        test_motion(mNew, mOld, mFrames, mGrowth, mGrowthOld, mSquares);
        puts("shadow fit in motion: whole texels and no more size changes than today's while the camera walks, "
             "passes a post, turns, zooms and quakes passed");
        return 0;
    }
    test_degenerate();
    test_hysteresis();
    test_slice_keeps();
    test_keep();
    test_split();
    test_snap();
    test_reach();
    test_conventions();
    test_fuzz(stats);
    test_steady(changed, changedOld, compared);
    if (report) {
        test_motion(mNew, mOld, mFrames, mGrowth, mGrowthOld, mSquares);
        print_report(stats, changed, changedOld, compared, mNew, mOld, mFrames, mGrowth, mGrowthOld, mSquares);
    }
    puts("shadow fit: every receiving point in its cascade, front and back, squares no wider than their points "
         "need or their slice of the view and kept while they serve, the range kept, the slices geometric, "
         "centres on the nearest whole "
         "texel at the half-texel boundary, today's conventions to the bit, whole texels where the camera moves "
         "less than a texel, the view's reach for casters, sky, single pixels and unfittable frames passed");
    return 0;
}
