#ifndef GAMEMODE_ACTOR_MARKS_H
#define GAMEMODE_ACTOR_MARKS_H

struct GameActor;
struct GameMode;
struct Vec3;

void game_mode_make_emblem_wnd(struct GameMode *self, struct GameActor *actor, int guild_id);

void *game_mode_make_emotion_effect(struct GameMode *self, const float *pos, int emotion,
                                 unsigned aid);

void *game_mode_make_miss_effect(struct GameMode *self, const float *pos, unsigned argb,
                              unsigned aid);

void *game_mode_make_msg_effect(struct GameMode *self, unsigned kind, const struct Vec3 *pos,
                             unsigned aid, unsigned char drawn);

void game_mode_make_word_display_wnd(struct GameMode *self, struct GameActor *actor);

#endif
