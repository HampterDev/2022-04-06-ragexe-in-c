#include "platform/code_page.h"
#ifndef CONTENTS_REPUTE_GROUPS_H
#define CONTENTS_REPUTE_GROUPS_H

#include "base.h"
#include "std/std.h"

struct ReputeGroupRecord {
    struct String id_text;
    struct String text;
    struct Vec ids;
};

struct ReputeGroupRecordPair {
    int group_id;
    struct ReputeGroupRecord record;
};
OFFSET_ASSERT(ReputeGroupRecordPair, record, 0x04);

struct JsonCppLoader;

unsigned char repute_groups_read(struct JsonCppLoader *loader,
                                            struct Tree *records);

struct ReputeGroup {
    struct String text;
    struct Vec ids;
};

struct ReputeGroupCopier {
    void *repute_mgr;
    struct Tree *groups;
};
SIZE_ASSERT(ReputeGroupCopier, 8);

struct ReputeGroupCopier *repute_groups_copy(struct ReputeGroupCopier *out,
                                                     struct TreeNode *first,
                                                     struct TreeNode *last,
                                                     void *repute_mgr, struct Tree *groups);

struct ReputeGroup *repute_group_assign(struct ReputeGroup *self,
                                                   const struct ReputeGroup *src);

#endif
