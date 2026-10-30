#ifndef UI_HOTKEYS_HOTKEYS_STD_H
#define UI_HOTKEYS_HOTKEYS_STD_H

#include "ui/ui_std.h"
#include <time.h>

struct String;
struct Tree;
struct TreeNode;

struct KeyCapBox {
    BYTE shown;
    int x, y;
    BYTE align;
    BYTE width;
    BYTE text_inset;
    BYTE height;
};
struct KeyCap {
    struct String label;
    struct KeyCapBox boxes[2];
};
struct KeyCap *key_cap_construct(struct KeyCap *self, struct String label,
                                           struct KeyCapBox box0, struct KeyCapBox box1);
void key_cap_destruct(struct KeyCap *self);
struct KeyCap *key_cap_assign(struct KeyCap *self, const struct KeyCap *other);

#endif
