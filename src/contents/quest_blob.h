#ifndef CONTENTS_QUEST_BLOB_H
#define CONTENTS_QUEST_BLOB_H
#include <stddef.h>

struct RenewQuestMgr;
struct QuestBlob {
    unsigned char *data;
    size_t size;
};

int quest_blob_save(const struct RenewQuestMgr *quests, struct QuestBlob *out);
void quest_blob_destroy(struct QuestBlob *blob);

int quest_blob_load(struct RenewQuestMgr *quests, const void *data, size_t size);
#endif
