// The shadow pass's fragment shader for the casters that drop no texel, the ground and the records
// without a texture (world_caster.hlsli): their depth from the light alone, with nothing that turns
// off the GPU's early depth test. Drawn after world_caster.hlsl's vertex shader.
#include "world_caster.hlsli"

float4 fs_main(CasterOut i) : SV_Target0
{
    return caster_depth(i);
}
