#include "navi/snapshot.h"
#include "navi/navi_manager.h"
#include "navi/navi_search.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1)
struct ResultRecord {
    int32_t kind;
    char map[20];
    int32_t x, y, name_size;
};
#pragma pack(pop)
SIZE_ASSERT(ResultRecord, 36);

int navi_results_save(const struct NaviManager *self, struct ReplayBuffer *out)
{
    struct ReplayBuffer buffer = {0};
    const struct SearchResult *result;
    unsigned char *position;
    size_t size = 4;
    int32_t count = 0;
    if (!self || !out) return 0;
    if (self->results.first == self->results.last) {
        replay_buffer_destroy(out);
        return 1;
    }
    for (result = self->results.first; result != self->results.last; ++result) {
        const struct NaviMap *map = result->obj;
        if (!map) continue;
        if ((result->kind != NAVI_RESULT_PLACE && result->kind != NAVI_RESULT_MAP) ||
            map->name.size > sizeof ((struct ResultRecord *)0)->map ||
            size > INT_MAX - sizeof(struct ResultRecord) ||
            result->name.size > INT_MAX - size - sizeof(struct ResultRecord)) return 0;
        size += sizeof(struct ResultRecord) + result->name.size;
        ++count;
    }
    buffer.data = malloc(size);
    if (!buffer.data) return 0;
    buffer.size = size;
    memcpy(buffer.data, &count, sizeof count);
    position = buffer.data + sizeof count;
    for (result = self->results.first; result != self->results.last; ++result) {
        const struct NaviMap *map = result->obj;
        struct ResultRecord record = {0};
        if (!map) continue;
        record.kind = result->kind;
        memcpy(record.map, string_cstr(&map->name), map->name.size);
        record.x = result->x; record.y = result->y;
        record.name_size = (int32_t)result->name.size;
        memcpy(position, &record, sizeof record);
        position += sizeof record;
        if (record.name_size) memcpy(position, string_cstr(&result->name), (size_t)record.name_size);
        position += record.name_size;
    }
    replay_buffer_destroy(out);
    *out = buffer;
    return 1;
}

static int results_valid(const void *data, size_t size)
{
    const unsigned char *bytes = data;
    size_t offset = 4;
    int32_t count, i;
    if (!data || size < 4 || size > INT_MAX) return 0;
    memcpy(&count, data, sizeof count);
    if (count < 0 || (size_t)count > (size - 4) / sizeof(struct ResultRecord)) return 0;
    for (i = 0; i < count; ++i) {
        struct ResultRecord record;
        if (size - offset < sizeof record) return 0;
        memcpy(&record, bytes + offset, sizeof record);
        offset += sizeof record;
        if ((record.kind != NAVI_RESULT_PLACE && record.kind != NAVI_RESULT_MAP) ||
            record.name_size < 0 || (size_t)record.name_size > size - offset) return 0;
        offset += (size_t)record.name_size;
    }
    return offset == size;
}

int navi_results_load(struct NaviManager *self, const void *data, size_t size)
{
    const unsigned char *position = data;
    int32_t count, i;
    if (!self || !results_valid(data, size)) return 0;
    memcpy(&count, position, sizeof count);
    position += sizeof count;
    search_results_clear(&self->results);
    for (i = 0; i < count; ++i) {
        struct ResultRecord record;
        struct NaviMap *map;
        const char *end;
        size_t map_size;
        memcpy(&record, position, sizeof record);
        position += sizeof record;
        end = memchr(record.map, 0, sizeof record.map);
        map_size = end ? (size_t)(end - record.map) : sizeof record.map;
        map = navi_map_by_name(self, record.map, map_size);
        if (map) {
            struct SearchResult result;
            search_result_init(&result, record.kind);
            result.obj = map;
            result.x = record.x; result.y = record.y;
            string_assign(&result.name, (const char *)position, (unsigned)record.name_size);
            search_results_push_copy(&self->results, &result);
            search_result_destroy(&result);
        }
        position += record.name_size;
    }
    return 1;
}
