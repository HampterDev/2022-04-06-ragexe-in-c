#include "resources/bitmap_internal.h"
#include <gif_lib.h>
#include <stdlib.h>
#include <string.h>
#include "platform/clock.h"

struct GifInput { const BYTE *next; size_t remaining; };

static int gif_read_memory(GifFileType *gif, GifByteType *out, int count)
{
    struct GifInput *input = gif->UserData;
    if (count < 0 || (size_t)count > input->remaining)
        return 0;
    memcpy(out, input->next, count);
    input->next += count;
    input->remaining -= count;
    return count;
}

static DWORD gif_color(GifColorType color)
{ return 0xff000000u | (DWORD)color.Red << 16 | (DWORD)color.Green << 8 | color.Blue; }

static void gif_control_reset(GraphicsControlBlock *control)
{
    memset(control, 0, sizeof *control);
    control->TransparentColor = NO_TRANSPARENT_COLOR;
}

static BYTE gif_extension(GifFileType *gif, GraphicsControlBlock *control, unsigned short *loops)
{
    GifByteType *block;
    int label, application = 0, first = 1;
    if (DGifGetExtension(gif, &label, &block) == GIF_ERROR) return 0;
    if (label == GRAPHICS_EXT_FUNC_CODE) {
        if (!block || block[0] != 4 ||
            DGifExtensionToGCB(block[0], block + 1, control) == GIF_ERROR ||
            control->DisposalMode > DISPOSE_PREVIOUS)
            return 0;
    }
    if (label == APPLICATION_EXT_FUNC_CODE && block && block[0] == 11)
        application = !memcmp(block + 1, "NETSCAPE2.0", 11) || !memcmp(block + 1, "ANIMEXTS1.0", 11);
    while (block) {
        if (application && !first && block[0] == 3 && block[1] == 1) {
            unsigned repeat = (unsigned)block[2] | (unsigned)block[3] << 8;
            *loops = (unsigned short)(repeat ? repeat + 1 : 0);
        }
        first = 0;
        if (DGifGetExtensionNext(gif, &block) == GIF_ERROR) return 0;
        if (label == GRAPHICS_EXT_FUNC_CODE && block) return 0;
    }
    if (label == PLAINTEXT_EXT_FUNC_CODE) gif_control_reset(control);
    return 1;
}

static BYTE gif_read_row(GifFileType *gif, const GraphicsControlBlock *control,
                         GifByteType *row, DWORD *pixels, int y)
{
    const GifImageDesc *image = &gif->Image;
    const ColorMapObject *palette = image->ColorMap ? image->ColorMap : gif->SColorMap;
    DWORD *out = pixels + (size_t)(image->Top + y) * gif->SWidth + image->Left;
    int x;
    if (!palette || (control->TransparentColor >= palette->ColorCount) ||
        DGifGetLine(gif, row, image->Width) == GIF_ERROR)
        return 0;
    for (x = 0; x < image->Width; ++x) {
        unsigned index = row[x];
        if (index >= (unsigned)palette->ColorCount) return 0;
        if ((int)index != control->TransparentColor)
            out[x] = gif_color(palette->Colors[index]);
    }
    return 1;
}

BYTE gif_res_load_gif_data(struct GifRes *self, const BYTE *data, int size, BYTE need24)
{
    struct GifInput input;
    struct GifFrames frames = {0};
    GraphicsControlBlock control;
    GifFileType *gif;
    GifRecordType record;
    DWORD *canvas = NULL, *pixels = NULL, background = 0;
    GifByteType *row = NULL;
    size_t count, bytes, i;
    unsigned short loops = 0;
    int width, height, error, complete = 0;
    if (!self || !data || size < 13 ||
        (memcmp(data, "GIF87a", 6) && memcmp(data, "GIF89a", 6))) return 0;
    input.next = data;
    input.remaining = size;
    gif = DGifOpen(&input, gif_read_memory, &error);
    if (!gif) return 0;
    width = gif->SWidth;
    height = gif->SHeight;
    count = bitmap_pixel_count(width, height);
    if (!count || (need24 && (width != 24 || height != 24))) goto done;
    bytes = count * sizeof(DWORD);
    if (gif->SColorMap) {
        if (gif->SBackGroundColor >= gif->SColorMap->ColorCount) goto done;
        background = gif_color(gif->SColorMap->Colors[gif->SBackGroundColor]);
    }
    canvas = bitmap_allocate_pixels(width, height);
    row = malloc(width);
    if (!canvas || !row) goto done;
    gif_control_reset(&control);
    while (DGifGetRecordType(gif, &record) != GIF_ERROR) {
        if (record == TERMINATE_RECORD_TYPE) { complete = frames.count != 0; break; }
        if (record == EXTENSION_RECORD_TYPE) {
            if (!gif_extension(gif, &control, &loops)) goto done;
        } else if (record == IMAGE_DESC_RECORD_TYPE) {
            struct GifFrame frame = {0};
            const GifImageDesc *image;
            int y, pass;
            static const int starts[] = {0, 4, 2, 1}, steps[] = {8, 8, 4, 2};
            if (frames.count >= BITMAP_MAX_BYTES / bytes ||
                DGifGetImageHeader(gif) == GIF_ERROR) goto done;
            image = &gif->Image;
            if (image->Width <= 0 || image->Height <= 0 || image->Left < 0 || image->Top < 0 ||
                image->Width > width - image->Left || image->Height > height - image->Top) goto done;
            if (!frames.count)
                for (i = 0; i < count; ++i)
                    canvas[i] = control.TransparentColor == NO_TRANSPARENT_COLOR ? background : 0;
            pixels = bitmap_allocate_pixels(width, height);
            if (!pixels) goto done;
            memcpy(pixels, canvas, bytes);
            for (pass = 0; pass < (image->Interlace ? 4 : 1); ++pass)
                for (y = image->Interlace ? starts[pass] : 0; y < image->Height;
                     y += image->Interlace ? steps[pass] : 1)
                    if (!gif_read_row(gif, &control, row, pixels, y)) goto done;
            frame.pixels = pixels;
            frame.delay = (unsigned short)(control.DelayTime < 10 ? 10 : control.DelayTime);
            if (!gif_frames_append(&frames, frame)) goto done;
            pixels = NULL;
            if (control.DisposalMode != DISPOSE_PREVIOUS) {
                memcpy(canvas, frame.pixels, bytes);
                if (control.DisposalMode == DISPOSE_BACKGROUND) {
                    DWORD fill = control.TransparentColor == NO_TRANSPARENT_COLOR ? background : 0;
                    int x;
                    for (y = 0; y < image->Height; ++y)
                        for (x = 0; x < image->Width; ++x)
                            canvas[(size_t)(image->Top + y) * width + image->Left + x] = fill;
                }
            }
            gif_control_reset(&control);
        } else goto done;
    }
done:
    DGifCloseFile(gif, &error);
    free(row);
    free(canvas);
    free(pixels);
    if (!complete) {
        gif_frames_destroy(&frames);
        return 0;
    }
    gif_resource_clear(self);
    self->frames = frames;
    self->bmp.width = width;
    self->bmp.height = height;
    self->bmp.hasAlpha = 1;
    self->bmp.pixels = frames.data[0].pixels;
    self->loopCount = loops;
    self->lastTime = platform_ticks_ms();
    self->animated = 1;
    return 1;
}
