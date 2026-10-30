#include "contents/quest_blob.h"
#include "contents/renew_quest.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum {
    QUEST_OPEN = 0xdac, QUEST_CLOSE = 0xdad, QUEST_PARAM = 0xdae,
    QUEST_LIST = 0xdb6, HEADER_SIZE = 6, SHORT_NAMES = 24, LONG_NAMES = 51
};

#pragma pack(push, 1)
struct SavedQuest {
    unsigned id;
    char active, zero;
    short hunt_count;
    struct QuestDeadline deadline;
    char zero_tail[6];
};
struct SavedHunt {
    int id, flags, monster;
    short min_level, max_level, done, goal;
    char name[LONG_NAMES];
};
#pragma pack(pop)
SIZE_ASSERT(SavedQuest, 24);
SIZE_ASSERT(SavedHunt, 71);

static size_t hunt_size(int names)
{
    return offsetof(struct SavedHunt, name) + (size_t)names;
}

static void write_header(unsigned char *out, unsigned short tag, int size)
{
    memcpy(out, &tag, sizeof tag);
    memcpy(out + sizeof tag, &size, sizeof size);
}

static int read_header(const unsigned char *data, size_t size, size_t *at,
                       unsigned short *tag, size_t *length)
{
    int bytes;
    if (size - *at < HEADER_SIZE)
        return 0;
    memcpy(tag, data + *at, sizeof *tag);
    memcpy(&bytes, data + *at + sizeof *tag, sizeof bytes);
    *at += HEADER_SIZE;
    if (bytes < 0 || (size_t)bytes > size - *at)
        return 0;
    *length = (size_t)bytes;
    return 1;
}

static int list_valid(const unsigned char *data, size_t size, int names)
{
    int count, i;
    size_t at = sizeof count;
    if (size < at)
        return 0;
    memcpy(&count, data, sizeof count);
    if (count < 0 || (size_t)count > (size - at) / sizeof(struct SavedQuest))
        return 0;
    for (i = 0; i < count; ++i) {
        struct SavedQuest quest;
        size_t bytes;
        if (size - at < sizeof quest)
            return 0;
        memcpy(&quest, data + at, sizeof quest);
        at += sizeof quest;
        if (quest.hunt_count < 0 || quest.hunt_count > RENEW_QUEST_HUNT_MAX)
            return 0;
        bytes = (size_t)quest.hunt_count * hunt_size(names);
        if (bytes > size - at)
            return 0;
        at += bytes;
    }
    return at == size;
}

static void copy_name(char out[LONG_NAMES], const char *name, size_t size)
{
    size_t n = 0;
    while (n < size && n < LONG_NAMES - 1 && name[n])
        ++n;
    memset(out, 0, LONG_NAMES);
    memcpy(out, name, n);
}

static void load_list(struct RenewQuestMgr *quests, const unsigned char *data, int names)
{
    int count, n;
    size_t at = sizeof count;
    memcpy(&count, data, sizeof count);
    for (n = 0; n < count; ++n) {
        struct SavedQuest saved;
        struct RenewQuestB quest;
        int i;
        memcpy(&saved, data + at, sizeof saved);
        at += sizeof saved;
        if (renew_quest_set_active(quests, saved.id, saved.active)) {
            at += (size_t)saved.hunt_count * hunt_size(names);
            continue;
        }
        quest_hunt_construct(&quest);
        quest.base.active = saved.active;
        quest.plain.deadline = saved.deadline;
        quest.plain.hunt_count = (char)saved.hunt_count;
        for (i = 0; i < saved.hunt_count; ++i) {
            struct SavedHunt hunt;
            memcpy(&hunt, data + at, hunt_size(names));
            at += hunt_size(names);
            quest.plain.hunt_id[i] = hunt.id;
            quest.plain.hunt_flags[i] = hunt.flags;
            quest.plain.hunt_mob[i] = hunt.monster;
            quest.plain.hunt_min_level[i] = hunt.min_level;
            quest.plain.hunt_max_level[i] = hunt.max_level;
            quest.plain.hunt_done[i] = hunt.done;
            quest.plain.hunt_goal[i] = hunt.goal;
            copy_name(quest.plain.hunt_name[i], hunt.name, (size_t)names);
        }
        renew_quest_read_ongoing(quests, saved.id, &quest);
        quest_hunt_destroy(&quest);
    }
}

int quest_blob_load(struct RenewQuestMgr *quests, const void *bytes, size_t size)
{
    const unsigned char *data = bytes;
    size_t at = 0, length, end = 0;
    unsigned short tag;
    int names = SHORT_NAMES;
    if (!quests || !data || size > INT_MAX ||
        !read_header(data, size, &at, &tag, &length) || tag != QUEST_OPEN || length)
        return -1;
    while (at < size) {
        if (!read_header(data, size, &at, &tag, &length))
            return -1;
        if (tag == QUEST_CLOSE) {
            if (length)
                return -1;
            end = at;
            break;
        }
        if (tag == QUEST_PARAM) {
            if (length != sizeof names)
                return -1;
            memcpy(&names, data + at, sizeof names);
            if (names != SHORT_NAMES && names != LONG_NAMES)
                return -1;
        } else if (tag == QUEST_LIST && !list_valid(data + at, length, names)) {
            return -1;
        }
        at += length;
    }
    if (!end)
        return -1;
    renew_quest_clear_ongoing(quests);
    names = SHORT_NAMES;
    at = HEADER_SIZE;
    while (at < end) {
        if (!read_header(data, end, &at, &tag, &length))
            return -1;
        if (tag == QUEST_PARAM)
            memcpy(&names, data + at, sizeof names);
        else if (tag == QUEST_LIST)
            load_list(quests, data + at, names);
        at += length;
    }
    quests->hunt_name_length = names;
    renew_quest_refresh_windows();
    return (int)end;
}

int quest_blob_save(const struct RenewQuestMgr *quests, struct QuestBlob *out)
{
    const struct TreeNode *head;
    struct TreeNode *node;
    struct QuestBlob result;
    size_t list_size = sizeof(int), at;
    int count = 0, names = LONG_NAMES;
    if (!quests || !out)
        return 0;
    head = quests->quests_b.head;
    for (node = tree_first(&quests->quests_b); node != head; node = tree_next(node)) {
        int hunts = TREE_VALUE(node, struct OngoingQuestEntry)->quest.plain.hunt_count;
        size_t bytes;
        if (hunts < 0 || hunts > RENEW_QUEST_HUNT_MAX)
            return 0;
        bytes = sizeof(struct SavedQuest) + (size_t)hunts * sizeof(struct SavedHunt);
        if (bytes > INT_MAX - 4 * HEADER_SIZE - sizeof names - list_size)
            return 0;
        list_size += bytes;
        ++count;
    }
    result.size = 4 * HEADER_SIZE + sizeof names + list_size;
    result.data = malloc(result.size);
    if (!result.data)
        return 0;
    write_header(result.data, QUEST_OPEN, 0);
    write_header(result.data + HEADER_SIZE, QUEST_PARAM, sizeof names);
    memcpy(result.data + 2 * HEADER_SIZE, &names, sizeof names);
    at = 2 * HEADER_SIZE + sizeof names;
    write_header(result.data + at, QUEST_LIST, (int)list_size);
    at += HEADER_SIZE;
    memcpy(result.data + at, &count, sizeof count);
    at += sizeof count;
    for (node = tree_first(&quests->quests_b); node != head; node = tree_next(node)) {
        const struct OngoingQuestEntry *entry = TREE_VALUE(node, struct OngoingQuestEntry);
        const struct RenewQuestPlain *plain = &entry->quest.plain;
        struct SavedQuest saved = {0};
        int i;
        saved.id = entry->id;
        saved.active = entry->quest.base.active;
        saved.hunt_count = plain->hunt_count;
        saved.deadline = plain->deadline;
        memcpy(result.data + at, &saved, sizeof saved);
        at += sizeof saved;
        for (i = 0; i < plain->hunt_count; ++i) {
            struct SavedHunt hunt = {0};
            hunt.id = (int)plain->hunt_id[i];
            hunt.flags = plain->hunt_flags[i];
            hunt.monster = plain->hunt_mob[i];
            hunt.min_level = plain->hunt_min_level[i];
            hunt.max_level = plain->hunt_max_level[i];
            hunt.done = plain->hunt_done[i];
            hunt.goal = plain->hunt_goal[i];
            copy_name(hunt.name, plain->hunt_name[i], sizeof plain->hunt_name[i]);
            memcpy(result.data + at, &hunt, sizeof hunt);
            at += sizeof hunt;
        }
    }
    write_header(result.data + at, QUEST_CLOSE, 0);
    quest_blob_destroy(out);
    *out = result;
    return 1;
}

void quest_blob_destroy(struct QuestBlob *blob)
{
    if (blob) {
        free(blob->data);
        blob->data = NULL;
        blob->size = 0;
    }
}
