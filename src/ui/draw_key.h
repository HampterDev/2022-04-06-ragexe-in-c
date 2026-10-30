#ifndef UI_DRAW_KEY_H
#define UI_DRAW_KEY_H

#include <stddef.h>

struct UIWindow;
struct ItemInfo;
struct SkillInfo;
struct String;

#define DRAW_KEY_START 0xcbf29ce484222325ull

unsigned long long draw_key_bytes(unsigned long long key, const void *data, size_t size);
unsigned long long draw_key_int(unsigned long long key, int value);
unsigned long long draw_key_text(unsigned long long key, const char *text);
unsigned long long draw_key_string(unsigned long long key, const struct String *text);
unsigned long long draw_key_item(unsigned long long key, const struct ItemInfo *item);
unsigned long long draw_key_skill(unsigned long long key, const struct SkillInfo *skill);
unsigned long long draw_key_children(unsigned long long key, const struct UIWindow *window);

#endif
