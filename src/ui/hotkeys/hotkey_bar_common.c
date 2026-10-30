#include "ui/hotkeys/hotkey_bars.h"
#include "session/session_jobs.h"
#include "ui/render/graphic_setting.h"

struct SkillCooldownNode *hotkey_cooldown_of(int skill_id)
{
    int key = skill_id;

    if (session_is_homunculus_skill(&g_session1, key) || session_is_merc_or_elemental_skill(&g_session1, key))
        key = SKILL_SHARED_DELAY;
    return skill_cooldowns_find(&g_session1.skillDelays, key);
}

static void add_timer(struct CooldownTimerList *timers, const struct SkillCooldown *cd, const struct CooldownSlot *s)
{
    struct CooldownTimer timer;

    cooldown_timer_init(&timer, s->slot, s->x, s->y, cd->total_ms, cd->end_time - cd->total_ms, 0, 0);
    cooldown_timers_append(timers, &timer);
}

void hotkey_cooldowns_draw(struct UIWindow *wnd, struct CooldownSlotList *slots, int quick, int sheet_arg)
{
    struct SkillCooldownNode *node = g_session1.skillDelays.head;

    while (node != NULL) {
        struct SkillCooldown *cd = &node->value;
        struct CooldownTimerList *timers = quick ? &cd->timers_quick : &cd->timers_bar;
        struct CooldownSlotNode *s;
        struct CooldownTimerNode *t;

        if (!skill_cooldown_running(cd)) {
            struct SkillCooldownNode *next;

            next = skill_cooldowns_remove(&g_session1.skillDelays, node);
            node = next;
            continue;
        }
        for (s = slots->head; s != NULL; s = s->next) {
            int id = s->value.skill_id;

            if (cd->skill_id == id || (cd->skill_id == SKILL_POST_DELAY && !session_is_homunculus_skill(&g_session1, id))) {
                if (cooldown_timers_find(timers, s->value.slot) == NULL)
                    add_timer(timers, cd, &s->value);
            }
            if ((session_is_homunculus_skill(&g_session1, id) || session_is_merc_or_elemental_skill(&g_session1, id))
                && cd->skill_id == SKILL_SHARED_DELAY) {
                if (cooldown_timers_find(timers, s->value.slot) == NULL)
                    add_timer(timers, cd, &s->value);
            }
        }
        for (t = timers->head; t != NULL; t = t->next)
            icon_sheet_queue(g_stateIconSheet, 0, 0, 0xc, 0xc, &t->value, sheet_arg, wnd->x, wnd->y);
        node = node->next;
    }
}

void hotkey_place_describe(struct UIWindow *wnd, struct UIWindow *describe, int x, int y)
{
    int sx = 0, sy = 0;
    int over = 0;
    int row = y / SHORTCUT_ROW_HEIGHT;

    ui_window_to_screen(wnd, &sx, &sy);
    if ((unsigned)(g_displayWidth - (x + sx)) < (unsigned)describe->width)
        over = describe->width - g_displayWidth + x + sx;
    if ((float)sy > (float)(double)(unsigned)g_displayHeight * 0.5f)
        ui_window_move(describe, sx - over + x, row * SHORTCUT_ROW_HEIGHT - describe->height + sy);
    else
        ui_window_move(describe, sx - over + x, sy + (row + 1) * SHORTCUT_ROW_HEIGHT);
}
