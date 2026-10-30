#include "game/emblem_snapshot.h"
#include "game/gamemode.h"
#include "web/name_card_snapshot.h"
#include "contents/guild_emblem_api.h"
#include "contents/guild_info.h"
#include "actor/actor_ops.h"
#include "fileio/file_api.h"
#include "replay/replay.h"
#include "replay/snapshot.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/app.h"

enum
{
    EMBLEMS_BEGIN = 0x32c8,
    EMBLEMS_END = 0x32c9,
    EMBLEM_FILE = 0x32ca,
    ACTOR_NAMES = 0x32cb,
    PLAYER_GUILD = 0x32cc,
    GUILD_VERSION = 0x32cd
};

static uint32_t read_word(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void write_word(unsigned char *p, uint32_t value)
{
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
    p[2] = (unsigned char)(value >> 16);
    p[3] = (unsigned char)(value >> 24);
}

static int valid_name(const unsigned char *name, size_t size)
{
    size_t i, stem = 0;
    char upper[5] = {0};
    if (!size || name[size - 1] == '.' || name[size - 1] == ' ')
        return 0;
    for (i = 0; i < size; ++i)
        if (name[i] < 32 || strchr("<>:\"/\\|?*", name[i]))
            return 0;
    while (stem < size && name[stem] != '.')
        ++stem;
    if (stem == 3 || stem == 4)
    {
        for (i = 0; i < stem; ++i)
            upper[i] = (char)(name[i] >= 'a' && name[i] <= 'z' ? name[i] - 'a' + 'A' : name[i]);
        if (!strcmp(upper, "CON") || !strcmp(upper, "PRN") || !strcmp(upper, "AUX") ||
            !strcmp(upper, "NUL"))
            return 0;
        if (stem == 4 && upper[3] >= '1' && upper[3] <= '9' &&
            (!memcmp(upper, "COM", 3) || !memcmp(upper, "LPT", 3)))
            return 0;
    }
    return 1;
}
static char *cache_path(const unsigned char *name, size_t size)
{
    static const char folder[] = "\\_tmpEmblem\\";
    size_t root = strlen(g_clientDir);
    char *path;
    if (size > SIZE_MAX - sizeof folder || root > SIZE_MAX - size - sizeof folder)
        return NULL;
    path = malloc(root + sizeof folder + size);
    if (!path)
        return NULL;
    memcpy(path, g_clientDir, root);
    memcpy(path + root, folder, sizeof folder - 1);
    memcpy(path + root + sizeof folder - 1, name, size);
    path[root + sizeof folder - 1 + size] = 0;
    return path;
}
static int contains_file(const struct SnapshotWriter *writer, const struct String *name)
{
    struct SnapshotReader reader = {{writer->buffer.data, writer->buffer.size}, 0};
    struct SnapshotRecord record;
    while (snapshot_next(&reader, &record) == 1)
        if (record.tag == EMBLEM_FILE && record.value.size >= 4 &&
            read_word(record.value.data) == name->size && record.value.size - 4 >= name->size &&
            !memcmp(record.value.data + 4, string_cstr(name), name->size))
            return 1;
    return 0;
}

static void append_file(struct SnapshotWriter *writer, int guild)
{
    struct String name;
    struct File file;
    unsigned char *payload = NULL;
    char *path;
    size_t size;
    if (writer->failed || guild <= 0)
        return;
    guild_mgr_emblem_file_name(g_guildMgr, &name, (unsigned)guild);
    if (!name.size || contains_file(writer, &name))
    {
        string_free(&name);
        return;
    }
    if (!valid_name((const unsigned char *)string_cstr(&name), name.size))
    {
        writer->failed = 1;
        string_free(&name);
        return;
    }
    path = cache_path((const unsigned char *)string_cstr(&name), name.size);
    if (!path)
    {
        writer->failed = 1;
        string_free(&name);
        return;
    }
    file_construct(&file);
    if (file_open(&file, path, 0, 1, 0))
    {
        if (file.size < 0 || name.size > (size_t)INT_MAX - 4 ||
            (size_t)file.size > INT_MAX - 4 - name.size)
        {
            writer->failed = 1;
        }
        else
        {
            size = 4 + name.size + (size_t)file.size;
            payload = malloc(size);
            if (!payload)
                writer->failed = 1;
            else
            {
                write_word(payload, name.size);
                memcpy(payload + 4, string_cstr(&name), name.size);
                if (!file_read(&file, payload + 4 + name.size, file.size))
                    writer->failed = 1;
                else
                    snapshot_write(writer, EMBLEM_FILE, payload, size);
            }
        }
    }
    free(payload);
    file_destruct(&file);
    free(path);
    string_free(&name);
}

int game_emblems_save(struct GameMode *mode, struct ReplayBuffer *out)
{
    struct SnapshotWriter writer = {0};
    struct ReplayBuffer names = {0};
    struct ListNode *head, *node;
    struct GameActor *player;
    unsigned char word[4];
    if (!mode || !mode->world || !mode->world->player || !g_guildMgr || !out ||
        !(head = mode->world->actors.head))
        return 0;
    player = (struct GameActor *)mode->world->player;
    snapshot_write(&writer, EMBLEMS_BEGIN, NULL, 0);
    if (!name_cards_snapshot_save(&mode->actor_names, &names))
        writer.failed = 1;
    else if (names.size > 4)
        snapshot_write(&writer, ACTOR_NAMES, names.data, names.size);
    replay_buffer_destroy(&names);
    append_file(&writer, player->guild_id);
    write_word(word, (uint32_t)player->guild_id);
    snapshot_write(&writer, PLAYER_GUILD, word, sizeof word);
    write_word(word, (uint32_t)guild_mgr_emblem_version(g_guildMgr, (unsigned)player->guild_id));
    snapshot_write(&writer, GUILD_VERSION, word, sizeof word);
    for (node = head->next; node != head && !writer.failed; node = node->next)
    {
        struct GameActor *actor = *LIST_VALUE(node, struct GameActor *);
        if (!actor)
            writer.failed = 1;
        else
            append_file(&writer, actor->guild_id);
    }
    snapshot_write(&writer, EMBLEMS_END, NULL, 0);
    return snapshot_finish(&writer, out);
}

static int validate_emblems(const void *data, size_t size)
{
    struct SnapshotReader reader;
    struct SnapshotRecord record;
    if (!snapshot_open(&reader, data, size, EMBLEMS_BEGIN))
        return -1;
    while (snapshot_next(&reader, &record) == 1)
    {
        if (record.tag == EMBLEMS_END)
            return record.value.size ? -1 : (int)reader.offset;
        if (record.tag == EMBLEMS_BEGIN)
            return -1;
        if (record.tag == PLAYER_GUILD || record.tag == GUILD_VERSION)
        {
            if (record.value.size != 4)
                return -1;
        }
        else if (record.tag == ACTOR_NAMES)
        {
            if (!name_cards_snapshot_valid(record.value.data, record.value.size))
                return -1;
        }
        else if (record.tag == EMBLEM_FILE)
        {
            uint32_t length;
            if (record.value.size < 4)
                return -1;
            length = read_word(record.value.data);
            if (length > record.value.size - 4 || !valid_name(record.value.data + 4, length))
                return -1;
        }
    }
    return -1;
}

static int cache_file(struct SnapshotSpan value)
{
    size_t length = read_word(value.data);
    char *path = cache_path(value.data + 4, length);
    struct File file;
    if (!path)
        return 0;
    file_construct(&file);
    if (!file_open(&file, path, 0, 1, 0))
    {
        FILE *cache;
        file_close(&file);
        cache = fopen(path, "wb");
        if (cache)
        {
            fwrite(value.data + 4 + length, 1, value.size - 4 - length, cache);
            fclose(cache);
        }
    }
    file_destruct(&file);
    free(path);
    return 1;
}

int game_emblems_load(struct GameMode *mode, const void *data, size_t size)
{
    struct SnapshotReader reader;
    struct SnapshotRecord record;
    int guild = 0, used = validate_emblems(data, size);
    if (!mode || !mode->world || !mode->world->player || used < 0)
        return -1;
    snapshot_open(&reader, data, (size_t)used, EMBLEMS_BEGIN);
    while (snapshot_next(&reader, &record) == 1 && record.tag != EMBLEMS_END)
    {
        if (record.tag == EMBLEM_FILE)
        {
            if (!cache_file(record.value))
                return -1;
        }
        else if (record.tag == ACTOR_NAMES)
        {
            if (!name_cards_snapshot_load(&mode->actor_names, record.value.data, record.value.size))
                return -1;
        }
        else if (record.tag == PLAYER_GUILD)
            guild = (int32_t)read_word(record.value.data);
        else if (record.tag == GUILD_VERSION)
        {
            game_actor_set_guild((struct GameActor *)mode->world->player, guild,
                                (int32_t)read_word(record.value.data));
            g_guildInfo.guild_id = guild;
        }
    }
    return used;
}
int game_emblems_restore(struct GameMode *mode, struct ReplayRecorder *recorder)
{
    struct ReplayBuffer chunk = {0};
    int loaded;
    if (!mode || !mode->world || !mode->world->player || !recorder)
        return 0;
    loaded = replay_read_chunk(recorder, 0x15, &chunk) &&
             (!chunk.size || game_emblems_load(mode, chunk.data, chunk.size) >= 0);
    replay_buffer_destroy(&chunk);
    return loaded;
}
