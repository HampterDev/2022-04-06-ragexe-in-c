#include "core/thread_text.h"
#include "world/mode_actions.h"
#include "game/drag.h"
#include "platform/code_page.h"
#include "platform/language.h"
#include "platform/language_api.h"
#include "session/option_pairs.h"
#include "session/skill_queries.h"
#include "skills/skill_info.h"
#include "session/number_codec.h"
#include "session/character_name.h"
#include "contents/client_lua_calls.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "game/context.h"
#include "contents/connection_api.h"
#ifndef UI_GAME_H
#define UI_GAME_H

#include "world/tooltip.h"
#include "platform/client_config.h"

#include "ui/window.h"
#include "world/mode_manager.h"

#include "world/mode_types.h"
#include "world/mode_dispatch.h"

#define MODE_MSG_UI_COMMAND 0
#define MODE_MSG_DRAG_END 0x12

#define CURSOR_ARROW 0
#define CURSOR_HAND 2

struct Session;

struct Language;


struct Time {
    __time32_t t;
};
struct TimeRecord {
    WORD year;
    WORD month;
    WORD day;
    WORD hour;
    WORD minute;
    WORD second;
    DWORD fraction;
};
struct Time *time_construct(struct Time *self, __time32_t t);
BYTE time_is_past(struct Time *self);
struct TimeRecord *time_get_as_record(struct Time *self, struct TimeRecord *out);
void time_destruct(struct Time *self);

#endif
