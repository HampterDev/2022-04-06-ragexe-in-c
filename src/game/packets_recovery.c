#include "game/packet_handlers.h"
#include "session/state.h"
#include "game/skill_use.h"
#include "session/session_windows.h"

enum {
    SP_HP = 5,
    SP_SP = 7,
    EF_HP_RECOVERY = 0x14b,
    EF_SP_RECOVERY = 0x14c,
};
#define COLOUR_HP 0xff00ff00u
#define COLOUR_SP 0xff0000ffu

#pragma pack(push, 1)
struct PacketRecovery {
    unsigned short packet_type;
    unsigned short var_id;
    short amount;
};
struct PacketRecovery2 {
    unsigned short packet_type;
    unsigned short var_id;
    int amount;
};
#pragma pack(pop)

static void recover(int *value, int amount, struct Player *player, unsigned colour, int effect_id)
{
    unsigned old_value, new_value;

    old_value = (unsigned)*value;
    new_value = old_value + (unsigned)amount;
    if (old_value != new_value) {
        *value = (int)new_value;
        session_refresh_basic_info(&g_session1);
    }
    actor_show_number((struct GameActor *)player, amount, colour);
    actor_launch_effect((struct GameActor *)player, effect_id);
}

void game_mode_on_recovery(struct GameMode *self, const void *packet, int version)
{
    struct Player *player = self->world->player;
    int amount;
    short var_id;

    if (version == 0) {
        amount = ((const struct PacketRecovery *)packet)->amount;
        var_id = (short)((const struct PacketRecovery *)packet)->var_id;
    } else {
        amount = ((const struct PacketRecovery2 *)packet)->amount;
        var_id = (short)((const struct PacketRecovery2 *)packet)->var_id;
    }
    if (var_id == SP_HP)
        recover(&g_session1.hp, amount, player, COLOUR_HP, EF_HP_RECOVERY);
    else if (var_id == SP_SP)
        recover(&g_session1.sp, amount, player, COLOUR_SP, EF_SP_RECOVERY);
}
