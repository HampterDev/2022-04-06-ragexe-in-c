#include "resources/wave.h"
#include "resources/resource_factories.h"
#include "resources/resource_ops.h"
#include "fileio/res_mgr.h"
#include "fileio/file.h"
#include "platform/miles_api.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct Res *wave_resource_create(void)
{
    struct Wave *wave = calloc(1, sizeof *wave);
    if (!wave)
        return NULL;
    res_construct(&wave->res);
    wave->res.ops = &wave_resource_ops;
    return &wave->res;
}

void wave_resource_reset(struct Wave *wave)
{
    void *data = wave->data;
    int miles_owned = wave->milesOwned;
    wave->data = NULL;
    wave->milesOwned = 0;
    if (data) {
        if (miles_owned)
            _AIL_mem_free_lock(data);
        else
            free(data);
    }
}

static uint32_t wave_u32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static int wave_riff_valid(const unsigned char *data, size_t size)
{
    size_t end, offset = 12;
    int format = 0, samples = 0;
    uint32_t length;
    if (size < 12 || memcmp(data, "RIFF", 4) || memcmp(data + 8, "WAVE", 4))
        return 0;
    length = wave_u32(data + 4);
    if (length < 4 || length > size - 8)
        return 0;
    end = (size_t)length + 8;
    while (offset < end) {
        const unsigned char *chunk = data + offset;
        if (end - offset < 8)
            return 0;
        length = wave_u32(chunk + 4);
        offset += 8;
        if (length > end - offset)
            return 0;
        if (!memcmp(chunk, "fmt ", 4)) {
            unsigned extra, tag;
            if (length < 16 || length == 17)
                return 0;
            tag = (unsigned)data[offset] | (unsigned)data[offset + 1] << 8;
            if (tag == 17 && length < 20)
                return 0;
            if (length >= 18) {
                extra = (unsigned)data[offset + 16] | (unsigned)data[offset + 17] << 8;
                if (extra > length - 18)
                    return 0;
            }
            format = 1;
        } else if (!memcmp(chunk, "data", 4)) {
            samples = length != 0;
        }
        offset += length;
        if ((length & 1) && offset < end)
            ++offset;
    }
    return format && samples;
}

static int wave_info_valid(const struct AilSoundInfo *info, const void *data, size_t size)
{
    uintptr_t base = (uintptr_t)data, start = (uintptr_t)info->data;
    uintptr_t initial = (uintptr_t)info->initial;
    if (start < base || start - base >= size || !info->dataLen || info->dataLen > size - (start - base))
        return 0;
    if (initial && (initial < base || initial - base >= size))
        return 0;
    if (!info->rate || info->bits <= 0 || info->channels <= 0 || !info->blockSize || !info->samples)
        return 0;
    return (uint64_t)info->samples * (unsigned)info->channels <= (uint64_t)info->dataLen * 2 &&
           (uint64_t)info->samples * (unsigned)info->channels * 2 + 44 <= UINT32_MAX;
}

static int wave_resource_load(struct Res *resource, const char *name)
{
    struct Wave *wave = (struct Wave *)resource;
    struct File file;
    unsigned char *encoded = NULL;
    void *decoded = NULL;
    unsigned decoded_size = 0;
    int kind, ok = 0, miles_owned = 0;
    if (!name)
        return 0;
    file_construct(&file);
    if (!file_open(&file, name, CFILE_MODE_READ, 1, 0) || file.size < 12 || !file.buffer)
        goto done;
    encoded = malloc((size_t)file.size);
    if (!encoded || !file_read(&file, encoded, file.size))
        goto done;
    if (!memcmp(encoded, "RIFF", 4) && !wave_riff_valid(encoded, (size_t)file.size))
        goto done;
    kind = _AIL_file_type(encoded, (unsigned)file.size);
    switch (kind) {
    case AIL_FILE_ADPCM: {
        struct AilSoundInfo info = {0};
        miles_owned = 1;
        if (!wave_riff_valid(encoded, (size_t)file.size) || !_AIL_WAV_info(encoded, &info) ||
            !wave_info_valid(&info, encoded, (size_t)file.size) ||
            !_AIL_decompress_ADPCM(&info, &decoded, &decoded_size))
            goto done;
        break;
    }
    case AIL_FILE_ASI:
        miles_owned = 1;
        if (!_AIL_decompress_ASI(encoded, (unsigned)file.size, name, &decoded, &decoded_size, NULL))
            goto done;
        break;
    default:
        decoded = encoded;
        decoded_size = (unsigned)file.size;
        encoded = NULL;
        break;
    }
    if (!decoded || !decoded_size)
        goto done;
    wave_resource_reset(wave);
    wave->data = decoded;
    wave->milesOwned = miles_owned;
    decoded = NULL;
    ok = 1;
done:
    if (decoded) {
        if (miles_owned)
            _AIL_mem_free_lock(decoded);
        else
            free(decoded);
    }
    free(encoded);
    file_destruct(&file);
    return ok;
}

static void wave_resource_destroy(struct Res *resource)
{
    wave_resource_reset((struct Wave *)resource);
    free(resource);
}

const struct ResourceOps wave_resource_ops = {
    .destroy = wave_resource_destroy,
    .load = wave_resource_load,
};
