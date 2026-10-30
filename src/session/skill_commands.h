#ifndef SESSION_SKILL_COMMANDS_H
#define SESSION_SKILL_COMMANDS_H

struct Session;
struct GameActor;

void session_drop_skill(struct Session *self, int id);
void game_mode_zc_use_skill_(void *session, int skill, int level, struct GameActor *caster,
                              struct GameActor *target);

#endif
