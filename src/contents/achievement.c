#include "session/state.h"
#include "core/memory.h"
#include "contents/achievement_internal.h"
#include "contents/game_session.h"
#include "session/session_achievements.h"

struct AchievementMgr *g_achievementDb;

static void destroy_tab(void *value)
{
    vec_free(&((struct AchievementTab *)value)->ids);
}

static struct AchievementMgr *achievement_construct(struct AchievementMgr *self)
{
    self->tab_stride = 10;
    tree_init(&self->achievements);
    tree_init(&self->tabs);
    return self;
}

static void achievement_destruct(struct AchievementMgr *self)
{
    tree_free(&self->achievements, achievement_info_entry_destroy);
    tree_free(&self->tabs, destroy_tab);
}

char achievement_create(void)
{
    struct AchievementMgr *mgr;

    if (g_achievementDb)
        return 0;
    mgr = (struct AchievementMgr *)mem_try_alloc(sizeof *mgr);
    if (mgr)
        achievement_construct(mgr);
    g_achievementDb = mgr;
    if (!mgr)
        return 0;
    if (achievement_load())
        return 1;
    achievement_destroy();
    return 0;
}

void achievement_destroy(void)
{
    if (!g_achievementDb)
        return;
    achievement_destruct(g_achievementDb);
    mem_free(g_achievementDb);
    g_achievementDb = 0;
}

static struct AchievementInfo *existing_info(struct AchievementMgr *self, int id)
{
    struct AchievementInfoEntry *entry = tree_find(&self->achievements, &id, tree_less_int);

    return entry ? &entry->info : 0;
}

char achievement_get_info(struct AchievementMgr *self, struct AchievementInfo *out, int id)
{
    struct AchievementInfo *info = existing_info(self, id);

    if (!info)
        return 0;
    achievement_info_assign(out, info);
    return 1;
}

static struct String *copy_text(struct String *out, const struct String *text)
{
    if (text) {
        string_copy(out, text);
    } else {
        string_init(out);
    }
    return out;
}

struct String *achievement_text1(struct AchievementMgr *self, struct String *out, int id)
{
    struct AchievementInfo *info = existing_info(self, id);

    return copy_text(out, info ? &info->text[1] : 0);
}

struct String *achievement_ui_text(struct AchievementMgr *self, struct String *out, int id)
{
    struct AchievementInfo *info = existing_info(self, id);

    return copy_text(out, info ? &info->ui_text : 0);
}

int achievement_info_number(struct AchievementMgr *self, int id)
{
    struct AchievementInfo *info = existing_info(self, id);

    return info ? info->info_number : -1;
}

int achievement_reward_title(struct AchievementMgr *self, int id)
{
    struct AchievementInfo *info = existing_info(self, id);

    return info ? info->reward_title : -1;
}

char achievement_has_reward(struct AchievementMgr *self, int id)
{
    struct AchievementInfo *info = existing_info(self, id);

    if (!info)
        return 0;
    return info->reward_item > 0 || info->reward_buff > 0 || info->reward_title > 0;
}

static char collect_one_tab(struct AchievementMgr *self, struct Tree *out, int key)
{
    struct Vec *ids = achievement_tab_at(&self->tabs, key);
    const int *id;

    for (id = ids->first; id != ids->last; id++) {
        struct AchievementInfo *info = achievement_at(&self->achievements, *id);

        if (!achievement_add(out, *id, info))
            return 0;
    }
    return 1;
}

char achievement_collect_tab(struct AchievementMgr *self, struct Tree *out, int major, int minor)
{
    int sub;

    tree_clear(out, achievement_info_entry_destroy);
    if (minor != -1)
        return collect_one_tab(self, out, self->tab_stride * major + minor);
    for (sub = 0; sub < self->tab_stride; sub++)
        if (!collect_one_tab(self, out, self->tab_stride * major + sub))
            return 0;
    return 1;
}

int achievement_tab_count(struct AchievementMgr *self, int major, int minor)
{
    int total = 0, sub;

    if (major == 0)
        return (int)self->achievements.size;
    if (minor != -1)
        return (int)VEC_COUNT(achievement_tab_at(&self->tabs, self->tab_stride * major + minor), int);
    for (sub = 0; sub < self->tab_stride; sub++)
        total += (int)VEC_COUNT(achievement_tab_at(&self->tabs, self->tab_stride * major + sub), int);
    return total;
}

static int is_completed(int id)
{
    struct SessionAchievementState state;

    return session_get_achievement(&g_session1, &state, id)->completed == 1;
}

static int count_completed_in_tab(struct AchievementMgr *self, int key)
{
    struct Vec *ids = achievement_tab_at(&self->tabs, key);
    const int *id;
    int count = 0;

    for (id = ids->first; id != ids->last; id++)
        count += is_completed(*id);
    return count;
}

int achievement_tab_count_flagged(struct AchievementMgr *self, int major, int minor)
{
    struct TreeNode *node;
    int count = 0, sub;

    if (major == 0) {
        for (node = tree_first(&self->achievements); node != self->achievements.head; node = tree_next(node))
            count += is_completed(TREE_VALUE(node, struct AchievementInfoEntry)->id);
        return count;
    }
    if (minor != -1)
        return count_completed_in_tab(self, self->tab_stride * major + minor);
    for (sub = 0; sub < self->tab_stride; sub++)
        count += count_completed_in_tab(self, self->tab_stride * major + sub);
    return count;
}

char achievement_find_tab(struct AchievementMgr *self, int id, int *major, int *minor)
{
    struct TreeNode *node;

    for (node = tree_first(&self->tabs); node != self->tabs.head; node = tree_next(node)) {
        const struct AchievementTab *tab = TREE_VALUE(node, const struct AchievementTab);
        const int *have;

        for (have = tab->ids.first; have != tab->ids.last; have++) {
            if (*have == id) {
                *major = tab->key / self->tab_stride;
                *minor = tab->key % self->tab_stride;
                return 1;
            }
        }
    }
    return 0;
}
