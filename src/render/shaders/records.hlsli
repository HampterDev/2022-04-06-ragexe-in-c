// The records render/gfx_gpu.c writes for a frame, read as storage buffers (structs GpuDraw,
// GpuView and GpuShadow there, the same layout: only 16-byte vectors and row-major matrices,
// compiled with -Zpr). A draw finds its record through the instance-rate attribute that holds its
// index; its view and shadow records through the indices in the draw record.

struct Draw
{
    uint4 stage0;     // the colour operation, its two arguments, the alpha operation (GFX_TOP_*, GFX_TA_*)
    uint4 stage0b;    // the alpha arguments, the coordinate index, flags (DRAW_TEXTURE, DRAW_BORDER)
    uint4 stage1;
    uint4 stage1b;
    uint4 mode;       // the kind (KIND_*), the alpha test's function (GFX_CMP_*, 0 off), fog on, the view
    float4 target;    // the alpha test's reference (0 to 255), half the target's width and height, alpha
                      // to coverage
    float4 fogColour;
    float4 border0;   // the border colour of each stage
    float4 border1;
    float4 colour;    // a mesh draw's colour (render/gfx.h, GfxMeshDraw)
    float4 uvU;       // its first texture's coordinates: u' = dot(float3(u, v, 1), uvU.xyz)
    float4 uvV;
    float4 uvScale;   // each stage's texture coordinates times this, and then at most uvLimit: a
    float4 uvLimit;   // target's texture is wider than the target (render/gfx_gpu.c, MARGIN), and a
                      // clamped read stays off the margin
    uint4 mesh;       // a model, its ground light, stage 1 on, the shadow record + 1 while receiving
};

struct View
{
    float4x4 toClip;  // from the draw's space to clip space
    float4x4 toView;  // from world space to view space (meshes)
    float4 fog;       // the fog's start and end in view depth, 1 / (end - start)
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

#define KIND_SCREEN 0     // pre-transformed: pixels with Direct3D 9's centres, z, rhw
#define KIND_WORLD 1       // GFX_VTX_WORLD: world coordinates through the camera's view
#define KIND_MESH 2
#define KIND_CASTER 3

#define DRAW_TEXTURE 1
#define DRAW_BORDER 2

#define TOP_DISABLE 1

// The alpha test: alpha (0 to 1, rounded to 0 to 255) against the reference, by the function
// (render/gfx.h: GFX_CMP_LESS, GFX_CMP_LESSEQUAL, GFX_CMP_GREATEREQUAL; 0, the test off, and any
// other pass everything).
bool alpha_passes(uint func, float alpha, float reference)
{
    float a = floor(alpha * 255 + 0.5);
    return func == 2 ? a < reference : func == 4 ? a <= reference : func == 7 ? a >= reference : true;
}
