#ifndef UI_QUESTS_SIGNBOARD_LIST_H
#define UI_QUESTS_SIGNBOARD_LIST_H

#include <stddef.h>
struct ClientLua;

enum SignBoardKind { SIGNBOARD_PICTURE = 1, SIGNBOARD_ICON = 2, SIGNBOARD_BOARD = 3 };

struct SignBoardEntry {
    int x, y;
    int height;
    int type;
    const char *map;
    const char *text;
    const char *image;
    const char *colour;
};

int signboards_load(struct ClientLua *lua);
void signboards_clear(void);
void signboards_place(void);

size_t signboards_count(void);
const struct SignBoardEntry *signboards_entry(size_t index);

#endif
