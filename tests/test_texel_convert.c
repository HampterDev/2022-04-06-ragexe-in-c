/* render/texel_convert.c: every 16-bit texel of each format widens by bit replication and packs back
   to itself; 8888 rows pass unchanged; the system surfaces' colour key turns transparent. */
#include "render/texel_convert.h"
#include "render/gfx.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static DWORD replicate(DWORD v, int bits)
{
    return bits == 4 ? v * 17 : bits == 5 ? (v << 3 | v >> 2) : (v << 2 | v >> 4);
}

static void check_format(int fmt)
{
    DWORD v;
    for (v = 0; v < 0x10000; v++) {
        DWORD argb = texel_unpack(fmt, v), want, a, r, g, b, back;
        unsigned char row[4];
        WORD in = (WORD)v;
        if (fmt == GFX_FMT_565) {
            a = 255;
            r = replicate(v >> 11, 5);
            g = replicate(v >> 5 & 63, 6);
            b = replicate(v & 31, 5);
        } else if (fmt == GFX_FMT_1555) {
            a = v & 0x8000 ? 255 : 0;
            r = replicate(v >> 10 & 31, 5);
            g = replicate(v >> 5 & 31, 5);
            b = replicate(v & 31, 5);
        } else {
            a = replicate(v >> 12, 4);
            r = replicate(v >> 8 & 15, 4);
            g = replicate(v >> 4 & 15, 4);
            b = replicate(v & 15, 4);
        }
        want = a << 24 | r << 16 | g << 8 | b;
        assert(argb == want);
        back = texel_pack(fmt, argb);
        assert(back == v);
        texel_rows_to_bgra(fmt, &in, 2, 1, 1, row, 4);
        assert(row[0] == b && row[1] == g && row[2] == r && row[3] == a);
    }
    assert(texel_bytes(fmt) == 2);
}

int main(void)
{
    DWORD rows[2][3] = { { 0x80ff0000u, 0x0000ff00u, 0xff0000ffu }, { 0x00ff00ffu, 0x12ff00ffu, 0x00123456u } };
    DWORD out[2][4];
    check_format(GFX_FMT_565);
    check_format(GFX_FMT_1555);
    check_format(GFX_FMT_4444);
    assert(texel_bytes(GFX_FMT_8888) == 4);
    assert(texel_unpack(GFX_FMT_8888, 0x12345678u) == 0x12345678u && texel_pack(GFX_FMT_8888, 0x12345678u) == 0x12345678u);

    /* rows a pitch apart, a rectangle narrower than the rows */
    memset(out, 0xee, sizeof out);
    texel_rows_to_bgra(GFX_FMT_8888, rows, sizeof rows[0], 2, 2, out, sizeof out[0]);
    assert(out[0][0] == 0x80ff0000u && out[0][1] == 0x0000ff00u && out[0][2] == 0xeeeeeeeeu);
    assert(out[1][0] == 0x00ff00ffu && out[1][1] == 0x12ff00ffu);

    /* the key's texels transparent with their colour kept, the others opaque */
    texel_rows_keyed_to_bgra(rows, sizeof rows[0], 3, 2, 0x00ff00ffu, out, sizeof out[0]);
    assert(out[0][0] == 0xffff0000u && out[0][1] == 0xff00ff00u && out[0][2] == 0xff0000ffu);
    assert(out[1][0] == 0x00ff00ffu && out[1][1] == 0xffff00ffu && out[1][2] == 0xff123456u);
    puts("texel conversion: 565, 1555 and 4444 widened and packed back for every texel; 8888 rows and keyed rows");
    return 0;
}
