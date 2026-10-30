#include "game/own_states.h"
#include "game/client_state.h"
#include "replay/replay.h"
#include "resources/state_icon_sheet.h"

int own_states_restore(struct ReplayRecorder *recorder)
{
    struct ReplayBuffer chunk = {0};
    size_t i;
    int loaded = replay_read_chunk(recorder, 0x11, &chunk);
    if (loaded && chunk.size) {
        loaded = own_states_load(chunk.data, chunk.size) >= 0;
        if (loaded) {
            for (i = 0; i < own_states_count(); ++i) {
                const struct OwnStateEntry *entry = own_states_at(i);
                if (entry->end_tick != OWN_STATE_NO_END)
                    icon_sheet_set_timer(g_stateIconSheet, (unsigned)entry->index,
                        (unsigned)entry->total_ms, entry->end_tick - (uint32_t)entry->remain_ms,
                        (uint32_t)entry->total_ms - (uint32_t)entry->remain_ms);
            }
            g_mapPropertyQuiet = 1;
        }
    }
    replay_buffer_destroy(&chunk);
    return loaded;
}
