#include "actor/actor_ops.h"
#include "effects/rag/rag_effect.h"
#include "actor/object_ops.h"
#include "ui/window_access.h"
#include "render/device_palette.h"
#include "world/pick_node.h"
#include "effects/durations.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_dispatch.h"
#include "game/context.h"
#include "actor/actor.h"
#include "actor/liveness.h"
#include "std/list.h"
#include "ui/window_types.h"
#include "world/state.h"
#include "ui/character/gauge.h"
#include "web/name_card_types.h"
#include "actor/game_actor.h"
#include "core/msg_str.h"
#include "session/session_jobs.h"
#include "session/session_skills.h"
#include "ui/wndmgr/wndmgr.h"
#include "game/mode_callbacks.h"
#include "game/input.h"
#include "game/mode_msg.h"

static float g_actorBalloonScale;
static INIT_ONCE g_actorBalloonScaleGuard;

static void actor_forget_window(struct GameActor *actor, const struct UIWindow *wnd)
{
    struct UIWindow **const slots[] = {
        &actor->balloon_wnd, &actor->chatroom_title_wnd, &actor->shop_title_wnd,
        &actor->recharge_gage_wnd, &actor->buy_shop_title_wnd, &actor->monster_gage_wnd,
    };
    size_t i;

    for (i = 0; i < sizeof slots / sizeof slots[0]; i++)
        if (*slots[i] == wnd)
            *slots[i] = NULL;
}

static void actor_list_forget_window(const struct List *list, const struct UIWindow *wnd)
{
    const struct ListNode *node;

    for (node = list->head->next; node != list->head; node = node->next)
        actor_forget_window(*LIST_VALUE(node, struct GameActor *), wnd);
}

void actors_forget_window(const struct UIWindow *wnd)
{
    struct World *world = game_current_world();

    if (world == NULL)
        return;
    if (world->player)
        actor_forget_window((struct GameActor *)world->player, wnd);
    actor_list_forget_window(&world->actors, wnd);
    actor_list_forget_window(&world->skills, wnd);
}

static float balloon_top(const struct GameActor *self)
{
    return (float)self->screen_y - self->scale * g_actorBalloonScale;
}

void game_actor_process_chat_balloon_(struct GameActor *self)
{
    if (once_begin(&g_actorBalloonScaleGuard)) {
        g_actorBalloonScale = (float)(g_renderer->height * 81 / 480);
        once_complete(&g_actorBalloonScaleGuard);
    }

    if (self->balloon_wnd) {
        if (timeGetTime() >= self->balloon_tick + 5000) {
            ui_window_mgr_post_quit2(&g_windowMgr, self->balloon_wnd);
            self->balloon_wnd = NULL;
        } else {
            float top = balloon_top(self) - (float)self->balloon_height;
            int x = self->screen_x - self->balloon_width / 2;
            int y, max_x, max_y;
            struct UIWindow *balloon;

            if (x < -3)
                x = -3;
            balloon = self->balloon_wnd;
            max_x = g_renderer->width - balloon->width + 3;
            if (x <= max_x)
                max_x = x;
            y = (int)top;
            if (y < -3)
                y = -3;
            max_y = g_renderer->height - balloon->height + 3;
            if (y <= max_y)
                max_y = y;
            ui_window_move(balloon, max_x, max_y);
        }
    }
    if (self->chatroom_title_wnd)
        ui_window_move(self->chatroom_title_wnd, self->screen_x - 0x46, (int)(balloon_top(self) - 17.0f));
    if (self->shop_title_wnd)
        ui_window_move(self->shop_title_wnd, self->screen_x - 0x46, (int)(balloon_top(self) - 17.0f - 17.0f));
    if (self->recharge_gage_wnd) {
        unsigned now;

        timeGetTime();
        now = timeGetTime();
        if (now > self->cast_end_tick) {
            render_object_send(self, NULL, 0x53, 0, 0, 0, 0);
            if (self->state == 8)
                actor_set_state(self, 0, 1);
        } else {
            int total, done;

            ui_window_move(self->recharge_gage_wnd, self->screen_x - 0x1e, (int)balloon_top(self));
            total = (int)(self->cast_end_tick - self->cast_start_tick);
            if (total < 0)
                total = 1;
            done = (int)timeGetTime() + (total - (int)self->cast_end_tick);
            ui_recharge_gage_set_amount((struct UIRechargeGage *)self->recharge_gage_wnd, done <= total ? done : total, total);
        }
    }
    if (self->buy_shop_title_wnd)
        ui_window_move(self->buy_shop_title_wnd, self->screen_x - 0x46, (int)(balloon_top(self) - 17.0f - 17.0f));
}

void game_actor_process_effect_state(struct GameActor *self)
{
    struct RagEffect *effect;
    Vec3 zero = { 0, 0, 0 };

    if (g_isSkipFrame > 0 && self->drawn == 1) {
        self->overlay_flags = 0;
        self->drawn = 0;
    }
    if (effect_state_none(self->effect_state)) {
        self->efst_effect_frames = 0x8ca0;
        return;
    }
    if (effect_state_bit0(self->effect_state) && (unsigned)self->efst_effect_frames >= (unsigned)g_effectDurations[0x16]) {
        self->efst_effect_frames = 0;
        effect = game_actor_launch_effect(self, 0x16, zero, 0.0f);
        if (effect)
            rag_effect_send_msg(effect, NULL, 0x2c, (timeGetTime() >> 4) % 10 + 1, 0, 0);
    }
    if (effect_state_bit13(self->effect_state) && (unsigned)self->efst_effect_frames >= (unsigned)g_effectDurations[0x21]) {
        self->efst_effect_frames = 0;
        effect = game_actor_launch_effect(self, 0x21, zero, 0.0f);
        if (effect)
            rag_effect_send_msg(effect, NULL, 0x2c, (timeGetTime() >> 4) % 10 + 1, 0, 0);
    }
    self->efst_effect_frames++;
}

void game_actor_process_body_state(struct GameActor *self)
{
    if ((unsigned)(self->body_state - 1) <= 1)
        self->motion_held = 1;
}

void game_actor_place_hp_gage(struct GameActor *self)
{
    struct UIWindow *gage = self->monster_gage_wnd;
    int x, y;

    if (!gage)
        return;
    if (!g_session1.showMonsterHp || !ui_monster_gauge_two_bars(gage) || is_effect_state_burrow(self->effect_state)
        || is_effect_state_star2(self->effect_state) || effect_state_bit6(self->effect_state) || g_windowMgr.hide_ui_state == 2) {
        ui_window_set_show(self->monster_gage_wnd, 0);
        return;
    }
    ui_window_set_show(self->monster_gage_wnd, 1);
    if (self->state == 3 && ui_monster_gauge_hp(self->monster_gage_wnd))
        ui_pc_gage_set_hp((struct UIPcGage *)self->monster_gage_wnd, 0, ui_monster_gauge_max_hp(self->monster_gage_wnd));
    gage = self->monster_gage_wnd;
    y = (int)((float)scale_y_from480(12) * self->scale + (float)self->screen_y);
    x = self->screen_x - gage->width / 2;
    ui_window_move(gage, x, y);
    ui_window_invalidate(self->monster_gage_wnd);
}

#define GAME_MODE_SEND(mode, msg, p1, p2, p3, p4) \
    mode_send_message((struct Mode *)(mode), (msg), (p1), (p2), (p3), (p4))

static void name_cut(struct String *s)
{
    const char *text = string_cstr(s);
    const char *hash;
    size_t at;

    if (!s->size)
        return;
    hash = memchr(text, '#', s->size);
    if (!hash)
        return;
    at = (size_t)(hash - text);
    if (at == (size_t)-1)
        return;
    string_resize(s, at, 0);
}

static void name_fill(struct String *s, struct GameMode *mode, int aid)
{
    struct ActorNameCard *card = game_mode_get_name_card(mode, (unsigned)aid);
    struct String tmp;

    if (!card->npc_title.size) {
        const char *text = msg_str(0x644);

        string_init(&tmp);
        string_assign_cstr(&tmp, text);
    } else {
        string_copy(&tmp, &card->npc_title);
    }
    string_assign(s, string_cstr(&tmp), tmp.size);
    string_free(&tmp);
}

void game_actor_process_will_be_attacked(struct GameActor *self)
{
    unsigned now = timeGetTime();
    struct ListNode *node = self->hit_queue.head->next;

    while (node != self->hit_queue.head) {
        struct ActorHit *hit = LIST_VALUE(node, struct ActorHit);

        if (now < hit->tick) {
            node = node->next;
            continue;
        }
        if (hit->damage > 0) {
            int target = hit->target_aid, attacker = hit->attacker_aid;
            char own[0x1f4], party[0x1f4];
            struct String target_name, attacker_name;

            party[0] = 0;
            own[0] = 0;
            game_mode_get_actor_name(game_current_mode(), &target_name, target);
            game_mode_get_actor_name(game_current_mode(), &attacker_name, attacker);
            name_cut(&target_name);
            name_cut(&attacker_name);
            if (!target_name.size)
                name_fill(&target_name, game_current_mode(), target);
            if (!attacker_name.size)
                name_fill(&attacker_name, game_current_mode(), attacker);
            if (session_is_pc_job(&g_session1, game_actor_get_job(self))) {
                if (g_session1.ownAid == target)
                    sprintf(own, msg_str(0x645), string_cstr(&attacker_name), hit->damage);
                else if (session_is_party_member_aid(&g_session1, (unsigned long)target, 1))
                    sprintf(party, msg_str(0x646), string_cstr(&target_name), string_cstr(&attacker_name),
                                hit->damage);
            } else {
                if (g_session1.ownAid == attacker)
                    sprintf(own, msg_str(0x647), string_cstr(&target_name), hit->damage);
                else if (session_is_party_member_aid(&g_session1, (unsigned long)attacker, 1))
                    sprintf(party, msg_str(0x648), string_cstr(&attacker_name), string_cstr(&target_name),
                                hit->damage);
            }
            if (strlen(own))
                ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)own, 0xffff, 0xf, 0);
            if (strlen(party))
                ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)party, 0xc8c8ff, 0x10, 0);
            string_free(&attacker_name);
            string_free(&target_name);
        }
        render_object_send(self, NULL, hit->kind, MSG_ARG_PTR(hit), 0, 0, 0);
        node = list_erase(&self->hit_queue, node, NULL);
    }
}

void game_actor_set_unit_cell(struct GameActor * self)
{
    struct GameMode *mode = game_current_mode();
    int cx, cy;

    game_mode_world_to_cell(mode, self->pos.x, self->pos.z, &cx, &cy);
    GAME_MODE_SEND(mode, 0xc, self->aid, cx, cy, 0);
}

static int g_actorPickLeast;
static INIT_ONCE g_actorPickLeastGuard;

#define ACTOR_JOB_IS(fn, self) (fn(&g_session1, game_actor_get_job((self))))

static int is_merc_or_elemental(const struct GameActor *self)
{
    unsigned k = self->object_type - 9u;

    return k <= 1;
}

static float lift_of_other_job(void *session, struct GameActor *self)
{
    return session_is_homun_or_merc_job(session, game_actor_get_job(self)) ? 200.0f : 0.0f;
}

static float pick_lift(struct GameActor *self)
{
    struct Session *session = (struct Session *)(&g_session1);
    int same = !g_shiftDown == !g_skillAimGood;

    if (session_is_siege_mode(&g_session1)) {
        if (ACTOR_JOB_IS(session_is_pc_job, self)) {
            int ally = session_is_own_guild_id(session, game_actor_get_guild_id(self));

            if (same)
                return ally ? 100.0f : 400.0f;
            return ally ? 400.0f : 100.0f;
        }
        if (same) {
            if (session_get_pet_acc_act_name(session, game_actor_get_job(self)) == 1)
                return 200.0f;
            if (job_is_monster(game_actor_get_job(self)))
                return self->object_type != 7 ? 400.0f : 50.0f;
            return lift_of_other_job(session, self);
        }
    } else if (same) {
        if (ACTOR_JOB_IS(session_is_pc_job, self))
            return 100.0f;
        if (session_get_pet_acc_act_name(session, game_actor_get_job(self)) == 1)
            return 200.0f;
        if (job_is_monster(game_actor_get_job(self)))
            return self->object_type != 7 ? 400.0f : 50.0f;
        return lift_of_other_job(session, self);
    } else if (ACTOR_JOB_IS(session_is_pc_job, self)) {
        return 400.0f;
    }
    if (session_get_pet_acc_act_name(session, game_actor_get_job(self)) == 1)
        return 100.0f;
    if (job_is_monster(game_actor_get_job(self)))
        return self->object_type != 7 ? 200.0f : 50.0f;
    return lift_of_other_job(session, self);
}

static void pick_widen(float *lo, float *hi, int least)
{
    int size = (int)(*hi - *lo);

    if (size < least) {
        float d = (float)((least - size) / 2);

        *hi = *hi + d;
        *lo = *lo - d;
    }
}

void game_actor_set_render_info(struct RenderObject *object, const struct ScreenBounds *rect, float a,
                                          float b)
{
    struct GameActor *self = (struct GameActor *)object;
    struct PickInfo *pick;
    float lift;

    if (self->pickable) {
        if (once_begin(&g_actorPickLeastGuard)) {
            g_actorPickLeast = (int)(renderer_scale_x(g_renderer) * 40.0f);
            once_complete(&g_actorPickLeastGuard);
        }
        lift = pick_lift(self);
        if (is_merc_or_elemental(self))
            lift = 50.0f;
        if (self->not_pickable != 1) {
            if (rect->left == (float)g_renderer->width && rect->top == (float)g_renderer->height && rect->right == 0.0f
                && rect->bottom == 0.0f)
                return;
            pick = mem_alloc(sizeof *pick);
            if (pick)
                memset(pick, 0, sizeof *pick);
            pick->left = rect->left < rect->right ? rect->left : rect->right;
            pick->top = rect->top;
            pick->near_z = rect->z + lift;
            pick->right = rect->left > rect->right ? rect->left : rect->right;
            pick->bottom = rect->bottom;
            pick->far_z = rect->z + lift;
            pick->aid = self->aid;
            pick->job = game_actor_get_job(self);
            pick->attackable_as_pvp = self->pk_mode;
            if (self->object_type == 7)
                pick->kind = 3;
            else if (session_is_homun_or_merc_job(&g_session1, game_actor_get_job(self)))
                pick->kind = 4;
            else
                pick->kind = 0;
            if (is_merc_or_elemental(self))
                pick->kind = 4;
            pick_widen(&pick->left, &pick->right, g_actorPickLeast);
            pick_widen(&pick->top, &pick->bottom, g_actorPickLeast);
            actor_pick_node_insert(&g_actorPickNode, pick);
        }
    }
    render_object_set_screen_position(object, a, b);
}
