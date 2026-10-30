#define GRANNY_IMPORT
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "resources/granny_res.h"
#include "owned_storage_test_allocator.h"

static void *playback_calloc(size_t count, size_t size)
{
    void *p = checked_malloc(count * size);
    if (p) memset(p, 0, count * size);
    return p;
}
#define calloc playback_calloc
#define free checked_free
#include "../src/resources/granny_playback.c"
#undef calloc
#undef free
#include "granny_sdk_test_support.h"

int main(void)
{
    struct C3dGrannyModelRes resource = {0};
    struct GrannyResModel model = {0};
    granny_model source = {0};
    granny_skeleton skeleton = {0};
    struct GrannyPlayback *first, *second;
    granny_world_pose *first_pose, *second_pose;
    unsigned before;
    int i, calls;

    skeleton.BoneCount = 3; source.Skeleton = &skeleton;
    resource.grannyModel = &source; resource.model = &model;
    assert(!granny_playback_create(NULL));
    fail_at = allocation_calls + 1;
    assert(!granny_playback_create(&resource)); fail_at = 0;
    for (i = 1; i <= 3; i++) {
        sdk_fail_at = sdk_calls + i;
        assert(!granny_playback_create(&resource)); sdk_fail_at = 0;
        sdk_assert_empty(); assert(!outstanding);
    }
    sdk_missing_skeleton = 1;
    assert(!granny_playback_create(&resource)); sdk_missing_skeleton = 0;
    skeleton.BoneCount = -1;
    assert(!granny_playback_create(&resource)); skeleton.BoneCount = 3;
    sdk_assert_empty(); assert(!outstanding);

    first = granny_playback_create(&resource); assert(first);
    calls = sdk_lookups;
    assert(!granny_playback_play(first, -1, 0));
    assert(!granny_playback_play(first, 5, 0));
    assert(!granny_playback_play(first, INT_MAX, 0) && calls == sdk_lookups);
    sdk_missing_animation = 1;
    assert(!granny_playback_play(first, 2, 1)); sdk_missing_animation = 0;
    sdk_fail_at = sdk_calls + 1;
    assert(!granny_playback_play(first, 2, 1)); sdk_fail_at = 0;
    assert(!sdk_control_count && !granny_playback_finished(first, 2));
    for (i = 0; i < 5; i++) assert(granny_playback_play(first, i, i));
    assert(sdk_control_count == 5);
    before = sdk_calls;
    assert(granny_playback_play(first, 2, 99) && sdk_calls == before);
    assert(sdk_controls[2].loops == 2);
    assert(granny_playback_remaining(first, 2) == 10.0f);
    first_pose = granny_playback_pose(first, 10.0f);
    assert(granny_playback_finished(first, 2));
    assert(!granny_playback_finished(first, -1));
    assert(first_pose == granny_playback_world_pose(first));
    assert(first_pose->time == 10.0f && sdk_sampled_id == sdk_played_id);
    assert(!granny_playback_pose(NULL, 1.0f) && !granny_playback_world_pose(NULL));

    second = granny_playback_create(&resource); assert(second);
    second_pose = granny_playback_pose(second, 3.0f);
    assert(first_pose != second_pose && first_pose->time == 10.0f && second_pose->time == 3.0f);
    granny_playback_pose(first, 20.0f);
    assert(first_pose->time == 20.0f && second_pose->time == 3.0f);
    granny_playback_destroy(second);
    assert(first_pose->alive && first_pose->time == 20.0f);
    granny_playback_stop(first); granny_playback_stop(first);
    assert(!sdk_control_count);
    granny_playback_destroy(first); granny_playback_destroy(NULL);
    sdk_assert_empty(); assert(!outstanding);

    for (i = 0; i < 10000; i++) {
        first = granny_playback_create(&resource); assert(first);
        assert(granny_playback_play(first, 2, 0));
        first_pose = granny_playback_pose(first, 7.0f);
        assert(first_pose->sampled_id == sdk_played_id && first_pose->time == 7.0f);
        granny_playback_destroy(first);
        sdk_assert_empty(); assert(!outstanding);
    }
    puts("Granny playback: owned SDK handles and independent poses, bounded animations, creation failures, control reuse and 10000 complete lifetimes passed");
    return 0;
}
