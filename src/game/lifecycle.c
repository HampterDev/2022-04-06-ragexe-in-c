#include "std/list.h"
#include "game/lifecycle.h"
#include "game/mode_callbacks.h"
#include "world/mode_callbacks.h"
#include "game/gamemode.h"
#include <stdlib.h>
#include "std/vec.h"
#include "ui/ui_std.h"
#include "ui/wndmgr/wndmgr.h"

const struct ModeOps g_gameModeVtable_ops = {
    game_mode_on_update,
    game_mode_send_msg,
};

struct GameMode *game_mode_create(void)
{
    struct GameMode *self = malloc(sizeof *self);
    int i;

    if (!self)
        return NULL;
    mode_construct(&self->base);
    self->base.ops = &g_gameModeVtable_ops;
    string_init(&self->whisper_setting_name);

    string_init(&self->exchange_partner);
    name_card_holder_init(&self->actor_names);
    tree_init(&self->awaiting_name);
    tree_init(&self->unit_cells);
    list_init(&self->pickup_requests);
    tree_init(&self->chat_room_owners);
    tree_init(&self->square_marks);
    tree_init(&self->triangle_marks);
    tree_init(&self->ping_marks);
    self->popup_entries.first = self->popup_entries.last = self->popup_entries.end = 0;
    list_init(&self->shown_unit_ids);
    self->loop_sounds.first = self->loop_sounds.last = self->loop_sounds.end = 0;
    tree_init(&self->quest_marks);
    string_init(&self->popup_target_name);
    string_init(&self->whisper_text);
    string_init(&self->whisper_target);
    string_init(&self->drag.display_name);
    string_init(&self->drag.name);
    string_init(&self->drag.name2);
    string_init(&self->own_chat_room.title);
    string_init(&self->own_chat_room.password);
    self->skill_use.type = 0;
    self->skill_use.skill_id = 0;
    self->skill_use.range = 0;
    self->skill_use.level = 0;
    self->skill_use_unchecked = 0;
    string_init(&self->illust_file);
    string_init(&self->last_chat_line);
    for (i = 0; i < 11; i++)
        string_init(&self->chat_history[i]);
    self->boss_x = 0;
    self->boss_y = 0;
    list_init(&self->listed);
    self->own_double_aids.first = self->own_double_aids.last = self->own_double_aids.end = 0;
    self->world = 0;
    self->view = 0;
    self->camera_free = 0;
    self->name_balloon_pos = 0;
    self->preferences_loaded = 0;
    self->mouse_pointer = 0;
    self->replay_paused_at = 0;
    self->replay_drop_taken = 0;
    self->status_bars = 0;
    return self;
}

static void loop_sounds_free(struct Vec *v)
{
    struct LoopSound *sound;

    for (sound = (struct LoopSound *)v->first; sound != (struct LoopSound *)v->last; sound++)
        string_free(&sound->name);
    vec_free(v);
}

__declspec(noinline) void game_mode_destruct(struct GameMode *self)
{
    int i;

    vec_free(&self->own_double_aids);
    list_free(&self->listed, NULL);
    for (i = 11; i-- > 0;)
        string_free(&self->chat_history[i]);
    string_free(&self->last_chat_line);
    string_free(&self->illust_file);
    string_free(&self->own_chat_room.password);
    string_free(&self->own_chat_room.title);
    string_free(&self->drag.name2);
    string_free(&self->drag.name);
    string_free(&self->drag.display_name);
    string_free(&self->whisper_target);
    string_free(&self->whisper_text);
    string_free(&self->popup_target_name);
    tree_free(&self->quest_marks, NULL);
    loop_sounds_free(&self->loop_sounds);
    list_free(&self->shown_unit_ids, NULL);
    vec_free(&self->popup_entries);
    tree_free(&self->ping_marks, NULL);
    tree_free(&self->triangle_marks, NULL);
    tree_free(&self->square_marks, NULL);
    tree_free(&self->chat_room_owners, NULL);
    list_free(&self->pickup_requests, NULL);
    tree_free(&self->unit_cells, NULL);
    tree_free(&self->awaiting_name, NULL);
    name_card_holder_destroy(&self->actor_names);
    string_free(&self->exchange_partner);

    string_free(&self->whisper_setting_name);
    mode_destruct(&self->base);
}

void game_mode_destroy(struct Mode *mode)
{
    struct GameMode *self = (struct GameMode *)mode;
    game_mode_destruct(self);
    free(self);
}
