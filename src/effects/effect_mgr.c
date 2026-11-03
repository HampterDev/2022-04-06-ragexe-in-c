#include "game/cells.h"
#include "contents/client_lua_calls.h"
#include "world/object_lookup.h"
#include "resources/attr_queries.h"
#include "session/state.h"
#include "core/memory.h"
#include "world/actor_lookup.h"
#include <string.h>
#include "effects/effect_mgr.h"
#include "effects/render_object.h"
#include "std/tree.h"
#include "game/context.h"
#include "actor/game_actor.h"

struct EffectMgr *g_effectMgr;

struct World;

#define EFFECT_ID_LEVEL99 0xc8
#define EFFECT_LEVEL150 0x3d2
#define EFFECT_LEVEL150_SUB 0x3d3

static struct Effect *effect_of(const struct ListNode *node)
{
    return *LIST_VALUE(node, struct Effect *);
}

static void destroy_owner_effects(void *value)
{
    list_free(&((struct OwnerEffects *)value)->effects, NULL);
}

static void destroy_tracked_effects(void *value)
{
    vec_free(&((struct TrackedEffects *)value)->effects);
}

static struct List *owner_effects(struct EffectMgr *self, int aid)
{
    struct OwnerEffects *owner = tree_find(&self->by_owner, &aid, tree_less_int);
    struct OwnerEffects fresh;

    if (!owner) {
        fresh.aid = aid;
        list_init(&fresh.effects);
        owner = tree_add(&self->by_owner, &fresh, sizeof fresh, tree_less_int, NULL);
    }
    return &owner->effects;
}

static unsigned char register_table(struct EffectMgr *self, const struct EffectTypeTable *table)
{
    size_t i;
    int type;

    for (i = 0; i < table->count; i++)
        for (type = table->rows[i].first; type <= table->rows[i].last; type++)
            if (!effect_mgr_register_factory(self, type, table->rows[i].create))
                return 0;
    return 1;
}

static unsigned char register_types(struct EffectMgr *self, const struct EffectTypeSources *types)
{
    size_t i;

    for (i = 0; i < types->count; i++) {
        const struct EffectTypeSource *source = &types->items[i];

        if (source->table ? !register_table(self, source->table) : !source->add(self))
            return 0;
    }
    return 1;
}

struct EffectMgr *effect_mgr_construct(struct EffectMgr *self, const struct EffectTypeSources *types)
{
    tree_init(&self->factories);
    tree_init(&self->by_owner);
    list_init(&self->removed);
    tree_init(&self->tracked);
    self->ready = 0;
    if (register_types(self, types))
        self->ready = 1;
    return self;
}

void effect_mgr_destruct(struct EffectMgr *self)
{
    effect_mgr_clear(self);
    tree_free(&self->tracked, destroy_tracked_effects);
    list_free(&self->removed, NULL);
    tree_free(&self->by_owner, destroy_owner_effects);
    tree_free(&self->factories, NULL);
}

unsigned char effect_mgr_create(const struct EffectTypeSources *types)
{
    struct EffectMgr *mgr;

    if (g_effectMgr)
        return 0;
    mgr = mem_try_alloc(sizeof(struct EffectMgr));
    if (mgr)
        effect_mgr_construct(mgr, types);
    g_effectMgr = mgr;
    if (!mgr)
        return 0;
    if (mgr->ready)
        return 1;
    effect_mgr_destroy_instance();
    return 0;
}

void effect_mgr_destroy_instance(void)
{
    if (g_effectMgr) {
        effect_mgr_destruct(g_effectMgr);
        mem_free(g_effectMgr);
        g_effectMgr = 0;
    }
}

unsigned char effect_mgr_register_factory(struct EffectMgr *self, int type, struct Effect *(*create)(void))
{
    struct EffectFactory entry;
    int added;

    entry.type = type;
    entry.create = create;
    tree_add(&self->factories, &entry, sizeof entry, tree_less_int, &added);
    return (unsigned char)added;
}

unsigned char effect_mgr_is_registered(struct EffectMgr *self, int type)
{
    return tree_find(&self->factories, &type, tree_less_int) != NULL;
}

struct Effect *effect_mgr_create_of_type(struct EffectMgr *self, int type)
{
    struct EffectFactory *entry = tree_find(&self->factories, &type, tree_less_int);

    if (!entry)
        return 0;
    return entry->create();
}

unsigned char effect_type_is_listed(int type)
{
    switch (type) {
    case 0x23: case 0x24: case 0x3c: case 0x72: case 0x89: case 0x8a: case 0x9a: case 0x9b:
    case 0x9f: case 0xa0: case 0xa4:
    case 0x127: case 0x128:
    case 0x14e: case 0x152: case 0x165: case 0x173: case 0x1a3: case 0x1a8: case 0x1aa:
    case 0x1bd: case 0x1be: case 0x1c1:
    case 0x1db: case 0x1dc: case 0x1dd: case 0x1de: case 0x1df: case 0x1e0: case 0x1e1:
    case 0x1e2: case 0x1e3: case 0x1e4:
    case 0x21e: case 0x220: case 0x224: case 0x225: case 0x246:
    case 0x28f: case 0x290:
    case 0x2ce: case 0x430: case 0x576:
        return 1;
    }
    return 0;
}

void effect_mgr_track(struct EffectMgr *self, struct Effect *effect)
{
    int type = effect->type;
    struct TrackedEffects *tracked;

    if (!EFFECT_TRACKED_TYPE(type))
        return;
    tracked = tree_get(&self->tracked, &type, sizeof type, sizeof *tracked, tree_less_int);
    vec_push(&tracked->effects, &effect, sizeof(struct Effect *));
}

void effect_mgr_add(struct EffectMgr *self, int aid, struct Effect *effect)
{
    list_push_back(owner_effects(self, aid), &effect, sizeof effect);
    effect_mgr_track(self, effect);
}

struct Effect *effect_mgr_spawn(struct EffectMgr *self, int aid, int type, Vec3 pos, int target_aid,
                                             int level, unsigned char reuse, int instance_key)
{
    struct Effect *effect;

    unsigned char force_render;

    if ((type < 0 || type > 60000) && !effect_type_is_listed(type))
        return 0;
    if ((type == EFFECT_LEVEL150 || type == EFFECT_LEVEL150_SUB)
        && effect_mgr_find_effect(self, aid, EFFECT_ID_LEVEL99))
        return 0;
    if (aid) {
        struct Effect *existing = effect_mgr_find_keyed(self, aid, instance_key, type);
        if (instance_key > 0) {
            if (existing)
                return 0;
        } else if (existing && (existing->has_instance_key || reuse)) {
            return existing;
        }
    }

    effect = effect_mgr_create_of_type(self, type);
    if (!effect)
        return 0;
    effect_set_owner(effect, aid);
    effect_set_target(effect, target_aid);
    effect->pos = pos;
    effect->type = type;
    if (level != -1)
        effect->level = level;
    if (instance_key > 0) {
        effect->instance_key = instance_key;
        effect->has_instance_key = 1;
    }
    effect->ops->init(effect);
    effect->ops->after_init(effect);
    effect_mgr_add(self, aid, effect);

    force_render = 0;

    client_lua_call(g_session1.luaState, "IsForceRenderEffect", "d>b", type, &force_render);
    effect->always_draw = force_render;
    return effect;
}

void effect_mgr_spawn_level99_aura(struct EffectMgr *self, void *actor, unsigned char spawn, int val1)
{
    struct Effect *effect;
    struct String texture;

    if (!actor || val1 != 0)
        return;
    game_actor_stop_aura_effects(actor);
    if (!spawn)
        return;
    effect = effect_mgr_spawn(self, ((struct RenderObject *)actor)->aid, EFFECT_ID_LEVEL99, *ACTOR_POS(actor), 0, -1,
                               0, 0);
    if (!effect)
        return;
    string_init(&texture);
    string_assign(&texture, "effect\\번개" "3.bmp", 16);
    effect->ops->set_texture(effect, texture);
    effect->ops->set_color(effect, 0x50, 0x50, 0xff);
}

struct Effect *effect_mgr_find_effect(struct EffectMgr *self, int aid, int type)
{
    struct OwnerEffects *owner = tree_find(&self->by_owner, &aid, tree_less_int);
    struct List *list;
    struct ListNode *n;

    if (!owner)
        return 0;
    list = &owner->effects;
    for (n = list->head->next; n != list->head; n = n->next)
        if (effect_of(n)->type == type)
            return effect_of(n);
    return 0;
}

struct Effect *effect_mgr_find_keyed(struct EffectMgr *self, int aid, int key, int type)
{
    struct OwnerEffects *owner = tree_find(&self->by_owner, &aid, tree_less_int);
    struct List *list;
    struct ListNode *n;

    if (!owner)
        return 0;
    list = &owner->effects;
    for (n = list->head->next; n != list->head; n = n->next)
        if (effect_of(n)->type == type && effect_of(n)->instance_key == key)
            return effect_of(n);
    return 0;
}

unsigned char effect_mgr_has_effect(struct EffectMgr *self, int aid, int type)
{
    return effect_mgr_find_effect(self, aid, type) != 0;
}

struct Effect *effect_mgr_tracked_at(struct EffectMgr *self, int type, unsigned index)
{
    struct TrackedEffects *tracked = tree_find(&self->tracked, &type, tree_less_int);
    struct Vec *v;

    if (!tracked)
        return 0;
    v = &tracked->effects;
    if (index >= VEC_COUNT(v, struct Effect *))
        return 0;
    return ((struct Effect **)v->first)[index];
}

void effect_mgr_remove(struct EffectMgr *self, struct Effect *effect, unsigned char unlink)
{
    struct OwnerEffects *owner;
    struct List *list;
    struct ListNode *n, *next;
    int aid;

    if (!effect)
        return;
    list_push_back(&self->removed, &effect, sizeof effect);
    if (unlink != 1)
        return;
    aid = effect->owner_aid;
    owner = tree_find(&self->by_owner, &aid, tree_less_int);
    if (!owner)
        return;
    list = &owner->effects;
    for (n = list->head->next; n != list->head; n = next) {
        next = n->next;
        if (effect_of(n) == effect)
            list_erase(list, n, NULL);
    }
}

void effect_mgr_remove_type(struct EffectMgr *self, int aid, int type)
{
    struct Effect *effect = effect_mgr_find_effect(self, aid, type);
    if (effect)
        effect_mgr_remove(self, effect, 1);
}

void effect_mgr_remove_keyed(struct EffectMgr *self, int aid, int key, int type)
{
    if (aid > 0 && key > 0)
        effect_mgr_remove(self, effect_mgr_find_keyed(self, aid, key, type), 1);
}

void effect_mgr_remove_owner(struct EffectMgr *self, int aid)
{
    struct OwnerEffects *owner = tree_find(&self->by_owner, &aid, tree_less_int);
    struct List *list;
    struct ListNode *n;

    if (!owner)
        return;
    list = &owner->effects;
    for (n = list->head->next; n != list->head; n = n->next)
        list_push_back(&self->removed, LIST_VALUE(n, struct Effect *), sizeof(struct Effect *));
    list_clear(list, NULL);
}

void effect_mgr_destroy_removed(struct EffectMgr *self)
{
    struct ListNode *n;

    for (n = self->removed.head->next; n != self->removed.head; n = n->next) {
        struct Effect *effect = effect_of(n);

        effect->ops->release(effect);
        if (effect)
            effect->ops->destroy(effect);
    }
    list_clear(&self->removed, NULL);
}

void effect_mgr_reset(struct EffectMgr *self)
{
    struct TreeNode *node;

    for (node = tree_first(&self->by_owner); node != self->by_owner.head; node = tree_next(node))
        effect_mgr_remove_owner(self, TREE_VALUE(node, struct OwnerEffects)->aid);
    tree_clear(&self->by_owner, destroy_owner_effects);
    effect_mgr_remove_owner(self, 0);
    tree_clear(&self->tracked, destroy_tracked_effects);
    effect_mgr_destroy_removed(self);
}

void effect_mgr_clear(struct EffectMgr *self)
{
    tree_clear(&self->factories, NULL);
    effect_mgr_reset(self);
}

void effect_mgr_process(struct EffectMgr *self)
{
    struct TreeNode *node;

    for (node = tree_first(&self->by_owner); node != self->by_owner.head; node = tree_next(node)) {
        struct OwnerEffects *owner = TREE_VALUE(node, struct OwnerEffects);
        struct ListNode *n, *next;

        for (n = owner->effects.head->next; n != owner->effects.head; n = next) {
            unsigned char removed = effect_remove_if_dead(effect_of(n));
            next = n->next;
            if (removed)
                list_erase(&owner->effects, n, NULL);
        }
        for (n = owner->effects.head->next; n != owner->effects.head; n = n->next)
            effect_step(effect_of(n), owner->aid);
    }
}

void effect_mgr_render(struct EffectMgr *self)
{
    struct TreeNode *node;

    for (node = tree_first(&self->by_owner); node != self->by_owner.head; node = tree_next(node)) {
        struct OwnerEffects *owner = TREE_VALUE(node, struct OwnerEffects);
        struct ListNode *n;

        for (n = owner->effects.head->next; n != owner->effects.head; n = n->next)
            effect_render_if_visible(effect_of(n));
    }
}

void *effect_find_actor(int aid)
{
    struct World *w = game_current_world();
    void *actor;

    if (!w)
        return NULL;
    if (g_session1.ownAid == aid)
        return game_current_player();
    actor = world_get_game_actor_by_aid(w, aid);
    if (!actor)
        actor = world_get_skill_by_aid(w, aid);
    return actor;
}

Vec3 *effect_player_position(Vec3 *out)
{
    struct RenderObject *player;
    struct GameMode *mode;
    struct C3dAttr *attr;

    out->z = 0.0f;
    out->x = 0.0f;
    out->y = 0.0f;
    player = (struct RenderObject *)game_current_player();
    if (player) {
        *out = *ACTOR_POS(player);
        return out;
    }
    mode = game_current_mode();
    attr = game_current_map_attr();
    if (!mode || !attr)
        return out;
    game_mode_cell_to_world(mode, g_session1.playerPosX, g_session1.playerPosY,
                             &out->x, &out->z);
    out->y = c3d_attr_get_height(attr, out->x, out->z);
    return out;
}
