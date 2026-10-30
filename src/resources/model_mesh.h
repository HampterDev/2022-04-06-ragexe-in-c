#ifndef RESOURCES_MODEL_MESH_H
#define RESOURCES_MODEL_MESH_H

struct AnimMesh;
struct File;

struct AnimMesh *model_mesh_read(struct File *file, int major, int minor,
                                const float *matrix);

#endif
