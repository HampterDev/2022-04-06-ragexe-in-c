#include "game/packet_handlers.h"
#include "game/gamemode.h"
#include "game/skill_cast.h"

#pragma pack(push, 1)
struct PacketUseSkillAck {
    unsigned short packet_type;
    unsigned src_aid;
    unsigned target_aid;
    unsigned short x;
    unsigned short y;
    unsigned short skill_id;
    int property;
    int delay_ms;
    unsigned char is_disposable;
    int attack_motion;
};
#pragma pack(pop)

void game_mode_on_use_skill_ack(struct GameMode *self, const struct PacketUseSkillAck *packet, int version)
{
    unsigned char is_disposable = 0;

    if (version == 0) {
        game_mode_zc_useskill_ack(self, packet->src_aid, packet->target_aid, packet->x, packet->y,
                                   packet->skill_id, packet->property, packet->delay_ms, is_disposable, 0);
    } else {
        is_disposable = packet->is_disposable;
        game_mode_zc_useskill_ack(self, packet->src_aid, packet->target_aid, packet->x, packet->y,
                                   packet->skill_id, packet->property, packet->delay_ms, is_disposable, 0);
    }
}

void game_mode_on_use_skill_ack3(struct GameMode *self, const struct PacketUseSkillAck *packet)
{
    game_mode_zc_useskill_ack(self, packet->src_aid, packet->target_aid, packet->x, packet->y, packet->skill_id,
                               packet->property, packet->delay_ms, packet->is_disposable, packet->attack_motion);
}
