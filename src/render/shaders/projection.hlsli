// The client's 4x3 matrices (render/gfx.h) and its projection of a point (render/rp_project.c,
// renderer_project_vertex and renderer_project_depth) on the GPU, in plain arithmetic: what the
// quads' vertices (instances.hlsli), the static world's views and cull (world_transform.hlsl,
// world_cull.hlsl) and the water's quads (water.hlsli) compute as the client's C code computes it.
// The GPU's floats are within a tolerance of the CPU's, not the same bits (plans/gpu-renderer.md,
// Decisions); the tests hold them to it.

// render/render_api.h: RENDER_NEAR_Z, RENDER_DEPTH_SCALE
#define NEAR_Z 10.0
#define DEPTH_SCALE (40000.0 / (40000.0 - 10.0))

// The client's 4x3 row-vector matrices as three float4 of its twelve floats in order: m[0..3],
// m[4..7], m[8..11].
struct Matrix43
{
    float4 r0, r1, r2;
};

// As a row-vector 4x4 matrix (render/world_layout.c, wl_view_clip's)
float4x4 m44(Matrix43 m)
{
    return float4x4(m.r0.xyz, 0.0,
                    float3(m.r0.w, m.r1.xy), 0.0,
                    float3(m.r1.zw, m.r2.x), 0.0,
                    m.r2.yzw, 1.0);
}

// A row-vector 4x4 matrix whose last column is (0, 0, 0, 1) as the client's 4x3
Matrix43 m43(float4x4 m)
{
    Matrix43 o;
    o.r0 = float4(m[0].xyz, m[1].x);
    o.r1 = float4(m[1].yz, m[2].xy);
    o.r2 = float4(m[2].z, m[3].xyz);
    return o;
}

// renderer_project_depth: the depth of a pre-transformed vertex of this rhw
float project_depth(float rhw)
{
    return (1.0 / rhw - NEAR_Z) * DEPTH_SCALE * rhw;
}

// renderer_project_vertex: a point's rhw and its place on the screen; screen holds Renderer.proj[0],
// proj[1], centreX and centreY
struct Projected
{
    float rhw, x, y;
};

// the projection of a point in view space
Projected project_view(float3 v, float4 screen)
{
    Projected o;
    o.rhw = 1.0 / v.z;
    o.x = v.x * o.rhw * screen.x + screen.z;
    o.y = v.y * o.rhw * screen.y + screen.w;
    return o;
}

// a point through the client's view matrix m, then projected
Projected project_point(float3 p, Matrix43 m, float4 screen)
{
    return project_view(mul(float4(p, 1.0), m44(m)).xyz, screen);
}
