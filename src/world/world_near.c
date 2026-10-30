#include "actor/actor_ops.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "game/context.h"
#include "core/fatal.h"
#include "contents/connection_api.h"
#include "world/world.h"
#include "world/mode.h"

#include <math.h>
#include <string.h>
#include "actor/types.h"
#include "session/session_names.h"
#include "std/list.h"

BYTE world_actor_matches_global(struct World *self, int aid)
{
    struct ListNode *head = self->actors.head, *n;

    for (n = head->next; n != head; n = n->next) {
        void *actor = *LIST_VALUE(n, void *);
        if (((struct GameActor *)actor)->aid == aid) {
            if (actor && g_guildInfo.master_gid == actor_get_char_id(actor))
                return 1;
            return 0;
        }
    }
    return 0;
}

static void world_update_near_flag(struct World *self, BYTE nearby)
{
#pragma pack(push, 1)
    struct {
        WORD id;
        DWORD extra;
        WORD one;
        BYTE flag;
    } packet;
#pragma pack(pop)

    if (self->nearFlag == 1) {
        if (!nearby)
            self->nearFlag = 0;
    } else if (nearby == 1) {
        packet.id = 0xbb0;
        packet.extra = (DWORD)g_guildInfo.master_gid;
        packet.one = 1;
        packet.flag = 1;
        rag_connection_send_packet(rag_connection_instance(),
            rag_connection_get_packet_length(rag_connection_instance(), 0xbb0), &packet);
        self->nearFlag = 1;
    }
}

static BYTE world_any_id_at_cell(struct World *self, const float *cell)
{
    float key = (float)self->attr->width * cell[1] + cell[0];
    struct CellIds *best = tree_find(&self->idsByCell, &key, cell_ids_less);
    struct ListNode *it;
    BYTE found;
    int id;

    if (!best) {
        found = 0;
    } else {
        for (it = best->ids.head->next; it != best->ids.head; it = it->next) {
            id = *LIST_VALUE(it, int);
            if (world_actor_matches_global(self, id))
                break;
        }
        found = it != best->ids.head;
    }
    return found;
}

BYTE world_any_matching_actor_near(struct World *self, const float *x, const float *y)
{
    struct ListNode *head, *n, *a;
    float cell[2], range;
    void *actor, *info;
    int id, cx, cy;
    float wx, wz;

    if (!self->player || game_actor_get_guild_id((struct GameActor *)self->player) < 1)
        return 0;
    if (self->idsByCell.size != 0) {
        for (cell[0] = *x - (float)self->nearRange; (float)self->nearRange + *x >= cell[0]; cell[0] += 1.0f) {
            for (cell[1] = *y - (float)self->nearRange; (float)self->nearRange + *y >= cell[1]; cell[1] += 1.0f) {
                if (world_any_id_at_cell(self, cell) == 1)
                    return 1;
            }
        }
    }
    head = self->idsNowhere.head;
    for (n = head->next; n != head; n = n->next) {
        id = *LIST_VALUE(n, int);
        for (a = self->actors.head->next; a != self->actors.head; a = a->next)
            if ((*LIST_VALUE(a, struct GameActor *))->aid == id)
                break;
        if (a == self->actors.head)
            continue;
        actor = *LIST_VALUE(a, void *);
        if (!actor || g_guildInfo.master_gid != actor_get_char_id(actor))
            continue;

        info = session_get_actor_by_aid(&g_session1, id);
        if (((struct GameActor *)info)->state == 1) {
            wx = 0;
            wz = 0;
            cx = 0;
            cy = 0;
            game_mode_world_to_cell(game_current_mode(),
                                 ((struct GameActor *)info)->pos.x + 0.5f,
                                 ((struct GameActor *)info)->pos.z + 0.5f, &cx, &cy);
            game_mode_cell_to_world(game_current_mode(), cx, cy, &wx, &wz);
            range = (float)self->nearRange;
            if (!(range >= (float)fabs((double)(wx - *x))))
                return 0;
            if (!(range >= (float)fabs((double)(wz - *y))))
                return 0;
            return 1;
        }
        range = (float)self->nearRange;
        if (!(range >= (float)fabs((double)(((struct GameActor *)info)->pos.x - *x))))
            return 0;
        if (!(range >= (float)fabs((double)(((struct GameActor *)info)->pos.z - *y))))
            return 0;
        return 1;
    }
    return 0;
}

void world_update_near_flag_for_id(struct World *self, int id, const float *pos)
{
    float player_x, player_z, dx, dz, range;
    BYTE nearby;

    if (!self->player || game_actor_get_guild_id((struct GameActor *)self->player) < 1)
        return;
    if (!world_actor_matches_global(self, id))
        return;
    player_x = ((struct RenderObject *)self->player)->pos.x;
    player_z = ((struct RenderObject *)self->player)->pos.z;
    range = (float)self->nearRange;
    dx = pos[0] - player_x;
    dz = pos[1] - player_z;
    nearby = range >= (float)fabs((double)dx) && range >= (float)fabs((double)dz);
    world_update_near_flag(self, nearby);
}

void world_update_near_flag_at(struct World *self, const float *x, const float *y)
{
    float fy = *y, fx = *x;
    world_update_near_flag(self, world_any_matching_actor_near(self, &fx, &fy));
}

void world_forget_id_in_cell_map(struct World *self, int id, BYTE notify)
{
    struct TreeNode *it = tree_first(&self->idsByCell);
    struct ListNode *head, *n;

    while (it != self->idsByCell.head) {
        struct CellIds *cell = TREE_VALUE(it, struct CellIds);

        head = cell->ids.head;
        for (n = head->next; n != head; n = n->next)
            if (*LIST_VALUE(n, int) == id)
                break;
        if (n != head) {
            if (notify && self->nearFlag && world_actor_matches_global(self, id)) {
                world_update_near_flag(self, 0);
            }
            list_erase(&cell->ids, n, NULL);
            return;
        }
        it = tree_next(it);
    }
}

void world_forget_id_in_list_ec(struct World *self, int id, BYTE notify)
{
    struct ListNode *head = self->idsNowhere.head, *n;

    for (n = head->next; n != head; n = n->next)
        if (*LIST_VALUE(n, int) == id)
            break;
    if (n == head)
        return;
    if (notify && self->nearFlag && world_actor_matches_global(self, id)) {
        world_update_near_flag(self, 0);
    }
    list_erase(&self->idsNowhere, n, NULL);
}
