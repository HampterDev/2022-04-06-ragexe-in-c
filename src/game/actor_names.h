#ifndef GAMEMODE_ACTOR_NAMES_H
#define GAMEMODE_ACTOR_NAMES_H

struct ActorNameCard;
struct GameMode;
struct String;

struct String *game_mode_get_actor_name(struct GameMode *self, struct String *out, int aid);

struct ActorNameCard *game_mode_get_name_card(struct GameMode *self, unsigned aid);

#endif
