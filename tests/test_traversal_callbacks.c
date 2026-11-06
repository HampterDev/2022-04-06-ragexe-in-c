#include "contents/guild_info.h"
#include "session/state.h"
#include "game/gamemode.h"
#include "world/model.h"
#include "actor/actor_ops.h"
#include "core/fatal.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Session g_session1;
struct GuildInfo g_guildInfo = {0};
static struct World world;
static struct C3dAttr attr;
static struct { unsigned char *memory; size_t size; } blocks[128];
static unsigned allocations, live_blocks, sends, node_creations, animations, bounds;
static unsigned world_to_cell, cell_to_world;
static struct GameMode mode;
static int transport;

void *mem_alloc(size_t size)
{
    unsigned i;
    unsigned char *p = malloc(size + 16);
    assert(p);
    memset(p, 0xa5, size + 16);
    for (i = 0; i < 128 && blocks[i].memory; ++i) {}
    assert(i < 128);
    blocks[i].memory = p;
    blocks[i].size = size;
    ++live_blocks;
    ++allocations;
    return p;
}

void mem_free(void *p)
{
    size_t size;
    unsigned i, j;
    if (!p) return;
    for (i = 0; i < 128 && blocks[i].memory != p; ++i) {}
    assert(i < 128);
    size = blocks[i].size;
    for (j = 0; j < 16; ++j) assert(blocks[i].memory[size + j] == 0xa5);
    free(p);
    blocks[i].memory = NULL;
    --live_blocks;
}

__declspec(noreturn) void fatal_error(const char *message)
{ (void)message; abort(); }

static struct GameActor player, master, other, second_master, actor_info;
static int player_guild, master_character, other_character, second_character;
static int character_id(struct GameActor *self)
{
    if (self == &master)
        return master_character;
    if (self == &second_master)
        return second_character;
    return self == &other ? other_character : 0;
}
int game_actor_get_guild_id(struct GameActor *self) { return self == &player ? player_guild : 0; }
static struct GameActorOps actor_ops;

struct GameMode *game_current_mode(void) { return (struct GameMode *)&mode; }
struct RagConnection *rag_connection_instance(void) { return (struct RagConnection *)&transport; }
int rag_connection_get_packet_length(struct RagConnection *self, int id)
{ assert(self == (struct RagConnection *)&transport && id == 0xbb0); return 9; }
char rag_connection_send_packet(struct RagConnection *self, int length, const void *packet)
{
    const unsigned char *p = packet;
    unsigned extra;
    assert(self == (struct RagConnection *)&transport && length == 9);
    memcpy(&extra, p + 2, 4);
    assert(p[0] == 0xb0 && p[1] == 0x0b && extra == (unsigned)g_guildInfo.master_gid);
    assert(p[6] == 1 && p[7] == 0 && p[8] == 1);
    ++sends;
    return 1;
}
void *session_get_actor_by_aid(void *session, int aid)
{
    assert(session == &g_session1 && aid == master.aid);
    return &actor_info;
}
void game_mode_world_to_cell(struct GameMode *owner, float x, float z, int *cx, int *cy)
{
    assert(owner == &mode);
    assert(x == actor_info.pos.x + 0.5f && z == actor_info.pos.z + 0.5f);
    *cx = 7; *cy = 8; ++world_to_cell;
}
void game_mode_cell_to_world(struct GameMode *owner, int cx, int cy, float *x, float *z)
{
    assert(owner == &mode && cx == 7 && cy == 8);
    *x = 10; *z = 20; ++cell_to_world;
}

static void append_pointer(struct List *list, void *value)
{
    list_push_back(list, &value, sizeof value);
}

static void world_proximity(void)
{
    struct CellIds *cell, made;
    float pos[2] = {12, 22}, x = 10, z = 20;
    unsigned before;
    actor_ops.get_char_id = character_id;
    player.ops = master.ops = other.ops = &actor_ops.base.base;
    player.pos.x = 10; player.pos.z = 20; player_guild = 1;
    master.aid = 42; master_character = g_guildInfo.master_gid;
    other.aid = 43; other_character = 999;
    world.player = (struct Player *)&player; world.attr = &attr; world.nearRange = 2; attr.width = 100;
    list_init(&world.actors); list_init(&world.idsNowhere);
    append_pointer(&world.actors, &other); append_pointer(&world.actors, &master);
    tree_init(&world.idsByCell);
    before = allocations;
    world_update_near_flag_for_id(&world, 42, pos);
    assert(world.nearFlag == 1 && sends == 1 && allocations == before);
    world_update_near_flag_for_id(&world, 42, pos);
    assert(sends == 1);
    pos[0] = 12.01f;
    world_update_near_flag_for_id(&world, 42, pos);
    assert(world.nearFlag == 0 && sends == 1);
    pos[0] = 10; pos[1] = 20;
    world_update_near_flag_for_id(&world, 43, pos);
    world_update_near_flag_for_id(&world, 999, pos);
    player_guild = 0;
    world_update_near_flag_for_id(&world, 42, pos);
    world.player = NULL;
    world_update_near_flag_for_id(&world, 42, pos);
    assert(!world.nearFlag && sends == 1 && allocations == before);
    world.player = (struct Player *)&player; player_guild = 1;
    pos[0] = NAN;
    world_update_near_flag_for_id(&world, 42, pos);
    assert(!world.nearFlag);
    pos[0] = 10; world.nearRange = -1;
    world_update_near_flag_for_id(&world, 42, pos);
    assert(!world.nearFlag);
    world.nearRange = 2;

    made.cell = 100 * 22.0f + 12;
    list_init(&made.ids);
    cell = tree_add(&world.idsByCell, &made, sizeof made, cell_ids_less, NULL);
    append_pointer(&cell->ids, (void *)(INT_PTR)43);
    append_pointer(&cell->ids, (void *)(INT_PTR)42);
    assert(world.idsByCell.head->parent == TREE_NODE_OF(cell));
    before = allocations;
    assert(world_any_matching_actor_near(&world, &x, &z) == 1);
    world_update_near_flag_at(&world, &x, &z);
    world_update_near_flag_at(&world, &x, &z);
    assert(world.nearFlag && sends == 2 && allocations == before);
    world_forget_id_in_cell_map(&world, 42, 0);
    assert(world.nearFlag && cell->ids.size == 1);
    assert(!world_any_matching_actor_near(&world, &x, &z));
    append_pointer(&cell->ids, (void *)(INT_PTR)42);
    world_forget_id_in_cell_map(&world, 42, 1);
    assert(!world.nearFlag && cell->ids.size == 1 && sends == 2);
    list_free(&cell->ids, NULL);
    tree_free(&world.idsByCell, NULL);
    tree_init(&world.idsByCell);

    append_pointer(&world.idsNowhere, (void *)(INT_PTR)999);
    append_pointer(&world.idsNowhere, (void *)(INT_PTR)43);
    append_pointer(&world.idsNowhere, (void *)(INT_PTR)42);
    second_master.ops = &actor_ops.base.base;
    second_master.aid = 44;
    second_character = g_guildInfo.master_gid;
    append_pointer(&world.actors, &second_master);
    append_pointer(&world.idsNowhere, (void *)(INT_PTR)44);
    actor_info.pos.x = 30; actor_info.pos.z = 20;
    assert(!world_any_matching_actor_near(&world, &x, &z));
    actor_info.pos.x = 12; actor_info.pos.z = 18;
    assert(world_any_matching_actor_near(&world, &x, &z));
    actor_info.state = 1;
    world_update_near_flag_at(&world, &x, &z);
    assert(world.nearFlag && sends == 3 && world_to_cell == 1 && cell_to_world == 1);
    world_forget_id_in_list_ec(&world, 43, 1);
    assert(world.nearFlag);
    world_forget_id_in_list_ec(&world, 42, 1);
    assert(!world.nearFlag && world.idsNowhere.size == 2 && sends == 3);
    list_free(&world.idsNowhere, NULL); list_free(&world.actors, NULL);
    tree_free(&world.idsByCell, NULL);
    assert(!live_blocks);
}

struct C3dModelRes *test_model_names(void);
static struct C3dModelRes *resource;
static struct C3dActorV2 *model;
static struct ModelResNode node_resources[2];
static int node_create(struct C3dNode *object, struct ModelResNode *res)
{
    struct C3dNode *node = object;
    assert(node->actor == &model->base && node_creations < 2);
    assert(res == &node_resources[node_creations++]);
    return 1;
}
void c3d_node_v2_animate(struct C3dNode *node, double time)
{ assert(node->actor == &model->base && time == 0 && model->nodeCount == 2); ++animations; }

static void model_creation(void)
{
    unsigned before = allocations;
    resource = test_model_names();
    {
        static struct ModelResNode *entries[2];
        unsigned i;
        resource->nodes = entries; resource->nodeCount = 2;
        for (i = 0; i < 2; ++i) {
            const char *name = i ? "second" : "first";
            entries[i] = &node_resources[i];
            node_resources[i].name.data = (char *)name;
            node_resources[i].name.size = strlen(name);
        }
    }
    assert(allocations == before);
    model = (struct C3dActorV2 *)model_actor_create(2);
    assert(model);
    before = allocations;
    assert(model->base.ops->create(&model->base, resource));
    assert(node_creations == 2 && animations == 2 && bounds == 2);
    assert(model->base.shadeType == 2 && model->fps == 20 && model->duration == 1500);
    assert(allocations == before + 2);
    model->base.ops->destroy(&model->base); model = NULL;
    assert(!live_blocks);
}

static struct C3dActorV1 *model_v1;
static struct ModelResNodeV1 resource_node_v1;
static unsigned v1_loaded, v1_posed, v1_measured;
static int load_v1(struct C3dNode *object, struct ModelResNode *res)
{
    assert(!model_v1->root && res == &resource_node_v1.base);
    assert(object->actor == &model_v1->base);
    ++v1_loaded;
    return 1;
}
void c3d_node_v1_animate(struct C3dNode *node, double time)
{ assert(node->actor == &model_v1->base && node == model_v1->root && !time && v1_loaded == 1); ++v1_posed; }
void c3d_node_get_bound(struct C3dNode *node, float *box, const float *parent)
{
    if (model_v1) {
        assert(node == model_v1->root && v1_posed == 1);
        ++v1_measured;
        return;
    }
    assert(node->actor == &model->base && animations == 2);
    ++bounds;
}
struct C3dNode *model_node_create(int version, struct C3dActor *actor,
                                struct C3dNode *parent, struct ModelResNode *res)
{
    size_t size = version == 1 ? sizeof(struct C3dNodeV1) : sizeof(struct C3dNodeV2);
    struct C3dNode *node = mem_alloc(size);
    assert((version == 1 || version == 2) && !parent);
    memset(node, 0, size);
    node->actor = actor;
    assert(version == 1 ? load_v1(node, res) : node_create(node, res));
    return node;
}
void model_node_destroy(struct C3dNode *node, int version)
{
    assert(version == (model_v1 ? 1 : 2));
    mem_free(node);
}
static void model_v1_creation(void)
{
    struct C3dModelRes res = {0};
    struct ModelResNode *nodes[] = {&resource_node_v1.base};
    res.nodes = nodes; res.nodeCount = 1;
    res.frameCount = 1234567; res.fps = 19.25f; res.shadeType = 1;
    model_v1 = (struct C3dActorV1 *)model_actor_create(1);
    assert(model_v1);
    assert(model_v1->base.ops->create(&model_v1->base, &res));
    assert(model_v1->frameCount == 1234567 && model_v1->base.shadeType == 1);
    assert(v1_loaded == 1 && v1_posed == 1 && v1_measured == 1);
    model_v1->base.ops->destroy(&model_v1->base);
    model_v1 = NULL;
    assert(!live_blocks);
}

static void model_picking(void)
{
    struct C3dActor actor = {0};
    struct Ray3 ray = {{2, 3, -5}, {0, 0, 1}, 10};
    int i;
    actor.obb.centre[0] = 2; actor.obb.centre[1] = 3;
    for (i = 0; i < 3; ++i) {
        actor.obb.extent[i] = 1;
        actor.obb.axis[i][i] = 1;
    }
    c3d_actor_pick(&actor, &ray);
    assert(actor.picked);
    actor.picked = 0; ray.length = 3;
    c3d_actor_pick(&actor, &ray);
    assert(!actor.picked);
    ray.length = 10; ray.origin.x = 5;
    c3d_actor_pick(&actor, &ray);
    assert(!actor.picked);
}

int main(void)
{
    g_guildInfo.master_gid = 321;
    world_proximity();
    model_creation();
    model_v1_creation();
    model_picking();
    puts("traversal callbacks: synchronous names, model ownership, proximity boundaries and packet transitions passed");
    return 0;
}
