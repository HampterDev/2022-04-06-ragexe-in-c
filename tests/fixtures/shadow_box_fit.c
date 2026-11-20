/* The CPU's shadow fit to boxes (tests/fixtures/shadow_box_fit.h), renderer_shadow_frame's code
   as the client had it at 5ea7b76b with the renderer's fields made arguments. Compiled as the
   client is: each float operation rounded once, in the order written. */
#include "shadow_box_fit.h"
#include <math.h>
#include <string.h>

#if defined(__clang__)
#pragma STDC FP_CONTRACT OFF
#endif

#define SHADOW_SPLIT 0.7f
#define SHADOW_MARGIN 8.0f
#define SHADOW_BIAS 4.0f
#define SHADOW_OFFSET 1.5f
#define SHADOW_SOFTNESS 0.5f

static int invert_4x3(const float *m, float *out)
{
    float det = m[0] * (m[4] * m[8] - m[5] * m[7]) - m[1] * (m[3] * m[8] - m[5] * m[6])
                + m[2] * (m[3] * m[7] - m[4] * m[6]);
    float inv;
    int i;

    if (!(det > 1e-12f || det < -1e-12f))
        return 0;
    inv = 1.0f / det;
    out[0] = (m[4] * m[8] - m[5] * m[7]) * inv;
    out[1] = (m[2] * m[7] - m[1] * m[8]) * inv;
    out[2] = (m[1] * m[5] - m[2] * m[4]) * inv;
    out[3] = (m[5] * m[6] - m[3] * m[8]) * inv;
    out[4] = (m[0] * m[8] - m[2] * m[6]) * inv;
    out[5] = (m[2] * m[3] - m[0] * m[5]) * inv;
    out[6] = (m[3] * m[7] - m[4] * m[6]) * inv;
    out[7] = (m[1] * m[6] - m[0] * m[7]) * inv;
    out[8] = (m[0] * m[4] - m[1] * m[3]) * inv;
    for (i = 0; i < 3; i++)
        out[9 + i] = -(m[9] * out[i] + m[10] * out[3 + i] + m[11] * out[6 + i]);
    return 1;
}

static float dot3(const float *a, const float *b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

/* a slice of the view from one view depth to another: its corners, the corner rays' at the
   nearer depth and then at the farther (bit 1 across, bit 2 down, as the rays), and the planes
   that bound it, inside where n . p + n[3] >= 0; world coordinates */
struct ViewSlice {
    float corner[8][3];
    float plane[6][4];
};

/* the points of the view found inside the boxes: their view depths and where they lie across the
   light (axes r and u) */
struct ShadowFit {
    const float *view, *r, *u;
    float nearest, farthest, lo[2], hi[2];
    int points;
};

static void view_slice(struct ViewSlice *s, const float *view, const float *eye, const float (*ray)[3], float d0,
                       float d1)
{
    static const unsigned char sides[4][2] = {{0, 1}, {2, 3}, {0, 2}, {1, 3}};
    float middle[3];
    int i, k;

    for (k = 0; k < 8; k++)
        for (i = 0; i < 3; i++)
            s->corner[k][i] = eye[i] + ray[k & 3][i] * (k < 4 ? d0 : d1);
    for (i = 0; i < 3; i++) {
        s->plane[0][i] = view[2 + 3 * i];
        s->plane[1][i] = -view[2 + 3 * i];
        middle[i] = ray[0][i] + ray[1][i] + ray[2][i] + ray[3][i];
    }
    s->plane[0][3] = view[11] - d0;
    s->plane[1][3] = d1 - view[11];
    for (k = 0; k < 4; k++) {
        const float *a = ray[sides[k][0]], *b = ray[sides[k][1]];
        float *n = s->plane[2 + k], sign;

        n[0] = a[1] * b[2] - a[2] * b[1];
        n[1] = a[2] * b[0] - a[0] * b[2];
        n[2] = a[0] * b[1] - a[1] * b[0];
        sign = dot3(n, middle) < 0.0f ? -1.0f : 1.0f;
        for (i = 0; i < 3; i++)
            n[i] *= sign;
        n[3] = -dot3(n, eye);
    }
}

static void fit_reset(struct ShadowFit *fit)
{
    fit->nearest = fit->lo[0] = fit->lo[1] = 1e30f;
    fit->farthest = fit->hi[0] = fit->hi[1] = -1e30f;
    fit->points = 0;
}

static void fit_extent(struct ShadowFit *fit, const float *centre, const float *half)
{
    const float *m = fit->view, *r = fit->r, *u = fit->u;
    float depth = m[2] * centre[0] + m[5] * centre[1] + m[8] * centre[2] + m[11], x = dot3(centre, r), y = dot3(centre, u);
    float dd = fabsf(m[2]) * half[0] + fabsf(m[5]) * half[1] + fabsf(m[8]) * half[2];
    float dx = fabsf(r[0]) * half[0] + fabsf(r[1]) * half[1] + fabsf(r[2]) * half[2];
    float dy = fabsf(u[0]) * half[0] + fabsf(u[1]) * half[1] + fabsf(u[2]) * half[2];

    fit->nearest = depth - dd < fit->nearest ? depth - dd : fit->nearest;
    fit->farthest = depth + dd > fit->farthest ? depth + dd : fit->farthest;
    fit->lo[0] = x - dx < fit->lo[0] ? x - dx : fit->lo[0];
    fit->hi[0] = x + dx > fit->hi[0] ? x + dx : fit->hi[0];
    fit->lo[1] = y - dy < fit->lo[1] ? y - dy : fit->lo[1];
    fit->hi[1] = y + dy > fit->hi[1] ? y + dy : fit->hi[1];
    fit->points++;
}

static void fit_point(struct ShadowFit *fit, const float *p)
{
    static const float none[3] = {0.0f, 0.0f, 0.0f};

    fit_extent(fit, p, none);
}

static void fit_segment(struct ShadowFit *fit, const float *a, const float *b, const float (*plane)[4])
{
    float t0 = 0.0f, t1 = 1.0f, p[3];
    int i, k;

    for (k = 0; k < 6; k++) {
        float fa = dot3(plane[k], a) + plane[k][3], fb = dot3(plane[k], b) + plane[k][3];

        if (fa < 0.0f && fb < 0.0f)
            return;
        if (fa < 0.0f)
            t0 = fa / (fa - fb) > t0 ? fa / (fa - fb) : t0;
        else if (fb < 0.0f)
            t1 = fa / (fa - fb) < t1 ? fa / (fa - fb) : t1;
    }
    if (t0 > t1)
        return;
    for (k = 0; k < 2; k++) {
        for (i = 0; i < 3; i++)
            p[i] = a[i] + (b[i] - a[i]) * (k ? t1 : t0);
        fit_point(fit, p);
    }
}

static int box_extent(const struct ShadowFitBox *box, float *centre, float *half)
{
    int i;

    for (i = 0; i < 3; i++) {
        if (!(box->min[i] > -1e30f && box->max[i] < 1e30f && box->min[i] <= box->max[i]))
            return 0;
        centre[i] = (box->min[i] + box->max[i]) * 0.5f;
        half[i] = (box->max[i] - box->min[i]) * 0.5f;
    }
    return 1;
}

static void fit_box(struct ShadowFit *fit, const struct ViewSlice *s, const struct ShadowFitBox *box)
{
    float plane[6][4] = {{0}}, corner[8][3], centre[3], half[3];
    int i, k, inside = 1, within = 1;

    if (!box_extent(box, centre, half))
        return;
    for (k = 0; k < 6; k++) {
        const float *n = s->plane[k];
        float mid = dot3(n, centre) + n[3], reach = fabsf(n[0]) * half[0] + fabsf(n[1]) * half[1] + fabsf(n[2]) * half[2];

        if (mid + reach < 0.0f)
            return;
        if (mid - reach < 0.0f) {
            inside = 0;
            within = within && k < 2;
        }
    }
    if (inside) {
        fit_extent(fit, centre, half);
        return;
    }
    for (k = 0; k < 8; k++)
        for (i = 0; i < 3; i++)
            corner[k][i] = k & (1 << i) ? box->max[i] : box->min[i];
    for (i = 0; i < 3; i++) {
        plane[2 * i][i] = 1.0f;
        plane[2 * i][3] = -box->min[i];
        plane[2 * i + 1][i] = -1.0f;
        plane[2 * i + 1][3] = box->max[i];
    }
    for (k = 0; k < 8; k++)
        for (i = 1; i < 8; i <<= 1)
            if (!(k & i)) {
                fit_segment(fit, corner[k], corner[k | i], s->plane);
                if (!within)
                    fit_segment(fit, s->corner[k], s->corner[k | i], plane);
            }
}

int box_fit(struct BoxFit *out, const struct ShadowFitCamera *camera, int width, int height, const float *light,
            const struct ShadowFitBox *boxes, size_t count)
{
    const float *view = camera->view;
    struct ShadowFit fit;
    struct ViewSlice slice;
    float inv[12], f[3], up[3] = {0, 0, 0}, r[3], u[3], eye[3], ray[4][3], centre[3], half[3];
    float len, nearest, farthest, maxDepth, lightNear = 1e30f, lightFar = -1e30f, range;
    float split[SHADOW_FIT_CASCADES + 1];
    size_t b;
    int i, j, k, c;

    memset(out, 0, sizeof *out);
    if (!invert_4x3(view, inv))
        return 0;
    len = sqrtf(dot3(light, light));
    if (!(len > 0.0f))
        return 0;
    for (i = 0; i < 3; i++)
        f[i] = light[i] / len;
    k = fabsf(f[0]) <= fabsf(f[1]) ? (fabsf(f[0]) <= fabsf(f[2]) ? 0 : 2) : (fabsf(f[1]) <= fabsf(f[2]) ? 1 : 2);
    up[k] = 1.0f;
    r[0] = up[1] * f[2] - up[2] * f[1];
    r[1] = up[2] * f[0] - up[0] * f[2];
    r[2] = up[0] * f[1] - up[1] * f[0];
    len = sqrtf(dot3(r, r));
    for (i = 0; i < 3; i++)
        r[i] /= len;
    u[0] = f[1] * r[2] - f[2] * r[1];
    u[1] = f[2] * r[0] - f[0] * r[2];
    u[2] = f[0] * r[1] - f[1] * r[0];

    for (b = 0; b < count; b++) {
        float mid, reach;

        if (!box_extent(&boxes[b], centre, half))
            continue;
        mid = dot3(centre, f);
        reach = fabsf(f[0]) * half[0] + fabsf(f[1]) * half[1] + fabsf(f[2]) * half[2];
        lightNear = mid - reach < lightNear ? mid - reach : lightNear;
        lightFar = mid + reach > lightFar ? mid + reach : lightFar;
    }
    if (lightNear > lightFar)
        return 0;
    lightNear -= SHADOW_MARGIN;
    range = lightFar + SHADOW_MARGIN - lightNear;

    maxDepth = camera->distance * 4.0f + 400.0f;
    maxDepth = maxDepth < camera->farZ ? maxDepth : camera->farZ;
    for (i = 0; i < 3; i++)
        eye[i] = inv[9 + i];
    for (k = 0; k < 4; k++) {
        float dx = ((k & 1 ? (float)width : 0.0f) - camera->centre[0]) / camera->proj[0];
        float dy = ((k & 2 ? (float)height : 0.0f) - camera->centre[1]) / camera->proj[1];

        for (i = 0; i < 3; i++)
            ray[k][i] = dx * inv[i] + dy * inv[3 + i] + inv[6 + i];
    }
    fit.view = view;
    fit.r = r;
    fit.u = u;
    fit_reset(&fit);
    view_slice(&slice, view, eye, ray, camera->nearZ, maxDepth);
    for (b = 0; b < count; b++)
        fit_box(&fit, &slice, &boxes[b]);
    if (!fit.points)
        return 0;
    nearest = fit.nearest > camera->nearZ ? fit.nearest : camera->nearZ;
    farthest = fit.farthest < maxDepth ? fit.farthest : maxDepth;
    farthest = (farthest > nearest ? farthest : nearest) + 1.0f;
    for (c = 0; c <= SHADOW_FIT_CASCADES; c++) {
        float t = (float)c / SHADOW_FIT_CASCADES;
        split[c] = SHADOW_SPLIT * nearest * powf(farthest / nearest, t) + (1.0f - SHADOW_SPLIT) * (nearest + (farthest - nearest) * t);
    }
    split[0] = nearest;
    split[SHADOW_FIT_CASCADES] = farthest;

    out->cascades = SHADOW_FIT_CASCADES;
    for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
        float square[2] = {0.0f, 0.0f}, radius = 8.0f, texel, w[4][4];

        fit_reset(&fit);
        view_slice(&slice, view, eye, ray, split[c], split[c + 1]);
        for (b = 0; b < count; b++)
            fit_box(&fit, &slice, &boxes[b]);
        if (fit.points) {
            square[0] = (fit.lo[0] + fit.hi[0]) * 0.5f;
            square[1] = (fit.lo[1] + fit.hi[1]) * 0.5f;
            radius = (fit.hi[0] - fit.lo[0] > fit.hi[1] - fit.lo[1] ? fit.hi[0] - fit.lo[0] : fit.hi[1] - fit.lo[1]) * 0.5f;
            radius = ceilf((radius * 1.05f + 4.0f + SHADOW_SOFTNESS) / 8.0f) * 8.0f;
        }
        texel = 2.0f * radius / SHADOW_FIT_SIZE;
        square[0] = floorf(square[0] / texel + 0.5f) * texel;
        square[1] = floorf(square[1] / texel + 0.5f) * texel;
        for (i = 0; i < 2; i++) {
            out->lo[c][i] = fit.points ? square[i] - radius : 1e30f;
            out->hi[c][i] = fit.points ? square[i] + radius : -1e30f;
        }
        for (i = 0; i < 3; i++) {
            w[i][0] = r[i] / radius;
            w[i][1] = u[i] / radius;
            w[i][2] = f[i] / range;
            w[i][3] = 0.0f;
        }
        w[3][0] = -square[0] / radius;
        w[3][1] = -square[1] / radius;
        w[3][2] = -lightNear / range;
        w[3][3] = 1.0f;
        for (i = 0; i < 4; i++)
            for (j = 0; j < 4; j++) {
                float sum = i == 3 ? w[3][j] : 0.0f;
                for (k = 0; k < 3; k++)
                    sum += inv[i * 3 + k] * w[k][j];
                out->fromView[c][i * 4 + j] = sum;
            }
        out->farDepth[c] = split[c + 1];
        out->offset[c] = SHADOW_OFFSET * texel;
        out->slant[c] = (SHADOW_SOFTNESS + texel) / range;
        out->blur[c] = SHADOW_SOFTNESS / (2.0f * radius);
    }
    out->depth[0] = lightNear;
    out->depth[1] = lightNear + range;
    for (i = 0; i < 3; i++) {
        out->axis[0][i] = r[i];
        out->axis[1][i] = u[i];
        out->axis[2][i] = f[i];
    }
    for (i = 0; i < 3; i++)
        out->lightView[i] = -(f[0] * view[i] + f[1] * view[3 + i] + f[2] * view[6 + i]);
    len = sqrtf(dot3(out->lightView, out->lightView));
    if (len > 0.0f)
        for (i = 0; i < 3; i++)
            out->lightView[i] /= len;
    out->bias = SHADOW_BIAS / range;
    return 1;
}
