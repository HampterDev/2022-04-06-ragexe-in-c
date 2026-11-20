/* The Granny shim's reading of a converted file's joints (resources/granny_gltf.c): a joint the
   skeleton does not have is bone 0, also one of 2^31 or more, which a signed comparison took as a
   negative bone. The file is made here, a glTF binary of a skeleton of two bones and two meshes
   whose joints are 32-bit integers (outside the glTF specification, which cgltf reads all the
   same): a rigid one on joint 0x80000001, which must stay a rigid mesh on bone 0 (render/skin_layout.h,
   skin_pack_vertex), and a skinned one whose joints name bones past the skeleton. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fileio/file_api.h"
#include "platform/clock.h"
#include "platform/system.h"
#include "render/skin_layout.h"
#include "resources/granny_sdk.h"

/* ---- what the shim reaches: the converted file served from memory ---- */

static unsigned char g_glb[2048];
static size_t g_glbSize;

struct File *file_construct(struct File *self)
{
    memset(self, 0, sizeof *self);
    return self;
}
unsigned char file_open(struct File *self, const char *name, int mode, unsigned char resolve, unsigned char diskOnly)
{
    (void)name; (void)mode; (void)resolve; (void)diskOnly;
    self->buffer = g_glb;
    self->size = (int)g_glbSize;
    return 1;
}
void file_destruct(struct File *self) { self->buffer = NULL; }
int64_t platform_perf_counter(void) { return 0; }
int64_t platform_perf_frequency(void) { return 1; }
void platform_debug_output(const char *text) { fputs(text, stderr); }

/* ---- the file ---- */

static const char g_json[] =
    "{\"asset\":{\"version\":\"2.0\"},\"buffers\":[{\"byteLength\":188}],"
    "\"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":36},{\"buffer\":0,\"byteOffset\":36,\"byteLength\":48},"
    "{\"buffer\":0,\"byteOffset\":84,\"byteLength\":6},{\"buffer\":0,\"byteOffset\":92,\"byteLength\":48},"
    "{\"buffer\":0,\"byteOffset\":140,\"byteLength\":48}],"
    "\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
    "{\"bufferView\":1,\"componentType\":5125,\"count\":3,\"type\":\"VEC4\"},"
    "{\"bufferView\":2,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"},"
    "{\"bufferView\":3,\"componentType\":5125,\"count\":3,\"type\":\"VEC4\"},"
    "{\"bufferView\":4,\"componentType\":5126,\"count\":3,\"type\":\"VEC4\"}],"
    "\"meshes\":[{\"name\":\"rigid\",\"extras\":{\"rigid\":true},"
    "\"primitives\":[{\"attributes\":{\"POSITION\":0,\"JOINTS_0\":1},\"indices\":2}]},"
    "{\"name\":\"skinned\",\"primitives\":[{\"attributes\":{\"POSITION\":0,\"JOINTS_0\":3,\"WEIGHTS_0\":4},\"indices\":2}]}],"
    "\"nodes\":[{\"name\":\"root\",\"children\":[1]},{\"name\":\"bone\"},{\"name\":\"rigid\",\"mesh\":0,\"skin\":0},"
    "{\"name\":\"skinned\",\"mesh\":1,\"skin\":0}],"
    "\"skins\":[{\"joints\":[0,1]}],\"scenes\":[{\"nodes\":[0,2,3]}],\"scene\":0}";

static void put32(unsigned char *at, uint32_t v)
{
    memcpy(at, &v, 4);
}

/* the glTF binary: its header, the JSON chunk padded with spaces, the binary chunk */
static void make_glb(void)
{
    static const float positions[9] = {0, 0, 0, 1, 0, 0, 0, 1, 0};
    static const uint32_t rigidJoints[12] = {0x80000001u, 0, 0, 0, 0x80000001u, 0, 0, 0, 0x80000001u, 0, 0, 0};
    static const uint16_t indices[4] = {0, 1, 2, 0};
    static const uint32_t skinJoints[12] = {0x80000000u, 1, 7, 0, 1, 0x80000000u, 0, 0, 0xffffffffu, 1, 0, 0};
    static const float weights[12] = {0.5f, 0.5f, 0, 0, 0.5f, 0.5f, 0, 0, 0.5f, 0.5f, 0, 0};
    unsigned char bin[188];
    size_t json = (sizeof g_json - 1 + 3) & ~(size_t)3, at = 0;
    memcpy(bin, positions, 36);
    memcpy(bin + 36, rigidJoints, 48);
    memcpy(bin + 84, indices, 8);
    memcpy(bin + 92, skinJoints, 48);
    memcpy(bin + 140, weights, 48);
    g_glbSize = 12 + 8 + json + 8 + sizeof bin;
    assert(g_glbSize <= sizeof g_glb);
    memcpy(g_glb, "glTF", 4);
    put32(g_glb + 4, 2);
    put32(g_glb + 8, (uint32_t)g_glbSize);
    at = 12;
    put32(g_glb + at, (uint32_t)json);
    memcpy(g_glb + at + 4, "JSON", 4);
    memset(g_glb + at + 8, ' ', json);
    memcpy(g_glb + at + 8, g_json, sizeof g_json - 1);
    at += 8 + json;
    put32(g_glb + at, (uint32_t)sizeof bin);
    memcpy(g_glb + at + 4, "BIN\0", 4);
    memcpy(g_glb + at + 8, bin, sizeof bin);
}

int main(void)
{
    char gr2 = 0;
    granny_file *f;
    granny_file_info *info;
    granny_model *model;
    granny_mesh *rigid, *skinned;
    granny_mesh_binding *binding;
    struct SkinSource src[3];
    struct SkinVertex packed;
    make_glb();
    f = _GrannyReadEntireFileFromMemory(1, &gr2);
    info = _GrannyGetFileInfo(f);
    assert(info && info->ModelCount == 1);
    model = info->Models[0];
    assert(model->Skeleton->BoneCount == 2 && model->MeshBindingCount == 2);
    rigid = model->MeshBindings[0].Mesh;
    skinned = model->MeshBindings[1].Mesh;
    /* the rigid mesh on joint 0x80000001: bone 0, and a rigid vertex on it */
    assert(_GrannyMeshIsRigid(rigid));
    binding = _GrannyNewMeshBinding(rigid, model->Skeleton, model->Skeleton);
    assert(binding && _GrannyGetMeshBindingToBoneIndices(binding)[0] == 0);
    _GrannyCopyMeshSkinVertices(rigid, 0, 3, src);
    skin_pack_vertex(&src[0], _GrannyGetMeshBindingToBoneIndices(binding)[0], model->Skeleton->BoneCount, &packed);
    assert(packed.flags == SKIN_RIGID && packed.bones[0] == 0 && packed.weights[0] == 1.0f);
    _GrannyFreeMeshBinding(binding);
    /* the skinned mesh: no binding; its joints past the skeleton bone 0 */
    assert(!_GrannyMeshIsRigid(skinned) && !_GrannyNewMeshBinding(skinned, model->Skeleton, model->Skeleton));
    _GrannyCopyMeshSkinVertices(skinned, 0, 3, src);
    assert(src[0].bones[0] == 0 && src[0].bones[1] == 1 && src[0].bones[2] == 0 && src[0].bones[3] == 0);
    assert(src[1].bones[0] == 1 && src[1].bones[1] == 0);
    assert(src[2].bones[0] == 0 && src[2].bones[1] == 1);
    assert(src[0].weights[0] == 0.5f && src[0].weights[1] == 0.5f);
    _GrannyFreeFile(f);
    puts("Granny glTF: joints past the skeleton, and of 2^31 or more, read as bone 0; a rigid mesh stays rigid.");
    return 0;
}
