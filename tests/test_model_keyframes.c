#include "render/keyframes.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CAPTURE_KEYFRAME_REFERENCE
unsigned char file_read(struct File *file, void *out, int size)
{ (void)file; (void)out; (void)size; abort(); }
#endif

#ifdef CAPTURE_KEYFRAME_REFERENCE
static struct TreeNode channel_head;
static struct { struct TreeNode links; int channel; struct Vec track; } channel_nodes[5];
static unsigned channel_mask;
struct TreeNode *std_tree_int_find(const struct Tree *tree, int key)
{
    assert(tree->head == &channel_head && key >= 0 && key < 5);
    return channel_mask & (1u << key) ? &channel_nodes[key].links : tree->head;
}
struct TreeNode *std_tree_int_find_or_insert(struct Tree *tree, int key, size_t size, size_t maximum)
{
    struct TreeNode *node = std_tree_int_find(tree, key);
    (void)maximum; assert(size == sizeof channel_nodes[0] && node != tree->head);
    return node;
}
static struct Vec view(void *items, size_t count, size_t size)
{
    struct Vec result = {items, (char *)items + count * size, (char *)items + count * size};
    return result;
}
#endif

static FILE *reference;
static unsigned samples;

static void sample(const float *value, size_t count)
{
#ifdef CAPTURE_KEYFRAME_REFERENCE
    assert(fwrite(value, sizeof *value, count, reference) == count);
#else
    float expected[12];
    size_t i;
    assert(count <= 12 && fread(expected, sizeof *expected, count, reference) == count);
    for (i = 0; i < count; ++i)
        assert((isnan(value[i]) && isnan(expected[i])) || !memcmp(&value[i], &expected[i], sizeof(float)));
#endif
    ++samples;
}

int main(int argc, char **argv)
{
    const int counts[] = {0, 1, 2, 5, 4};
    const float times[] = {-5, 0, 1, 5, 10, 19, 30, 55};
    unsigned trial, ti, operation, k;
    assert(argc == 2);
#ifdef CAPTURE_KEYFRAME_REFERENCE
    reference = fopen(argv[1], "wb");
#else
    reference = fopen(argv[1], "rb");
#endif
    assert(reference);
    for (trial = 0; trial < 65; ++trial) {
        struct KeyPos pos[5];
        struct KeyVec vec[5];
        struct KeyQuat quat[5];
        struct KeyScaleAxis scale[5];
        struct KeyFloat channels[5][3];
        float base[12] = {1,0,0, 0,0,1, 0,-1,0, 8,-4,2};
        size_t count = (size_t)counts[trial % 5];
#ifdef CAPTURE_KEYFRAME_REFERENCE
        struct Vec tracks[4];
        struct Tree uv = {&channel_head, 0};
        channel_mask = trial % 32;
#else
        struct KeyFloatTrack uv[5] = {0};
#endif
        for (k = 0; k < 5; ++k) {
            int frame = (int)k * 10 + (trial % 3 == 0 ? 5 : 0);
            float x = (float)((int)trial - 20 + (int)k) * 0.125f;
            float angle = (float)(trial + k) * 0.0625f;
            unsigned j;
            if (trial % 5 == 4 && k == 2) frame = 10;
            pos[k].frame = vec[k].frame = quat[k].frame = scale[k].frame = frame;
            pos[k].x = vec[k].x = scale[k].x = x;
            pos[k].y = vec[k].y = scale[k].y = x * 2;
            pos[k].z = vec[k].z = scale[k].z = 1 - x;
            vec[k].data = 0x12345678;
            quat[k].x = 0; quat[k].y = float_sin(angle); quat[k].z = 0; quat[k].w = float_cos(angle);
            scale[k].qx = scale[k].qy = scale[k].qz = 0; scale[k].qw = 1;
            for (j = 0; j < 3; ++j) {
                channels[k][j].frame = (int)j * 10;
                channels[k][j].value = x + (float)j * 0.25f;
            }
#ifdef CAPTURE_KEYFRAME_REFERENCE
            channel_nodes[k].channel = (int)k;
            channel_nodes[k].track = view(channels[k], 3, sizeof channels[k][0]);
#else
            if ((trial % 32) & (1u << k)) { uv[k].keys = channels[k]; uv[k].count = 3; }
#endif
        }
#ifdef CAPTURE_KEYFRAME_REFERENCE
        tracks[0] = view(pos, count, sizeof pos[0]);
        tracks[1] = view(quat, count, sizeof quat[0]);
        tracks[2] = view(scale, count, sizeof scale[0]);
        tracks[3] = view(vec, count, sizeof vec[0]);
#endif
        for (ti = 0; ti < sizeof times / sizeof times[0]; ++ti) {
            for (operation = 0; operation < 7; ++operation) {
                float out[12];
                for (k = 0; k < 12; ++k) out[k] = (float)k * 0.125f;
#ifdef CAPTURE_KEYFRAME_REFERENCE
                switch (operation) {
                case 0: V1PosKeys_Matrix(&tracks[0], (int)times[ti], out); break;
                case 1: V1RotKeys_Matrix(&tracks[1], (int)times[ti], out); break;
                case 2: V1ScaleKeys_Matrix(&tracks[2], (int)times[ti], out); break;
                case 3: V2PosKeys_Apply(&tracks[3], times[ti], base, out); break;
                case 4: V2RotKeys_Apply(&tracks[1], times[ti], base, out); break;
                case 5: V2ScaleKeys_Apply(&tracks[3], times[ti], out); break;
                default: UvKeys_Eval(&uv, times[ti], out); break;
                }
#else
                switch (operation) {
                case 0: key_pos_matrix(pos, count, (int)times[ti], out); break;
                case 1: key_quat_matrix(quat, count, (int)times[ti], out); break;
                case 2: key_scale_matrix(scale, count, out); break;
                case 3: key_vec_translation(vec, count, times[ti], base, out); break;
                case 4: key_quat_apply(quat, count, times[ti], base, out); break;
                case 5: key_vec_scale(vec, count, times[ti], out); break;
                default: key_uv_channels_eval(uv, times[ti], out); break;
                }
#endif
                sample(out, operation == 6 ? 5 : 12);
            }
        }
    }
#ifndef CAPTURE_KEYFRAME_REFERENCE
    assert(fgetc(reference) == EOF);
    {
        struct KeyFloat key = {10, 7};
        struct KeyFloatTrack channels[5] = {{0}};
        struct KeyVec position = {0,1,2,3,0};
        const float base[12] = {1,0,0, 0,1,0, 0,0,1, 8,-4,2};
        float uv[5] = {0,0,1,1,0}, matrix[12];
        channels[1].keys = &key; channels[1].count = 1;
        key_uv_channels_eval(channels, -5, uv);
        assert(uv[0] == 0 && uv[1] == 7 && uv[2] == 1 && uv[3] == 1 && uv[4] == 0);
        key.value = 9;
        key_uv_channels_eval(channels, NAN, uv);
        assert(uv[1] == 7);
        memcpy(matrix, base, sizeof matrix);
        key_vec_translation(&position, 1, NAN, base, matrix);
        assert(!memcmp(matrix, base, sizeof matrix));
    }
#endif
    assert(!fclose(reference) && samples == 3640);
    printf("model keyframes: %u archived interpolation samples match\n", samples);
    return 0;
}
