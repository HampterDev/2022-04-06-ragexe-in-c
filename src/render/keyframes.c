#include "render/keyframes.h"
#include "fileio/file_api.h"
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define PI_F 3.141592f

static void key_quat_slerp_spin(struct KeyQuat *out, float t,
    const struct KeyQuat *a, const struct KeyQuat *b, int spin);

void key_pos_matrix(const struct KeyPos *keys, size_t count, int frame,
                                  float *out)
{
    unsigned i;
    float s, w;

    if (count == 0) {
        *(struct Mat34 *)out = g_identityMatrix;
        return;
    }
    if (count == 1) {
        mat_set_translation(out, keys[0].x, keys[0].y, keys[0].z);
        return;
    }
    for (i = 1; keys[i].frame <= frame; i++)
        if (i + 1 >= (unsigned)count)
            return;
    s = (float)(frame - keys[i - 1].frame) / (float)(keys[i].frame - keys[i - 1].frame);
    w = (float)(1.0 - (double)s);
    mat_set_translation(out, keys[i - 1].x * w + keys[i].x * s, keys[i - 1].y * w + keys[i].y * s,
               keys[i - 1].z * w + keys[i].z * s);
}

void key_quat_matrix(const struct KeyQuat *keys, size_t count, int frame,
                                   float *out)
{
    struct KeyQuat q;
    unsigned i;

    if (count == 0) {
        *(struct Mat34 *)out = g_identityMatrix;
        return;
    }
    if (count == 1) {
        quat_to_matrix_b(out, &keys[0]);
        return;
    }
    for (i = 1; keys[i].frame <= frame; i++)
        if (i + 1 >= (unsigned)count)
            return;
    if (keys[i - 1].w == 1.0f && keys[i].w == 1.0f) {
        *(struct Mat34 *)out = g_identityMatrix;
        return;
    }
    {
        float s = (float)(frame - keys[i - 1].frame) / (float)(keys[i].frame - keys[i - 1].frame);
        key_quat_slerp_spin(&q, s, &keys[i - 1], &keys[i], 0);
    }
    quat_to_matrix_b(out, &q);
}

void key_scale_matrix(const struct KeyScaleAxis *keys, size_t count, float *out)
{
    if (count == 0) {
        *(struct Mat34 *)out = g_identityMatrix;
        return;
    }
    if (count == 1)
        mat_set_scale(out, keys[0].x, keys[0].y, keys[0].z);
}

void key_vec_translation(const struct KeyVec *keys, size_t count, float t,
                                           const float *base, float *m)
{
    struct KeyPos before, after;
    unsigned i;
    float s, w;

    if (count == 0 || isnan(t)) {
        m[9] = base[9];
        m[10] = base[10];
        m[11] = base[11];
        return;
    }
    if (count == 1) {
        m[9] = keys[0].x;
        m[10] = keys[0].y;
        m[11] = keys[0].z;
        return;
    }
    if (t >= (float)keys[count - 1].frame) {
        m[9] = keys[count - 1].x;
        m[10] = keys[count - 1].y;
        m[11] = keys[count - 1].z;
        return;
    }
    for (i = 0; i < (unsigned)count; i++) {
        if ((float)keys[i].frame > t) {
            if (i == 0) {
                before.frame = 0;
                before.x = base[9];
                before.y = base[10];
                before.z = base[11];
            } else {
                memcpy(&before, &keys[i - 1], sizeof before);
            }
            memcpy(&after, &keys[i], sizeof after);
            break;
        }
    }
    s = (t - (float)before.frame) / (float)(after.frame - before.frame);
    w = 1.0f - s;
    m[9] = before.x * w + after.x * s;
    m[10] = before.y * w + after.y * s;
    m[11] = before.z * w + after.z * s;
}

static void quat_blend_shortest(struct KeyQuat *out, const struct KeyQuat *a, const struct KeyQuat *b,
                                float s)
{
    float ax = a->x, ay = a->y, az = a->z, aw = a->w;
    float bx = b->x, by = b->y, bz = b->z, bw = b->w;
    float dot = ((bx * ax + by * ay) + bz * az) + bw * aw;
    float wa, wb;

    if (dot < 0.0f) {
        ax = -ax;
        ay = -ay;
        az = -az;
        aw = -aw;
        dot = -dot;
    }
    if (dot > -0.95f) {
        wa = 1.0f - s;
        wb = s;
        if (!(dot > 0.95f)) {
            float theta = (float)acos((double)dot);
            float sin_theta = float_sin(theta);

            wa = float_sin(wa * theta) / sin_theta;
            wb = float_sin(theta * s) / sin_theta;
        }
    } else {
        bx = -ay;
        by = ax;
        bz = -aw;
        bw = az;
        wa = float_sin((0.5f - s) * PI_F);
        wb = float_sin(s * PI_F);
    }
    out->frame = 0;
    out->x = bx * wb + wa * ax;
    out->y = by * wb + wa * ay;
    out->z = bz * wb + az * wa;
    out->w = bw * wb + aw * wa;
}

void key_quat_apply(const struct KeyQuat *keys, size_t count, float t,
                                    const float *base, float *m)
{
    struct Mat34 rot = g_identityMatrix, prior;
    struct KeyQuat a, b, q, from_base;
    int frame_a, frame_b;
    float s;
    unsigned i;

    if (count == 0) {
        rot = *(const struct Mat34 *)base;
    } else if (count == 1) {
        quat_to_matrix(rot.m, &keys[0]);
    } else if (t >= (float)keys[count - 1].frame) {
        quat_to_matrix(rot.m, &keys[count - 1]);
    } else {
        a.x = a.y = a.z = 0.0f;
        a.w = 1.0f;
        b.x = b.y = b.z = 0.0f;
        b.w = 1.0f;
        frame_a = frame_b = 0;
        for (i = 0; i < (unsigned)count; i++) {
            if ((float)keys[i].frame > t) {
                if (i == 0) {
                    a = *matrix_to_quat(base, &from_base);
                    frame_a = 0;
                } else {
                    a = keys[i - 1];
                    frame_a = a.frame;
                }
                b = keys[i];
                frame_b = b.frame;
                break;
            }
        }
        s = 0.0f;
        if (frame_b != frame_a)
            s = (t - (float)frame_a) / (float)(frame_b - frame_a);
        quat_blend_shortest(&q, &a, &b, s);
        quat_to_matrix(rot.m, &q);
    }
    prior = *(struct Mat34 *)m;
    mat_multiply(m, prior.m, rot.m);
}

void key_vec_scale(const struct KeyVec *keys, size_t count, float t,
                                        float *out)
{
    struct Mat34 scale;
    float ax = 1.0f, ay = 1.0f, az = 1.0f, bx = 1.0f, by = 1.0f, bz = 1.0f, s, w;
    int frame_a = 0, frame_b = 0;
    unsigned i;

    if (count == 0) {
        *(struct Mat34 *)out = g_identityMatrix;
        return;
    }
    if (count == 1) {
        mat_set_scale(out, keys[0].x, keys[0].y, keys[0].z);
        return;
    }
    if (t >= (float)keys[count - 1].frame) {
        mat_set_scale(out, keys[count - 1].x, keys[count - 1].y, keys[count - 1].z);
        return;
    }
    scale = g_identityMatrix;
    for (i = 0; i < (unsigned)count; i++) {
        if ((float)keys[i].frame > t) {
            if (i != 0) {
                frame_a = keys[i - 1].frame;
                ax = keys[i - 1].x;
                ay = keys[i - 1].y;
                az = keys[i - 1].z;
            }
            frame_b = keys[i].frame;
            bx = keys[i].x;
            by = keys[i].y;
            bz = keys[i].z;
            break;
        }
    }
    s = 0.0f;
    if (frame_b != frame_a)
        s = (t - (float)frame_a) / (float)(frame_b - frame_a);
    w = 1.0f - s;
    mat_set_scale(scale.m, bx * s + w * ax, by * s + w * ay, bz * s + az * w);
    *(struct Mat34 *)out = scale;
}

static void key_quat_slerp_spin(struct KeyQuat *out, float t,
                                     const struct KeyQuat *a, const struct KeyQuat *b, int spin)
{
    float dot = ((b->x * a->x + b->y * a->y) + a->z * b->z) + a->w * b->w;
    char negative = 0.0f > dot;
    float cos_theta = negative ? -dot : dot;
    float wa, wb;

    if ((double)1e-6f > 1.0 - (double)cos_theta) {
        wa = (float)(1.0 - (double)t);
        wb = t;
    } else {
        float theta = (float)acos((double)cos_theta);
        float sin_theta = float_sin(theta);
        float angle = ((float)spin * PI_F + theta) * t;

        wa = float_sin(theta - angle) / sin_theta;
        wb = float_sin(angle) / sin_theta;
    }
    if (negative)
        wb = -wb;
    out->x = a->x * wa + b->x * wb;
    out->y = wa * a->y + b->y * wb;
    out->z = a->z * wa + b->z * wb;
    out->w = a->w * wa + b->w * wb;
}

void key_quat_from_axis_angle(struct KeyQuat *key, const float axis[3], float angle)
{
    float half = angle * 0.5f;
    float s = float_sin(half);

    key->frame = 0;
    key->x = s * axis[0];
    key->y = s * axis[1];
    key->z = s * axis[2];
    key->w = float_cos(half);
}

void key_uv_channels_eval(const struct KeyFloatTrack channels[KEYFLOAT_CHANNELS], float t,
                          float out[KEYFLOAT_CHANNELS])
{
    int channel;
    if (isnan(t)) return;
    for (channel = 0; channel < KEYFLOAT_CHANNELS; ++channel) {
        const struct KeyFloat *keys = channels[channel].keys;
        size_t count = channels[channel].count, j;
        float s;
        if (!count) continue;
        if (count == 1 || t >= (float)keys[count - 1].frame) {
            out[channel] = keys[count - 1].value;
            continue;
        }
        j = 1;
        while (!((float)keys[j].frame > t) && j + 1 < count) ++j;
        s = (t - (float)keys[j - 1].frame) / (float)(keys[j].frame - keys[j - 1].frame);
        out[channel] = (1.0f - s) * keys[j - 1].value + keys[j].value * s;
    }
}

static int read_key_count(struct File *file, size_t size, size_t *count)
{
    int value;
    if (!file_read(file, &value, sizeof value) || value < 0 || (size_t)value > INT_MAX / size)
        return 0;
    *count = (size_t)value;
    return 1;
}

static int read_key_array(struct File *file, size_t size, void **keys, size_t *count)
{
    size_t n;
    void *data;
    if (!read_key_count(file, size, &n)) return 0;
    data = n ? malloc(n * size) : NULL;
    if (n && (!data || !file_read(file, data, (int)(n * size)))) {
        free(data);
        return 0;
    }
    *keys = data;
    *count = n;
    return 1;
}

int key_vec_read(struct File *file, struct KeyVec **keys, size_t *count)
{
    void *data;
    size_t n;
    if (!read_key_array(file, sizeof **keys, &data, &n)) return 0;
    free(*keys);
    *keys = data;
    *count = n;
    return 1;
}

int key_quat_read(struct File *file, struct KeyQuat **keys, size_t *count)
{
    void *data;
    size_t n;
    if (!read_key_array(file, sizeof **keys, &data, &n)) return 0;
    free(*keys);
    *keys = data;
    *count = n;
    return 1;
}

static void uv_track_clear(struct UvKeyTrack *track)
{
    size_t i;
    for (i = 0; i < KEYFLOAT_CHANNELS; ++i) {
        free(track->channels[i].keys);
        track->channels[i].keys = NULL;
        track->channels[i].count = 0;
    }
}

void key_uv_clear(struct UvAnimation *animation)
{
    size_t i;
    for (i = 0; i < animation->count; ++i) uv_track_clear(&animation->tracks[i]);
    free(animation->tracks);
    animation->tracks = NULL;
    animation->count = 0;
}

static int read_uv_channels(struct File *file, struct UvKeyTrack *track)
{
    size_t count, i;
    if (!read_key_count(file, 1, &count)) return 0;
    for (i = 0; i < count; ++i) {
        int channel;
        size_t n;
        if (!file_read(file, &channel, sizeof channel) || !read_key_count(file, sizeof(struct KeyFloat), &n))
            return 0;
        if (channel >= 0 && channel < KEYFLOAT_CHANNELS) {
            struct KeyFloatTrack *out = &track->channels[channel];
            struct KeyFloat *keys;
            if (!n) continue;
            if (out->count > INT_MAX / sizeof *keys - n) return 0;
            keys = realloc(out->keys, (out->count + n) * sizeof *keys);
            if (!keys) return 0;
            out->keys = keys;
            if (!file_read(file, keys + out->count, (int)(n * sizeof *keys))) return 0;
            out->count += n;
        } else {
            struct KeyFloat skipped[32];
            while (n) {
                size_t batch = n < 32 ? n : 32;
                if (!file_read(file, skipped, (int)(batch * sizeof *skipped))) return 0;
                n -= batch;
            }
        }
    }
    return 1;
}

static int compare_uv_texture(const void *left, const void *right)
{
    int a = ((const struct UvKeyTrack *)left)->texture;
    int b = ((const struct UvKeyTrack *)right)->texture;
    return (a > b) - (a < b);
}

int key_uv_read(struct File *file, struct UvAnimation *animation)
{
    struct UvAnimation result = {0};
    struct UvKeyTrack track = {0};
    size_t count, i, found;
    if (!read_key_count(file, sizeof track, &count)) return 0;
    for (i = 0; i < count; ++i) {
        if (!file_read(file, &track.texture, sizeof track.texture) || !read_uv_channels(file, &track)) goto fail;
        for (found = 0; found < result.count; ++found)
            if (result.tracks[found].texture == track.texture) break;
        if (found == result.count) {
            struct UvKeyTrack *tracks = realloc(result.tracks, (result.count + 1) * sizeof *tracks);
            if (!tracks) goto fail;
            result.tracks = tracks;
            ++result.count;
        } else uv_track_clear(&result.tracks[found]);
        result.tracks[found] = track;
        memset(&track, 0, sizeof track);
    }
    if (result.count > 1) qsort(result.tracks, result.count, sizeof *result.tracks, compare_uv_texture);
    key_uv_clear(animation);
    *animation = result;
    return 1;
fail:
    uv_track_clear(&track);
    key_uv_clear(&result);
    return 0;
}
