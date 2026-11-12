// The records render/gfx_gpu.c writes for a frame, read as storage buffers (structs GpuDraw,
// GpuView and GpuShadow there, the same layout: only 16-byte vectors and row-major matrices,
// compiled with -Zpr). A draw finds its record through the instance-rate attribute that holds its
// index; its view and shadow records through the indices in the draw record.

struct Draw
{
    uint4 mode;       // the kind (KIND_*), the alpha test (ALPHA_*), fog on, the view (the wobble's:
                      // its first row in the row tables, rows.hlsl)
    float4 target;    // the alpha test's reference (0 to 255), half the target's width and height, alpha
                      // to coverage
    uint4 stages;     // the combination (render/draw_modes.h, enum DrawCombine), the stages that read
                      // opaque black outside 0 to 1 (bit 0 stage 0, bit 1 stage 1), the ground's
                      // lightmap, the quads' fog table (64 float4 each, render/vertex_fog.h)
    float4 fog;       // the quads' fog: its table's base, direction and count
    float4 fogColour;
    float4 colour;    // a world draw's colour (render/gfx.h, GfxWorldDraw)
    float4 uvScale;   // each stage's texture coordinates times this, and then at most uvLimit: a
    float4 uvLimit;   // target's texture is wider than the target (render/gfx_gpu.c, MARGIN), and a
                      // clamped read stays off the margin
    uint4 mesh;       // a model, its ground light, -, the shadow record + 1 while receiving
};

struct View
{
    float4x4 toClip;  // from the draw's space to clip space
};

struct Shadow
{
    float4x4 fromView[4];  // view space to each cascade's texture space, texel centres as drawn
    float4 farDepth;
    float4 offset;
    float4 blur;
    float4 slant;
    float4 lightView;      // towards the light, view space; w: the bias
    float4 ambient;        // the ambient light; w: the darkness (at least 1 / 255)
    float4 mapSize;        // x: the maps' size in texels (render/gfx.h, GFX_SHADOW_SIZE)
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

// A point times a row-major matrix in the order Direct3D 9's shader compiler multiplied it in (fxc,
// vs_2_0 and vs_3_0: y, then x, then z, then the translation), so that a vertex lands on the
// pixel the Direct3D 9 client put it on, to the last bit as far as the GPU allows.
float4 transform(float3 p, float4x4 m)
{
    float4 r = m[1] * p.y;
    r = mad(p.x, m[0], r);
    r = mad(p.z, m[2], r);
    return r + m[3];
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

// Draw.mode.x, the kind of draw (render/gfx_gpu.c, KIND_*), where a shader tells them apart:
#define KIND_WORLD 1       // GFX_VTX_WORLD: world coordinates through the camera's view

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
