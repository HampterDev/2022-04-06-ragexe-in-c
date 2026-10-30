#include <stdlib.h>
#include "resources/granny_res.h"
#include "resources/granny_playback.h"

enum { GRANNY_ANIMATION_COUNT = 5 };

struct GrannyPlayback {
    struct C3dGrannyModelRes *resource;
    granny_model_instance *instance;
    granny_skeleton *skeleton;
    granny_local_pose *local_pose;
    granny_world_pose *world_pose;
    granny_control *controls[GRANNY_ANIMATION_COUNT];
};

struct GrannyPlayback *granny_playback_create(struct C3dGrannyModelRes *resource)
{
    struct GrannyPlayback *playback;

    if (!resource || !resource->grannyModel || !resource->model)
        return NULL;
    playback = calloc(1, sizeof *playback);
    if (!playback)
        return NULL;
    playback->instance = _GrannyInstantiateModel(resource->grannyModel);
    if (!playback->instance)
        goto fail;
    playback->skeleton = _GrannyGetSourceSkeleton(playback->instance);
    if (!playback->skeleton || playback->skeleton->BoneCount < 0)
        goto fail;
    playback->local_pose = _GrannyNewLocalPose(playback->skeleton->BoneCount);
    if (!playback->local_pose)
        goto fail;
    playback->world_pose = _GrannyNewWorldPose(playback->skeleton->BoneCount);
    if (!playback->world_pose)
        goto fail;
    playback->resource = resource;
    return playback;

fail:
    granny_playback_destroy(playback);
    return NULL;
}

void granny_playback_stop(struct GrannyPlayback *playback)
{
    int i;

    if (!playback)
        return;
    for (i = 0; i < GRANNY_ANIMATION_COUNT; i++) {
        if (playback->controls[i]) {
            _GrannySetControlActive(playback->controls[i], 0);
            _GrannyFreeControl(playback->controls[i]);
            playback->controls[i] = NULL;
        }
    }
}

void granny_playback_destroy(struct GrannyPlayback *playback)
{
    if (!playback)
        return;
    granny_playback_stop(playback);
    if (playback->instance)
        _GrannyFreeModelInstance(playback->instance);
    if (playback->local_pose)
        _GrannyFreeLocalPose(playback->local_pose);
    if (playback->world_pose)
        _GrannyFreeWorldPose(playback->world_pose);
    free(playback);
}

int granny_playback_play(struct GrannyPlayback *playback, int animation, int loops)
{
    granny_control *control;
    granny_animation *source;

    if (!playback || (unsigned)animation >= GRANNY_ANIMATION_COUNT)
        return 0;
    control = playback->controls[animation];
    if (control) {
        _GrannySetControlActive(control, 1);
        return 1;
    }
    source = granny_animation_cache_get(playback->resource->boneRes,
                                        playback->resource->id, animation);
    if (!source)
        return 0;
    control = _GrannyPlayControlledAnimation(0.0f, source, playback->instance);
    if (!control)
        return 0;
    _GrannySetControlLoopCount(control, loops);
    playback->controls[animation] = control;
    return 1;
}

static granny_control *playback_control(const struct GrannyPlayback *playback, int animation)
{
    if (!playback || (unsigned)animation >= GRANNY_ANIMATION_COUNT)
        return NULL;
    return playback->controls[animation];
}

float granny_playback_remaining(const struct GrannyPlayback *playback, int animation)
{
    granny_control *control = playback_control(playback, animation);

    return control ? _GrannyGetControlDurationLeft(control) : 0.0f;
}

int granny_playback_finished(const struct GrannyPlayback *playback, int animation)
{
    granny_control *control = playback_control(playback, animation);

    return control && !(_GrannyGetControlDurationLeft(control) > 0.0f);
}

granny_world_pose *granny_playback_world_pose(const struct GrannyPlayback *playback)
{
    return playback ? playback->world_pose : NULL;
}

granny_world_pose *granny_playback_pose(struct GrannyPlayback *playback, float time)
{
    int bones;

    if (!playback)
        return NULL;
    bones = playback->skeleton->BoneCount;
    _GrannySetModelClock(playback->instance, time);
    _GrannySampleModelAnimations(playback->instance, 0, bones, playback->local_pose);
    _GrannyBuildWorldPose(playback->skeleton, 0, bones, playback->local_pose, NULL, playback->world_pose);
    return playback->world_pose;
}
