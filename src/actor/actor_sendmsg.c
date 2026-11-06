#include "ui/window_types.h"
#include "actor/actor_ops.h"
#include "effects/rag/rag_effect.h"
#include "actor/object_ops.h"
#include "ui/window_access.h"
#include "ui/chat/readable_links.h"
#include "contents/sound.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "game/context.h"
#include "contents/connection_api.h"
#include "actor/actor.h"
#include "std/std.h"
#include "actor/game_actor.h"
#include "game/actor_entry.h"
#include "game/gamemode.h"
#include "resources/attr_types.h"
#include "ui/chat/balloon.h"
#include "effects/skills/skill_view.h"
#include "effects/skills/msg_effect.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_skills.h"
#include "session/session_state.h"
#include "ui/character/gauge.h"
#include "ui/chat/chat_room_wnd.h"
#include "ui/shops/shops.h"
#include "ui/wndmgr/wndmgr.h"

static const char deft_stab_wave[] = "effect\\ABC_DEFT_STAB2.wav";

static struct RagEffect *sm_launch(struct GameActor *self, int effect)
{
    Vec3 zero = { 0.0f, 0.0f, 0.0f };
    return game_actor_launch_effect(self, effect, zero, 0.0f);
}

static struct GameObject *sm_msg_effect(struct GameActor *self, struct GameMode *mode, int kind, int drawn)
{
    struct GameObject *obj = game_mode_make_msg_effect(mode, kind, &self->pos, self->aid, drawn);

    list_push_back(&self->attached, &obj, sizeof obj);
    render_object_send(obj, self, 0x32, 0, 0, 0, 0);
    return obj;
}

static void sm_hold(struct GameActor *self, struct GameMode *mode, int *slot, int kind)
{
    if (*slot == 0)
        *slot = (int)sm_msg_effect(self, mode, kind, 1);
}

static void sm_drop(int *slot)
{
    if (*slot == 0)
        return;
    render_object_send((void *)*slot, NULL, 0x35, 0, 0, 0, 0);
    *slot = 0;
}

static const int sm_cart_kinds[28] = {
    0x15, 0x1e, 0x01, 0x19, 0x10, 0x1a, 0x11, 0x1b, 0x12, 0x1c, 0x13, 0x1d, 0x2d, 0x2e,
    0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x41, 0x42,
};

static int sm_is_monster_job(int job)
{
    return job >= 0x2d && job < 0x3e8;
}

static void sm_hit_tail(struct GameActor *self)
{
    if (self->dies_after_hits && self->hit_queue.size <= 1)
        render_object_send(self, NULL, 0xc, 0, 0, 0, 0);
}

static void sm_hit(struct GameActor *self, struct GameMode *mode, long long msg, const struct ActorHit *info)
{
    int damage = info->damage;
    int src = info->attacker_aid;
    int effect = info->effect;
    int hit_effect = info->hit_effect;
    float motion = info->motion;
    struct World *world = mode->world;
    struct GameActor *attacker;
    int job, number, style;
    unsigned argb, extra_msg;
    long long weapon = -1;
    unsigned char heal = 0;

    if (info->cell_x != -1 && info->cell_y != -1) {
        game_mode_cell_to_world(mode, info->cell_x, info->cell_y, &self->slide_x, &self->slide_z);
    } else {
        self->slide_x = self->pos.x;
        self->slide_z = self->pos.z;
    }
    job = game_actor_get_job(self);
    if (g_session1.ownAid == src)
        attacker = (struct GameActor *)world->player;
    else
        attacker = world_get_game_actor_by_aid(world, src);
    if (attacker) {
        job = game_actor_get_job(attacker);
        weapon = actor_get_weapon(attacker);
    }
    if (msg == 0x5e)
        damage = 0;
    if (damage < 0 && damage != -29999) {
        damage = -damage;
        heal = 1;
    }

    if (damage == 0) {
        if (msg == 0x5e) {
            render_object_send(self, NULL, 0x5f, 0, 0, 0, 0);
        } else if (attacker) {
            if (sm_is_monster_job(job) || job_is_monster(job)) {
                if (self->aid == g_session1.ownAid
                    && (session_has_status(&g_session1, 0x13)
                        || (self->aid == g_session1.ownAid && session_has_status(&g_session1, 0x68)))) {
                    render_object_send(self, NULL, 0x7e, 0, 0, 0, 0);
                    sm_hit_tail(self);
                    return;
                }
                render_object_send(attacker, NULL, 0x59, 0xffffffffu, 0, 0, 0);
            } else {
                render_object_send(attacker, NULL, 0x59, 0xffff0000u, 0, 0, 0);
            }
        }
        sm_hit_tail(self);
        return;
    }

    if (g_session1.ownAid != src)
        self->miss_count = 0;
    else
        self->own_hit_damage = damage;
    self->hit_tick = timeGetTime();

    if (info->hit_effect == -2) {
        self->hit_flash = 0;
    } else {
        struct GameActor *player = (struct GameActor *)mode->world->player;
        const char *wave;

        self->hit_flash = 1;
        if ((unsigned long long)(msg - 0x60) <= 1) {
            wave = session_get_weapon_hit_wave(&g_session1, -1);
        } else if (session_is_pc_job(&g_session1, game_actor_get_job(self))) {
            wave = session_get_hit_wave_name(&g_session1, game_actor_get_job(self));
        } else {
            int w = (int)weapon;
            int aj = attacker ? game_actor_get_job(attacker) : 0;

            if (attacker && (aj == 0xc || aj == 0xfad || aj == 0xfc3 || aj == 0xfdb || aj == 0xfe1
                             || aj == 0x1005 || aj == 0x109e || aj == 0x10a4)) {
                if (w > 1000)
                    w = session_get_weapon_view_id(w);
            } else if (weapon > 1000) {
                w = session_get_weapon_view_id(w);
            }
            wave = session_get_weapon_hit_wave(&g_session1, w);
        }
        sound_play_wave(wave, self->pos.x - player->pos.x, 0.0f, self->pos.z - player->pos.z,
                        250, 40, 1.0f);
    }

    if (effect == 0x61f) {
        Vec3 at = self->pos;
        int dx = rand() % 10, dy = rand() % 10;
        struct Effect *e;

        switch (rand() % 4) {
        case 0: at.x = (float)dx + at.x; at.y = (float)dy + at.y; break;
        case 1: at.x = (float)dx + at.x; at.y = at.y - (float)dy; break;
        case 2: at.x = at.x - (float)dx; at.y = (float)dy + at.y; break;
        case 3: at.x = at.x - (float)dx; at.y = at.y - (float)dy; break;
        }
        e = (struct Effect *)effect_mgr_spawn(g_effectMgr, 0, 0x61f, at, 0, -1, 0, 0);
        e->ops->set_spin(e, (float)(rand() % 360));
    } else if (effect == 0x854) {
        struct GameActor *player = (struct GameActor *)game_current_player();

        if (player)
            sound_play_wave(deft_stab_wave, player->pos.x - self->pos.x, 0.0f,
                            player->pos.z - self->pos.z, 250, 40, 1.0f);
    } else {
        Vec3 zero = { 0.0f, 0.0f, 0.0f };

        if (!effect_mgr_spawn(g_effectMgr, self->aid, effect, zero, 0, -1, 0, 0)) {
            struct RagEffect *e = sm_launch(self, effect);

            if (e)
                rag_effect_send_msg(e, NULL, 0x2c, 1, 0, 0);
        }
    }

    if (hit_effect == 0x7a) {
        float roty = self->roty + 90.0f;

        self->roty = roty;
        if (roty > 360.0f)
            self->roty = roty - 360.0f;
    } else if (!skill_action_mgr_damage_hit(skill_action_mgr_instance(), info->skill_id, (SkillActor *)(attacker), (SkillActor *)(self))) {
        Vec3 zero = { 0.0f, 0.0f, 0.0f };

        if (!effect_mgr_spawn(g_effectMgr, self->aid, hit_effect, zero, 0, -1, 0, 0)) {
            struct RagEffect *e = sm_launch(self, hit_effect);

            if (e) {
                rag_effect_send_msg(e, NULL, 0x2c, 1, 0, 0);
                if ((hit_effect == 0xd8 || hit_effect == 0xd9) && attacker) {
                    int at = (int)&attacker->pos;

                    rag_effect_send_msg(e, NULL, 0xe, at, 0, 0);
                }
            }
        }
    }

    number = damage;
    extra_msg = 0;
    if (heal) {
        argb = 0x72;
        style = 0;
    } else if (msg == 0x5d) {
        extra_msg = 0x5c;
        style = 0x10;
        argb = 0xffffff00;
    } else if ((game_actor_get_job(self) >= 0x2d && game_actor_get_job(self) < 0x3e8)
               || job_is_monster(game_actor_get_job(self))) {
        number = info->value;
        style = 0x16;
        argb = 0xffffffff;
        if (msg == 0x81) {
            extra_msg = 0x83;
        } else if (msg == 0x84) {
        } else if (msg == 0x6f) {
            render_object_send(self, NULL, 0x58, (long long)damage, info->is_sp_damage ? 0xff1e90ffu : 0xffffffffu, 0x15, 0);
            argb = 0xffffff00;
        } else if (msg == 0x94) {
            render_object_send(self, NULL, 0x5c, 0, 0, 0, 0);
            render_object_send(self, NULL, 0x58, (long long)damage, 0xffffff00u, 0x10, 0);
            argb = 0xffffff00;
        } else {
            number = damage;
            argb = 0xffffffff;
            style = 0xe;
        }
    } else {
        argb = 0xffff0000;
        style = 0xe;
    }
    if (info->is_sp_damage)
        argb = 0xff1e90ff;
    if (extra_msg)
        render_object_send(self, NULL, extra_msg, 0, 0, 0, 0);
    render_object_send(self, NULL, 0x58, (long long)number, (long long)argb, (long long)(unsigned)style, 0);

    if (!heal) {
        int pc = session_is_pc_job(&g_session1, game_actor_get_job(self));

        if (self->state != 3 && !(pc && self->state == 2) && msg != 0x27 && msg != 0x61) {
            actor_set_state(self, 4, 1);
            if (motion > 1.0f)
                self->plays = motion;
            else
                render_object_set_motion_delay((struct RenderObject *)self, pc ? self->motion_delay * motion : motion);
        }
    }

    sm_hit_tail(self);
}

static struct UIWindow *sm_window(struct UIWindow **slot, struct UIWindow *made, int width, int height)
{
    if (*slot == NULL) {
        *slot = made;
        ui_window_create(made, width, height);
        ui_window_mgr_add_to_window_list(&g_windowMgr, *slot);
    }
    return *slot;
}

static void sm_close_window(struct UIWindow **slot)
{
    if (*slot) {
        ui_window_mgr_post_quit2(&g_windowMgr, *slot);
        *slot = NULL;
    }
}

static void sm_shop_title(struct GameActor *self, struct UIWindow **slot, int buying, const char *text)
{
    struct UIWindow *w = *slot;

    if (w == NULL)
        w = sm_window(slot, (struct UIWindow *)ui_merchant_shop_title_new(buying), 0x8c, 0x22);
    ui_window_send_msg(w, NULL, 0x22, (int)text, self->balloon_font, 0, 0);
    ui_window_send_msg(*slot, NULL, 0x1c, self->aid, 0, 0, 0);
    w = *slot;
    ui_window_move(w, self->screen_x - 0x46, self->screen_y - scale_y_from480(0x6e));
    if (!g_windowMgr.show_shop_titles)
        ui_window_set_show(*slot, 0);
}

static void sm_chat_balloon(struct GameActor *self, const char *text, long long par2)
{
    struct String s = {0};
    struct UIWindow *w;

    string_assign_cstr(&s, text);
    if (g_windowMgr.new_chat_wnd)
        ui_new_chat_wnd_readable_links(g_windowMgr.new_chat_wnd, &s, string_view(&s));
    navi_links_readable_links(&s, string_view(&s));
    if (s.size)
        text = string_cstr(&s);
    if (strlen(text) != 0) {
        self->balloon_tick = timeGetTime();
        w = self->balloon_wnd;
        if (w == NULL) {
            void *mem = mem_alloc(sizeof(struct UITransBalloonText));

            w = sm_window(&self->balloon_wnd, mem ? ui_trans_balloon_text_construct(mem) : NULL, 8, 8);
        }
        if (w) {
            struct UIBalloonText *balloon = (struct UIBalloonText *)w;

            balloon->font_code =
                self->aid == g_session1.ownAid ? (WORD)(short)((signed char)g_session1.font) : (WORD)self->balloon_font;
            ui_balloon_text_set_text(balloon, text, 0x23);
            if (par2) {
                balloon->colour = (COLORREF)par2;
                balloon->outline = 0;
            }
            self->balloon_width = self->balloon_wnd->width;
            self->balloon_height = self->balloon_wnd->height;
            w = self->balloon_wnd;
            ui_window_move(w, self->screen_x - self->balloon_width / 2, self->screen_y - scale_y_from480(0x6e));
        }
    }
    string_free(&s);
}

static void sm_spawn_or_launch(struct GameActor *self, int effect)
{
    if (!effect_mgr_spawn(g_effectMgr, self->aid, effect, self->pos, 0, -1, 0, 0))
        sm_launch(self, effect);
}

static void sm_level_effects(struct GameActor *self, int first, int second)
{
    if (g_session1.auraFull == 1)
        sm_spawn_or_launch(self, first);
    sm_spawn_or_launch(self, second);
}

static int sm_level_new(struct GameActor *self)
{
    self->aura_on = 1;
    if (g_session1.baseLevel == (signed char)self->aura_level || effect_state_bit6(self->effect_state)
        || is_effect_state_hiding(self->effect_state) || is_effect_state_cloaking(self->effect_state))
        return 0;
    return 1;
}

#define SM_PICK(var, id) do { if ((id) > 0) (var) = (id); } while (0)

static int sm_kind_has_owner(const struct GameActor *self)
{
    unsigned kind = self->object_type - 8u;

    return kind <= 6 && kind != 3 && kind != 4;
}

static int sm_walk(struct GameActor *self, const struct ActorMoveEntryInfo *info)
{
    if (!game_actor_find_path(self, info->src_x, info->src_y, info->sub_x, info->sub_y, info->dst_x, info->dst_y,
                             info->speed))
        return 0;
    self->cell_x = info->dst_x;
    self->cell_y = info->dst_y;
    actor_set_state(self, 1, 1);
    self->path_cell = 0;
    return 1;
}

static void sm_place(struct GameActor *self, struct GameMode *mode, const struct ActorEntryInfo *info)
{
    game_mode_cell_to_world(mode, info->x, info->y, &self->pos.x, &self->pos.z);
    self->pos.y = c3d_attr_get_height(mode->world->attr, self->pos.x,
                                     self->pos.z);
}

static void sm_appear_fields(struct GameActor *self, const struct ActorEntryInfo *info)
{
    self->roty = (float)((info->dir + 4) * 45);
    self->xsize = info->x_size;
    self->ysize = info->y_size;
    self->cell_x = info->x;
    self->cell_y = info->y;
}

static void sm_appear_stand(struct GameActor *self, struct GameMode *mode, const struct ActorEntryInfo *info)
{
    if (sm_kind_has_owner(self))
        self->owner_aid = info->char_id;
    sm_place(self, mode, info);
    sm_appear_fields(self, info);
    self->entry_head = info->head;
    game_actor_set_attr_state(self, info->body_state, info->effect_state, info->health_state, 0);
    actor_set_state(self, 0, 1);
    game_actor_set_guild(self, info->guild_id, info->emblem_version);
}

static void sm_appear_new(struct GameActor *self, struct GameMode *mode, struct ActorEntryInfo *info,
                          long long par2)
{
    struct ReplayRecorder *replay;

    if (sm_kind_has_owner(self))
        self->owner_aid = info->char_id;
    self->aid = info->aid;
    info->job = game_actor_state_job(info->job, info->effect_state);
    actor_set_job_model(self, info->job, info->sex);
    sm_place(self, mode, info);
    sm_appear_fields(self, info);
    self->walk_speed = info->speed;
    self->entry_head = info->head;
    game_actor_set_attr_state(self, info->body_state, info->effect_state, info->health_state, 0);
    replay = replay_recorder_instance();
    if (replay->state == REPLAY_STATE_STOPPED) {
        switch (info->state) {
        case 1: actor_set_state(self, 3, 1); break;
        case 2: actor_set_state(self, 6, 1); break;
        default: actor_set_state(self, 0, 1); break;
        }
    } else {
        actor_set_state(self, 0, 1);
    }
    self->pk_mode = info->is_pk_mode;
    if (info->job == 0x2d2 || info->job == 0x508)
        game_actor_set_guild(self, info->guild_id, info->emblem_version);
    actor_set_honor_virtue(self, info->honor, info->virtue);
    if (par2 == 1) {
        self->argb = 0x44ffffff;
    } else if (par2 == 2 && !effect_state_bit6(info->effect_state)) {
        sm_launch(self, 0x158);
    }
}

static void sm_moving_fields(struct GameActor *self, struct ActorMoveEntryInfo *info)
{
    info->job = game_actor_state_job(info->job, info->effect_state);
    self->entry_head = info->head;
    actor_set_job_model(self, info->job, info->sex);
    game_actor_set_attr_state(self, info->body_state, info->effect_state, info->health_state, 0);
    self->pk_mode = info->is_pk_mode;
    game_actor_set_guild(self, info->guild_id, info->emblem_version);
    actor_set_honor_virtue(self, info->honor, info->virtue);
}

static void sm_moving_new(struct GameActor *self, struct ActorMoveEntryInfo *info)
{
    struct GameMode *mode;
    struct C3dAttr *attr;

    if (sm_kind_has_owner(self))
        self->owner_aid = info->char_id;
    self->aid = info->aid;
    sm_moving_fields(self, info);
    self->walk_speed = info->speed;
    self->path_start = info->move_start_time;
    mode = game_current_mode();
    game_mode_cell_to_world(mode, info->src_x, info->src_y, &self->pos.x, &self->pos.z);
    attr = mode->world->attr;
    self->pos.x = (float)attr->zoom * 0.0625f * (float)(info->sub_x - 8) + self->pos.x;
    attr = mode->world->attr;
    self->pos.z = (float)attr->zoom * 0.0625f * (float)(info->sub_y - 8) + self->pos.z;
    self->pos.y = c3d_attr_get_height(mode->world->attr, self->pos.x, self->pos.z);
    if (!sm_walk(self, info))
        actor_set_state(self, 0, 1);
}

static void sm_moving(struct GameActor *self, struct ActorMoveEntryInfo *info)
{
    if (sm_kind_has_owner(self))
        self->owner_aid = info->char_id;
    self->walk_speed = info->speed;
    self->path_start = info->move_start_time;
    sm_moving_fields(self, info);
    sm_walk(self, info);
}

void game_actor_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct GameActor *self = (struct GameActor *)object;
    struct GameActor *sender = (struct GameActor *)source;

    struct GameMode *mode = game_current_mode();
    void *info = MSG_PTR(par1);
    unsigned char shown = 1;
    struct ListNode *node;
    int job;

    if (self->aid != g_session1.ownAid
        && (is_effect_state_hiding(self->effect_state) || is_effect_state_cloaking(self->effect_state)
            || effect_state_bit6(self->effect_state)))
        shown = 0;

    switch (msg) {
    case 0x00: {
        const struct ActorMove *move = info;

        self->path_start = move->start_tick;
        if (game_actor_find_path(self, move->src_x, move->src_y, move->sub_x, move->sub_y, move->dst_x, move->dst_y,
                                self->walk_speed)) {
            self->cell_x = move->dst_x;
            self->cell_y = move->dst_y;
            actor_set_state(self, 1, 1);
            self->path_cell = 0;
        } else {
            actor_set_state(self, 0, 1);
        }
        break;
    }
    case 0x03:
        sm_appear_stand(self, mode, info);
        break;
    case 0x04:
        sm_appear_new(self, mode, info, par2);
        break;
    case 0x05: case 0x7b:
        sm_moving(self, info);
        break;
    case 0x06: case 0x7a:
        sm_moving_new(self, info);
        break;
    case 0x07:
        sm_chat_balloon(self, (const char *)info, par2);
        break;
    case 0x0b:
        game_actor_msg_attack(self, MSG_PTR(par1));
        break;
    case 0x0c:
        if (self->state != 3)
            actor_set_state(self, 3, 1);
        break;
    case 0x13: {
        int sx, sy, ux, uy, speed = (int)par3;
        unsigned char ok, snap;

        self->path_start = session_get_server_time(&g_session1);
        game_mode_world_to_sub_cell(mode, self->pos.x, self->pos.z, &sx, &sy, &ux, &uy);
        if (speed == 0)
            speed = self->body_relocation ? 0x37 : 2;
        ok = game_actor_find_path(self, sx, sy, ux, uy, (int)par1, (int)par2, speed);
        snap = par4 == 0;
        self->cell_x = (int)par1;
        self->cell_y = (int)par2;
        if (ok) {
            struct GameMode *m = game_current_mode();

            game_mode_cell_to_world(m, (int)par1, (int)par2, &self->pos.x, &self->pos.z);
            self->pos.y = c3d_attr_get_height(m->world->attr, self->pos.x,
                                             self->pos.z);
            actor_clear_unit_cell(self);
            if (snap == 1) {
                self->slide_x = self->pos.x;
                self->slide_z = self->pos.z;
            }
            actor_set_state(self, 1, snap);
            self->path_cell = 0;
            if ((int)par3 == 0) {
                self->glide = 1;
            } else {
                self->glide = 0;
                self->motion_held = 1;
            }
        } else {
            if (g_session1.homun_command[3]) {
                g_session1.homun_command[1] = 0;
                g_session1.homun_command[2] = 0;
                g_session1.homun_command[3] = 0;
            }
            if (g_session1.merc_command[3]) {
                g_session1.merc_command[1] = 0;
                g_session1.merc_command[2] = 0;
                g_session1.merc_command[3] = 0;
            }
            actor_set_state(self, 0, snap);
        }
        break;
    }
    case 0x14:
        sm_launch(self, 7);
        break;
    case 0x15:
        if ((unsigned long long)par1 < 0x5d) {
            struct GameObject *obj;

            if (par1 == 0x22) {
                if (self->emotion_mute)
                    break;
                self->emotion_mute = (int)game_mode_make_emotion_effect(mode, &self->pos.x, 0x22, self->aid);
                list_push_back(&self->attached, &self->emotion_mute, sizeof self->emotion_mute);
                obj = (struct GameObject *)self->emotion_mute;
            } else {
                obj = game_mode_make_emotion_effect(mode, &self->pos.x, (int)par1, self->aid);
                list_push_back(&self->attached, &obj, sizeof obj);
            }
            render_object_send(obj, self, 0x32, 0, 0, 0, 0);
        } else {
            game_send_message(0x21, 0, 0, 0, 0);
        }
        break;
    case 0x1a: case 0x27: case 0x5d: case 0x5e: case 0x6f: case 0x81: case 0x84: case 0x94:
        sm_hit(self, mode, msg, info);
        break;
    case 0x60: case 0x61:
        self->hit_argb = 0xffff0000;
        if (((const struct ActorHit *)info)->hit_effect == 0xc6)
            self->hit_argb = 0xff042ba4;
        else if (((const struct ActorHit *)info)->hit_effect == 0xb8)
            self->hit_argb = 0xff505050;
        sm_hit(self, mode, msg, info);
        break;
    case 0x1b: {
        struct ActorHit *hit = info;

        if (par2) {
            if (hit->kind != 0x94)
                hit->kind = 0x6f;
            hit->value = (int)par2;
        }
        list_push_back(&self->hit_queue, hit, sizeof *hit);
        break;
    }
    case 0x82: {
        struct ActorHit *hit = info;

        if (par2) {
            hit->value = (int)par2;
            hit->kind = 0x81;
        }
        list_push_back(&self->hit_queue, hit, sizeof *hit);
        break;
    }
    case 0x85: {
        struct ActorHit *hit = info;

        if (par2) {
            hit->value = (int)par2;
            hit->kind = 0x84;
        }
        list_push_back(&self->hit_queue, hit, sizeof *hit);
        break;
    }
    case 0x1c:
        if (session_is_homunculus_aid(&g_session1, self->aid)) {
            if (g_windowMgr.homun_info_wnd)
                ui_window_mgr_post_quit1(&g_windowMgr, 0x71);
            if (g_windowMgr.homun_skill_list_wnd)
                ui_window_mgr_post_quit1(&g_windowMgr, 0x72);
            session_clear_homun_commands(&g_session1);
            g_session1.homun_ai_flags[0] = 0;
        }
        if (session_is_mercenary_aid(&g_session1, self->aid)) {
            if (g_windowMgr.mer_info_wnd)
                ui_window_mgr_post_quit1(&g_windowMgr, 0x7d);
            if (g_windowMgr.mer_skill_list_wnd)
                ui_window_mgr_post_quit1(&g_windowMgr, 0x7e);
            session_clear_merc_commands(&g_session1);
            g_session1.mercPresent = 0;
            break;
        }
        if (self->hit_queue.size == 0 || session_is_pc_job(&g_session1, game_actor_get_job(self)))
            actor_set_state(self, 3, 1);
        else
            self->dies_after_hits = 1;
        break;
    case 0x20: {
        struct UIWindow *w = self->chatroom_title_wnd;

        if (w == NULL)
            w = sm_window(&self->chatroom_title_wnd, (struct UIWindow *)ui_chat_room_title_new(), 0x8c, 0x22);
        ui_window_send_msg(w, NULL, 0x22, (int)par1, self->balloon_font, 0, 0);
        w = self->chatroom_title_wnd;
        ui_window_move(w, self->screen_x - 0x46, self->screen_y - scale_y_from480(0x6e));
        break;
    }
    case 0x21:
        sm_close_window(&self->chatroom_title_wnd);
        break;
    case 0x24: {
        struct RagEffect *e = sm_launch(self, 0x44);

        if (e)
            rag_effect_send_msg(e, (struct RenderObject *)self, 0x32, 0, 0, 0);
        break;
    }
    case 0x25:
        game_actor_skill_on_target(self, MSG_PTR(par1));
        break;
    case 0x26:
        game_actor_am_groundskill(self, MSG_PTR(par1));
        break;
    case 0x2b: {
        const struct ActorStates *states = info;

        game_actor_set_attr_state(self, states->body_state, states->effect_state, states->health_state, 1);
        self->pk_mode = states->pk_mode;
        break;
    }
    case 0x30: {
        struct ListNode *head = self->attached.head;
        int value;

        for (node = head->next; node != head; node = node->next)
            if (*LIST_VALUE(node, int) == (int)sender)
                break;
        if (node == head)
            break;
        value = *LIST_VALUE(node, int);
        if (self->cart == value) self->cart = 0;
        else if (self->falcon == value) self->falcon = 0;
        else if (self->emotion_mark == value) self->emotion_mark = 0;
        else if (self->frozen_mark == value) self->frozen_mark = 0;
        else if (self->curse_mark == value) self->curse_mark = 0;
        else if (self->sleep_mark == value) self->sleep_mark = 0;
        else if (self->stun_mark == value) self->stun_mark = 0;
        else if (self->msg_mark_1 == value) self->msg_mark_1 = 0;
        else if (self->msg_mark_2 == value) self->msg_mark_2 = 0;
        else if (self->emotion_mark_a == value) self->emotion_mark_a = 0;
        else if (self->emotion_mute == value) self->emotion_mute = 0;
        else if (self->cursor_mark == value) self->cursor_mark = 0;
        else if (self->lassitude_mark == value) self->lassitude_mark = 0;
        list_erase(&self->attached, node, NULL);
        break;
    }
    case 0x33: {
        int kind = self->mark_kind, plus;

        if (is_effect_state_hiding(self->effect_state) || is_effect_state_cloaking(self->effect_state) || self->cart)
            break;
        if (session_is_any_super_novice_job(&g_session1, game_actor_get_job(self)))
            kind = 0;
        plus = session_is_baby_job(&g_session1, game_actor_get_job(self)) ? 1 : 0;
        self->cart = (int)sm_msg_effect(self, mode, sm_cart_kinds[plus + kind * 2], 1);
        break;
    }
    case 0x34: case 0x8a:
        sm_drop(&self->cart);
        break;
    case 0x36:
        sm_close_window(&self->shop_title_wnd);
        break;
    case 0x37:
        sm_shop_title(self, &self->shop_title_wnd, 0, (const char *)info);
        break;
    case 0x38: sm_hold(self, mode, &self->emotion_mark, 2); break;
    case 0x39: sm_drop(&self->emotion_mark); break;
    case 0x3a:
        if (self->frozen_mark == 0)
            self->frozen_mark = (int)sm_msg_effect(self, mode, 3, shown);
        break;
    case 0x3b: case 0x6b: sm_drop(&self->frozen_mark); break;
    case 0x3d:
        self->cell_x = (int)par1;
        self->cell_y = (int)par2;
        game_mode_cell_to_world(mode, (int)par1, (int)par2, &self->pos.x, &self->pos.z);
        break;
    case 0x3e: sm_msg_effect(self, mode, 4, shown); break;
    case 0x3f: sm_msg_effect(self, mode, 5, 1); break;
    case 0x43: sm_hold(self, mode, &self->curse_mark, 6); break;
    case 0x44: sm_drop(&self->curse_mark); break;
    case 0x45:
        if (self->sleep_mark == 0) {
            self->sleep_mark = (int)sm_msg_effect(self, mode, 7, 1);
            actor_set_state(self, 0xa, 1);
        }
        break;
    case 0x46:
        if (self->sleep_mark) {
            render_object_send((void *)self->sleep_mark, NULL, 0x35, 0, 0, 0, 0);
            self->sleep_mark = 0;
            actor_set_state(self, self->resume_state, 1);
        }
        break;
    case 0x47: sm_hold(self, mode, &self->stun_mark, 8); break;
    case 0x48: sm_drop(&self->stun_mark); break;
    case 0x49: sm_hold(self, mode, &self->emotion_mark_a, 9); break;
    case 0x4a: sm_drop(&self->emotion_mark_a); break;
    case 0x4b: sm_hold(self, mode, &self->msg_mark_1, 0xa); break;
    case 0x4c: sm_drop(&self->msg_mark_1); break;
    case 0x4e: sm_hold(self, mode, &self->msg_mark_2, 0xb); break;
    case 0x4f: sm_drop(&self->msg_mark_2); break;
    case 0x51:
        self->attack_target_aid = (int)par1;
        break;
    case 0x52: {
        unsigned now;

        if (self->recharge_gage_wnd == NULL) {
            struct UIWindow *w;

            w = (struct UIWindow *)ui_recharge_gage_new();
            self->recharge_gage_wnd = w;
            ui_window_create(w, 0x3c, 6);
            ui_window_move(self->recharge_gage_wnd, self->screen_x - 0x1e, (int)((float)self->screen_y - self->scale * (float)(g_renderer->height * 0x51 / 480)));
            ui_window_mgr_add_to_window_list(&g_windowMgr, self->recharge_gage_wnd);
        }
        now = timeGetTime();
        self->cast_end_tick = (int)par1 + now;
        self->cast_start_tick = now;
        break;
    }
    case 0x53:
        if (self->recharge_gage_wnd) {
            ui_window_mgr_post_quit2(&g_windowMgr, self->recharge_gage_wnd);
            self->recharge_gage_wnd = NULL;
            if (self->aid == g_session1.ownAid) {
                if (g_session1.progressBarPending == 1) {
                    short packet = 0x2f1;
                    int len = rag_connection_get_packet_length(rag_connection_instance(), 0x2f1);

                    rag_connection_send_packet(rag_connection_instance(), len, &packet);
                }
                g_session1.progressBarPending = 0;
            }
        }
        break;
    case 0x58:
        if ((session_is_siege_mode(&g_session1) || session_get_battlefield_state(&g_session1))
            && !session_is_admin_option(&g_session1, 7))
            break;
        if (par1 == -29999)
            break;
        if (self->aid == g_session1.ownAid
            && (session_has_status(&g_session1, 0x22) || session_has_status(&g_session1, 0x15b)))
            game_actor_show_number(self, rand() % 0x55f1 + 10000, (unsigned)par2, (int)par3);
        else
            game_actor_show_number(self, (int)par1, (unsigned)par2, (int)par3);
        break;
    case 0x59:
        if (g_session1.makeMissEffect) {
            struct GameObject *obj = game_mode_make_miss_effect(mode, &self->pos.x, (unsigned)par1, self->aid);

            list_push_back(&self->attached, &obj, sizeof obj);
            render_object_send(obj, self, 0x32, 0, 0, 0, 0);
            self->miss_count++;
        }
        break;
    case 0x5c: sm_msg_effect(self, mode, 0xc, 1); break;
    case 0x5f: sm_msg_effect(self, mode, 0xd, 1); break;
    case 0x62:
        self->state = 0;
        self->slant_depth = 0;
        self->pickable = 1;
        game_complete_item_use();
        actor_set_state(self, 0, 1);
        break;
    case 0x63: {
        unsigned until;

        self->aid = -1;
        self->aura_level = 0;
        self->has_shadow = 0;
        until = timeGetTime() + 0x1fe;
        self->fade_until = until;
        if (until == 0)
            self->fade_until = 1;
        break;
    }
    case 0x65: {
        int kind;
        struct GameObject *obj;

        if (self->falcon)
            break;
        job = game_actor_get_job(self);
        switch (job) {
        case 0xfac: kind = 0x17; break;
        case 0xfc2: kind = 0x18; break;
        case 0xfd2: kind = 0x20; break;
        case 0xfd3: kind = 0x21; break;
        case 0xfd4: kind = 0x22; break;
        case 0xfd8: case 0xfde: case 0x1002: kind = 0x35; break;
        case 0x10a1: case 0x10b6: kind = 0x45; break;
        default: kind = 0xe; break;
        }
        obj = sm_msg_effect(self, mode, kind, 1);
        render_object_set_motion_delay((struct RenderObject *)obj, 6.0f);
        self->falcon = (int)obj;
        break;
    }
    case 0x66:
        if (self->falcon) {
            render_object_send((void *)self->falcon, NULL, 0x35, 0, 0, 0, 0);
            ((struct MsgEffect *)self->falcon)->drawn = 0;
            self->falcon = 0;
        }
        break;
    case 0x6c: {
        actor_set_state(self, 2, 1);
        self->attack_motion_count = (float)actor_get_attack_motion(self);
        actor_set_unit_cell(self);
        self->start_tick = timeGetTime();
        self->motion = 4;
        render_object_set_motion_delay((struct RenderObject *)self, 1.0f);
        self->action += game_actor_dir_of((struct GameObject *)self, self->roty);
        if (self->dies_after_hits && self->hit_queue.size <= 1)
            render_object_send(self, NULL, 0xc, 0, 0, 0, 0);
        break;
    }
    case 0x76:
        if (self->emotion_mute)
            game_remove_object((void *)self->emotion_mute);
        break;
    case 0x7d: {
        int first, second;

        if (!sm_level_new(self) || job_is_monster(game_actor_get_job(self)))
            break;
        if (game_actor_get_job(self) >= 0x52c && game_actor_get_job(self) <= 0x553)
            break;
        if (game_actor_get_job(self) >= 0x792 && game_actor_get_job(self) <= 0x7a3)
            break;
        if (game_actor_get_job(self) == 0x6c4)
            break;
        self->aura_level = (unsigned char)g_session1.baseLevel;
        first = g_session1.auras[0] > 0 ? g_session1.auras[0] : 0xc8;
        second = g_session1.auras[1] > 0 ? g_session1.auras[1] : 0x3d0;
        if (session_is_baby_job(&g_session1, game_actor_get_job(self))) {
            SM_PICK(first, g_session1.auras[2]);
            SM_PICK(second, g_session1.auras[3]);
        } else if (game_actor_get_job(self) > 0xfa0
                   || session_is_super_novice_job(&g_session1, game_actor_get_job(self))) {
            SM_PICK(first, g_session1.auras[4]);
            SM_PICK(second, g_session1.auras[5]);
        } else if (session_is_homunculus_job(&g_session1, game_actor_get_job(self))) {
            SM_PICK(first, g_session1.auras[6]);
            SM_PICK(second, g_session1.auras[7]);
        } else if (game_actor_get_job(self) >= 0x666) {
            break;
        }
        sm_level_effects(self, first, second);
        break;
    }
    case 0x7e: {
        Vec3 zero = { 0.0f, 0.0f, 0.0f };

        game_actor_launch_effect(self, 0x150, zero, 0.0f);
        break;
    }
    case 0x7f: {
        self->argb_locked = 1;
        if (self->saved_tint[0] == 0 && self->saved_tint[1] == 0 && self->saved_tint[2] == 0 && self->saved_tint[3] == 0) {
            self->saved_tint[0] = (unsigned char)(self->argb >> 24);
            self->saved_tint[1] = (unsigned char)(self->argb >> 16);
            self->saved_tint[2] = (unsigned char)(self->argb >> 8);
            self->saved_tint[3] = (unsigned char)(self->argb >> 0);
        }
        self->tint_alpha = 0xff;
        self->tint_rgb[0] = (unsigned char)par1;
        self->tint_rgb[1] = (unsigned char)par2;
        self->tint_rgb[2] = (unsigned char)par3;
        self->argb = ((unsigned)par1 << 16) | ((unsigned)par2 << 8) | (unsigned)par3 | 0xff000000;
        break;
    }
    case 0x80:
        memset(self->saved_tint, 0, sizeof self->saved_tint);
        self->argb_locked = 0;
        break;
    case 0x83: sm_msg_effect(self, mode, 0x16, 1); break;
    case 0x86: sm_hold(self, mode, &self->cursor_mark, 0x1f); break;
    case 0x87: sm_drop(&self->cursor_mark); break;
    case 0x89: {
        int kind;

        if (self->cart)
            break;
        job = game_actor_get_job(self);
        switch (job) {
        case 0xfd8: case 0xfde: case 0xff4: case 0xff5: kind = 0x23; break;
        case 0x1002: case 0x100f: kind = 0x2a; break;
        case 0x10a1: case 0x10b6: kind = 0x46; break;
        default: kind = 0; break;
        }
        if (kind)
            self->cart = (int)sm_msg_effect(self, mode, kind, 1);
        break;
    }
    case 0x8b:
        sm_launch(self, 0x316);
        break;
    case 0x8c:
        render_object_send(self, NULL, 0x6d, 0x316, 0, 0, 0);
        break;
    case 0x8d:
        sm_shop_title(self, &self->buy_shop_title_wnd, 1, (const char *)info);
        break;
    case 0x8e:
        sm_close_window(&self->buy_shop_title_wnd);
        break;
    case 0x90:
        if (session_is_pc_job(&g_session1, game_actor_get_job(self))) {
            struct RagEffect *e = sm_launch(self, 0xa6);
            void *angel;

            if (e)
                rag_effect_send_msg(e, NULL, 0x2c, 0xffff, 0xf3, 0x64);
            angel = effect_mgr_find_effect(g_effectMgr, self->aid, 0x474);
            if (angel) {
                Vec3 at = { 0.0f, 0.0f, 0.0f };

                game_mode_cell_to_world(game_current_mode(), (int)par1, (int)par2, &at.x,
                                     &at.z);
                at.y = self->pos.y;
                fallen_angel_effect_set_position(angel, at);
            }
        }
        render_object_send(self, NULL, 0x13, (long long)(int)par1, (long long)(int)par2, 0xf, 0);
        render_object_set_action((struct RenderObject *)self, self->base_action, 1);
        self->motion = 7;
        self->motion_held = 1;
        break;
    case 0x91: {
        struct UIWindow *w;

        if (par1 == -1 || par2 == -1) {
            sm_close_window(&self->monster_gage_wnd);
            break;
        }
        if (par3 == 1)
            sm_close_window(&self->monster_gage_wnd);
        w = self->monster_gage_wnd;
        if (w == NULL)
            w = sm_window(&self->monster_gage_wnd, (struct UIWindow *)ui_monster_gage_new(),
                          self->is_boss == 2 ? 0x78 : self->is_boss == 1 ? 0x3c : 0x5a, 5);
        ui_pc_gage_set_hp((struct UIPcGage *)w, (int)par1, (int)par2);
        w = self->monster_gage_wnd;
        ui_window_move(w, self->screen_x - w->width / 2,
                 (int)((float)scale_y_from480(0xc) * self->scale + (float)self->screen_y));
        break;
    }
    case 0x92: case 0x93: {
        int first = 0x3fe, second = 0x3ff;

        if (!sm_level_new(self))
            break;
        self->aura_level = (unsigned char)g_session1.baseLevel;
        if (session_is_homunculus_job(&g_session1, game_actor_get_job(self))) {
            first = 0x3d2;
            second = 0x3d3;
            SM_PICK(first, g_session1.auras[13]);
            SM_PICK(second, g_session1.auras[14]);
        } else if (msg == 0x93) {
            first = 0x3d2;
            second = 0x3d3;
            SM_PICK(first, g_session1.auras[9]);
            SM_PICK(second, g_session1.auras[10]);
        } else if (msg == 0x92) {
            SM_PICK(first, g_session1.auras[11]);
            SM_PICK(second, g_session1.auras[12]);
        }
        sm_level_effects(self, first, second);
        break;
    }
    case 0x96: {
        int first = 0x4ca, second = 0x4cb;

        if (!sm_level_new(self))
            break;
        self->aura_level = (unsigned char)g_session1.baseLevel;
        SM_PICK(first, g_session1.auras[15]);
        SM_PICK(second, g_session1.auras[16]);
        sm_level_effects(self, first, second);
        break;
    }
    case 0x97:
        self->aura_on = 1;
        if (g_session1.baseLevel == (signed char)self->aura_level || effect_state_bit6(self->effect_state) == 1
            || is_effect_state_hiding(self->effect_state) == 1 || is_effect_state_cloaking(self->effect_state) == 1)
            break;
        self->aura_level = (unsigned char)g_session1.baseLevel;
        sm_level_effects(self, g_session1.maxLevelEffect4th, g_session1.maxLevelEffect4thSub);
        break;
    case 0x98: sm_hold(self, mode, &self->lassitude_mark, 0x43); break;
    case 0x99: sm_drop(&self->lassitude_mark); break;
    case 0x9a: sm_hold(self, mode, &self->misfortune_mark, 0x44); break;
    case 0x9b: sm_drop(&self->misfortune_mark); break;
    case 0x9c: sm_hold(self, mode, &self->handicap_dots_mark, 9); break;
    case 0x9d: sm_drop(&self->handicap_dots_mark); break;
    case 0x6e:
        actor_effect_state(self, mode, par1, par2, par3, par4);
        break;
    default:
        able_to_make_effect_send_msg(object, source, msg, par1, par2, par3, par4);
        return;
    }
}
