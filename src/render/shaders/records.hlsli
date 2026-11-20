// The draw records render/gfx_gpu.c writes for a frame (struct GpuDraw there, the same layout: only
// 16-byte vectors and row-major matrices, compiled with -Zpr), read as storage buffers or, a draw
// of the static world's, as its uniform. A quad's draw finds its record through the quad's record
// in the frame's quads (instances.hlsli), a Granny actor's through its instance's header among the
// frame's quads (skin.hlsl), the water's through its uniform, the wobble's through the
// instance-rate attribute that holds its index.

struct Draw
{
    uint4 mode;       // -, the alpha test (ALPHA_*), fog on, the water's view among the frame's,
                      // which render/gfx_gpu.c hands it as its uniform (the wobble's: its first row
                      // in the row tables, rows.hlsl)
    float4 target;    // the alpha test's reference (0 to 255), half the target's width and height, alpha
                      // to coverage
    uint4 stages;     // the combination (render/draw_modes.h, enum DrawCombine), the stages that read
                      // opaque black outside 0 to 1 (bit 0 stage 0, bit 1 stage 1) and whether stage
                      // 0 reads a layer of an atlas array (bit 2, quad.hlsli), the ground's lightmap, -
    float4 fog;       // the fog (render/gfx.h, GfxFog): the quads' start, end, origin and span
                      // (quad.hlsli, vertex_fog); the world's start and end as view depths and
                      // 1 / (end - start) (render/gfx_gpu.c, view_fog)
    float4 fogColour;
    float4 colour;    // a world draw's colour (render/gfx.h, GfxWorldDraw)
    float4 uvScale;   // each stage's texture coordinates times this, and then at most uvLimit: a
    float4 uvLimit;   // target's texture is wider than the target (render/gfx_gpu.c, MARGIN), and a
                      // clamped read stays off the margin
    uint4 mesh;       // a model, its ground light, -, whether the frame has shadows (cascades.hlsli)
};

// A stage's texture coordinates as its texture is sampled (Draw.uvScale).
float2 stage_uv(float2 uv, float2 scale, float2 limit)
{
    return min(uv * scale, limit);
}

// A stage's texel: opaque black outside 0 to 1 where the stage reads with a border
// (render/draw_modes.h, DRAW_ADDRESS_BORDER), what it sampled elsewhere.
float4 bordered(float4 sampled, float2 uv, bool border)
{
    return border && any(abs(uv - 0.5) > 0.5) ? float4(0, 0, 0, 1) : sampled;
}

// A point times a row-major matrix, the point a row with w 1.
float4 transform(float3 p, float4x4 m)
{
    return mul(float4(p, 1), m);
}

// Clip space for a pre-transformed vertex at `window`, Direct3D 9's window coordinates (pixel
// centres on whole numbers), in a pass whose viewport is the target moved by half a pixel (0.5,
// 0.5, width, height; `half` is half its size): snapped to the 1/256 of a pixel Direct3D 9's
// rasterizer snapped it to, rounding to even. The GPU's viewport transform lands within a few
// ten-thousandths of a pixel of that point, half a pixel on; the 1/1024 more keeps it on the same
// 1/256 whether the GPU rounds or truncates there.
float2 snapped_ndc(float2 window, float2 half)
{
    float2 p = round(window * 256) / 256 + 1.0 / 1024;
    return float2(p.x / half.x - 1, 1 - p.y / half.y);
}

// render/draw_modes.h, enum DrawAlphaTest
#define ALPHA_AT_LEAST 1
#define ALPHA_BELOW 2

// Whether a pixel of this alpha (0 to 1, rounded to 0 to 255) passes the alpha test against the
// reference.
bool alpha_passes(uint test, float alpha, float reference)
{
    float a = floor(alpha * 255 + 0.5);
    return test == ALPHA_AT_LEAST ? a >= reference : test == ALPHA_BELOW ? a < reference : true;
}

// The alpha test of a draw, or alpha to coverage in its place: the edge where alpha crosses the
// reference, a pixel wide, spread over the samples. Answers false where the pixel is dropped.
bool alpha_test(inout float alpha, Draw d)
{
    if (d.target.w != 0) {
        float a = alpha * 255;
        alpha = saturate((a - d.target.x) / max(fwidth(a), 1.0 / 16) + 0.5);
        return alpha > 0;
    }
    return alpha_passes(d.mode.y, alpha, d.target.x);
}
