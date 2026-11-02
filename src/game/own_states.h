#ifndef GAMEMODE_OWN_STATES_H
#define GAMEMODE_OWN_STATES_H
#include <stddef.h>
#include <stdint.h>

enum { OWN_STATE_NO_END = 999999 };
struct OwnStateEntry {
    int32_t index;
    uint32_t end_tick;
    int32_t value;
    int32_t total_ms;
    int32_t remain_ms;
};

size_t own_states_count(void);
const struct OwnStateEntry *own_states_at(size_t position);
const struct OwnStateEntry *own_states_find(int index);
int own_states_push(const struct OwnStateEntry *entry);
void own_states_erase_at(size_t position);
void own_states_remove(int low, int high);
int own_states_set_present(int index, int present);
void own_states_clear(void);
void own_states_destroy(void);

#endif
