#ifndef CAPTCHA_TEST_BITMAP_H
#define CAPTCHA_TEST_BITMAP_H
static void put16(unsigned char *p, unsigned value) { p[0] = (unsigned char)value; p[1] = (unsigned char)(value >> 8); }
static void put32(unsigned char *p, unsigned value) { put16(p, value); put16(p + 2, value >> 16); }

static unsigned char *make_bitmap(int custom, int bits, unsigned palette_count, size_t *size)
{
    size_t offset, i;
    unsigned char *data;
    unsigned count = bits == 8 ? (palette_count ? palette_count : 256) : 0;
    offset = custom ? (bits == 24 ? 17 : 21 + count * 4) : 54 + count * 4;
    *size = offset + CAPTCHA_WIDTH * CAPTCHA_HEIGHT * (bits / 8);
    data = calloc(1, *size); assert(data);
    if (custom) {
        memcpy(data, BMP_CLIENT_MAGIC, 12);
        put32(data + 12, (((unsigned)CAPTCHA_WIDTH << 16 | (CAPTCHA_HEIGHT + 1)) + 0x1929300u) << 1);
        data[16] = bits == 24 ? 0 : 1;
        if (bits == 8) put32(data + 17, palette_count);
    } else {
        put16(data, 0x4d42); put32(data + 2, (unsigned)*size); put32(data + 10, (unsigned)offset);
        put32(data + 14, 40); put32(data + 18, CAPTCHA_WIDTH); put32(data + 22, CAPTCHA_HEIGHT);
        put16(data + 26, 1); put16(data + 28, bits); put32(data + 46, palette_count);
    }
    for (i = offset; i < *size; ++i) data[i] = (unsigned char)(i % (count ? count : 256));
    return data;
}

#endif
