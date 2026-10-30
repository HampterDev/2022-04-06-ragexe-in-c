#ifndef GRANNY_SDK_TEST_SUPPORT_H
#define GRANNY_SDK_TEST_SUPPORT_H

struct granny_model_instance { int alive, id; float time; granny_model *source; };
struct granny_animation { int index; };
struct granny_control {
    int alive, loops, active;
    granny_animation *animation;
    granny_model_instance *instance;
};
struct granny_local_pose { int alive, bones, sampled_id; float time; };
struct granny_world_pose { int alive, bones, sampled_id; float time, matrix[16]; };
struct granny_mesh_binding {
    int alive, bones[1];
    granny_mesh *mesh;
    granny_skeleton *from, *to;
};

static granny_control sdk_controls[40];
static granny_animation sdk_animations[] = {{0},{1},{2},{3},{4}};
static unsigned sdk_calls, sdk_fail_at;
static int sdk_instances, sdk_control_count, sdk_locals, sdk_worlds, sdk_bindings;
static int sdk_next_id, sdk_played_id, sdk_sampled_id, sdk_lookups;
static int sdk_missing_animation, sdk_missing_skeleton;
static float sdk_sampled_time;

static int sdk_failure(void) { return ++sdk_calls == sdk_fail_at; }
static void sdk_assert_empty(void)
{
    assert(!sdk_instances && !sdk_control_count && !sdk_locals && !sdk_worlds && !sdk_bindings);
}

granny_model_instance *__stdcall _GrannyInstantiateModel(granny_model *model)
{
    granny_model_instance *instance;
    assert(model);
    if (sdk_failure()) return NULL;
    instance = calloc(1, sizeof *instance); assert(instance);
    instance->alive = 1; instance->id = ++sdk_next_id; instance->source = model;
    sdk_instances++;
    return instance;
}
void __stdcall _GrannyFreeModelInstance(granny_model_instance *instance)
{
    int i;
    assert(instance && instance->alive);
    for (i = 0; i < 40; i++)
        assert(!sdk_controls[i].alive || sdk_controls[i].instance != instance);
    instance->alive = 0; sdk_instances--; free(instance);
}
granny_skeleton *__stdcall _GrannyGetSourceSkeleton(granny_model_instance *instance)
{ assert(instance->alive); return sdk_missing_skeleton ? NULL : instance->source->Skeleton; }
granny_local_pose *__stdcall _GrannyNewLocalPose(int bones)
{
    granny_local_pose *pose;
    assert(bones >= 0);
    if (sdk_failure()) return NULL;
    pose = calloc(1, sizeof *pose); assert(pose);
    pose->alive = 1; pose->bones = bones; sdk_locals++;
    return pose;
}
void __stdcall _GrannyFreeLocalPose(granny_local_pose *pose)
{ assert(pose && pose->alive); pose->alive = 0; sdk_locals--; free(pose); }
granny_world_pose *__stdcall _GrannyNewWorldPose(int bones)
{
    granny_world_pose *pose;
    assert(bones >= 0);
    if (sdk_failure()) return NULL;
    pose = calloc(1, sizeof *pose); assert(pose);
    pose->alive = 1; pose->bones = bones; sdk_worlds++;
    return pose;
}
void __stdcall _GrannyFreeWorldPose(granny_world_pose *pose)
{ assert(pose && pose->alive); pose->alive = 0; sdk_worlds--; free(pose); }
granny_animation *granny_animation_cache_get(struct C3dGrannyBoneRes *bones, int job, int part)
{
    (void)bones;
    assert(job == 0 && part >= 0 && part < 5);
    sdk_lookups++;
    return sdk_missing_animation ? NULL : &sdk_animations[part];
}
granny_control *__stdcall _GrannyPlayControlledAnimation(float start, granny_animation *animation,
                                                        granny_model_instance *instance)
{
    int i;
    assert(start == 0 && instance->alive);
    if (sdk_failure()) return NULL;
    for (i = 0; i < 40 && sdk_controls[i].alive; i++) {}
    assert(i < 40);
    memset(&sdk_controls[i], 0, sizeof sdk_controls[i]);
    sdk_controls[i].alive = sdk_controls[i].active = 1;
    sdk_controls[i].instance = instance; sdk_controls[i].animation = animation;
    sdk_played_id = instance->id; sdk_control_count++;
    return &sdk_controls[i];
}
void __stdcall _GrannySetControlLoopCount(granny_control *control, int loops)
{ assert(control->alive && control->instance->alive); control->loops = loops; }
void __stdcall _GrannySetControlActive(granny_control *control, BYTE active)
{ assert(control->alive && control->instance->alive); control->active = active; }
void __stdcall _GrannyFreeControl(granny_control *control)
{
    assert(control->alive && control->instance->alive && !control->active);
    control->alive = 0; sdk_control_count--;
}
float __stdcall _GrannyGetControlDurationLeft(granny_control *control)
{ assert(control->alive && control->instance->alive); return 10.0f - control->instance->time; }
void __stdcall _GrannySetModelClock(granny_model_instance *instance, float time)
{ assert(instance->alive); instance->time = time; }
void __stdcall _GrannySampleModelAnimations(granny_model_instance *instance, int first, int count,
                                           granny_local_pose *pose)
{
    assert(instance->alive && first == 0 && count == pose->bones && pose->alive);
    pose->sampled_id = instance->id; pose->time = instance->time;
    sdk_sampled_id = instance->id; sdk_sampled_time = instance->time;
}
void __stdcall _GrannyBuildWorldPose(granny_skeleton *skeleton, int first, int count,
                                    granny_local_pose *local, const float *offset, granny_world_pose *world)
{
    assert(skeleton && first == 0 && count == skeleton->BoneCount);
    assert(local->alive && world->alive && count == local->bones && count == world->bones && !offset);
    world->time = local->time; world->sampled_id = local->sampled_id;
    memset(world->matrix, 0, sizeof world->matrix);
    world->matrix[0] = world->matrix[5] = world->matrix[10] = world->matrix[15] = 1.0f;
    world->matrix[12] = local->time;
}
float *__stdcall _GrannyGetWorldPoseComposite4x4Array(granny_world_pose *world)
{ assert(world->alive); return world->matrix; }
float *__stdcall _GrannyGetWorldPoseComposite4x4(granny_world_pose *world, int bone)
{ assert(world->alive && bone == 0); return world->matrix; }
granny_mesh_binding *__stdcall _GrannyNewMeshBinding(granny_mesh *mesh, granny_skeleton *from,
                                                   granny_skeleton *to)
{
    granny_mesh_binding *binding;
    assert(mesh && from && to);
    if (sdk_failure()) return NULL;
    binding = calloc(1, sizeof *binding); assert(binding);
    binding->alive = 1; binding->mesh = mesh; binding->from = from; binding->to = to;
    sdk_bindings++;
    return binding;
}
void __stdcall _GrannyFreeMeshBinding(granny_mesh_binding *binding)
{ assert(binding->alive); binding->alive = 0; sdk_bindings--; free(binding); }
int *__stdcall _GrannyGetMeshBindingToBoneIndices(granny_mesh_binding *binding)
{ assert(binding->alive); return binding->bones; }
GrannyClock __stdcall _GrannyGetSystemSeconds(void)
{ GrannyClock clock = {{0}}; return clock; }
float __stdcall _GrannyGetSecondsElapsed(const GrannyClock *start, const GrannyClock *end)
{ assert(start && end); return 0.25f; }

#endif
