// The frame's views of the static world's transform slots (render/world_layout.h, WlSlot): for
// each slot its matrices as the Direct3D 9 client's CPU computed them for a model node's draw
// (projection.hlsli): a model node's view the node's matrix times its parent's view, up to the
// model's own matrix times the camera's view, as the original drew a model's nodes
// (tests/test_static_world_maps.c keeps that code, ref_node), then to clip space by the projection;
// and its place in the world, the node's matrix times its parent's up to the model's matrix, which
// the cull moves an animated record's bounds by and the shadow casters are drawn through.
// The slot that is the world itself takes the camera's view as it is.
#include "projection.hlsli"
#include "world.hlsli"

#define SLOT_WORLD 1     // WlSlot.link.y: the world itself
#define CHAIN_MOST 64    // render/gfx.h, GFX_WORLD_CHAIN

struct Slot
{
    Matrix43 local;  // the node's (or the model's) matrix
    int4 link;       // the parent slot (-1: the camera's view), flags, the frame's animated matrix (-1: none)
};

StructuredBuffer<Slot> slots : register(t0, space0);
StructuredBuffer<Matrix43> animated : register(t1, space0);
RWStructuredBuffer<WorldView> views : register(u0, space1);
RWStructuredBuffer<Matrix43> placed : register(u1, space1);

cbuffer Frame : register(b0, space2)
{
    Matrix43 camera;          // the camera's view (render/gfx.h, GfxWorldView.view)
    float4x4 projection;      // view space to clip space
    uint4 counts;             // the slots
};

float4x4 slot_local(Slot s)
{
    if (s.link.z >= 0)
        return m44(animated[s.link.z]);
    return m44(s.local);
}

[numthreads(64, 1, 1)]
void cs_main(uint3 id : SV_DispatchThreadID)
{
    uint slot = id.x;
    if (slot >= counts.x)
        return;
    Slot self = slots[slot];
    float4x4 m = m44(camera), world = float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
    if (self.link.y != SLOT_WORLD) {
        uint chain[CHAIN_MOST];
        uint n = 0;
        int at = (int)slot;
        while (at >= 0 && n < CHAIN_MOST) {
            chain[n++] = (uint)at;
            at = slots[at].link.x;
        }
        // from the model down to the slot: each node's matrix, then its parent's
        [loop] for (uint k = n; k-- > 0;) {
            float4x4 local = slot_local(slots[chain[k]]);
            m = mul(local, m);
            world = k == n - 1 ? local : mul(local, world);
        }
    }
    WorldView v;
    v.toClip = mul(m, projection);
    v.toView = m;
    views[slot] = v;
    placed[slot] = m43(world);
}
