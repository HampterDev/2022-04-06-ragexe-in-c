#include "effects/effect_math.h"
#include "game/context.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static int camera_present, projections, camera_reads;
static float camera[12];
struct Renderer *g_renderer;

int game_copy_view_matrix(float matrix[12])
{
    ++camera_reads;
    if (!camera_present)
        return 0;
    memcpy(matrix, camera, sizeof camera);
    return 1;
}

void renderer_project_point(struct Renderer *renderer, const Vec3 *point, const float *view,
                            float *x, float *y, float *rhw)
{
    assert(renderer == g_renderer && memcmp(view, camera, sizeof camera) == 0);
    ++projections;
    *x = point->x;
    *y = point->y;
    *rhw = 1.0f;
}

static void check_heading(Vec3 from, Vec3 to, double expected)
{
    int old_reads = camera_reads, old_projections = projections;
    assert(fabs(effect_screen_heading(from, to) - expected) < 0.0001);
    assert(camera_reads == old_reads + 1 && projections == old_projections + 2);
}

int main(void)
{
    Vec3 origin = {0, 0, 0}, right = {1, 0, 0}, left = {-1, 0, 0};
    Vec3 down = {0, 1, 0}, up = {0, -1, 0}, diagonal = {1, 1, 0};

    assert(effect_screen_heading(origin, right) == 0.0 && projections == 0);
    camera_present = 1;
    memcpy(camera, g_identityMatrix.m, sizeof camera);
    check_heading(origin, right, 0);
    check_heading(origin, down, 90);
    check_heading(origin, left, 180);
    check_heading(origin, up, 270);
    check_heading(origin, diagonal, 45);
    check_heading(origin, origin, 0);
    camera_present = 0;
    assert(effect_screen_heading(origin, up) == 0.0 && projections == 12);
    puts("effect heading: missing camera, shared projection, quadrants and coincident points passed");
    return 0;
}
