// The client's quads and triangles as instances (render/gfx.h, gfx_draw_quad and
// gfx_draw_vertices): the record of each in the frame's quads, as render/quads.c makes it, and the
// vertex it stands for at each corner, made from it as the client's C code made it, in plain
// arithmetic (projection.hlsli): quad_vertex, which quad.hlsl draws and
// tests/shaders/quad_probe.hlsl hands back to tests/test_quad_instances.c, which holds it to a
// tolerance of that code. The file that includes this one declares the frame's quads
// (StructuredBuffer<uint4> quads) and includes projection.hlsli first.
//
// The frame's quads hold the records and what they name: the matrices of QUAD_WORLD records and of
// corner sets, the corner sets of QUAD_CORNERS records and the vertices of QUAD_SCREEN ones, each
// starting on a row of four words.

// A record: a header, then what its kind holds (render/quads.c). The header: x the kind, its flags
// and (bits 16 to 31) the layer of the atlas array its stage 0 reads; y the draw's record; z the
// colour (a D3DCOLOR) where the kind has one; w a value of the kind.
#define QUAD_SPRITE 1   // render_object_submit_spr_quad's: (x0, x1, top, bottom); (the bottom row's
                        // rhw, the top row's, the flipped screen's x and y); (cosine, sine, the
                        // bottom row's z, the top row's); uv
#define QUAD_RECT 2     // a rectangle: (x0, x1, top, bottom); (the top row's z, the bottom row's);
                        // uv; w: the rhw
#define QUAD_SCREEN 3   // pre-transformed vertices, w the word of the first of them (7 words each:
                        // x, y, z, rhw, the colour, u, v), corner i the i-th
#define QUAD_WORLD 4    // points projected as renderer_project_vertex projects them, through the
                        // matrix whose first row is w (four rows: its view, then proj[0], proj[1],
                        // centreX, centreY): (x, y, z, the rhw value), (x, y, z, the z value),
                        // (x, y, z), (x, y, z); u and v of each corner, then the second stage's
                        // where QUAD_STAGE2, then each corner's z where QUAD_Z_CORNERS
#define QUAD_CORNERS 5  // a triangle of three corners of the set whose first row is z (render/quads.c,
                        // quad_corner_words: x the rules' flags, y the colour, z the row of its
                        // matrix, as QUAD_WORLD's, w the word of its first corner after the set's
                        // first; the rules' values (the rhw's, the z's, the lift) where its rules
                        // take any; the upright point (x, y, z) where QUAD_UPRIGHT; then x, y, z, u
                        // and v of each corner), w the set's corners its corners 0 to 2 are, a byte
                        // each
#define QUAD_KIND 15
#define QUAD_TRIANGLE 16   // corners 0 to 2 alone: corner 3 is corner 2 again
#define QUAD_SNAP 32       // a flat draw in a multisampled pass: corners onto pixel boundaries
#define QUAD_TURNED 64     // QUAD_SPRITE: turned about its centre
#define QUAD_STAGE2 128    // QUAD_WORLD: the second stage's u and v of each corner follow
#define QUAD_RHW_ADD 256   // QUAD_WORLD, a corner set: the rhw is the projection's plus the value
#define QUAD_RHW_VALUE 512 //   or the value; otherwise the projection's
#define QUAD_Z_VALUE 1024  // QUAD_WORLD, a corner set: z is the value
#define QUAD_Z_ADD 2048    //   or the depth of the rhw plus the value; otherwise the depth of the rhw
#define QUAD_Z_CORNERS 3072 // QUAD_WORLD: each corner's z is given, in a row after the coordinates
#define QUAD_Z 3072
#define QUAD_VIEW 4096     // QUAD_WORLD: the points are in view space already; the matrix gives the
                           // projection alone
#define QUAD_UPRIGHT 8192  // a corner set: a triangle's corner's x is the upright point's, projected,
                           // moved by half the distance between its corners 0 and 1 projected
                           // (corner 0 right, corner 1 left, corner 2 right where QUAD_RIGHT), its y
                           // the projection's plus the lift
#define QUAD_RIGHT 16384   // QUAD_CORNERS: corner 2 of an upright triangle stands right

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

// Point p projected through the matrix whose first row is m (in view space already: through its
// projection alone)
Projected projected(float3 p, uint m, bool viewSpace)
{
    if (viewSpace)
        return project_view(p, quad_float4(m + 3));
    Matrix43 view;
    view.r0 = quad_float4(m);
    view.r1 = quad_float4(m + 1);
    view.r2 = quad_float4(m + 2);
    return project_point(p, view, quad_float4(m + 3));
}

// Corner k of the set whose header row is `set` (s its header): its first word
uint set_corner(uint set, uint4 s, uint k)
{
    return set * 4 + s.w + k * 5;
}

// Corner k of the set, projected
Projected set_point(uint set, uint4 s, uint k)
{
    uint w = set_corner(set, s, k);
    return projected(float3(asfloat(quad_word(w)), asfloat(quad_word(w + 1)), asfloat(quad_word(w + 2))), s.z,
                     false);
}

// A projected point's rhw and depth by the rules of `flags` (QUAD_RHW_*, QUAD_Z_*) with their values
// (a corner whose z is given, QUAD_Z_CORNERS, takes it after)
void apply_rules(inout Corner o, Projected pr, uint flags, float rhwValue, float zValue)
{
    float rhw = pr.rhw;
    if ((flags & QUAD_RHW_ADD) != 0)
        rhw = pr.rhw + rhwValue;
    else if ((flags & QUAD_RHW_VALUE) != 0)
        rhw = rhwValue;
    uint zmode = flags & QUAD_Z;
    float z = zValue;
    if (zmode == QUAD_Z_ADD)
        z = project_depth(rhw) + zValue;
    else if (zmode == 0)
        z = project_depth(rhw);
    o.x = pr.x;
    o.y = pr.y;
    o.z = z;
    o.rhw = rhw;
}

// What an upright triangle's corners stand on (QUAD_UPRIGHT): its set's upright point's projected x
// and half the distance between its corners 0's and 1's projected x; zeros for any other record.
struct Upright
{
    float x, half;
};

Upright upright_of(uint4 h)
{
    Upright u;
    u.x = u.half = 0.0;
    if ((h.x & QUAD_KIND) == QUAD_CORNERS) {
        uint4 s = quads[h.z];
        if ((s.x & QUAD_UPRIGHT) != 0) {
            u.x = projected(quad_float4(h.z + 2).xyz, s.z, false).x;
            u.half = abs(set_point(h.z, s, h.w & 255).x - set_point(h.z, s, (h.w >> 8) & 255).x) * 0.5;
        }
    }
    return u;
}

// The vertex at `corner` of the instance whose record starts at row `at` (u its upright_of);
// nothing (zeros) for a kind the shader does not know
Corner corner_of(uint at, uint4 h, uint corner, Upright u)
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
        float zb = turn.w;
        float za = turn.z;
        if ((h.x & QUAD_TURNED) != 0) {
            float c = turn.x, sn = turn.y;
            float hy = (bottom - top) * 0.5;
            float hx = (x1 - x0) * 0.5;
            float cy = (top + hy) + b.w;
            float cx = (x0 + hx) + b.z;
            float hy_s = hy * sn, hx_c = hx * c, hx_s = hx * sn, hy_c = hy * c;
            float x, y;
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
            float x = (right ? x1 : x0) + 0.5;
            float y = (low ? bottom : top) + 0.5;
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
    } else if (kind == QUAD_SCREEN) {
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
    } else if (kind == QUAD_WORLD || kind == QUAD_CORNERS) {
        // the point, its matrix, its rules and their values, of the record or of its set; one
        // projection for both
        float3 p;
        float4 values;
        uint m, flags;
        if (kind == QUAD_CORNERS) {
            uint4 s = quads[h.z];
            uint w = set_corner(h.z, s, (h.w >> (corner * 8)) & 255);
            p = float3(asfloat(quad_word(w)), asfloat(quad_word(w + 1)), asfloat(quad_word(w + 2)));
            m = s.z;
            flags = s.x;
            values = quad_float4(h.z + 1);
            o.bytes = colour_bytes(s.y);
            o.u0 = asfloat(quad_word(w + 3));
            o.v0 = asfloat(quad_word(w + 4));
        } else {
            p = quad_float4(at + 1 + corner).xyz;
            m = h.w;
            flags = h.x;
            values = float4(quad_float4(at + 1).w, quad_float4(at + 2).w, 0.0, 0.0);
            o.u0 = quad_float4(at + 5)[corner];
            o.v0 = quad_float4(at + 6)[corner];
        }
        apply_rules(o, projected(p, m, (flags & QUAD_VIEW) != 0), flags, values.x, values.y);
        if ((flags & QUAD_UPRIGHT) != 0) {
            float side = corner == 0 || (corner == 2 && (h.x & QUAD_RIGHT) != 0) ? 1.0 : -1.0;
            o.x = u.x + side * u.half;
            o.y = values.z + o.y;
        }
        if ((flags & QUAD_STAGE2) != 0) {
            o.u1 = quad_float4(at + 7)[corner];
            o.v1 = quad_float4(at + 8)[corner];
        } else {
            o.u1 = o.u0;
            o.v1 = o.v0;
        }
        if (kind == QUAD_WORLD && (flags & QUAD_Z) == QUAD_Z_CORNERS)
            o.z = quad_float4(at + ((flags & QUAD_STAGE2) != 0 ? 9 : 7))[corner];
    }
    return o;
}

// The pixel boundary before the first pixel centre at or after v, v snapped to 1/256 of a pixel
// rounding halves up: where a multisampled pass moves an edge of a flat draw's vertex (snapped), as
// the C code computed it in double. scaled + 0.5 may round in float, so the half is compared with
// what lies above the floor. From 2^23 / 256 on, v is a whole number of 256ths already (and v * 256
// may not be a float), so it is its own snap. Each step is exact, whatever the GPU fuses.
float pixel_boundary(float v)
{
    float scaled = v * 256.0;
    float below = floor(scaled);
    float whole = scaled - below >= 0.5 ? below + 1.0 : below;
    float snap = abs(scaled) >= 8388608.0 ? v : whole * (1.0 / 256.0);
    return ceil(snap) - 0.5;
}

// The value at (ox, oy) from a triangle's first vertex on the plane through its values a, b and
// c, the triangle's edges from that vertex being (dx1, dy1) and (dx2, dy2) and det their cross
// product
float plane_value(float a, float b, float c, float dx1, float dy1, float dx2, float dy2, float det, float ox, float oy)
{
    float d1 = b - a, d2 = c - a;
    return a + (d1 * dy2 - d2 * dy1) / det * ox + (d2 * dx1 - d1 * dx2) / det * oy;
}

// A colour byte at (ox, oy) on its triangle's plane, rounded and clamped to a byte (not a number
// gives 0)
float plane_byte(float a, float b, float c, float dx1, float dy1, float dx2, float dy2, float det, float ox, float oy)
{
    float r = floor(plane_value(a, b, c, dx1, dy1, dx2, dy2, det, ox, oy) + 0.5);
    r = r > 255.0 ? 255.0 : r;
    return r >= 0.0 ? r : 0.0;
}

// Direct3D 9 drew flat draws without multisampling: every sample of a pixel was covered when its
// centre was. In a multisampled pass a flat draw's vertex moves onto the pixel boundary that keeps
// that coverage (pixel_boundary) and takes the values its triangle's plane has there (a flat
// triangle's values are affine on the screen), so each pixel centre reads what it read before. A
// quad's triangles are (0, 1, 2) and (2, 1, 3); a vertex takes the plane of the first that has it,
// unless that one has no area: a cross product of its edges below 1e-6, as the C code tested it,
// or no larger than the rounding of the corners and of the product itself may make it (the corners'
// largest coordinate times the edges and the products, by 2^-20), where its plane would be noise.
bool no_area(Corner a, Corner b, Corner c, out float dx1, out float dy1, out float dx2, out float dy2, out float det)
{
    dx1 = b.x - a.x;
    dy1 = b.y - a.y;
    dx2 = c.x - a.x;
    dy2 = c.y - a.y;
    det = dx1 * dy2 - dx2 * dy1;
    float most = max(max(max(abs(a.x), abs(a.y)), max(abs(b.x), abs(b.y))), max(abs(c.x), abs(c.y)));
    float noise = abs(dx1 * dy2) + abs(dx2 * dy1) + most * (abs(dx1) + abs(dy1) + abs(dx2) + abs(dy2));
    return abs(det) < max(1e-6, noise * (1.0 / 1048576.0)) || det != det;
}

// k0 to k3: the quad's corners (a triangle's k3 is its k2)
Corner snapped(Corner k0, Corner k1, Corner k2, Corner k3, uint corner, bool threeCorners)
{
    Corner mine = k0, a = k0, b = k1, c = k2;
    float dx1, dy1, dx2, dy2, det;
    if (corner == 1)
        mine = k1;
    else if (corner == 2)
        mine = k2;
    else if (corner == 3) {
        mine = k3;
        a = k2;
        c = k3;
    }
    if (no_area(a, b, c, dx1, dy1, dx2, dy2, det)) {
        if (corner == 0 || corner == 3 || threeCorners)
            return mine;
        // the first triangle has no area: the second moves 1 and 2
        a = k2;
        c = k3;
        if (no_area(a, b, c, dx1, dy1, dx2, dy2, det))
            return mine;
    }
    float sx = pixel_boundary(mine.x), sy = pixel_boundary(mine.y);
    if (sx == mine.x && sy == mine.y)
        return mine;
    float ox = sx - a.x, oy = sy - a.y;
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
// coordinates.
struct QuadVertex
{
    float x, y, z, rhw;
    float4 colour;
    float4 uv;
};

QuadVertex quad_vertex(uint at, uint corner)
{
    uint4 h = quads[at];
    bool threeCorners = (h.x & QUAD_TRIANGLE) != 0;
    uint c = threeCorners && corner == 3 ? 2 : corner;
    Upright u = upright_of(h);
    Corner v, k[4];
    // the corners the vertex needs, its own or, snapped, every corner of its quad: one loop, so that
    // the shader holds one copy of corner_of
    bool snap = (h.x & QUAD_SNAP) != 0;
    uint last = snap ? (threeCorners ? 2 : 3) : c;
    [loop] for (uint i = snap ? 0 : c; i <= last; i++) {
        Corner one = corner_of(at, h, i, u);
        if (i == 0)
            k[0] = one;
        else if (i == 1)
            k[1] = one;
        else if (i == 2)
            k[2] = one;
        else
            k[3] = one;
    }
    if (snap) {
        if (threeCorners)
            k[3] = k[2];
        v = snapped(k[0], k[1], k[2], k[3], c, threeCorners);
    } else if (c == 0) {
        v = k[0];
    } else if (c == 1) {
        v = k[1];
    } else if (c == 2) {
        v = k[2];
    } else {
        v = k[3];
    }
    QuadVertex o;
    o.x = v.x;
    o.y = v.y;
    o.z = v.z;
    o.rhw = v.rhw;
    o.colour = v.bytes.zyxw / 255.0;
    o.uv = float4(v.u0, v.v0, v.u1, v.v1);
    return o;
}
