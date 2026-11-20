// Granny's actors skinned, lit and projected (render/gfx.h, gfx_draw_skin) as the CPU did each
// vertex before: what skin.hlsl draws and tests/shaders/skin_probe.hlsl hands back to
// tests/test_skin_instances.c, which compares it with that CPU code within a tolerance. The file
// that includes this one declares the frame's quads (StructuredBuffer<uint4> quads), among which
// an actor's pose lies as render/skin_layout.c packs it (skin_pose_rows): a header, its meshes'
// space in the view, its light, the projection, and its bones' matrices.

#define SKIN_RIGID 1u        // render/skin_layout.h: a vertex of a rigid mesh
#define SKIN_POSE_HEADER 7u  // the rows before a pose's bones
#define SKIN_RIGID_STILL 1u  // the pose's flag: rigid meshes as they lie, without their bone

// A vertex of a model as the device keeps it (render/skin_layout.h, SkinVertex)
struct SkinVertex
{
    float3 position;
    float3 normal;
    float2 uv;
    float4 weights;
    uint4 bones;
    uint flags;
};

// What a vertex becomes in a pose: its place in view space, its colour (r, g, b, a, each 0 to 1)
// and its texture coordinates
struct Skinned
{
    float3 view;
    float4 colour;
    float2 uv;
};

float4 pose_row(uint row)
{
    return asfloat(quads[row]);
}

// A point (w 1) or a direction (w 0) through the three columns from row `at` on
float3 through(float4 p, uint at)
{
    return float3(dot(p, pose_row(at)), dot(p, pose_row(at + 1)), dot(p, pose_row(at + 2)));
}

// Vertex v in the pose whose rows start at `pose`. Granny's deformer moved a skinned vertex by the
// sum of its weighted bones' matrices, its normal likewise without their translation, a weight of 0
// skipped (its bone is not read: a bone the file left Inf or NaN moves no vertex it does not
// weight); a rigid mesh's by its one bone (but the guild flag's, which lie as they are), its
// normal as it lies. The
// pose's view matrix takes the mesh's space into view space. The light: b, the clamped cosine
// between the normal and the light, times the scale plus the offset, cut to a whole number as the
// C cast did; each of blue, green and red is b plus that byte of the pose's colour, modulo 256.
Skinned skin_vertex(uint pose, SkinVertex v)
{
    uint4 h = quads[pose];
    uint bones = max(h.x, 1u);
    bool rigid = (v.flags & SKIN_RIGID) != 0;
    float3 p = v.position, n = v.normal;
    if (!rigid || (h.y & SKIN_RIGID_STILL) == 0) {
        float3 sp = 0, sn = 0;
        [unroll] for (uint k = 0; k < 4; k++) {
            uint at = pose + SKIN_POSE_HEADER + min(v.bones[k], bones - 1) * 3;
            if (v.weights[k] != 0) {
                sp += v.weights[k] * through(float4(v.position, 1), at);
                sn += v.weights[k] * through(float4(v.normal, 0), at);
            }
        }
        p = sp;
        if (!rigid)
            n = sn;
    }
    Skinned o;
    o.view = through(float4(p, 1), pose + 1);
    float4 light = pose_row(pose + 4);
    uint b = (uint)(saturate(dot(n, light.xyz)) * light.w + asfloat(h.w));
    uint3 add = uint3(h.z >> 16, h.z >> 8, h.z) & 255;
    o.colour = float4(float3((b + add) & 255), (float)((h.y >> 8) & 255)) / 255;
    o.uv = v.uv;
    return o;
}

// A view-space point of the pose in clip space for a target of half the size `half`: at the pixel
// and depth the CPU gave the pre-transformed vertex it made of the point (render/rp_project.c,
// renderer_project_vertex: x * proj.x / z + centre.x, y likewise, the depth (z - near) * scale / z),
// which the GPU divides by w = z and moves by the viewport as Direct3D 9 placed that vertex.
float4 skin_clip(uint pose, float3 v, float2 half)
{
    float4 projection = pose_row(pose + 5), depth = pose_row(pose + 6);
    return float4((v.x * projection.x + v.z * projection.z) / half.x - v.z,
                  v.z - (v.y * projection.y + v.z * projection.w) / half.y,
                  (v.z - depth.x) * depth.y, v.z);
}
