#ifndef GAMEMODE_MODE_MISC_H
#define GAMEMODE_MODE_MISC_H

struct GameMode;
struct String;

unsigned char game_mode_carries_item(struct GameMode *self, int item_id);

struct String *game_mode_get_map_name(struct GameMode *self, struct String *out);

struct String *game_mode_location_text(struct GameMode *self, struct String *out);

void game_mode_add_world_object(struct GameMode *self, void *object);

char game_mode_has_resurrection_token(struct GameMode *self);

void game_mode_refresh_own_hp(struct GameMode *self);

char *game_mode_short_name(struct String name);

void game_mode_mouse_pointer_msg2(struct GameMode *self, int value);

#endif
