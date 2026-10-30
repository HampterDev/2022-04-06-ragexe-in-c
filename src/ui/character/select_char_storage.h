#ifndef UI_CHARACTER_SELECT_CHAR_STORAGE_H
#define UI_CHARACTER_SELECT_CHAR_STORAGE_H

#include "ui/character/char_preview.h"
#include <stdint.h>

#define SELECT_CHAR_PER_PAGE 15
#define SELECT_CHAR_INFO_LINES 12

struct UIWindow;
struct SelectCharStorage {
    struct UIWindow *info_lines[SELECT_CHAR_INFO_LINES];
    struct UIWindow *names[SELECT_CHAR_PER_PAGE];
    struct UIWindow *slots[SELECT_CHAR_PER_PAGE];
    struct CharPreview *previews;
    unsigned char *empty;
    short *headgear;
    int slot_count, page_count, per_page;
    int usable_slots, listed_slots;
};

void select_char_init_storage(struct SelectCharStorage *storage,
                             int64_t usable_slots, int max_slot);
void select_char_destroy_storage(struct SelectCharStorage *storage);

#endif
