#include "ui/window_access.h"
#include <string.h>
#include "contents/repute_mgr.h"
#include "contents/client_res.h"
#include "contents/ui_window.h"
#include "contents/ui_window_mgr.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "ui/ui_std.h"

static struct ReputeMgr *g_reputeMgr;

struct ReputeMgr *repute_mgr_instance(void)
{
    if (!g_reputeMgr) {
        struct ReputeMgr *manager = mem_try_alloc(sizeof *manager);
        g_reputeMgr = manager ? repute_mgr_construct(manager) : NULL;
    }
    return g_reputeMgr;
}

void repute_mgr_destroy(void)
{
    if (!g_reputeMgr)
        return;
    repute_mgr_destruct(g_reputeMgr);
    mem_free(g_reputeMgr);
    g_reputeMgr = NULL;
}

struct ReputeMgr *repute_mgr_construct(struct ReputeMgr *self)
{
    tree_init(&self->infos);
    tree_init(&self->groups);
    self->no_info = 1;
    lua_script_construct(&self->lua);
    return self;
}

void repute_mgr_destruct(struct ReputeMgr *self)
{
    lua_script_destruct(&self->lua);
    tree_free(&self->groups, repute_group_entry_destroy);
    tree_free(&self->infos, repute_info_entry_destroy);
}

struct ReputeGroup *repute_group_assign(struct ReputeGroup *self,
                                                   const struct ReputeGroup *src)
{
    if (self != src)
        string_assign(&self->text, string_cstr(&src->text), src->text.size);
    vec_assign(&self->ids, src->ids.first, src->ids.last, sizeof(long long));
    return self;
}

void repute_info_store(const struct ReputeInfoStore *self,
                                  const struct ReputeInfoRecordPair *rec)
{
    struct ReputeInfo info, *dst;
    struct ReputeInfoEntry *entry;
    struct String converted, invisible;
    struct String *text;
    unsigned long long key;

    string_init(&info.name);
    info.points = 0;

    text = text_utf8_to_codepage(&converted, &rec->record.name, g_codePage);
    string_assign(&info.name, string_cstr(text), text->size);
    string_free(&converted);
    info.max_point_positive = rec->record.max_point_positive;
    info.max_point_negative = rec->record.max_point_negative;

    text_utf8_to_codepage(&invisible, &rec->record.visibility, g_codePage);
    if (string_equal_cstr(&invisible, "VISIBLE_TRUE"))
        info.visibility = REPUTE_SHOWN;
    else
        info.visibility = (long long)(string_equal_cstr(&invisible, "VISIBLE_FALSE") ^ 1) + 1;

    key = (unsigned long long)(long long)rec->id;
    entry = tree_get(self->infos, &key, sizeof key, sizeof *entry, tree_less_uint64);
    dst = &entry->info;
    if (dst != &info)
        string_assign(&dst->name, string_cstr(&info.name), info.name.size);
    dst->max_point_positive = info.max_point_positive;
    dst->max_point_negative = info.max_point_negative;
    dst->visibility = info.visibility;
    dst->points = info.points;
    dst->highlighted = 0;

    string_free(&invisible);
    string_free(&info.name);
}

#define REPUTE_WND_MSG_TEXT  0x16
#define REPUTE_WND_MSG_REFRESH_LIST    0x88
#define MSG_REPUTE_CHANGED   0xef5

static int info_listed(const struct ReputeInfo *info)
{
    return info->visibility == REPUTE_SHOWN
        || (info->visibility == REPUTE_SHOWN_WITH_POINTS && info->points != 0);
}

static void add_group_infos(const struct ReputeMgr *self, struct Vec *out, const struct ReputeGroup *group)
{
    const long long *id;

    for (id = (const long long *)group->ids.first; id != (const long long *)group->ids.last; id++) {
        struct ReputeInfoEntry *entry = tree_find(&self->infos, id, tree_less_uint64);

        if (entry && info_listed(&entry->info))
            *(struct ReputeInfo **)vec_add(out, 1, sizeof(struct ReputeInfo *)) = &entry->info;
    }
}

/* A NULL manager (its allocation failed) has no groups and no infos, as one whose file did not load. */
struct Vec *repute_mgr_group_names(struct ReputeMgr *self, struct Vec *out)
{
    struct TreeNode *it;

    out->first = out->last = out->end = 0;
    if (!self)
        return out;
    for (it = tree_first(&self->groups); it != self->groups.head; it = tree_next(it))
        string_vec_push_copy(out, &TREE_VALUE(it, struct ReputeGroupEntry)->group.text);
    return out;
}

struct Vec *repute_mgr_group_infos(struct ReputeMgr *self, struct Vec *out,
                                                   const struct String *group_name)
{
    struct TreeNode *it;

    out->first = out->last = out->end = 0;
    if (!self)
        return out;
    for (it = tree_first(&self->groups); it != self->groups.head; it = tree_next(it)) {
        const struct ReputeGroup *group = &TREE_VALUE(it, struct ReputeGroupEntry)->group;

        if (string_equal(&group->text, group_name))
            add_group_infos(self, out, group);
    }
    return out;
}

struct Vec *repute_mgr_all_infos(struct ReputeMgr *self, struct Vec *out)
{
    struct TreeNode *it;

    out->first = out->last = out->end = 0;
    if (!self)
        return out;
    for (it = tree_first(&self->groups); it != self->groups.head; it = tree_next(it))
        add_group_infos(self, out, &TREE_VALUE(it, struct ReputeGroupEntry)->group);
    return out;
}

struct Vec *repute_mgr_search_infos(struct ReputeMgr *self, struct Vec *out,
                                                    const struct String *text)
{
    struct TreeNode *it;
    const char *needle = string_cstr(text);
    size_t needle_len = text->size;

    out->first = out->last = out->end = 0;
    if (!self)
        return out;
    for (it = tree_first(&self->infos); it != self->infos.head; it = tree_next(it)) {
        struct ReputeInfo *info = &TREE_VALUE(it, struct ReputeInfoEntry)->info;
        const char *name = string_cstr(&info->name), *end, *at;
        int found = 0;

        if (needle_len > info->name.size)
            continue;
        if (needle_len == 0) {
            found = 1;
        } else {
            end = name + (info->name.size - needle_len + 1);
            for (at = name; (at = (const char *)memchr(at, needle[0], end - at)) != 0; at++) {
                if (memcmp(at, needle, needle_len) == 0) {
                    found = 1;
                    break;
                }
            }
        }
        if (found && info_listed(info))
            vec_push(out, &info, sizeof info);
    }
    return out;
}

void repute_mgr_on_changed(struct ReputeMgr *self, unsigned long long group_id,
                                      unsigned long long info_id)
{
    struct UIWindow *wnd = ui_window_open(REPUTE_WND_ID);
    struct ReputeInfoEntry *entry;

    if (!wnd)
        return;
    if (group_id == 0) {
        ui_window_send(wnd, NULL, REPUTE_WND_MSG_TEXT, (int)msg_str(MSG_REPUTE_CHANGED), 0, 0, 0);
    } else {
        struct ReputeGroupEntry *group = self ? tree_find(&self->groups, &group_id, tree_less_uint64) : NULL;

        if (group)
            ui_window_send(wnd, NULL, REPUTE_WND_MSG_TEXT, (int)string_cstr(&group->group.text), 0, 0, 0);
    }
    entry = self ? tree_find(&self->infos, &info_id, tree_less_uint64) : NULL;
    if (entry) {
        entry->info.highlighted = 1;
        ui_window_invalidate(wnd);
    }
}

void repute_mgr_set_points(struct ReputeMgr *self, unsigned long long info_id, long long points)
{
    struct ReputeInfoEntry *entry = self ? tree_find(&self->infos, &info_id, tree_less_uint64) : NULL;

    if (entry)
        entry->info.points = points;
}

void repute_mgr_set_info_result(struct ReputeMgr *self, unsigned char flag)
{
    void *wnd;

    if (self)
        self->no_info = (unsigned char)(flag ^ 1);
    if (flag != 1)
        return;
    wnd = ui_window_find(REPUTE_WND_ID);
    if (wnd)
        ui_window_send(wnd, NULL, REPUTE_WND_MSG_REFRESH_LIST, 0, 0, 0, 0);
}
