#include "ui/cooldown.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "world/mode_dispatch.h"
#include "game/context.h"
#include "session/csession.h"
#include "session/session_options.h"

#include <string.h>

#define GUILD_SKILL_FIRST   10000
#define GUILD_SKILL_LAST    10019

#define MODE_SENDMSG(gm, msg, a, b, c, d) \
    mode_send_message((struct Mode *)(gm), (msg), (a), (b), (c), (d))
#define MSG_HOTKEY_CHANGED  0x139

static struct HotkeySlot *row_slot(struct Session *self, int row, int i)
{
    return (struct HotkeySlot *)((BYTE *)self + (row == 0 ? offsetof(struct Session, hotkeys0)
                                                         : offsetof(struct Session, hotkeys1)) + 7 * i);
}

static int paged(const struct Session *self, int row, int slot)
{
    int page, at;

    if (row == 0)
        page = self->hotkeyPage0;
    else if (row == 1)
        page = self->hotkeyPage1;
    else
        return slot;
    if (!page)
        return slot;
    at = slot + page * 9;
    return at < 0x24 ? at : at - 0x24;
}

void session_set_bar_hotkey(struct Session *self, BYTE isSkill, int id, int level,
                                     int slot, int row)
{
    int base = paged(self, row, self->shortenSkillPage * 9);
    int target = paged(self, row, slot);
    struct GameMode *gm = game_current_mode();
    struct HotkeySlot *s;
    int i;

    if (id == 0) {
        if (isSkill) {
            struct SkillCooldownNode *cd;
            struct CooldownTimerNode *t;

            for (cd = g_session1.skillDelays.head; cd != NULL; cd = cd->next) {
                for (t = cd->value.timers_bar.head; t != NULL; t = t->next)
                    if (t->value.slot == slot)
                        break;
                if (t != NULL) {
                    cooldown_timers_remove(&cd->value.timers_bar, t);
                    break;
                }
            }
        }
        if (row == 0 || row == 1) {
            s = row_slot(self, row, target);
            s->is_skill = 0;
            s->id = 0;
            s->level = 0;
        }
        MODE_SENDMSG(gm, MSG_HOTKEY_CHANGED, target, row, 0, 0);
        return;
    }
    for (i = base; i < base + 9; i++) {
        struct HotkeySlot have;
        struct SkillCooldownNode *cd;

        if (row != 0 && row != 1)
            continue;
        have = *row_slot(self, row, i);
        if (!(id == have.id && level == (short)have.level && isSkill == have.is_skill)) {
            if (skill_info_is_level_used(have.id) || id != have.id || isSkill != have.is_skill)
                continue;
        }
        for (cd = g_session1.skillDelays.head; cd != NULL; cd = cd->next)
            if (cd->value.skill_id == id)
                break;
        if (cd != NULL && skill_cooldown_running(&cd->value)) {
            int k = self->shortenSkillPage * 9 - base + i;
            struct CooldownTimerNode *t;

            for (t = cd->value.timers_bar.head; t != NULL; t = t->next)
                if (t->value.slot == k)
                    break;
            if (t != NULL)
                cooldown_timers_remove(&cd->value.timers_bar, t);
        }
        s = row_slot(self, row, i);
        s->is_skill = 0;
        s->id = 0;
        s->level = 0;
        MODE_SENDMSG(gm, MSG_HOTKEY_CHANGED, i, row, 0, 0);
    }
    if (row == 0 || row == 1) {
        s = row_slot(self, row, target);
        s->level = (WORD)level;
        s->is_skill = isSkill;
        s->id = id;
    }
    MODE_SENDMSG(gm, MSG_HOTKEY_CHANGED, target, row, 0, 0);
}

void session_clear_hotkey_tab0(struct Session *self)
{
    int i;

    for (i = 0; i < HOTKEY_SLOTS; i++) {
        self->hotkeys0[i].is_skill = 0;
        self->hotkeys0[i].id = 0;
        self->hotkeys0[i].level = 0;
    }
}

void session_clear_hotkey_tab1(struct Session *self)
{
    memset(self->hotkeys1, 0, sizeof self->hotkeys1);
}

void session_clear_shorten_skill_levels(struct Session *self)
{
    memset(self->shortenSkillLevels, 0, sizeof self->shortenSkillLevels);
}

void session_clear_quick_slots(struct Session *self)
{
    self->quickSlots[0].is_skill = 0;
    self->quickSlots[0].id = 0;
    self->quickSlots[0].level = 0;
    self->quickSlots[1].is_skill = 0;
    self->quickSlots[1].id = 0;
    self->quickSlots[1].level = 0;
}

void session_refresh_guild_skill_hotkeys(struct Session *self)
{
    int i;

    for (i = 0; i < HOTKEY_SLOTS; i++) {
        const struct HotkeySlot *slot = &self->hotkeys0[i];
        if (slot->id && slot->is_skill && (unsigned)(slot->id - GUILD_SKILL_FIRST) <= GUILD_SKILL_LAST - GUILD_SKILL_FIRST)
            session_set_bar_hotkey(self, 1, 0, 0, i, 0);
    }
    for (i = 0; i < HOTKEY_SLOTS; i++) {
        const struct HotkeySlot *slot = &self->hotkeys1[i];
        if (slot->id && slot->is_skill && (unsigned)(slot->id - GUILD_SKILL_FIRST) <= GUILD_SKILL_LAST - GUILD_SKILL_FIRST)
            session_set_bar_hotkey(self, 1, 0, 0, i, 1);
    }
}

struct HotKey *session_get_hot_key(struct Session *self, struct HotKey *out,
                                                 int index, int b)
{
    const char *text1 = NULL, *text2 = NULL;

    out->modifier = 0;
    out->code = 0;
    string_init(&out->label);
    string_init(&out->action);
    if (client_lua_call(g_session1.luaState, "GetHotKey", "dd>ddss", index + 1, b, &out->code, &out->modifier, &text1, &text2)) {
        string_assign_cstr(&out->label, text1);
        string_assign_cstr(&out->action, text2);
    }
    return out;
}
