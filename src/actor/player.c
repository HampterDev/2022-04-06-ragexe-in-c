#include "ui/window_types.h"
#include "actor/actor_ops.h"
#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "navi/navi_manager.h"
#include "contents/sound.h"
#include "game/client_state.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "world/mode_dispatch.h"
#include "ui/window_access.h"
#include "game/context.h"
#include "actor/actor.h"
#include "actor/pc.h"
#include "actor/player.h"
#include "std/std.h"
#include "actor/sprite_render.h"
#include "game/gamemode.h"
#include "render/render_api.h"
#include "session/session_jobs.h"
#include "session/session_skills.h"
#include "session/session_state.h"
#include "session/session_talk.h"
#include "game/input.h"
#include "game/net_clock.h"
#include "platform/clock.h"
#include "effects/skills/msg_effect.h"

void player_process_costume_body(struct Player *self, unsigned char show);

static void player_send_owed_5b(struct Player *self)
{
    int a, b;

    if (!self->owed_walk || (self->state != 2 && self->state != 5 && self->state != 6))
        return;
    a = self->owed_walk_x;
    b = self->owed_walk_y;
    self->owed_walk = 0;
    self->owed_walk_x = -1;
    self->owed_walk_y = -1;
    render_object_send(self, NULL, 0x5b, a, b, 0, 0);
}

#define MODE_SEND(mode, msg, a, b, c) \
    mode_send_message((struct Mode *)(mode), (msg), (a), (b), (c), 0)

unsigned char player_on_process(struct GameObject *object)
{
    struct Player *self = (struct Player *)object;

    struct GameActor *actor = (struct GameActor *)self;
    struct ActorMove req;
    struct UIWindow *wnd;
    unsigned char result;
    struct GameMode *mode;

    game_actor_process_body_state(actor);
    game_actor_process_effect_state(actor);
    game_actor_process_will_be_attacked(actor);
    game_actor_process_chat_balloon_(actor);
    if (self->hit_flash && platform_ticks_ms() > self->hit_tick + 100) {
        self->hit_argb = 0xffff0000;
        self->hit_flash = 0;
    }
    if (self->argb != 0xffffffff)
        render_object_modify_argb((struct RenderObject *)self, 8, 8, 8, 8);
    mode = game_current_mode();
    if (self->move_request_pending && platform_ticks_ms() > self->move_request_tick + (g_session1.averagePing >> 1)) {
        game_mode_world_to_sub_cell(mode, self->pos.x, self->pos.z, &req.src_x, &req.src_y, &req.sub_x, &req.sub_y);
        req.start_tick = session_get_server_time(&g_session1);
        req.dst_x = self->requested_cell_x;
        req.dst_y = self->requested_cell_y;
        render_object_send(self, NULL, 0, (intptr_t)&req, 0, 0, 0);
    }
    /* The walk the player asked for since the last frame leaves now, and is in the predicted
       world and drawn in this frame (game/own_body.c). Of several clicks of one frame the
       newest is the one that counts. Nothing spaces the requests: a click is not held back
       for the one before it (docs/custom-review.md, "The own walk is drawn from the
       predicted world"). */
    if (self->asked_walk) {
        self->asked_walk = 0;
        MODE_SEND(mode, 0x8a, self->asked_walk_x, self->asked_walk_y, 0);
    }
    result = actor_process_state(self);
    render_object_process_sound((struct RenderObject *)self);
    if (self->wave_pending) {
        if (game_current_player())
            sound_play_wave((const char *)self->wave_name, 1.0f);
        self->wave_pending = 0;
    }
    player_process_costume_body(self, game_actor_get_transform_job(actor) > 0);
    if (g_session1.cameraInfoShown) {
        wnd = ui_window_find(0x121);
        if (wnd)
            ui_window_invalidate(wnd);
    }
    return result;
}

void player_set_state(struct GameActor * base_actor, int state)
{
    struct Player * self = (struct Player *)base_actor;

    if (self->state == 3)
        return;
    if (state != 1) {
        self->requested_cell_x = -1;
        self->requested_cell_y = -1;
    }
    player_send_owed_5b(self);
    if (state == 3) {
        struct GameMode *mode = game_current_mode();

        mode_send_message((struct Mode *)mode, 5, 0, 0, 0, 0);
        game_mode_launch_dummy_effect(mode, &self->pos, 0x174, 0x12c, 0);
        g_wobbleFramesLeft = 0;
    }
    pc_set_state((struct GameActor *)self, state);
}

unsigned char player_process_state(struct GameActor * base_actor)
{
    struct Player * self = (struct Player *)base_actor;

    return pc_process_state((struct GameActor *)self);
}

#define MODE_SEND1(mode, a, b) \
    mode_send_message((struct Mode *)(mode), (a), (b), 0, 0, 0)

static void player_msg(struct Player *self, int msg)
{
    render_object_send(self, NULL, msg, 0, 0, 0, 0);
}

static void player_walk_msg(struct Player *self, int x, int y)
{
    render_object_send(self, NULL, 0x5b, x, y, 0, 0);
}

/* Talkie Box and Graffiti take a text: the ground skill's window asks for it, and sends the skill
   with it (CZ_USE_SKILL_TOGROUND_WITHTALKBOX2, game_mode_msg_use_skill_to_ground_talkbox). That
   request is the fork's, and no intent: the player is walked into reach for neither. */
enum { SKILL_HT_TALKIEBOX = 0x7d, SKILL_RG_GRAFFITI = 0xdc, WND_TALKBOX_TRAP_INPUT = 0x54, WND_GRAFFITI_STRBOX = 0x65 };
enum { SKILL_TEXT_MSG_SET_CELL = 0x44 };

static int player_ground_text_skill(int skill_id)
{
    return skill_id == SKILL_HT_TALKIEBOX || skill_id == SKILL_RG_GRAFFITI;
}

static void player_ask_ground_text(struct Player *self)
{
    struct GroundSkillUse cast;
    struct UIWindow *wnd;

    cast.skill_id = self->skill_id;
    cast.level = self->skill_level;
    cast.x = self->skill_cell_x;
    cast.y = self->skill_cell_y;
    wnd = ui_window_open(self->skill_id == SKILL_HT_TALKIEBOX ? WND_TALKBOX_TRAP_INPUT : WND_GRAFFITI_STRBOX);
    if (wnd)
        ui_window_send(wnd, NULL, SKILL_TEXT_MSG_SET_CELL, (intptr_t)&cast, 0, 0, 0);
}

static int player_burrow_skill(int skill, int hi, int with_250)
{
    if (hi)
        return 0;
    return skill == 0x89 || skill == 0xd4 || skill == 0x211 || skill == 0x212 || (with_250 && skill == 0x250);
}

static void player_msg_turn(struct Player *self, struct GameMode *mode, int x, int y)
{
    float fx, fz, toward, roty;
    int deg, side, head, dir;

    if (self->state != 0 && self->state != 4 && self->state != 6)
        return;
    game_mode_cell_to_world(mode, x, y, &fx, &fz);
    toward = render_object_calc_dir(self->pos.x, self->pos.z, fx, fz);
    toward = (float)((game_mode_angle_to_direction((float)((double)toward + 22.5)) + 4) * 45);
    deg = (int)(self->roty - toward);
    if (deg < 0)
        deg += 360;
    side = deg / 45;
    if (side < 0)
        side += 8;
    else if (side >= 8)
        side -= 8;
    roty = toward;
    if (side == 0 || side == 4) {
        self->head_dir = 0;
        self->roty = roty;
    } else if (side == 1) {
        if (self->head_dir == 1) {
            self->head_dir = 0;
            self->roty = roty;
        } else {
            self->head_dir = 1;
        }
    } else if (side == 2 || side == 3) {
        self->head_dir = 1;
        self->roty = roty + 45.0f;
        if (self->roty >= 360.0f)
            self->roty -= 360.0f;
    } else if (side == 7) {
        if (self->head_dir == 2) {
            self->head_dir = 0;
            self->roty = roty;
        } else {
            self->head_dir = 2;
        }
    } else {
        roty -= 45.0f;
        self->head_dir = 2;
        self->roty = roty;
        if (0.0f > roty)
            self->roty = roty + 360.0f;
    }
    head = self->head_dir;
    self->motion = head;
    dir = game_mode_angle_to_direction(self->roty);
    self->roty = (float)((dir + 4) * 45);
    MODE_SEND(mode, 0x8b, game_mode_angle_to_direction(self->roty), head, 0);
}

void player_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct Player *self = (struct Player *)object;

    struct GameMode *mode = game_current_mode();
    int p1 = (int)par1, p1_hi = (int)(par1 >> 32), p2 = (int)par2, p2_hi = (int)(par2 >> 32), p3 = (int)par3;
    int st;

    /* A swing announced while a move of the player is under way to the server: that move
       cancels it there, so it is not shown here. */
    if (msg == ACTOR_MSG_SWING_BEGIN && game_mode_own_body_walk_waits())
        return;
    if (msg < 0 || msg > 0x95)
        {
            pc_send_msg(object, source, msg, par1, par2, par3, par4);
            return;
        }
    switch ((int)msg) {
    case 0x8:
        actor_set_state(self, 0);
        break;
    case 0x9:
        self->move_request_pending = 1;
        self->move_request_tick = platform_ticks_ms();
        self->requested_cell_x = p1;
        self->requested_cell_y = p2;
        break;
    case 0xa:
        self->asked_walk = 0;
        self->intent_skill = 0;
        self->target_aid = p1;
        self->intent_hold = p2 == 1;
        game_mode_send_intent(mode, INTENT_ATTACK, p2, (unsigned)p1, 0, 0, 0, 0);
        break;
    case 0xd:
        self->asked_walk = 0;
        self->intent_skill = 0;
        self->intent_hold = 0;
        MODE_SEND(mode, 0x1a, 0, 0, 0);
        break;
    case 0xf:
        if (self->intent_hold) {
            self->intent_hold = 0;
            game_mode_send_intent(mode, INTENT_RELEASE, 0, 0, 0, 0, 0, 0);
        }
        break;
    case 0x10:
        player_msg_turn(self, mode, p1, p2);
        break;
    case 0x11:
        player_msg(self, 0xd);
        st = self->state;
        /* In states 5 and 6 the walk waits for the state to end (owed_walk). During a swing it
           goes out now: the server cancels the swing or lets it land. */
        if (st == 5 || st == 6) {
            self->owed_walk = 1;
            self->owed_walk_x = p1;
            self->owed_walk_y = p2;
        } else {
            self->owed_walk = 0;
            self->owed_walk_x = -1;
            self->owed_walk_y = -1;
            player_walk_msg(self, p1, p2);
        }
        break;
    case 0x12:
        if (is_effect_state_hiding(self->effect_state))
            break;
        self->asked_walk = 0;
        self->intent_skill = 0;
        self->intent_hold = 0;
        self->target_aid = p1;
        game_mode_send_intent(mode, INTENT_PICKUP, 0, (unsigned)p1, 0, 0, 0, 0);
        break;
    case 0x1e:
    case 0x1f:
        if (is_effect_state_hiding(self->effect_state))
            break;
        if (self->state == 0)
            MODE_SEND(mode, 0x89, 2, self->target_aid, 0);
        else if (self->state == 6)
            MODE_SEND(mode, 0x89, 3, self->target_aid, 0);
        break;
    case 0x29:
        if (is_effect_state_hiding(self->effect_state) && !player_burrow_skill(p2, p2_hi, 1))
            break;
        /* The intent goes out when the level arrives (0x5a). */
        self->intent_skill = INTENT_SKILL_ACTOR;
        self->intent_hold = 0;
        self->target_aid = p1;
        self->skill_id = p2;
        self->asked_walk = 0;
        break;
    case 0x3c:
        /* the target is out of reach: the server brings the player into reach, the client does not chase */
        break;
    case 0x41:
        if (is_effect_state_hiding(self->effect_state) && !player_burrow_skill(p1, p1_hi, 0))
            break;
        self->intent_skill = INTENT_SKILL_GROUND;
        self->intent_hold = 0;
        self->skill_id = p1;
        self->skill_cell_x = p2;
        self->skill_cell_y = p3;
        self->target_aid = 0;
        self->asked_walk = 0;
        break;
    case 0x5a:
        self->skill_level = p1;
        if (self->intent_skill == INTENT_SKILL_ACTOR)
            game_mode_send_intent(mode, INTENT_SKILL_ACTOR, 0, (unsigned)self->target_aid, 0, 0, self->skill_id,
                                  self->skill_level);
        else if (self->intent_skill == INTENT_SKILL_GROUND && player_ground_text_skill(self->skill_id))
            player_ask_ground_text(self);
        else if (self->intent_skill == INTENT_SKILL_GROUND)
            game_mode_send_intent(mode, INTENT_SKILL_GROUND, 0, 0, self->skill_cell_x, self->skill_cell_y,
                                  self->skill_id, self->skill_level);
        self->intent_skill = 0;
        break;
    case 0x5b:
        /* kept, the newest winning: it goes out with the frame (player_on_process) */
        self->asked_walk = 1;
        self->asked_walk_x = p1;
        self->asked_walk_y = p2;
        break;
    default:
        pc_send_msg(object, source, msg, par1, par2, par3, par4);
        return;
    }
}

void player_process_costume_body(struct Player *self, unsigned char show)
{
    if (is_effect_state_cloaking(self->effect_state) || effect_state_bit6(self->effect_state) || is_effect_state_hiding(self->effect_state))
        show = 0;
    if (show) {
        if (!self->costume_body_obj) {
            struct GameObject *obj = game_mode_make_msg_effect(game_current_mode(), 0x2c, &self->pos,
                                                 self->aid, 1);

            msg_effect_attach((struct MsgEffect *)obj, &self->attached);
            render_object_send(obj, self, 0x32, 0, 0, 0, 0);
            self->costume_body_obj = obj;
        }
    } else if (self->costume_body_obj) {
        render_object_send(self->costume_body_obj, NULL, 0x35, 0, 0, 0, 0);
        self->costume_body_obj = 0;
    }
}

const struct GameActorOps player_ops = {
    .base = {
        .base = {
            .destroy = player_deleting_destruct,
            .on_process = player_on_process,
            .send_msg = player_send_msg,
            .render = pc_render,
            .get_direction = game_actor_dir_of,
        },
        .render_body = game_actor_set_render_info,
        .set_action = game_actor_set_action,
        .update_motion = render_object_update_motion_base,
        .get_effect_state = game_actor_get_effect_state,
    },
    .process_state = player_process_state,
    .set_state = player_set_state,
    .set_unit_cell = actor_default_noop,
    .clear_unit_cell = actor_default_noop,
    .set_job_model = actor_default_pair,
    .set_look = pc_set_spr_act,
    .set_look_with_state = pc_set_spr_act_with_state,
    .set_spr_job = pc_set_spr_job,
    .set_head = pc_set_head,
    .set_weapon = pc_set_weapon,
    .set_weapon_look = pc_set_weapon_look,
    .set_accessory = pc_set_accessory,
    .set_accessory2 = pc_set_accessory2,
    .set_accessory3 = pc_set_accessory3,
    .set_shield = pc_set_shield,
    .set_appearance_value = pc_set_shoes,
    .set_imf_name = pc_set_imf_name,
    .set_head_pal_name = pc_set_head_pal_name,
    .set_body_pal_name = pc_set_body_pal_name,
    .set_body_palette = pc_set_body_palette,
    .set_head_palette = pc_set_head_palette,
    .get_weapon = pc_get_weapon,
    .walk_motion = game_actor_walk_motion,
    .get_attack_motion = game_actor_get_attack_motion,
    .make_corpse = game_actor_make_corpse,
    .set_attack_speed = pc_set_delay_factor,
    .set_honor = pc_set_honor,
    .set_honor_virtue = pc_set_honor_virtue,
    .get_char_id = pc_get_char_id,
    .set_robe = pc_set_robe,
    .set_body_style = pc_set_body_style,
    .set_mounted = pc_set_riding,
    .get_sprite_variant = pc_get_sprite_variant,
    .set_sprite_variant = pc_set_sprite_variant,
    .get_skip_arrow = pc_get_skip_arrow,
    .set_skip_arrow = pc_set_skip_arrow,
    .refresh_accessory3 = pc_refresh_accessory3,
    .get_head_extent = pc_get_head_extent,
};

static void player_init(struct Player *self, const float position[3], int cell_x, int cell_y)
{
    int saved_body_state, saved_effect_state, saved_health_state, sex, job;

    self->pos.x = position[0];
    self->pos.y = position[1];
    self->pos.z = position[2];
    self->requested_cell_x = cell_x;
    self->requested_cell_y = cell_y;
    self->skill_cell_x = 0;
    self->skill_cell_y = 0;
    self->roty = (float)((g_session1.playerDir + 4) * 45);
    self->aid = g_session1.ownAid;
    self->guild_id = g_guildInfo.guild_id;
    self->move_request_tick = 0;
    self->move_request_pending = 0;
    self->target_aid = 0;
    self->pickable = 1;
    self->skill_id = 0;
    self->skill_level = 0;
    self->screen_x = g_renderer->centreX;
    self->screen_y = g_renderer->centreY;
    self->owed_walk = 0;
    self->owed_walk_x = -1;
    self->owed_walk_y = -1;
    self->asked_walk = 0;
    self->asked_walk_x = -1;
    self->asked_walk_y = -1;
    self->intent_skill = 0;
    self->intent_hold = 0;
    actor_set_state(self, 0);
    self->walk_speed = g_session1.walkSpeed;
    self->is_pc = 1;
    game_actor_set_sex((struct GameActor *)self, session_get_sex(&g_session1));

    saved_effect_state = self->effect_state;
    saved_body_state = self->body_state;
    saved_health_state = self->health_state;
    self->body_state = g_session1.bodyState;
    self->effect_state = g_session1.effectState;
    self->health_state = g_session1.healthState;
    actor_set_body_style(self, g_session1.body, 0);
    sex = session_get_sex(&g_session1);
    job = session_get_drawn_job(&g_session1);
    {
        struct ActorLook look = { job, sex, g_session1.hairStyle, g_session1.weapon,
            { g_session1.headBottom, g_session1.headTop, g_session1.headMid },
            g_session1.shield, g_session1.hairColor, g_session1.clothesColor };
        actor_set_look_with_state(self, &look, g_session1.effectState);
    }
    actor_set_robe(self, g_session1.robe, 0);
    if (g_session1.ownDressUp)
        render_object_send(self, NULL, 0x6e, 0x358, 1, g_session1.ownDressUp, 0);
    self->effect_state = saved_effect_state;
    self->body_state = saved_body_state;
    self->health_state = saved_health_state;
    game_actor_set_attr_state((struct GameActor *)self, g_session1.bodyState, g_session1.effectState, g_session1.healthState, 0);

    render_object_send(self, NULL, 0x6d, 0xa4, 0, 0, 0);
    {
        Vec3 none = { 0.0f, 0.0f, 0.0f };

        self->status_icons = game_actor_launch_effect((struct GameActor *)self, 0xa4, none, 0);
    }
    if (!g_mapPropertyQuiet)
        player_refresh_status_effects(self);
    self->object_type = 0;
    self->costume_body_obj = 0;
}

struct Player *player_create(const float position[3], int cell_x, int cell_y)
{
    struct Player *self = mem_alloc(sizeof *self);
    if (!self)
        return NULL;
    pc_construct((struct Pc *)self);
    self->ops = &player_ops.base.base;
    player_init(self, position, cell_x, cell_y);
    return self;
}

void player_deleting_destruct(struct GameObject *object)
{
    struct Player *self = (struct Player *)object;

    self->ops = &player_ops.base.base;
    pc_destruct((struct Pc *)self);
    mem_free(self);
}
