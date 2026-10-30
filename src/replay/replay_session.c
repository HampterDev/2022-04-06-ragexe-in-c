#include "replay/replay.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1)
struct ReplayTlvHeader { short tag; DWORD length; };
#pragma pack(pop)

struct SessionField { short tag; size_t offset, size; };
#define FIELD(tag, member) {tag, offsetof(struct ReplaySessionInfo, member), sizeof(((struct ReplaySessionInfo *)0)->member)}
static const struct SessionField session_fields[] = {
    FIELD(REPLAY_TAG_SERVICE_TYPE, serviceType),
    FIELD(REPLAY_TAG_SERVER_TYPE, serverType),
    FIELD(REPLAY_TAG_SEX, sex),
    FIELD(REPLAY_TAG_NAME, name),
    FIELD(REPLAY_TAG_MAP_DATA, mapData),
    FIELD(REPLAY_TAG_MAP_TYPE, mapType),
    FIELD(REPLAY_TAG_PLAYER_X, playerX),
    FIELD(REPLAY_TAG_PLAYER_Y, playerY),
    FIELD(REPLAY_TAG_PLAYER_DIRECTION, playerDirection),
    FIELD(REPLAY_TAG_TOTAL_TIME, totalTime),
    FIELD(REPLAY_TAG_RECORD_COUNT, recordCount),
    FIELD(REPLAY_TAG_DATA_END, dataEnd),
    FIELD(REPLAY_TAG_OPTIONS, options),
    FIELD(REPLAY_TAG_MAGIC, magicCopy)
};
#undef FIELD

int replay_build_session_chunk(const struct ReplaySessionInfo *info, struct ReplayBuffer *out)
{
    struct ReplayBuffer result = {0};
    struct ReplayTlvHeader header = {REPLAY_TAG_BEGIN, 0};
    size_t i, pos;
    result.size = (2 + sizeof session_fields / sizeof session_fields[0]) * sizeof header;
    for (i = 0; i < sizeof session_fields / sizeof session_fields[0]; ++i)
        result.size += session_fields[i].size;
    result.data = malloc(result.size);
    if (!result.data)
        return 0;
    memcpy(result.data, &header, sizeof header);
    pos = sizeof header;
    for (i = 0; i < sizeof session_fields / sizeof session_fields[0]; ++i) {
        const struct SessionField *field = &session_fields[i];
        header.tag = field->tag;
        header.length = (DWORD)field->size;
        memcpy(result.data + pos, &header, sizeof header);
        pos += sizeof header;
        memcpy(result.data + pos, (const unsigned char *)info + field->offset, field->size);
        pos += field->size;
    }
    header.tag = REPLAY_TAG_END;
    header.length = 0;
    memcpy(result.data + pos, &header, sizeof header);
    replay_buffer_destroy(out);
    *out = result;
    return 1;
}

int replay_parse_session_chunk(struct ReplaySessionInfo *info, const void *data, size_t size)
{
    struct ReplaySessionInfo result = *info;
    const unsigned char *bytes = data;
    struct ReplayTlvHeader header;
    size_t pos, i;
    unsigned seen = 0;
    if (!data || size < sizeof header || size > INT_MAX)
        return -1;
    memcpy(&header, bytes, sizeof header);
    if (header.tag != REPLAY_TAG_BEGIN || header.length)
        return -1;
    pos = sizeof header;
    while (size - pos >= sizeof header) {
        memcpy(&header, bytes + pos, sizeof header);
        pos += sizeof header;
        if (header.length > size - pos)
            return -1;
        if (header.tag == REPLAY_TAG_END) {
            if (header.length || pos != size || seen != (1u << (sizeof session_fields / sizeof session_fields[0])) - 1)
                return -1;
            *info = result;
            return (int)pos;
        }
        if (header.tag == REPLAY_TAG_BEGIN)
            return -1;
        for (i = 0; i < sizeof session_fields / sizeof session_fields[0]; ++i) {
            const struct SessionField *field = &session_fields[i];
            if (header.tag != field->tag)
                continue;
            if (header.length != field->size)
                return -1;
            seen |= 1u << i;
            memcpy((unsigned char *)&result + field->offset, bytes + pos, field->size);
            break;
        }
        pos += header.length;
    }
    return -1;
}
