#ifndef GAMEMODE_MINIMAP_H
#define GAMEMODE_MINIMAP_H

#include "game/packet_handlers.h"
#include "game/quest_marks.h"
#include "game/gamemode.h"
#include "game/client.h"

enum MinimapMarkKind {
    MARK_TEXTURE = 0,
    MARK_TRIANGLE = 1,
    MARK_SQUARE = 2,
    MARK_CROSS = 3,
    MARK_PLAYER_ARROW = 4,
    MARK_NAVI_TARGET = 5,
    MARK_NAVI = 6,
};

void game_mode_draw_minimap_triangle(float x, float y, float r, unsigned color);
void game_mode_draw_minimap_mark(struct GameMode *self, const struct TextureUV *uv, int x, int y, int size,
                                         struct String texture, int kind, unsigned color, char clamp);
void game_mode_draw_member_marks(struct GameMode *self, const struct TextureUV *uv);
void game_mode_erase_ping_mark(struct GameMode *self, unsigned id);
void game_mode_draw_minimap(struct GameMode *self);

struct PACKET_ZC_QUEST_NOTIFY_EFFECT;
struct PACKET_ZC_SEARCH_STORE_POS;

void game_mode_set_actor_mark_flag(struct GameMode *self, int aid, int flag);

#endif
