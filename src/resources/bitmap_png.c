#include "resources/bitmap_internal.h"
#include <png.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

struct PngDecode {
    png_structp png;
    png_infop info;
    const BYTE *next;
    size_t remaining;
    BYTE *rgba;
    png_bytep *rows;
    DWORD *pixels;
};

static void png_decode_destroy(struct PngDecode *decode)
{
    if (decode->png)
        png_destroy_read_struct(&decode->png, &decode->info, NULL);
    free(decode->rows);
    free(decode->rgba);
    free(decode->pixels);
    free(decode);
}

static void png_decode_error(png_structp png, png_const_charp message)
{
    (void)message;
    png_longjmp(png, 1);
}

static void png_read_memory(png_structp png, png_bytep out, png_size_t count)
{
    struct PngDecode *decode = png_get_io_ptr(png);

    if (count > decode->remaining)
        png_error(png, "Truncated PNG input");
    memcpy(out, decode->next, count);
    decode->next += count;
    decode->remaining -= count;
}

BYTE bitmap_res_load_png_data(struct BitmapRes *self, const BYTE *data, int size)
{
    struct PngDecode *decode;
    png_uint_32 width, height, x, y;
    size_t row_bytes, pixel_count;
    int color_type, bit_depth, transparency;

    if (!self || !data || size < 8 || png_sig_cmp(data, 0, 8))
        return 0;
    decode = calloc(1, sizeof *decode);
    if (!decode)
        return 0;
    decode->next = data;
    decode->remaining = (size_t)size;
    decode->png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, png_decode_error, NULL);
    if (!decode->png) {
        png_decode_destroy(decode);
        return 0;
    }
    if (setjmp(png_jmpbuf(decode->png))) {
        png_decode_destroy(decode);
        return 0;
    }
    decode->info = png_create_info_struct(decode->png);
    if (!decode->info)
        png_error(decode->png, "Cannot allocate PNG information");
    png_set_read_fn(decode->png, decode, png_read_memory);
    png_read_info(decode->png, decode->info);
    width = png_get_image_width(decode->png, decode->info);
    height = png_get_image_height(decode->png, decode->info);
    if (!width || !height || width > INT_MAX || height > INT_MAX ||
        !bitmap_pixel_count((int)width, (int)height))
        png_error(decode->png, "PNG dimensions exceed pixel storage");
    pixel_count = (size_t)width * height;
    color_type = png_get_color_type(decode->png, decode->info);
    bit_depth = png_get_bit_depth(decode->png, decode->info);
    transparency = png_get_valid(decode->png, decode->info, PNG_INFO_tRNS) != 0;
    png_set_interlace_handling(decode->png);
    if (bit_depth == 16)
        png_set_strip_16(decode->png);
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(decode->png);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(decode->png);
    if (transparency)
        png_set_tRNS_to_alpha(decode->png);
    if (!(color_type & PNG_COLOR_MASK_ALPHA) && !transparency)
        png_set_add_alpha(decode->png, 0xff, PNG_FILLER_AFTER);
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(decode->png);
    png_read_update_info(decode->png, decode->info);
    row_bytes = png_get_rowbytes(decode->png, decode->info);
    if (png_get_channels(decode->png, decode->info) != 4 ||
        png_get_bit_depth(decode->png, decode->info) != 8 ||
        row_bytes != (size_t)width * 4 || height > (size_t)-1 / sizeof *decode->rows)
        png_error(decode->png, "Unsupported PNG output layout");

    decode->rows = malloc((size_t)height * sizeof *decode->rows);
    decode->rgba = malloc(pixel_count * 4);
    if (!decode->rows || !decode->rgba)
        png_error(decode->png, "Cannot allocate PNG rows");
    for (y = 0; y < height; ++y)
        decode->rows[y] = decode->rgba + (size_t)y * row_bytes;
    png_read_image(decode->png, decode->rows);
    decode->pixels = malloc(pixel_count * sizeof *decode->pixels);
    if (!decode->pixels)
        png_error(decode->png, "Cannot allocate PNG pixels");
    for (y = 0; y < height; ++y) {
        const BYTE *source = decode->rows[y];
        DWORD *target = decode->pixels + (size_t)y * width;

        for (x = 0; x < width; ++x, source += 4)
            target[x] = (DWORD)source[3] << 24 | (DWORD)source[0] << 16 |
                        (DWORD)source[1] << 8 | source[2];
    }
    bitmap_replace_pixels(self, decode->pixels, (int)width, (int)height, 1);
    decode->pixels = NULL;
    png_decode_destroy(decode);
    return 1;
}
