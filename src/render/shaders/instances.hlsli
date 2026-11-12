// The client's quads and triangles as instances (render/gfx.h, gfx_draw_quad and
// gfx_draw_vertices): the record of each in the frame's quads, as render/quads.c makes it, and the
// vertex it stands for at each corner, made from it exactly as the client's C code made it
// (exact.hlsli): quad_vertex, which quad.hlsl draws and tests/shaders/quad_probe.hlsl hands back
// to tests/test_quad_instances.c. The file that includes this one declares the frame's quads
// (StructuredBuffer<uint4> quads) and includes exact.hlsli first.
//
// The frame's quads hold the records and what they name: the matrices of QUAD_WORLD records and
// the vertices of QUAD_SCREEN and QUAD_CAMERA ones, each starting on a row of four words.

// A record: a header, then what its kind holds (render/quads.c). The header: x the kind, its flags
// and (bits 16 to 31) the layer of the atlas array its stage 0 reads; y the draw's record; z the
// colour (a D3DCOLOR) where the kind has one; w a value of the kind.
#define QUAD_SPRITE 1   // render_object_submit_spr_quad's: (x0, x1, top, bottom); (the bottom row's
                        // rhw, the top row's, the flipped screen's x and y); (cosine, sine); uv
#define QUAD_RECT 2     // a rectangle: (x0, x1, top, bottom); (the top row's z, the bottom row's);
                        // uv; w: the rhw
#define QUAD_SCREEN 3   // pre-transformed vertices, w the word of the first of them (7 words each:
                        // x, y, z, rhw, the colour, u, v), corner i the i-th
#define QUAD_WORLD 4    // points projected as renderer_project_vertex projects them, through the
                        // matrix whose first row is w (four rows: its view, then proj[0], proj[1],
                        // centreX, centreY): (x, y, z, the rhw added), (x, y, z, the z added),
                        // (x, y, z), (x, y, z); u and v of each corner, then the second stage's
                        // where QUAD_STAGE2
#define QUAD_CAMERA 5   // world points through the camera's view (the draw's view, quad.hlsl), as
                        // Direct3D 9 drew them: vertices as QUAD_SCREEN's, with the rhw the CPU
                        // gave them
#define QUAD_KIND 15
#define QUAD_TRIANGLE 16   // corners 0 to 2 alone: corner 3 is corner 2 again
#define QUAD_SNAP 32       // a flat draw in a multisampled pass: corners onto pixel boundaries
#define QUAD_TURNED 64     // QUAD_SPRITE: turned about its centre
#define QUAD_STAGE2 128    // QUAD_WORLD: the second stage's u and v of each corner follow
#define QUAD_RHW_ADD 256   // QUAD_WORLD: the rhw is the projection's plus the one added
#define QUAD_Z_ZERO 1024   // QUAD_WORLD: z is 0
#define QUAD_Z_ADD 2048    //   or the depth of the rhw plus the z added; otherwise the depth of the rhw
#define QUAD_Z 3072
#define QUAD_VIEW 4096     // QUAD_WORLD: the points are in view space already; the matrix gives the
                           // projection alone

// One vertex as the client made it: a pre-transformed vertex (render/quads.h, TexDrawVertex) with
// its colour's bytes as numbers, in the D3DCOLOR's order in memory (b, g, r, a), and both stages'
// coordinates
struct Corner
{
    float x, y, z, rhw;
    float4 bytes;
    float u0, v0, u1, v1;
};

float4 colour_bytes(uint c)
{
    return float4((float)(c & 255), (float)((c >> 8) & 255), (float)((c >> 16) & 255), (float)(c >> 24));
}

float4 quad_float4(uint row)
{
    return asfloat(quads[row]);
}

uint quad_word(uint word)
{
    return quads[word >> 2][word & 3];
}

// The vertex at `corner` of the instance whose record starts at row `at`; nothing (zeros) for a
// kind the shader does not know
Corner corner_of(uint at, uint4 h, uint corner)
{
    Corner o;
    uint kind = h.x & QUAD_KIND;
    bool right = (corner & 1) != 0, low = (corner & 2) != 0;
    o.x = o.y = o.z = o.rhw = 0.0;
    o.u0 = o.v0 = o.u1 = o.v1 = 0.0;
    o.bytes = colour_bytes(h.z);
    if (kind == QUAD_SPRITE) {
        float4 a = quad_float4(at + 1), b = quad_float4(at + 2), turn = quad_float4(at + 3), uv = quad_float4(at + 4);
        float x0 = a.x, x1 = a.y, top = a.z, bottom = a.w;
        precise float zb = project_depth(b.y);
        precise float za = project_depth(b.x);
        if ((h.x & QUAD_TURNED) != 0) {
            float c = turn.x, sn = turn.y;
            precise float hy = (bottom - top) * 0.5;
            precise float hx = (x1 - x0) * 0.5;
            precise float cy = (top + hy) + b.w;
            precise float cx = (x0 + hx) + b.z;
            precise float hy_s = hy * sn, hx_c = hx * c, hx_s = hx * sn, hy_c = hy * c;
            precise float x, y;
            if (corner == 0) {
                x = ((-hx_c - -hy_s) + cx) + 0.5;
                y = ((-hy_c + -hx_s) + cy) + 0.5;
            } else if (corner == 1) {
                x = ((hx_c - -hy_s) + cx) + 0.5;
                y = ((-hy_c + hx_s) + cy) + 0.5;
            } else if (corner == 2) {
                x = ((-hx_c - hy_s) + cx) + 0.5;
                y = ((hy_c + -hx_s) + cy) + 0.5;
            } else {
                x = ((hx_c - hy_s) + cx) + 0.5;
                y = ((hy_c + hx_s) + cy) + 0.5;
            }
            o.x = x;
            o.y = y;
        } else {
            precise float x = (right ? x1 : x0) + 0.5;
            precise float y = (low ? bottom : top) + 0.5;
            o.x = x;
            o.y = y;
        }
        o.z = low ? za : zb;
        o.rhw = b.y;
        o.u0 = right ? uv.z : uv.x;
        o.v0 = low ? uv.w : uv.y;
        o.u1 = o.u0;
        o.v1 = o.v0;
    } else if (kind == QUAD_RECT) {
        float4 a = quad_float4(at + 1), z = quad_float4(at + 2), uv = quad_float4(at + 3);
        o.x = right ? a.y : a.x;
        o.y = low ? a.w : a.z;
        o.z = low ? z.y : z.x;
        o.rhw = asfloat(h.w);
        o.u0 = right ? uv.z : uv.x;
        o.v0 = low ? uv.w : uv.y;
        o.u1 = o.u0;
        o.v1 = o.v0;
    } else if (kind == QUAD_SCREEN || kind == QUAD_CAMERA) {
        uint v = h.w + corner * 7;
        o.x = asfloat(quad_word(v));
        o.y = asfloat(quad_word(v + 1));
        o.z = asfloat(quad_word(v + 2));
        o.rhw = asfloat(quad_word(v + 3));
        o.bytes = colour_bytes(quad_word(v + 4));
        o.u0 = asfloat(quad_word(v + 5));
        o.v0 = asfloat(quad_word(v + 6));
        o.u1 = o.u0;
        o.v1 = o.v0;
    } else if (kind == QUAD_WORLD) {
        float4 p = quad_float4(at + 1 + corner);
        uint m = h.w;
        Projected pr;
        if ((h.x & QUAD_VIEW) != 0)
            pr = project_view(p.xyz, quad_float4(m + 3));
        else
            pr = project_point(p.xyz, quad_float4(m), quad_float4(m + 1), quad_float4(m + 2), quad_float4(m + 3));
        float add = quad_float4(at + 1).w, zAdd = quad_float4(at + 2).w;
        precise float rhw = pr.rhw;
        if ((h.x & QUAD_RHW_ADD) != 0)
            rhw = pr.rhw + add;
        uint zmode = h.x & QUAD_Z;
        precise float z = 0.0;
        if (zmode == QUAD_Z_ADD)
            z = project_depth(rhw) + zAdd;
        else if (zmode != QUAD_Z_ZERO)
            z = project_depth(rhw);
        o.x = pr.x;
        o.y = pr.y;
        o.z = z;
        o.rhw = rhw;
        o.u0 = quad_float4(at + 5)[corner];
        o.v0 = quad_float4(at + 6)[corner];
        if ((h.x & QUAD_STAGE2) != 0) {
            o.u1 = quad_float4(at + 7)[corner];
            o.v1 = quad_float4(at + 8)[corner];
        } else {
            o.u1 = o.u0;
            o.v1 = o.v0;
        }
    }
    return o;
}

// A colour byte at (ox, oy) on its triangle's plane, rounded and clamped to a byte (not a number
// gives 0)
float plane_byte(float a, float b, float c, float dx1, float dy1, float dx2, float dy2, float det, float ox, float oy)
{
    precise float v = plane_value(a, b, c, dx1, dy1, dx2, dy2, det, ox, oy);
    precise float r = floor(v + 0.5);
    r = r > 255.0 ? 255.0 : r;
    r = r >= 0.0 ? r : 0.0;
    return r;
}

// Direct3D 9 drew flat draws without multisampling: every sample of a pixel was covered when its
// centre was. In a multisampled pass a flat draw's vertex moves onto the pixel boundary that keeps
// that coverage (pixel_boundary) and takes the values its triangle's plane has there (a flat
// triangle's values are affine on the screen), so each pixel centre reads what it read before. A
// quad's triangles are (0, 1, 2) and (2, 1, 3); a vertex takes the plane of the first that has it,
// unless that one has no area.
Corner snapped(uint at, uint4 h, uint corner)
{
    Corner mine = corner_of(at, h, corner);
    Corner a = corner_of(at, h, corner == 3 ? 2 : 0);
    Corner b = corner_of(at, h, 1);
    Corner c = corner_of(at, h, corner == 3 ? 3 : 2);
    precise float dx1 = b.x - a.x, dy1 = b.y - a.y, dx2 = c.x - a.x, dy2 = c.y - a.y;
    precise float det = dx1 * dy2 - dx2 * dy1;
    if (det > -F_FLAT_DET && det < F_FLAT_DET) {
        if (corner == 0 || corner == 3 || (h.x & QUAD_TRIANGLE) != 0)
            return mine;
        // the first triangle has no area: the second moves 1 and 2
        a = corner_of(at, h, 2);
        c = corner_of(at, h, 3);
        dx1 = b.x - a.x;
        dy1 = b.y - a.y;
        dx2 = c.x - a.x;
        dy2 = c.y - a.y;
        det = dx1 * dy2 - dx2 * dy1;
        if (det > -F_FLAT_DET && det < F_FLAT_DET)
            return mine;
    }
    precise float sx = pixel_boundary(mine.x), sy = pixel_boundary(mine.y);
    if (sx == mine.x && sy == mine.y)
        return mine;
    precise float ox = sx - a.x, oy = sy - a.y;
    mine.x = sx;
    mine.y = sy;
    mine.z = plane_value(a.z, b.z, c.z, dx1, dy1, dx2, dy2, det, ox, oy);
    mine.bytes.x = plane_byte(a.bytes.x, b.bytes.x, c.bytes.x, dx1, dy1, dx2, dy2, det, ox, oy);
    mine.bytes.y = plane_byte(a.bytes.y, b.bytes.y, c.bytes.y, dx1, dy1, dx2, dy2, det, ox, oy);
    mine.bytes.z = plane_byte(a.bytes.z, b.bytes.z, c.bytes.z, dx1, dy1, dx2, dy2, det, ox, oy);
    mine.bytes.w = plane_byte(a.bytes.w, b.bytes.w, c.bytes.w, dx1, dy1, dx2, dy2, det, ox, oy);
    mine.u0 = plane_value(a.u0, b.u0, c.u0, dx1, dy1, dx2, dy2, det, ox, oy);
    mine.v0 = plane_value(a.v0, b.v0, c.v0, dx1, dy1, dx2, dy2, det, ox, oy);
    mine.u1 = plane_value(a.u1, b.u1, c.u1, dx1, dy1, dx2, dy2, det, ox, oy);
    mine.v1 = plane_value(a.v1, b.v1, c.v1, dx1, dy1, dx2, dy2, det, ox, oy);
    return mine;
}

// What the vertex shader takes of corner `corner` (0 top left, 1 top right, 2 bottom left, 3 bottom
// right) of the instance whose record starts at row `at`: the vertex as the client made it, a
// triangle's corner 3 being its corner 2, moved onto pixel boundaries where QUAD_SNAP; its colour as
// Direct3D 9's vertex unit made floats of its bytes (r, g, b, a, each byte / 255); both stages'
// coordinates. QUAD_CAMERA's x, y and z are a world point, which quad.hlsl takes through the view.
struct QuadVertex
{
    float x, y, z, rhw;
    float4 colour;
    float4 uv;
};

QuadVertex quad_vertex(uint at, uint corner)
{
    uint4 h = quads[at];
    uint c = (h.x & QUAD_TRIANGLE) != 0 && corner == 3 ? 2 : corner;
    Corner v;
    if ((h.x & QUAD_SNAP) != 0)
        v = snapped(at, h, c);
    else
        v = corner_of(at, h, c);
    QuadVertex o;
    o.x = v.x;
    o.y = v.y;
    o.z = v.z;
    o.rhw = v.rhw;
    o.colour = float4(div_rn(v.bytes.z, 255.0), div_rn(v.bytes.y, 255.0), div_rn(v.bytes.x, 255.0),
                      div_rn(v.bytes.w, 255.0));
    o.uv = float4(v.u0, v.v0, v.u1, v.v1);
    return o;
}
