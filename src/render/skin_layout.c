#include "render/skin_layout.h"

#include <string.h>

static uint8_t bounded_bone(int bone, int bones)
{
    return (uint8_t)(bone >= 0 && bone < bones ? bone : 0);
}

void skin_pack_vertex(const struct SkinSource *in, int rigidBone, int bones, struct SkinVertex *out)
{
    int k;

    memset(out, 0, sizeof *out);
    memcpy(out->position, in->position, sizeof out->position);
    memcpy(out->normal, in->normal, sizeof out->normal);
    memcpy(out->uv, in->uv, sizeof out->uv);
    if (bones > SKIN_BONES_MAX)
        bones = SKIN_BONES_MAX;
    if (rigidBone >= 0) {
        out->bones[0] = bounded_bone(rigidBone, bones);
        out->weights[0] = 1.0f;
        out->flags = SKIN_RIGID;
        return;
    }
    for (k = 0; k < 4; k++) {
        out->bones[k] = bounded_bone(in->bones[k], bones);
        out->weights[k] = in->weights[k];
    }
}

int skin_pack_indices(const uint16_t *in, uint32_t count, uint32_t vertices, uint32_t base, uint32_t *out)
{
    uint32_t i;

    if (vertices && base > 0xffffffffu - (vertices - 1))
        return 0;
    for (i = 0; i < count; i++) {
        if (in[i] >= vertices)
            return 0;
        out[i] = base + in[i];
    }
    return 1;
}

void skin_pose_kind(struct SkinPose *pose, int kind, int intensity, int red, int green, int blue)
{
    if (kind == SKIN_ACTOR_LIT) {
        pose->scale = (float)intensity;
        pose->offset = (float)(0xff - intensity);
        pose->add = ((uint32_t)(red + 0x38) & 0xff) << 16 | ((uint32_t)(green + 0x38) & 0xff) << 8
                    | ((uint32_t)(blue + 0x38) & 0xff);
        pose->rigidStill = 0;
        return;
    }
    pose->scale = 100.0f;
    pose->offset = 155.0f;
    pose->add = 0;
    pose->rigidStill = kind == SKIN_ACTOR_GUILD_FLAG;
}

size_t skin_pose_size(uint32_t boneCount)
{
    if (boneCount > SKIN_BONES_MAX)
        return 0;
    return SKIN_POSE_HEADER + 3 * (size_t)(boneCount ? boneCount : 1);
}

static float float_bits(uint32_t u)
{
    float f;

    memcpy(&f, &u, sizeof f);
    return f;
}

static void column(float *row, const float *m, int stride, int k)
{
    row[0] = m[k];
    row[1] = m[stride + k];
    row[2] = m[2 * stride + k];
    row[3] = m[3 * stride + k];
}

size_t skin_pose_rows(const struct SkinPose *pose, float (*rows)[4])
{
    size_t n = skin_pose_size(pose->boneCount);
    uint32_t b, bones = pose->boneCount ? pose->boneCount : 1;
    uint32_t offset;
    int k;

    if (!n)
        return 0;
    memcpy(&offset, &pose->offset, sizeof offset);
    rows[0][0] = float_bits(bones);
    rows[0][1] = float_bits((pose->rigidStill ? 1u : 0u) | (uint32_t)pose->alpha << 8);
    rows[0][2] = float_bits(pose->add);
    rows[0][3] = float_bits(offset);
    for (k = 0; k < 3; k++)
        column(rows[1 + k], pose->view, 3, k);
    rows[4][0] = pose->light[0];
    rows[4][1] = pose->light[1];
    rows[4][2] = pose->light[2];
    rows[4][3] = pose->scale;
    rows[5][0] = pose->proj[0];
    rows[5][1] = pose->proj[1];
    rows[5][2] = pose->centre[0];
    rows[5][3] = pose->centre[1];
    rows[6][0] = pose->nearZ;
    rows[6][1] = pose->depthScale;
    rows[6][2] = rows[6][3] = 0.0f;
    for (b = 0; b < bones; b++)
        for (k = 0; k < 3; k++) {
            if (pose->boneCount)
                column(rows[SKIN_POSE_HEADER + b * 3 + k], pose->bones + b * 16, 4, k);
            else
                memset(rows[SKIN_POSE_HEADER + b * 3 + k], 0, sizeof rows[0]);
        }
    return n;
}

void skin_vertex_position(const struct SkinVertex *v, const float *bones, uint32_t boneCount, float out[3])
{
    const float *p = v->position;
    int k;

    if (v->flags & SKIN_RIGID) {
        memcpy(out, p, 3 * sizeof *out);
        return;
    }
    out[0] = out[1] = out[2] = 0.0f;
    if (!boneCount)
        return;
    for (k = 0; k < 4; k++) {
        const float *m;
        float w = v->weights[k];

        if (w == 0.0f)
            continue;
        m = bones + (v->bones[k] < boneCount ? v->bones[k] : boneCount - 1) * 16;
        out[0] += w * (p[0] * m[0] + p[1] * m[4] + p[2] * m[8] + m[12]);
        out[1] += w * (p[0] * m[1] + p[1] * m[5] + p[2] * m[9] + m[13]);
        out[2] += w * (p[0] * m[2] + p[1] * m[6] + p[2] * m[10] + m[14]);
    }
}

int skin_batch_join(const struct SkinBatch *batch, uint32_t first, uint32_t count, int pose)
{
    if (batch->instances == 1 && pose == batch->pose && first == batch->first + batch->count
        && batch->count <= 0xffffffffu - count)
        return SKIN_JOIN_EXTEND;
    if (first == batch->first && count == batch->count)
        return SKIN_JOIN_INSTANCE;
    return SKIN_JOIN_NEW;
}
