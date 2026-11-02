#ifndef _WIN32
#include "resources/granny_sdk.h"

GrannyClock _GrannyGetSystemSeconds(void)
{
    GrannyClock clock = {{0}};
    return clock;
}

float _GrannyGetSecondsElapsed(const GrannyClock *start, const GrannyClock *end)
{
    (void)start;
    (void)end;
    return 0;
}

float _GrannyGetControlDurationLeft(granny_control *control)
{
    (void)control;
    return 0;
}

BYTE _GrannyVersionsMatch_(int major, int minor, int customization, int build)
{
    (void)major;
    (void)minor;
    (void)customization;
    (void)build;
    return 0;
}

granny_file *_GrannyReadEntireFileFromMemory(int size, void *memory)
{
    (void)size;
    (void)memory;
    return NULL;
}

granny_file_info *_GrannyGetFileInfo(granny_file *file)
{
    (void)file;
    return NULL;
}

void _GrannyFreeFileSection(granny_file *file, int section)
{
    (void)file;
    (void)section;
}

void _GrannyFreeFile(granny_file *file)
{
    (void)file;
}

granny_local_pose *_GrannyNewLocalPose(int bones)
{
    (void)bones;
    return NULL;
}

void _GrannyFreeLocalPose(granny_local_pose *pose)
{
    (void)pose;
}

granny_world_pose *_GrannyNewWorldPose(int bones)
{
    (void)bones;
    return NULL;
}

void _GrannyFreeWorldPose(granny_world_pose *pose)
{
    (void)pose;
}

granny_model_instance *_GrannyInstantiateModel(granny_model *model)
{
    (void)model;
    return NULL;
}

void _GrannyFreeModelInstance(granny_model_instance *instance)
{
    (void)instance;
}

granny_skeleton *_GrannyGetSourceSkeleton(granny_model_instance *instance)
{
    (void)instance;
    return NULL;
}

granny_mesh_binding *_GrannyNewMeshBinding(granny_mesh *mesh, granny_skeleton *from, granny_skeleton *to)
{
    (void)mesh;
    (void)from;
    (void)to;
    return NULL;
}

void _GrannyFreeMeshBinding(granny_mesh_binding *binding)
{
    (void)binding;
}

int _GrannyGetMeshIndexCount(granny_mesh *mesh)
{
    (void)mesh;
    return 0;
}

void _GrannyCopyMeshIndices(granny_mesh *mesh, int bytesPerIndex, void *out)
{
    (void)mesh;
    (void)bytesPerIndex;
    (void)out;
}

int _GrannyGetMeshVertexCount(granny_mesh *mesh)
{
    (void)mesh;
    return 0;
}

BYTE _GrannyMeshIsRigid(granny_mesh *mesh)
{
    (void)mesh;
    return 0;
}

void _GrannyCopyMeshVertices(granny_mesh *mesh, const granny_data_type_definition *type, void *out)
{
    (void)mesh;
    (void)type;
    (void)out;
}

const granny_data_type_definition *_GrannyGetMeshVertexType(granny_mesh *mesh)
{
    (void)mesh;
    return NULL;
}

granny_mesh_deformer *_GrannyNewMeshDeformer(const granny_data_type_definition *from, const granny_data_type_definition *to, int deformation)
{
    (void)from;
    (void)to;
    (void)deformation;
    return NULL;
}

void _GrannyFreeMeshDeformer(granny_mesh_deformer *deformer)
{
    (void)deformer;
}

granny_texture *_GrannyGetMaterialTextureByType(granny_material *material, int type)
{
    (void)material;
    (void)type;
    return NULL;
}

BYTE _GrannyTextureHasAlpha(granny_texture *texture)
{
    (void)texture;
    return 0;
}

void _GrannyCopyTextureImage(granny_texture *texture, int image, int mip, const granny_pixel_layout *layout, int width, int height, int stride, void *pixels)
{
    (void)texture;
    (void)image;
    (void)mip;
    (void)layout;
    (void)width;
    (void)height;
    (void)stride;
    (void)pixels;
}

granny_control *_GrannyPlayControlledAnimation(float start, granny_animation *animation, granny_model_instance *instance)
{
    (void)start;
    (void)animation;
    (void)instance;
    return NULL;
}

void _GrannySetControlLoopCount(granny_control *control, int count)
{
    (void)control;
    (void)count;
}

void _GrannySetControlActive(granny_control *control, BYTE active)
{
    (void)control;
    (void)active;
}

void _GrannyFreeControl(granny_control *control)
{
    (void)control;
}

void _GrannySetModelClock(granny_model_instance *instance, float t)
{
    (void)instance;
    (void)t;
}

void _GrannySampleModelAnimations(granny_model_instance *instance, int first, int count, granny_local_pose *localPose)
{
    (void)instance;
    (void)first;
    (void)count;
    (void)localPose;
}

void _GrannyBuildWorldPose(granny_skeleton *skeleton, int first, int count, granny_local_pose *localPose, const float *offset, granny_world_pose *worldPose)
{
    (void)skeleton;
    (void)first;
    (void)count;
    (void)localPose;
    (void)offset;
    (void)worldPose;
}

void _GrannyFreeCompletedModelControls(granny_model_instance *instance)
{
    (void)instance;
}

float *_GrannyGetWorldPoseComposite4x4Array(granny_world_pose *worldPose)
{
    (void)worldPose;
    return NULL;
}

float *_GrannyGetWorldPoseComposite4x4(granny_world_pose *worldPose, int bone)
{
    (void)worldPose;
    (void)bone;
    return NULL;
}

int *_GrannyGetMeshBindingToBoneIndices(granny_mesh_binding *binding)
{
    (void)binding;
    return NULL;
}

int *_GrannyGetMeshTriangleGroups(granny_mesh *mesh)
{
    (void)mesh;
    return NULL;
}

void *_GrannyGetMeshVertices(granny_mesh *mesh)
{
    (void)mesh;
    return NULL;
}

void _GrannyDeformVertices(granny_mesh_deformer *deformer, const int *bones, const float *composite, int count, const void *from, void *to)
{
    (void)deformer;
    (void)bones;
    (void)composite;
    (void)count;
    (void)from;
    (void)to;
}

int _GrannyGetMeshBytesPerIndex(granny_mesh *mesh)
{
    (void)mesh;
    return 0;
}

void *_GrannyGetMeshIndices(granny_mesh *mesh)
{
    (void)mesh;
    return NULL;
}

const granny_pixel_layout *GrannyRGBA8888PixelFormat;
const granny_data_type_definition *GrannyPNT332VertexType;
#endif
