#include "contents/achievement_internal.h"
#include "std/tree.h"

static void achievement_resources_free(struct Vec *v)
{
    struct AchievementResource *it;

    for (it = v->first; it != v->last; it++)
        string_free(&it->text);
    vec_free(v);
}

static void achievement_resources_copy(struct Vec *dst, const struct Vec *src)
{
    const struct AchievementResource *it;
    struct AchievementResource *out;

    vec_init(dst);
    out = vec_add(dst, VEC_COUNT(src, struct AchievementResource), sizeof *out);
    for (it = src->first; it != src->last; it++, out++) {
        string_copy(&out->text, &it->text);
        out->target = it->target;
        out->goto_id = it->goto_id;
    }
}

static void achievement_resources_assign(struct Vec *dst, const struct Vec *src)
{
    struct Vec copy;

    achievement_resources_copy(&copy, src);
    achievement_resources_free(dst);
    *dst = copy;
}

void achievement_info_init(struct AchievementInfo *info)
{
    string_init(&info->text[0]);
    string_init(&info->text[1]);
    string_init(&info->text[2]);
    info->info_number = 0;
    info->ui_type = 0;
    info->reward_item = -1;
    info->reward_title = -1;
    info->reward_buff = -1;
    vec_init(&info->resources);
    string_init(&info->ui_text);
}

void achievement_info_copy(struct AchievementInfo *dst, const struct AchievementInfo *src)
{
    string_copy(&dst->text[0], &src->text[0]);
    string_copy(&dst->text[1], &src->text[1]);
    string_copy(&dst->text[2], &src->text[2]);
    dst->info_number = src->info_number;
    dst->reward_item = src->reward_item;
    dst->reward_title = src->reward_title;
    dst->reward_buff = src->reward_buff;
    achievement_resources_copy(&dst->resources, &src->resources);
    dst->ui_type = src->ui_type;
    string_copy(&dst->ui_text, &src->ui_text);
}

void achievement_info_assign(struct AchievementInfo *dst, const struct AchievementInfo *src)
{
    string_assign(&dst->text[0], string_cstr(&src->text[0]), src->text[0].size);
    string_assign(&dst->text[1], string_cstr(&src->text[1]), src->text[1].size);
    string_assign(&dst->text[2], string_cstr(&src->text[2]), src->text[2].size);
    dst->info_number = src->info_number;
    dst->reward_item = src->reward_item;
    dst->reward_title = src->reward_title;
    dst->reward_buff = src->reward_buff;
    if (&dst->resources != &src->resources)
        achievement_resources_assign(&dst->resources, &src->resources);
    dst->ui_type = src->ui_type;
    string_assign(&dst->ui_text, string_cstr(&src->ui_text), src->ui_text.size);
}

void achievement_info_destruct(struct AchievementInfo *info)
{
    string_free(&info->ui_text);
    achievement_resources_free(&info->resources);
    string_free(&info->text[2]);
    string_free(&info->text[1]);
    string_free(&info->text[0]);
}

void achievement_info_entry_destroy(void *value)
{
    achievement_info_destruct(&((struct AchievementInfoEntry *)value)->info);
}

struct AchievementInfo *achievement_at(struct Tree *map, int id)
{
    struct AchievementInfoEntry *entry = tree_find(map, &id, tree_less_int);

    if (!entry) {
        entry = tree_get(map, &id, sizeof id, sizeof *entry, tree_less_int);
        achievement_info_init(&entry->info);
    }
    return &entry->info;
}

char achievement_add(struct Tree *map, int id, const struct AchievementInfo *info)
{
    struct AchievementInfoEntry entry;

    if (tree_find(map, &id, tree_less_int))
        return 0;
    entry.id = id;
    achievement_info_copy(&entry.info, info);
    tree_add(map, &entry, sizeof entry, tree_less_int, NULL);
    return 1;
}

struct Vec *achievement_tab_at(struct Tree *tabs, int key)
{
    struct AchievementTab *tab = tree_get(tabs, &key, sizeof key, sizeof *tab, tree_less_int);

    return &tab->ids;
}
