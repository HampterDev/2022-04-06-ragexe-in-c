#include "contents/champion_monster.h"
#include "contents/champion_monster_effects.h"
#include "ui/window_access.h"
#include "contents/sound.h"
#include "game/client_state.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "game/context.h"
#include "actor/actor.h"
#include "actor/pc.h"
#include "game/gamemode.h"
#include "session/session_party.h"
#include "session/session_skills.h"
#include "session/session_state.h"
#include "actor/game_actor.h"
#include "actor/attr_state.h"
#include "effects/skills/msg_effect.h"

int g_playerBlind = 0;

static int own_poison_screen_effect;
struct Player;

#define AS_SEND(self, msg, par1) render_object_send((self), NULL, (msg), (par1), 0, 0, 0)
#define AS_SEND0(self, msg) AS_SEND((self), (msg), 0)

static void as_play_wave(const struct GameActor *self, const struct RenderObject *player, const char *name)
{
    if (player)
        sound_play_wave(name, self->pos.x - player->pos.x, 0.0f, self->pos.z - player->pos.z, 250, 40,
                        1.0f);
}

static void as_play_wave_at_player(const struct GameActor *self, const char *name)
{
    as_play_wave(self, (const struct RenderObject *)game_current_player(), name);
}

static void as_launch(struct GameActor *self, int effect)
{
    Vec3 zero = { 0.0f, 0.0f, 0.0f };

    game_actor_launch_effect(self, effect, zero, 0.0f);
}

static void as_rgb(struct GameActor *self, unsigned r, unsigned g, unsigned b)
{
    self->argb = (self->argb & 0xff000000) | (r << 16) | (g << 8) | b;
}

static void as_hidden(struct GameActor *self)
{
    if (self->mark_kind > 0)
        AS_SEND0(self, 0x34);
    if (effect_state_bit4(self->effect_state))
        AS_SEND0(self, 0x66);
    if (session_is_guild_member_aid(&g_session1, self->aid) == 0)
        game_actor_drop_siege_window(self);
    if (self->word_display_wnd) {
        ui_window_mgr_post_quit2(&g_windowMgr, self->word_display_wnd);
        self->word_display_wnd = NULL;
    }
}

static void as_shown(struct GameActor *self)
{
    if (self->mark_kind > 0)
        AS_SEND0(self, 0x33);
    if (effect_state_bit4(self->effect_state))
        AS_SEND0(self, 0x65);
    game_actor_show_siege_emblem(self);
    if (self->aura_on)
        game_actor_show_level_aura(self);
}

#define AS_FORM(changed, is)                                                   \
    if (changed(self->effect_state, old_effect))                                    \
        AS_SEND0(self, is(self->effect_state) ? 0x69 : 0x6a)

static void as_mark_attached(struct GameActor *self, unsigned char value)
{
    struct ListNode *node;

    if (self->attached.size == 0)
        return;
    for (node = self->attached.head->next; node != self->attached.head; node = node->next)
        (*LIST_VALUE(node, struct MsgEffect *))->drawn = value;
}

void game_actor_set_attr_state(struct GameActor *self, int body, int effect, int health,
                                         unsigned char sound)
{
    int old_body = self->body_state;
    int old_effect = self->effect_state;
    const struct RenderObject *player;
    struct ListNode *node;
    int me, poisoned;

    self->body_state = body;
    self->effect_state = effect;
    self->health_state = health;
    player = (const struct RenderObject *)game_current_player();

    if (effect_state_bit0_changed(self->effect_state, old_effect) && effect_state_bit0(self->effect_state) && player)
        as_play_wave(self, player, "effect\\EF_Sight.wav");
    if (effect_state_bit13_changed(self->effect_state, old_effect) && effect_state_bit13(self->effect_state) && player)
        as_play_wave(self, player, "effect\\EF_Sight.wav");

    if (effect_state_bit1_changed(self->effect_state, old_effect)) {
        game_actor_stop_aura_effects(self);
        if (is_effect_state_burrow(self->effect_state)) {
            if (sound) {
                AS_SEND0(self, 0x3f);
                if (game_actor_get_job(self) != 0x19 && game_actor_get_job(self) != 0x107e)
                    as_launch(self, 0x10);
            }
            self->revealed = 0;
            if (self->aid != g_session1.ownAid && session_is_admin_option(&g_session1, 7) == 0) {
                self->has_shadow = 0;
                AS_SEND(self, 0x6d, 0x9f);
                AS_SEND0(self, 0x23);
            }
            as_hidden(self);
        } else {
            if (sound) {
                AS_SEND0(self, 0x3f);
                game_complete_item_use();
            }
            self->revealed = 1;
            if (self->aid != g_session1.ownAid)
                self->has_shadow = 1;
            as_shown(self);
        }
    }

    if (effect_state_bit2_changed(self->effect_state, old_effect)) {
        game_actor_stop_aura_effects(self);
        if (is_effect_state_star2(self->effect_state)) {
            self->visible = 0;
            self->revealed = 0;
            if (self->aid != g_session1.ownAid && session_is_admin_option(&g_session1, 7) == 0) {
                self->has_shadow = 0;
                AS_SEND0(self, 0x23);
                AS_SEND(self, 0x6d, 0x9f);
            }
            as_hidden(self);
        } else {
            self->visible = 1;
            self->revealed = 1;
            if (self->aid != g_session1.ownAid)
                self->has_shadow = 1;
            as_shown(self);
        }
    }

    if (!is_effect_state_burrow(self->effect_state) && !is_effect_state_star2(self->effect_state) &&
        !effect_state_bit6(self->effect_state)) {
        if (effect_state_bit4_changed(self->effect_state, old_effect))
            AS_SEND0(self, effect_state_bit4(self->effect_state) ? 0x65 : 0x66);
    }

    AS_FORM(effect_state_bit5_changed, effect_state_bit5);
    AS_FORM(effect_state_bit15_changed, effect_state_bit15);

    if (effect_state_bit6_changed(self->effect_state, old_effect)) {
        game_actor_stop_aura_effects(self);
        if (effect_state_bit6(self->effect_state)) {
            int shown = session_is_admin_option(&g_session1, 8);

            self->has_shadow = 0;
            if (shown) {
                self->visible = 1;
            } else {
                AS_SEND(self, 0x6d, 0x9f);
                self->visible = 0;
                as_mark_attached(self, self->aid == g_session1.ownAid);
            }
            as_hidden(self);
        } else {
            self->visible = 1;
            self->has_shadow = 1;
            if (!self->revealed && self->aid != g_session1.ownAid)
                self->has_shadow = 0;
            as_mark_attached(self, 1);
            as_shown(self);
        }
    }

    champion_monster_apply_effects(g_championMonster, game_actor_get_job(self), self, self->aid);

    if (old_body != self->body_state) {
        switch (old_body) {
        case 1:
            as_play_wave_at_player(self, "_stone_explosion.wav");
            game_complete_item_use();
            break;
        case 2:
            AS_SEND0(self, 0x3b);
            AS_SEND0(self, 0x3e);
            as_play_wave_at_player(self, "_frozen_explosion.wav");
            self->motion_finished = 1;
            self->motion_held = 0;
            self->argb_locked = 0;
            as_rgb(self, 0xff, 0xff, 0xff);
            game_complete_item_use();
            break;
        case 3:
            AS_SEND0(self, 0x48);
            game_complete_item_use();
            break;
        case 4:
            AS_SEND0(self, 0x46);
            game_complete_item_use();
            break;
        case 7:
            AS_SEND0(self, 0x8c);
            break;
        case 8:
            while (able_to_make_effect_is_effect_exist(self, 0x322))
                AS_SEND(self, 0x6d, 0x322);
            break;
        }
        switch (self->body_state) {
        case 1:
            self->motion_finished = 1;
            self->motion_held = 1;
            self->argb_locked = 1;
            as_rgb(self, 0x40, 0x40, 0x40);
            break;
        case 2:
            AS_SEND0(self, 0x3a);
            self->motion_finished = 1;
            self->motion_held = 1;
            self->argb_locked = 1;
            as_rgb(self, 0x00, 0x80, 0xff);
            if (sound)
                as_play_wave_at_player(self, "_stonecurse.wav");
            break;
        case 3:
            AS_SEND0(self, 0x47);
            if (sound)
                as_play_wave_at_player(self, "_stun.wav");
            break;
        case 4:
            AS_SEND0(self, 0x45);
            break;
        case 6:
            self->argb_locked = 1;
            as_rgb(self, 0x80, 0x80, 0x80);
            break;
        case 7:
            AS_SEND0(self, 0x8b);
            break;
        case 8:
            if (!able_to_make_effect_is_effect_exist(self, 0x322))
                as_launch(self, 0x322);
            break;
        }
    }

    me = self->aid == g_session1.ownAid;
    poisoned = 0;
    if (self->health_state & 0x80) {
        if (me)
            own_poison_screen_effect = 1;
        self->argb_locked = 1;
        as_rgb(self, 0xff, 0x32, 0xff);
        poisoned = 1;
    } else if (self->health_state & 0x01) {
        int colour_stays = 0;

        if (me)
            own_poison_screen_effect = 1;
        for (node = self->effects.head->next; node != self->effects.head; node = node->next) {
            struct RagEffect *rag = *LIST_VALUE(node, struct RagEffect *);

            if (rag && rag->effect_id == 0x32c)
                colour_stays = 1;
        }
        if (!colour_stays) {
            self->argb_locked = 1;
            as_rgb(self, 0xff, 0x80, 0xff);
        }
        poisoned = 1;
    }
    if (poisoned) {
        if (sound) {
            struct GameMode *mode = game_current_mode();
            struct RenderObject *player = (struct RenderObject *)(struct GameActor *)mode->world->player;

            as_play_wave(self, player, "_poison.wav");
            if (self->aid == g_session1.ownAid &&
                !able_to_make_effect_is_effect_exist((struct GameActor *)mode->world->player, 0x14f))
                as_launch(self, 0x14f);
        }
    } else if (me) {
        if (own_poison_screen_effect == 1)
            AS_SEND(self, 0x6d, 0x14f);
        own_poison_screen_effect = 0;
    }
    if (self->health_state & 0x02) {
        AS_SEND0(self, 0x43);
        self->argb_locked = 1;
        as_rgb(self, 0xc8, 0x32, 0x32);
        if (sound)
            as_play_wave_at_player(self, "_curse.wav");
    } else {
        AS_SEND0(self, 0x44);
    }
    if (self->health_state & 0x04) {
        if (sound)
            as_play_wave_at_player(self, "_silence.wav");
        AS_SEND0(self, 0x49);
    } else {
        AS_SEND0(self, 0x4a);
    }
    AS_SEND0(self, (self->health_state & 0x20) ? 0x4b : 0x4c);
    if ((self->health_state & 0x08) && sound)
        as_play_wave_at_player(self, "_confusion.wav");
    if (self->health_state & 0x10) {
        if (self->aid == g_session1.ownAid)
            g_playerBlind = 1;
        if (sound) {
            struct GameMode *mode = game_current_mode();
            struct RenderObject *player = (struct RenderObject *)(struct GameActor *)mode->world->player;

            as_play_wave(self, player, "_blind.wav");
            if (self->aid == g_session1.ownAid &&
                !able_to_make_effect_is_effect_exist((struct GameActor *)mode->world->player, 0x14e))
                as_launch(self, 0x14e);
        }
    } else if (self->aid == g_session1.ownAid) {
        if (g_playerBlind == 1)
            AS_SEND(self, 0x6d, 0x14e);
        g_playerBlind = 0;
    }
    if (self->body_state == 0)
        self->motion_held = 0;
    if (!(self->health_state & 0x83) && self->body_state != 2 && self->body_state != 1 && self->body_state != 6 &&
        game_actor_get_counter(self, 3) != 1) {
        self->argb_locked = 0;
        as_rgb(self, 0xff, 0xff, 0xff);
    }
    if (self->aid == g_session1.ownAid && !g_mapPropertyQuiet) {
        struct Player *player = game_current_player();
        if (player)
            player_refresh_status_effects(player);
    }
}
