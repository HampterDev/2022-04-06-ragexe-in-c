#include "navi/snapshot_records.h"
#include "navi/navi_manager.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct ObjectRecord {
    int32_t type, sprite, x, y;
    struct SnapshotSpan name, alias, map;
    int32_t destination_x, destination_y;
    struct SnapshotSpan destination;
};

struct RecordCursor { struct SnapshotSpan input; size_t offset; };

static int read_bytes(struct RecordCursor *cursor, void *out, size_t size)
{
    if (size > cursor->input.size - cursor->offset) return 0;
    memcpy(out, cursor->input.data + cursor->offset, size);
    cursor->offset += size;
    return 1;
}

static int read_text(struct RecordCursor *cursor, struct SnapshotSpan *text)
{
    int16_t size;
    if (!read_bytes(cursor, &size, sizeof size) || size < 0 ||
        (size_t)size > cursor->input.size - cursor->offset) return 0;
    text->data = cursor->input.data + cursor->offset;
    text->size = (size_t)size;
    cursor->offset += (size_t)size;
    return 1;
}

static int read_object(struct SnapshotSpan input, int link, struct ObjectRecord *record)
{
    struct RecordCursor cursor = {input, 0};
    if (!input.data || input.size > INT_MAX ||
        !read_bytes(&cursor, &record->type, 4) ||
        !read_bytes(&cursor, &record->sprite, 4) ||
        !read_bytes(&cursor, &record->x, 4) ||
        !read_bytes(&cursor, &record->y, 4) ||
        !read_text(&cursor, &record->name) ||
        !read_text(&cursor, &record->alias) ||
        !read_text(&cursor, &record->map)) return 0;
    if (link && (!read_bytes(&cursor, &record->destination_x, 4) ||
                 !read_bytes(&cursor, &record->destination_y, 4) ||
                 !read_text(&cursor, &record->destination))) return 0;
    return cursor.offset == input.size;
}

static size_t text_length(struct SnapshotSpan text)
{
    const unsigned char *end = memchr(text.data, 0, text.size);
    return end ? (size_t)(end - text.data) : text.size;
}

static struct NaviObject *resolve_object(struct NaviManager *manager, const struct ObjectRecord *record)
{
    struct NaviMap *map;
    struct NaviObject *object;
    const char *name;
    size_t size;
    if (!(record->type >= 100 && record->type <= 198) &&
        !(record->type >= 300 && record->type <= 398)) return NULL;
    map = navi_map_by_name(manager, (const char *)record->map.data, text_length(record->map));
    if (!map) return NULL;
    size = text_length(record->name);
    if (record->type < 100 || record->type > 104)
        return navi_map_mob_by_name(map, (const char *)record->name.data, size);
    object = navi_map_npc_at(map, record->x, record->y);
    if (!object) return NULL;
    name = navi_object_name(object);
    return strlen(name) == size && !memcmp(name, record->name.data, size) ? object : NULL;
}

int navi_link_record_valid(struct SnapshotSpan record)
{
    struct ObjectRecord object;
    return !record.size || read_object(record, 1, &object);
}

int navi_result_record_valid(struct SnapshotSpan record)
{
    int32_t kind;
    struct ObjectRecord object;
    if (!record.data || record.size < 4 || record.size > INT_MAX) return 0;
    memcpy(&kind, record.data, 4);
    record.data += 4;
    record.size -= 4;
    if (kind == NAVI_RESULT_PLACE || kind == NAVI_RESULT_MAP) return 1;
    if (kind == NAVI_RESULT_NPC || kind == NAVI_RESULT_MOB)
        return read_object(record, 0, &object);
    return record.size == 0;
}

int navi_link_record_load(struct NaviManager *manager, struct SnapshotSpan record, struct NaviLink **out)
{
    struct ObjectRecord object;
    struct NaviMap *map, *destination;
    if (!manager || !out || !navi_link_record_valid(record)) return 0;
    *out = NULL;
    if (!record.size) return 1;
    read_object(record, 1, &object);
    if (object.type < 199 || object.type > 299) return 1;
    map = navi_map_by_name(manager, (const char *)object.map.data, text_length(object.map));
    destination = navi_map_by_name(manager, (const char *)object.destination.data, text_length(object.destination));
    if (map && destination)
        *out = navi_map_find_link_route(map, object.x, object.y, destination, object.destination_x, object.destination_y);
    return 1;
}

int navi_result_record_load(struct NaviManager *manager, struct SnapshotSpan record, struct SearchResult *out)
{
    int32_t kind;
    struct ObjectRecord object;
    if (!manager || !out || !navi_result_record_valid(record)) return 0;
    memcpy(&kind, record.data, 4);
    record.data += 4;
    record.size -= 4;
    search_result_init(out, kind);
    if (kind == NAVI_RESULT_PLACE || kind == NAVI_RESULT_MAP) {
        out->obj = navi_map_by_name(manager, (const char *)record.data, text_length(record));
        if (!out->obj) out->kind = NAVI_RESULT_PLACE;
    } else if (kind == NAVI_RESULT_NPC || kind == NAVI_RESULT_MOB) {
        read_object(record, 0, &object);
        out->obj = resolve_object(manager, &object);
    }
    return 1;
}

static void put_bytes(unsigned char **position, const void *bytes, size_t size)
{
    if (size) memcpy(*position, bytes, size);
    *position += size;
}

static void put_text(unsigned char **position, const char *text, size_t size)
{
    int16_t length = (int16_t)size;
    put_bytes(position, &length, sizeof length);
    put_bytes(position, text, size);
}

static int save_object(struct NaviObject *object, struct NaviLink *link, const int32_t *kind, struct ReplayBuffer *out)
{
    struct ReplayBuffer buffer = {0};
    const char *name, *alias, *map, *destination = NULL;
    size_t name_size, alias_size, map_size, destination_size = 0;
    struct NaviPoint position, end;
    struct NaviMap *owner;
    int32_t fields[4];
    unsigned char *p;
    if (!object) { replay_buffer_destroy(out); return 1; }
    owner = navi_object_map(object);
    if (!owner || (link && !link->destMap)) return 0;
    name = navi_object_name(object);
    alias = navi_object_alias(object);
    map = navi_map_name(owner);
    name_size = strlen(name); alias_size = strlen(alias); map_size = strlen(map);
    if (link) {
        destination = navi_link_dest_map_name(link);
        destination_size = strlen(destination);
        navi_link_dest_pos(link, &end);
    }
    if (name_size > INT16_MAX || alias_size > INT16_MAX || map_size > INT16_MAX || destination_size > INT16_MAX) return 0;
    buffer.size = (link ? 32 : 22) + name_size + alias_size + map_size + destination_size + (kind ? 4 : 0);
    buffer.data = malloc(buffer.size);
    if (!buffer.data) return 0;
    p = buffer.data;
    if (kind) put_bytes(&p, kind, 4);
    navi_object_position(object, &position);
    fields[0] = navi_object_type(object); fields[1] = navi_object_sprite(object);
    fields[2] = position.x; fields[3] = position.y;
    put_bytes(&p, fields, sizeof fields);
    put_text(&p, name, name_size); put_text(&p, alias, alias_size); put_text(&p, map, map_size);
    if (link) {
        int32_t coordinates[2] = {end.x, end.y};
        put_bytes(&p, coordinates, sizeof coordinates);
        put_text(&p, destination, destination_size);
    }
    replay_buffer_destroy(out);
    *out = buffer;
    return 1;
}

int navi_link_record_save(struct NaviLink *link, struct ReplayBuffer *out)
{
    return out && save_object(link ? &link->base : NULL, link, NULL, out);
}

int navi_result_record_save(const struct SearchResult *result, struct ReplayBuffer *out)
{
    struct ReplayBuffer buffer = {0};
    const char *name = NULL;
    size_t length = 0;
    int32_t kind;
    if (!result || !out) return 0;
    if (!result->obj) { replay_buffer_destroy(out); return 1; }
    kind = result->kind;
    if (kind == NAVI_RESULT_NPC || kind == NAVI_RESULT_MOB)
        return save_object(result->obj, NULL, &kind, out);
    if (kind == NAVI_RESULT_PLACE || kind == NAVI_RESULT_MAP) {
        name = navi_map_name(result->obj);
        length = strlen(name);
        if (length > INT_MAX - 4) return 0;
    }
    buffer.size = 4 + length;
    buffer.data = malloc(buffer.size);
    if (!buffer.data) return 0;
    memcpy(buffer.data, &kind, 4);
    if (length) memcpy(buffer.data + 4, name, length);
    replay_buffer_destroy(out);
    *out = buffer;
    return 1;
}
