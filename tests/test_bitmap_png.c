#include "resources/bitmap.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct PngFixture {
    const BYTE *data;
    size_t size;
    int width, height;
    const DWORD *pixels;
};
#include "png_fixtures.h"

static unsigned allocations, attempts;
static int fail_after = -1;

void *png_test_malloc(size_t size)
{
    void *p;
    if (fail_after >= 0 && attempts++ == (unsigned)fail_after)
        return NULL;
    p = malloc(size);
    assert(p);
    ++allocations;
    return p;
}

void *png_test_calloc(size_t count, size_t size)
{
    void *p = png_test_malloc(count * size);
    if (p)
        memset(p, 0, count * size);
    return p;
}

void png_test_free(void *p)
{
    if (p) {
        assert(allocations);
        --allocations;
        free(p);
    }
}

static void reject(const BYTE *data, int size)
{
    struct BitmapRes before = {0}, bitmap;
    DWORD old_pixels = 0x12345678;

    before.width = 9;
    before.height = 7;
    before.hasAlpha = 1;
    before.pixels = &old_pixels;
    bitmap = before;
    assert(!bitmap_res_load_png_data(&bitmap, data, size));
    assert(memcmp(&before, &bitmap, sizeof bitmap) == 0);
    assert(old_pixels == 0x12345678 && !allocations);
}

static void accept(const struct PngFixture *f, const BYTE *data, int size)
{
    struct BitmapRes bitmap = {0};

    assert(bitmap_res_load_png_data(&bitmap, data, size));
    assert(bitmap.width == f->width && bitmap.height == f->height);
    assert(memcmp(bitmap.pixels, f->pixels, f->width * f->height * sizeof(DWORD)) == 0);
    bitmap_resource_clear(&bitmap);
    assert(!allocations);
}

static size_t image_end(const struct PngFixture *f)
{
    size_t i;

    for (i = 8; i + 4 <= f->size; ++i)
        if (!memcmp(f->data + i, "IEND", 4))
            return i - 4;
    return f->size;
}

int main(void)
{
    SYSTEM_INFO info;
    BYTE *pages;
    DWORD old;
    size_t i, n, boundaries = 0;
    int fail;

    GetSystemInfo(&info);
    pages = VirtualAlloc(NULL, 2 * info.dwPageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(pages && VirtualProtect(pages + info.dwPageSize, info.dwPageSize, PAGE_NOACCESS, &old));
    reject(NULL, 8);
    reject(pages + info.dwPageSize, -1);
    for (i = 0; i < sizeof fixtures / sizeof fixtures[0]; ++i) {
        struct BitmapRes bitmap = {0};
        const struct PngFixture *f = &fixtures[i];
        BYTE *copy = malloc(f->size);
        assert(copy && f->size <= info.dwPageSize);
        assert(bitmap_res_load_png_data(&bitmap, f->data, (int)f->size));
        assert(bitmap.width == f->width && bitmap.height == f->height && bitmap.hasAlpha == 1);
        assert(memcmp(bitmap.pixels, f->pixels, f->width * f->height * sizeof(DWORD)) == 0);
        assert(allocations == 1);
        assert(bitmap_res_make_gray_copy(&bitmap, 0) && allocations == 2);
        assert(bitmap_res_load_png_data(&bitmap, f->data, (int)f->size));
        assert(!bitmap.grayPixels && allocations == 1);
        bitmap_resource_clear(&bitmap);

        for (n = 0; n < f->size; ++n) {
            BYTE *at = pages + info.dwPageSize - n;
            memcpy(at, f->data, n);
            if (n < image_end(f))
                reject(at, (int)n);
            else
                accept(f, at, (int)n);
            ++boundaries;
        }
        memcpy(copy, f->data, f->size);
        copy[29] ^= 0x80;
        reject(copy, (int)f->size);
        memcpy(copy, f->data, f->size);
        copy[f->size - 1] ^= 0x80;
        accept(f, copy, (int)f->size);
        free(copy);
    }
    reject(png_oversized, sizeof png_oversized);
    for (fail = 0; fail < 4; ++fail) {
        fail_after = fail;
        attempts = 0;
        reject(png_rgba8, sizeof png_rgba8);
    }
    fail_after = -1;
    VirtualFree(pages, 0, MEM_RELEASE);
    printf("PASS: 12 PNG formats, %u guarded truncations, CRC failures, oversized images and allocation cleanup\n",
           (unsigned)boundaries);
    return 0;
}
