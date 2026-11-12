#ifndef RENDER_VERTEX_FOG_H
#define RENDER_VERTEX_FOG_H

/* custom: the original's vertex fog on the GPU, to the bit. The client computed on the CPU, for
   every pre-transformed vertex, the share of its colour the vertex keeps from the fog, in 1/255
   steps (vertex_fog_step), and Direct3D 9 interpolated it across the triangle. The GPU cannot
   repeat that computation exactly (Vulkan allows its divisions 2.5 units in the last place, and
   its compilers may fuse operations), so it looks the step up instead: for the fog's start and
   end, the rhw at which each step begins is a table (vertex_fog_table) that the quad shader
   searches with comparisons alone, which are exact (vertex_fog_lookup does it as the shader does).
   Pure: it uses nothing else of the client. */

#define VERTEX_FOG_ENTRIES 256

/* The step, from 0 (all fog) to 255 (none), of a vertex of reciprocal view depth rhw:
   t = ((1 / rhw - origin) / span - start) / (end - start), without the second division where end
   equals start, clamped to 0..1, and (1 - t) * 255 rounded down, each operation in single
   precision, as the client computed it. */
int vertex_fog_step(float rhw, float start, float end, float origin, float span);

/* Where the steps change: for every rhw > 0, vertex_fog_step gives
   base + direction * min(count, the number of entries of rhw[] that are at most rhw); base is the
   step of every rhw up to the smallest normal float. The entries are in ascending order; those
   past count are +infinity. */
struct VertexFogTable {
    float base, direction, count;
    float rhw[VERTEX_FOG_ENTRIES];
};

void vertex_fog_table(float start, float end, float origin, float span, struct VertexFogTable *out);
/* The step of rhw > 0 by the table, found as the quad shader finds it (render/shaders/quad.hlsl): a
   binary search of eight comparisons. */
int vertex_fog_lookup(const struct VertexFogTable *table, float rhw);

#endif
