// What the shadow pass's vertex shader (world_caster.hlsl) hands its two fragment shaders, and what
// both write: world_caster.hlsl's for the casters whose texels its alpha test drops, the models'
// textured records, and world_caster_solid.hlsl's for the others, the ground and the records
// without a texture, which drop none and so keep the GPU's early depth test
// (render/world_layout.h, wl_caster_array).

struct CasterOut
{
    float4 position : SV_Position;
    [[vk::location(0)]] float depth : TEXCOORD0;
    [[vk::location(1)]] float2 uv : TEXCOORD1;
    // the texture's layer, whether the record is a model's
    [[vk::location(2)]] nointerpolation uint2 layer : TEXCOORD2;
};

// a caster's depth from the light, negative for a model's
float4 caster_depth(CasterOut i)
{
    return float4(i.depth * (i.layer.y != 0 ? -1 : 1), 0, 0, 1);
}
