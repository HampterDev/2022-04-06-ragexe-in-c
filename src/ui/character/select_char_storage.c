#include "ui/character/select_char_storage.h"
#include "std/container_memory.h"
#include "core/fatal.h"
#include "core/memory.h"
#include <string.h>

void select_char_init_storage(struct SelectCharStorage *self,
                                     int64_t usable_slots, int max_slot)
{
    int64_t slots = (int64_t)max_slot + 1;
    int64_t pages;
    int i;

    if (usable_slots < 0 || max_slot < -1)
        fatal_error("invalid character slot count");
    if (slots < usable_slots)
        slots = usable_slots;
    pages = slots / SELECT_CHAR_PER_PAGE + (slots % SELECT_CHAR_PER_PAGE != 0);
    if (pages > (SIZE_MAX / sizeof *self->previews) / SELECT_CHAR_PER_PAGE)
        fatal_error("too many character slots");
    self->usable_slots = self->listed_slots = (int)usable_slots;
    self->per_page = SELECT_CHAR_PER_PAGE;
    self->page_count = (int)pages;
    self->slot_count = self->per_page * self->page_count;
    memset(self->info_lines, 0, sizeof self->info_lines);
    memset(self->names, 0, sizeof self->names);
    memset(self->slots, 0, sizeof self->slots);
    self->previews = std_allocate(self->slot_count, sizeof *self->previews,
                                 SIZE_MAX / sizeof *self->previews);
    self->empty = std_allocate(self->slot_count, sizeof *self->empty, SIZE_MAX);
    self->headgear = std_allocate(self->slot_count, sizeof *self->headgear,
                                 SIZE_MAX / sizeof *self->headgear);
    for (i = 0; i < self->slot_count; i++) {
        char_preview_init(&self->previews[i]);
        self->empty[i] = 1;
    }
}

void select_char_destroy_storage(struct SelectCharStorage *self)
{
    int i;

    for (i = self->slot_count; i-- > 0;)
        char_preview_destroy(&self->previews[i]);
    mem_free(self->previews);
    mem_free(self->empty);
    mem_free(self->headgear);
    self->previews = NULL;
    self->empty = NULL;
    self->headgear = NULL;
    self->slot_count = 0;
}
