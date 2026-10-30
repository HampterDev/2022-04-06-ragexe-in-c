#include "world/scene_graph.h"
#include <math.h>

#define OBB_EPS ((double)1e-6f)

static float obb_abs(float x)
{
    return (float)fabs((double)x);
}

BYTE obb_overlaps_box(const void *obb, const float *box)
{
    const float *e = (const float *)obb, *c = e + 3;
    const float *axisX = e + 6, *axisY = e + 9, *axisZ = e + 12;
    float h0, h1, h2, d0, d1, d2, t0, t1, t2;
    float r00, r01, r02, r10, r11, r12, r20, r21, r22;

    h0 = (box[3] - box[0]) * 0.5f;
    h1 = (box[4] - box[1]) * 0.5f;
    h2 = (box[5] - box[2]) * 0.5f;
    d0 = c[0] - (box[0] + box[3]) * 0.5f;
    d1 = c[1] - (box[1] + box[4]) * 0.5f;
    d2 = c[2] - (box[2] + box[5]) * 0.5f;

    t0 = (axisX[1] * d1 + axisX[0] * d0) + axisX[2] * d2;
    r00 = obb_abs(axisX[0]);
    r01 = obb_abs(axisX[1]);
    r02 = obb_abs(axisX[2]);
    if ((double)obb_abs(t0) - ((double)((r00 * h0 + r01 * h1) + r02 * h2) + (double)e[0]) > OBB_EPS)
        return 0;
    t1 = (axisY[1] * d1 + axisY[0] * d0) + axisY[2] * d2;
    r10 = obb_abs(axisY[0]);
    r11 = obb_abs(axisY[1]);
    r12 = obb_abs(axisY[2]);
    if ((double)obb_abs(t1) - ((double)((r11 * h1 + r10 * h0) + r12 * h2) + (double)e[1]) > OBB_EPS)
        return 0;
    t2 = (axisZ[1] * d1 + axisZ[0] * d0) + axisZ[2] * d2;
    r20 = obb_abs(axisZ[0]);
    r21 = obb_abs(axisZ[1]);
    r22 = obb_abs(axisZ[2]);
    if ((double)obb_abs(t2) - ((double)((r20 * h0 + r21 * h1) + r22 * h2) + (double)e[2]) > OBB_EPS)
        return 0;

    if ((double)obb_abs(d0) - ((double)((e[1] * r10 + e[0] * r00) + r20 * e[2]) + (double)h0) > OBB_EPS)
        return 0;
    if ((double)obb_abs(d1) - ((double)((e[1] * r11 + e[0] * r01) + e[2] * r21) + (double)h1) > OBB_EPS)
        return 0;
    if ((double)obb_abs(d2) - ((double)((e[1] * r12 + e[0] * r02) + e[2] * r22) + (double)h2) > OBB_EPS)
        return 0;

    if ((double)obb_abs(axisY[0] * t2 - axisZ[0] * t1)
        - ((double)(r01 * h2 + r02 * h1) + (double)(r20 * e[1] + e[2] * r10)) > OBB_EPS)
        return 0;
    if ((double)obb_abs(axisY[1] * t2 - axisZ[1] * t1)
        - ((double)(r02 * h0 + r00 * h2) + (double)(e[1] * r21 + e[2] * r11)) > OBB_EPS)
        return 0;
    if ((double)obb_abs(axisY[2] * t2 - axisZ[2] * t1)
        - ((double)(r00 * h1 + r01 * h0) + (double)(e[1] * r22 + e[2] * r12)) > OBB_EPS)
        return 0;
    if ((double)obb_abs(axisZ[0] * t0 - axisX[0] * t2)
        - ((double)(r11 * h2 + r12 * h1) + (double)(r20 * e[0] + e[2] * r00)) > OBB_EPS)
        return 0;
    if ((double)obb_abs(axisZ[1] * t0 - axisX[1] * t2)
        - ((double)(e[0] * r21 + e[2] * r01) + (double)(r12 * h0 + r10 * h2)) > OBB_EPS)
        return 0;
    if ((double)obb_abs(axisZ[2] * t0 - axisX[2] * t2)
        - ((double)(e[0] * r22 + r02 * e[2]) + (double)(r11 * h0 + r10 * h1)) > OBB_EPS)
        return 0;
    if ((double)obb_abs(axisX[0] * t1 - axisY[0] * t0)
        - ((double)(r22 * h1 + r21 * h2) + (double)(e[0] * r10 + e[1] * r00)) > OBB_EPS)
        return 0;
    if ((double)obb_abs(axisX[1] * t1 - axisY[1] * t0)
        - ((double)(r01 * e[1] + e[0] * r11) + (double)(r20 * h2 + r22 * h0)) > OBB_EPS)
        return 0;
    if ((double)obb_abs(axisX[2] * t1 - axisY[2] * t0)
        - ((double)(r21 * h0 + r20 * h1) + (double)(r02 * e[1] + e[0] * r12)) > OBB_EPS)
        return 0;
    return 1;
}
