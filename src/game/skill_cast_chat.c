#include "game/skill_cast.h"
#include "game/skill_use.h"
#include "game/actor_names.h"
#include "web/name_card_types.h"
#include "skills/skill_info.h"
#include "session/state.h"
#include "session/party_queries.h"
#include "std/string.h"
#include <string.h>
#include "core/msg_str.h"
#include "std/std.h"

enum {
    MSG_CAST_SOMEONE = 0x644,
    MSG_CAST_BY_PARTY = 0x64c,
    MSG_CAST_ON = 0x658,
    MSG_CAST_SKILL = 0x659,
    CHAT_COLOR_OWN_CAST = 0xf,
    CHAT_COLOR_PARTY_CAST = 0x10,
};

void skill_cast_chat_line(struct GameActor *caster, struct GameActor *target, int skill_id,
                        unsigned char quiet)
{
    struct String line, name, skill;
    char buf[0x100];
    int is_party, color;

    if (quiet == 1)
        return;
    is_party = session_is_party_member_aid(&g_session1, caster->aid, 1) &&
               (unsigned)g_session1.ownAid != caster->aid;
    if ((unsigned)g_session1.ownAid != caster->aid && !is_party)
        return;

    memset(buf, 0, sizeof buf);
    color = CHAT_COLOR_OWN_CAST;
    string_init(&line);
    if (is_party) {
        const char *someone;

        color = CHAT_COLOR_PARTY_CAST;
        game_mode_get_actor_name(game_current_mode(), &name, caster->aid);
        someone = msg_str(MSG_CAST_SOMEONE);
        string_assign_cstr(&name, someone);
        _snprintf_s(buf, sizeof buf, (size_t)-1, msg_str(MSG_CAST_BY_PARTY),
                       string_cstr(&name));
        string_append_cstr(&line, buf);
        string_free(&name);
    }
    if (target) {
        unsigned size;

        game_mode_get_actor_name(game_current_mode(), &name, target->aid);
        size = name.size;
        if (size) {
            const char *text = string_cstr(&name);
            const char *hash = memchr(text, '#', size);

            if (hash && (unsigned)(hash - text) != (unsigned)-1) {
                unsigned at = (unsigned)(hash - text);

                string_resize(&name, at, 0);
                size = name.size;
            }
        }
        if (!size) {
            struct ActorNameCard *record = game_mode_get_name_card(game_current_mode(), target->aid);
            struct String other;

            string_init(&other);
            if (record->npc_title.size == 0)
                string_assign_cstr(&other, msg_str(MSG_CAST_SOMEONE));
            else
                string_copy(&other, &record->npc_title);
            string_assign(&name, string_cstr(&other), other.size);
            string_free(&other);
        }
        _snprintf_s(buf, sizeof buf, (size_t)-1, msg_str(MSG_CAST_ON), string_cstr(&name));
        string_append_cstr(&line, buf);
        string_free(&name);
    }
    {
        const char *text = skill_name((unsigned short)skill_id);

        string_init(&skill);
        string_assign_cstr(&skill, text);
    }
    _snprintf_s(buf, sizeof buf, (size_t)-1, msg_str(MSG_CAST_SKILL), string_cstr(&skill));
    string_append_cstr(&line, buf);
    if (line.size)
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (int)string_cstr(&line), 0xffff,
                              color, 0);
    string_free(&skill);
    string_free(&line);
}
