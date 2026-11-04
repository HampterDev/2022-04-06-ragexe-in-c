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
typedef struct granny_mesh_deformer granny_mesh_deformer;
typedef struct granny_control granny_control;
typedef struct granny_animation granny_animation;
typedef struct granny_material granny_material;
typedef struct granny_data_type_definition granny_data_type_definition;
typedef struct granny_pixel_layout granny_pixel_layout;

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
#define GRANNY_DEFORM_POSITION_NORMAL   2

typedef struct GrannyClock { unsigned char bytes[16]; } GrannyClock;
SIZE_ASSERT(GrannyClock, 16);

#ifndef GRANNY_IMPORT
#ifdef PLATFORM_RAD_DLLS
#define GRANNY_IMPORT __declspec(dllimport)
#else
#define GRANNY_IMPORT
#endif
#endif
GRANNY_IMPORT GrannyClock __stdcall _GrannyGetSystemSeconds(void);
GRANNY_IMPORT float __stdcall _GrannyGetSecondsElapsed(const GrannyClock *start, const GrannyClock *end);
GRANNY_IMPORT float __stdcall _GrannyGetControlDurationLeft(granny_control *control);
GRANNY_IMPORT BYTE __stdcall _GrannyVersionsMatch_(int major, int minor, int customization, int build);
GRANNY_IMPORT granny_file *__stdcall _GrannyReadEntireFileFromMemory(int size, void *memory);
GRANNY_IMPORT granny_file_info *__stdcall _GrannyGetFileInfo(granny_file *file);
GRANNY_IMPORT void __stdcall _GrannyFreeFileSection(granny_file *file, int section);
GRANNY_IMPORT void __stdcall _GrannyFreeFile(granny_file *file);
GRANNY_IMPORT granny_local_pose *__stdcall _GrannyNewLocalPose(int bones);
GRANNY_IMPORT void __stdcall _GrannyFreeLocalPose(granny_local_pose *pose);
GRANNY_IMPORT granny_world_pose *__stdcall _GrannyNewWorldPose(int bones);
GRANNY_IMPORT void __stdcall _GrannyFreeWorldPose(granny_world_pose *pose);
GRANNY_IMPORT granny_model_instance *__stdcall _GrannyInstantiateModel(granny_model *model);
GRANNY_IMPORT void __stdcall _GrannyFreeModelInstance(granny_model_instance *instance);
GRANNY_IMPORT granny_skeleton *__stdcall _GrannyGetSourceSkeleton(granny_model_instance *instance);
GRANNY_IMPORT granny_mesh_binding *__stdcall _GrannyNewMeshBinding(granny_mesh *mesh, granny_skeleton *from,
                                                                            granny_skeleton *to);
GRANNY_IMPORT void __stdcall _GrannyFreeMeshBinding(granny_mesh_binding *binding);
GRANNY_IMPORT int __stdcall _GrannyGetMeshIndexCount(granny_mesh *mesh);
GRANNY_IMPORT void __stdcall _GrannyCopyMeshIndices(granny_mesh *mesh, int bytesPerIndex, void *out);
GRANNY_IMPORT int __stdcall _GrannyGetMeshVertexCount(granny_mesh *mesh);
GRANNY_IMPORT BYTE __stdcall _GrannyMeshIsRigid(granny_mesh *mesh);
GRANNY_IMPORT void __stdcall _GrannyCopyMeshVertices(granny_mesh *mesh, const granny_data_type_definition *type,
                                                             void *out);
GRANNY_IMPORT const granny_data_type_definition *__stdcall _GrannyGetMeshVertexType(granny_mesh *mesh);
GRANNY_IMPORT granny_mesh_deformer *__stdcall _GrannyNewMeshDeformer(const granny_data_type_definition *from,
                                                                              const granny_data_type_definition *to,
                                                                              int deformation);
GRANNY_IMPORT void __stdcall _GrannyFreeMeshDeformer(granny_mesh_deformer *deformer);
GRANNY_IMPORT granny_texture *__stdcall _GrannyGetMaterialTextureByType(granny_material *material, int type);
GRANNY_IMPORT BYTE __stdcall _GrannyTextureHasAlpha(granny_texture *texture);
GRANNY_IMPORT void __stdcall _GrannyCopyTextureImage(granny_texture *texture, int image, int mip,
                                                             const granny_pixel_layout *layout, int width, int height,
                                                             int stride, void *pixels);
GRANNY_IMPORT granny_control *__stdcall _GrannyPlayControlledAnimation(float start, granny_animation *animation,
                                                                                granny_model_instance *instance);
GRANNY_IMPORT void __stdcall _GrannySetControlLoopCount(granny_control *control, int count);
GRANNY_IMPORT void __stdcall _GrannySetControlActive(granny_control *control, BYTE active);
GRANNY_IMPORT void __stdcall _GrannyFreeControl(granny_control *control);
GRANNY_IMPORT void __stdcall _GrannySetModelClock(granny_model_instance *instance, float t);
GRANNY_IMPORT void __stdcall _GrannySampleModelAnimations(granny_model_instance *instance, int first, int count,
                                                                  granny_local_pose *localPose);
GRANNY_IMPORT void __stdcall _GrannyBuildWorldPose(granny_skeleton *skeleton, int first, int count,
                                                           granny_local_pose *localPose, const float *offset,
                                                           granny_world_pose *worldPose);
GRANNY_IMPORT void __stdcall _GrannyFreeCompletedModelControls(granny_model_instance *instance);
GRANNY_IMPORT float *__stdcall _GrannyGetWorldPoseComposite4x4Array(granny_world_pose *worldPose);
GRANNY_IMPORT float *__stdcall _GrannyGetWorldPoseComposite4x4(granny_world_pose *worldPose, int bone);
GRANNY_IMPORT int *__stdcall _GrannyGetMeshBindingToBoneIndices(granny_mesh_binding *binding);
GRANNY_IMPORT int *__stdcall _GrannyGetMeshTriangleGroups(granny_mesh *mesh);
GRANNY_IMPORT void *__stdcall _GrannyGetMeshVertices(granny_mesh *mesh);
GRANNY_IMPORT void __stdcall _GrannyDeformVertices(granny_mesh_deformer *deformer, const int *bones,
                                                           const float *composite, int count, const void *from, void *to);
GRANNY_IMPORT int __stdcall _GrannyGetMeshBytesPerIndex(granny_mesh *mesh);
GRANNY_IMPORT void *__stdcall _GrannyGetMeshIndices(granny_mesh *mesh);
GRANNY_IMPORT extern const granny_pixel_layout *GrannyRGBA8888PixelFormat;
GRANNY_IMPORT extern const granny_data_type_definition *GrannyPNT332VertexType;

#endif
