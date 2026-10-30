#include "game/mode_msg.h"
#include "game/session_bridge.h"
#include "game/packets_entry.h"
#include "session/session_party.h"
#include "ui/ui_std.h"

#define TRIANGLE_MARK_COLOR 0xffffa6d2

int game_mode_msg_set_square_mark(struct GameMode *self, unsigned aid, int x, int y)
{
    struct PartyMember member;
    struct MemberMark *mark = tree_get(&self->square_marks, &aid, sizeof aid, sizeof *mark, tree_less_uint);

    mark->x = x;
    mark->y = y;
    mark->color = session_get_party_member(&g_session1, &member, aid)->color;
    party_member_info_destroy(&member);
    return 0;
}

int game_mode_msg_set_triangle_mark(struct GameMode *self, unsigned id, int x, int y)
{
    struct MemberMark *mark = tree_get(&self->triangle_marks, &id, sizeof id, sizeof *mark, tree_less_uint);

    mark->x = x;
    mark->y = y;
    mark->color = TRIANGLE_MARK_COLOR;
    return 0;
}

int game_mode_msg_remove_mark(struct Tree *marks, unsigned id)
{
    tree_erase_key(marks, &id, tree_less_uint, NULL);
    return 0;
}

int game_mode_msg_clear_marks(struct Tree *marks)
{
    tree_clear(marks, NULL);
    return 0;
}

int game_mode_msg_set_unit_cell(struct GameMode *self, unsigned aid, int x, int y)
{
    struct UnitCell *cell = tree_get(&self->unit_cells, &aid, sizeof aid, sizeof *cell, tree_less_uint);

    cell->x = x;
    cell->y = y;
    return 0;
}

int game_mode_msg_actor_removed(struct GameMode *self, unsigned aid)
{
    tree_erase_key(&self->unit_cells, &aid, tree_less_uint, NULL);
    return 0;
}
