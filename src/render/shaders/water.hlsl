// The map's water (render/gfx.h, gfx_world_draw_water): a quad over each cell the frame draws,
// built here from the cell's place in the map's water and the waves' phase as the original built
// it on the CPU each frame (water.hlsli), then drawn through the camera's view, its colour and
// texture coordinates interpolated by hand, with the quads' pixels (quad.hlsli), the fog by the rhw
// the CPU's projection gave its corners. A quad the CPU left out lies outside the view.
#include "water.hlsli"
#include "quad.hlsli"

// the cells (struct GpuWaterCell, three uint4 each), then the table of sines by whole degree,
// render/math3d.c's g_sinTable, four floats a uint4
StructuredBuffer<uint4> cells : register(t1, space0);
StructuredBuffer<Water> waters : register(t2, space0);

cbuffer WaterFrame : register(b0, space1)
{
    uint4 frame;          // x: the draw's record, y: the waves the frame has, z: the cells
    WaterCamera camera;   // the view the CPU tested and keyed the quads in
    float4x4 toClip;      // the camera's view to clip space (render/gfx_gpu.c, struct GpuView)
};

WaterCell cell_at(uint i)
{
    WaterCell c;
    c.cell = cells[i * 3];
    c.corners01 = asfloat(cells[i * 3 + 1]);
    c.corners23 = asfloat(cells[i * 3 + 2]);
    return c;
}

float sine(int degrees)
{
    return asfloat(cells[frame.z * 3 + (uint)degrees / 4][(uint)degrees % 4]);
}

struct WaterIn
{
    [[vk::location(0)]] uint cell : TEXCOORD0;      // per instance: the cell
    [[vk::location(1)]] float4 diffuse : TEXCOORD1; // per instance: the bytes of its D3DCOLOR: b, g, r, a
};

// a quad's corners as triangles 0 1 2 and 2 1 3, as the client's quads are drawn (render/gfx_gpu.c,
// quad_triangles)
static const uint strip[6] = {0, 1, 2, 2, 1, 3};

Varyings vs_main(WaterIn i, uint vertex : SV_VertexID)
{
    Draw d = draws[frame.x];
    WaterCell c = cell_at(i.cell);
    bool waved = c.cell.z < frame.y;  // a cell of a block the frame has no wave of draws nothing
    Water w = waters[waved ? c.cell.z : 0];
    uint corner = strip[vertex % 6];
    int diagonal = (int)c.cell.x + (int)c.cell.y;
    float3 heights = float3(wave_height(w, sine(wave_degrees(w, diagonal - 1))),
                            wave_height(w, sine(wave_degrees(w, diagonal))),
                            wave_height(w, sine(wave_degrees(w, diagonal + 1))));
    WaterQuad q = water_quad(c, camera, heights);
    Varyings o;
    o.draw = frame.x;
    o.layer = 0;
    if (!waved || !q.drawn) {
        o.position = float4(2, 2, 2, 1);
        o.diffuse = o.uv = 0;
        o.fogQ = 0;
        return o;
    }
    o.position = transform(q.p[corner], toClip);
    float q1 = 1 / o.position.w;
    // the original's quarters of the texture: ((float)(x % 4) + 1.0f) * 0.25f and so on
    float2 side = float2((corner & 1) != 0 ? 1.0 : 0.0, (corner & 2) != 0 ? 1.0 : 0.0);
    float2 uv = (float2(c.cell.xy % 4) + side) * 0.25;
    o.diffuse = i.diffuse.zyxw * q1;
    o.uv = float4(uv, uv) * q1;
    o.fogQ = float2(d.mode.z != 0 ? vertex_fog(q.rhw[corner], d.fog) * q1 : q1, q1);
    return o;
}

float4 fs_main(Varyings i) : SV_Target0
{
    return quad_pixel(i);
}
