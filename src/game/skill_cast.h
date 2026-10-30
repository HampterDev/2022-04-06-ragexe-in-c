#ifndef GAMEMODE_SKILL_CAST_H
#define GAMEMODE_SKILL_CAST_H

struct GameActor;

void skill_cast_chat_line(struct GameActor *caster, struct GameActor *target, int skill_id,
                        unsigned char quiet);

struct GameMode;
void game_mode_zc_useskill_ack(struct GameMode *mode, unsigned src_aid,
                                           unsigned target_aid, int x, int y, int skill_id, int property,
                                           unsigned delay_ms, unsigned char is_disposable, int attack_motion);

#endif
