#ifndef RESOURCES_GRANNY_SDK_H
#define RESOURCES_GRANNY_SDK_H

#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"
#include "base.h"

typedef struct granny_file granny_file;
typedef struct granny_model_instance granny_model_instance;
typedef struct granny_local_pose granny_local_pose;
typedef struct granny_world_pose granny_world_pose;
typedef struct granny_mesh_binding granny_mesh_binding;
typedef struct granny_control granny_control;
typedef struct granny_animation granny_animation;
typedef struct granny_material granny_material;
typedef struct granny_pixel_layout granny_pixel_layout;
struct SkinSource;

typedef struct granny_texture {
    const char *FromFileName;
    int TextureType;
    int Width;
    int Height;
    int Encoding;
    int SubFormat;
    int Layout[9];
    int ImageCount;
    void *Images;
    void *ExtendedData[2];
} granny_texture;

typedef struct granny_skeleton {
    const char *Name;
    int BoneCount;
    void *Bones;
    int LODType;
} granny_skeleton;

typedef struct granny_material_binding {
    granny_material *Material;
} granny_material_binding;

typedef struct granny_mesh {
    const char *Name;
    void *PrimaryVertexData;
    int MorphTargetCount;
    void *MorphTargets;
    void *PrimaryTopology;
    int MaterialBindingCount;
    granny_material_binding *MaterialBindings;
    int BoneBindingCount;
    void *BoneBindings;
    void *ExtendedData[2];
} granny_mesh;

typedef struct granny_model_mesh_binding {
    granny_mesh *Mesh;
} granny_model_mesh_binding;

typedef struct granny_model {
    const char *Name;
    granny_skeleton *Skeleton;
    BYTE InitialPlacement[0x44];
    int MeshBindingCount;
    granny_model_mesh_binding *MeshBindings;
} granny_model;

typedef struct granny_file_info {
    void *ArtToolInfo;
    void *ExporterInfo;
    const char *FromFileName;
    int FileType;
    int TextureCount;
    granny_texture **Textures;
    int MaterialCount;
    granny_material **Materials;
    int SkeletonCount;
    granny_skeleton **Skeletons;
    int VertexDataCount;
    void **VertexDatas;
    int TriTopologyCount;
    void **TriTopologies;
    int MeshCount;
    granny_mesh **Meshes;
    int ModelCount;
    granny_model **Models;
    int TrackGroupCount;
    void **TrackGroups;
    int AnimationCount;
    granny_animation **Animations;
} granny_file_info;

#define GRANNY_COLOR_MAP_TEXTURE_TYPE   0
#define GRANNY_DIFFUSE_COLOR_TEXTURE    2

typedef struct GrannyClock { unsigned char bytes[16]; } GrannyClock;
SIZE_ASSERT(GrannyClock, 16);

GrannyClock _GrannyGetSystemSeconds(void);
float _GrannyGetSecondsElapsed(const GrannyClock *start, const GrannyClock *end);
float _GrannyGetControlDurationLeft(granny_control *control);
BYTE _GrannyVersionsMatch_(int major, int minor, int customization, int build);
granny_file *_GrannyReadEntireFileFromMemory(int size, void *memory);
granny_file_info *_GrannyGetFileInfo(granny_file *file);
void _GrannyFreeFile(granny_file *file);
granny_local_pose *_GrannyNewLocalPose(int bones);
void _GrannyFreeLocalPose(granny_local_pose *pose);
granny_world_pose *_GrannyNewWorldPose(int bones);
void _GrannyFreeWorldPose(granny_world_pose *pose);
granny_model_instance *_GrannyInstantiateModel(granny_model *model);
void _GrannyFreeModelInstance(granny_model_instance *instance);
granny_skeleton *_GrannyGetSourceSkeleton(granny_model_instance *instance);
granny_mesh_binding *_GrannyNewMeshBinding(granny_mesh *mesh, granny_skeleton *from,
                                           granny_skeleton *to);
void _GrannyFreeMeshBinding(granny_mesh_binding *binding);
int _GrannyGetMeshIndexCount(granny_mesh *mesh);
void _GrannyCopyMeshIndices(granny_mesh *mesh, int bytesPerIndex, void *out);
int _GrannyGetMeshVertexCount(granny_mesh *mesh);
BYTE _GrannyMeshIsRigid(granny_mesh *mesh);
/* custom: vertices first to first + count of a mesh, at most its own, as the converted file holds
   them, for the device to skin (render/skin_layout.h); the bones they name are the skeleton's. */
void _GrannyCopyMeshSkinVertices(granny_mesh *mesh, int first, int count, struct SkinSource *out);
granny_texture *_GrannyGetMaterialTextureByType(granny_material *material, int type);
BYTE _GrannyTextureHasAlpha(granny_texture *texture);
void _GrannyCopyTextureImage(granny_texture *texture, int image, int mip,
                             const granny_pixel_layout *layout, int width, int height,
                             int stride, void *pixels);
granny_control *_GrannyPlayControlledAnimation(float start, granny_animation *animation,
                                               granny_model_instance *instance);
void _GrannySetControlLoopCount(granny_control *control, int count);
void _GrannySetControlActive(granny_control *control, BYTE active);
void _GrannyFreeControl(granny_control *control);
void _GrannySetModelClock(granny_model_instance *instance, float t);
void _GrannySampleModelAnimations(granny_model_instance *instance, int first, int count,
                                  granny_local_pose *localPose);
void _GrannyBuildWorldPose(granny_skeleton *skeleton, int first, int count,
                           granny_local_pose *localPose, const float *offset,
                           granny_world_pose *worldPose);
float *_GrannyGetWorldPoseComposite4x4Array(granny_world_pose *worldPose);
int *_GrannyGetMeshBindingToBoneIndices(granny_mesh_binding *binding);
int *_GrannyGetMeshTriangleGroups(granny_mesh *mesh);
extern const granny_pixel_layout *GrannyRGBA8888PixelFormat;

#endif
