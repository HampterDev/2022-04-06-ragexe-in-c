#ifndef GAMEMODE_QUEST_MARKS_H
#define GAMEMODE_QUEST_MARKS_H

struct GameMode;
struct QuestMarkValue {
    short x, y;
    short effect, icon;
};

struct QuestMarkValue *game_mode_get_quest_mark(struct GameMode *self, unsigned npc_id);

void game_mode_send_quest_marks(struct GameMode *self);

#endif
