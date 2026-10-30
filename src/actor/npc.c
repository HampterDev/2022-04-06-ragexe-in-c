#include "ui/window_types.h"
#include "actor/actor_ops.h"
#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "render/renderer.h"
#include "game/own_states.h"
#include "session/state.h"
#include "world/mode_dispatch.h"
#include "game/context.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "actor/actor.h"
#include "actor/game_actor.h"
#include "actor/sprite_render.h"
#include "std/std.h"
#include "game/gamemode.h"
#include "login/merc_ai_types.h"
#include "login/merc_ai.h"
#include "session/session_jobs.h"
#include "session/session_lua.h"
#include "session/session_names.h"
#include "session/session_party.h"
#include "session/session_talk.h"

const char monster_spr_fmt[] = "몬스터\\%s.spr";
const char monster_act_fmt[] = "몬스터\\%s.act";

#define NPC_MARK_ID(self)  (((short *)&(self)->mark_id)[0])
#define NPC_MARK_SUB(self) (((short *)&(self)->mark_id)[1])
#define MODE_SEND(mode, msg, a, b, c, d) \
    mode_send_message((struct Mode *)(mode), (msg), (a), (b), (c), (d))
#define NPC_IS_MERCENARY(self) \
    (game_actor_get_job((struct GameActor *)(self)) >= 0x1781 && \
     game_actor_get_job((struct GameActor *)(self)) <= 0x179e)

static void npc_launch(struct Npc *self, int type)
{
    Vec3 none = { 0.0f, 0.0f, 0.0f };

    game_actor_launch_effect((struct GameActor *)self, type, none, 0.0f);
}

static void npc_place_window(struct Npc *self, struct UIWindow *wnd)
{
    int lines = g_renderer->height * 12 / 480;

    ui_window_move(wnd, self->screen_x - 30, (int)((float)lines * self->scale + (float)self->screen_y));
    ui_window_invalidate(wnd);
}

void npc_update_quest_mark(struct Npc *self)
{
    int aid = self->aid;
    struct GameMode *mode;
    struct QuestMarkValue *rec;
    struct GameObject *obj;
    short id;

    if (session_is_npc(&g_session1, aid, game_actor_get_job((struct GameActor *)self)) != 1)
        return;
    mode = game_current_mode();
    rec = game_mode_get_quest_mark(mode, self->aid);
    if (!rec || rec->effect == 9999) {
        if (self->mark) {
            render_object_send(self->mark, NULL, 0x35, 0, 0, 0, 0);
            self->mark = NULL;
            self->mark_id = 9999;
        }
        return;
    }
    id = rec->effect;
    if (self->mark) {
        if (id == NPC_MARK_ID(self) && rec->icon == NPC_MARK_SUB(self))
            return;
        render_object_send(self->mark, NULL, 0x35, 0, 0, 0, 0);
        self->mark = NULL;
        self->mark_id = 9999;
        id = rec->effect;
    }
    obj = game_mode_make_msg_effect(mode, (id < 6 ? 0x24 : 0x36) + id, &self->pos, self->aid, 1);
    list_push_back(&self->attached, &obj, sizeof obj);
    render_object_send(obj, self, 0x32, 0, 0, 0, 0);
    self->mark = obj;
    NPC_MARK_ID(self) = rec->effect;
    NPC_MARK_SUB(self) = rec->icon;
}

static void npc_run_ai(struct Npc *self, struct MercenaryAI **slot, int kind, int user_ai, const char *stock,
                       const char *user)
{
    WIN32_FIND_DATAA found;
    HANDLE h;

    if (!*slot) {
        struct MercenaryAI *ai = mem_alloc(sizeof *ai);

        *slot = ai ? merc_ai_construct(ai, kind) : NULL;
        return;
    }
    if ((*slot)->L) {
        merc_ai_run(*slot, self->aid);
        return;
    }
    if (user_ai) {
        h = FindFirstFileA(".\\AI\\USER_AI", &found);
        if (h == INVALID_HANDLE_VALUE)
            CreateDirectoryA(".\\AI\\USER_AI", NULL);
        FindClose(h);
        CopyFileA(stock, user, TRUE);
        merc_ai_load(*slot, user);
    } else {
        merc_ai_load(*slot, stock);
    }
    render_object_send(self, NULL, 0x22, 0, 0, 0, 0);
}

unsigned char npc_on_process(struct GameObject *object)
{
    struct Npc *self = (struct Npc *)object;

    struct GameActor *actor = (struct GameActor *)self;
    unsigned char result;
    int msg, job;

    if (!game_actor_process_fade_out(actor))
        return 0;
    if (self->hit_flash && timeGetTime() > self->hit_tick + 100) {
        self->hit_argb = 0xffff0000;
        self->hit_flash = 0;
    }
    job = game_actor_get_job(actor);
    if (job == 0x68 && timeGetTime() > self->last_effect_tick + self->xsize * 1000) {
        self->last_effect_tick = timeGetTime();
        npc_launch(self, self->walk_speed);
    }
    if (job == 0x2d || job == 0x8b || job == 0x68) {
        result = 1;
    } else {
        if (job == 0x6f) {
            game_actor_process_will_be_attacked(actor);
        } else {
            game_actor_process_body_state(actor);
            game_actor_process_effect_state(actor);
            game_actor_process_will_be_attacked(actor);
            if (self->argb != 0xffffffff)
                render_object_modify_argb((struct RenderObject *)self, 8, 8, 8, 8);
        }
        game_actor_process_chat_balloon_(actor);
        result = actor_process_state(self);
    }
    if (self->object_type != 7)
        render_object_process_sound((struct RenderObject *)self);
    if (session_is_homunculus_aid(&g_session1, self->aid))
        npc_run_ai(self, &self->homun_ai, 0, g_session1.hoUserAI, ".\\AI\\AI.lua", ".\\AI\\USER_AI\\AI.lua");
    if (session_is_mercenary_aid(&g_session1, self->aid))
        npc_run_ai(self, &self->merc_ai, 1, g_session1.merc_command[4], ".\\AI\\AI_M.lua", ".\\AI\\USER_AI\\AI_M.lua");
    if (session_is_own_aid(&g_session1, self->aid))
        render_object_send(self, NULL, 0x22, 0, 0, 0, 0);
    if (self->player_gage)
        npc_place_window(self, self->player_gage);
    if (self->pc_gage)
        npc_place_window(self, self->pc_gage);
    game_actor_place_hp_gage(actor);
    if ((g_session1.homun_ai_flags[0] && g_session1.homun_command[0] == self->aid) || (g_session1.mercPresent && g_session1.merc_command[0] == self->aid))
        msg = 0x86;
    else
        msg = 0x87;
    render_object_send(self, NULL, msg, 0, 0, 0, 0);
    npc_update_quest_mark(self);
    return result;
}

static void npc_name_rect(struct Npc *self, const float *matrix, float half_w, float half_h, int tall)
{
    float sx, sy, rhw, s;
    struct ScreenBounds rect;
    const struct Renderer *r = g_renderer;

    renderer_project_point(g_renderer, &self->pos, matrix, &sx, &sy, &rhw);
    s = (float)renderer_screen_scale(g_renderer, rhw);
    s = s / g_spriteScaleDivisor;
    self->scale = s;
    rect.left = sx - r->scaleX * half_w * s;
    rect.top = sy - r->scaleY * half_h * s;
    rect.right = r->scaleX * half_w * s + sx;
    rect.bottom = tall ? r->scaleY * half_h * s + sy : sy;
    rect.z = rhw;
    render_object_render_body((struct RenderObject *)self, &rect, sx, sy);
}

void npc_render(struct GameObject *object, const float *matrix)
{
    struct Npc *self = (struct Npc *)object;

    struct RenderObject *ro = (struct RenderObject *)self;
    int job;

    switch (game_actor_get_job((struct GameActor *)self)) {
    case 0x6f:
        npc_name_rect(self, matrix, (float)(self->xsize / 2), (float)(self->ysize / 2), 0);
        return;
    case 0x2d:
        if (!self->revealed) {
            render_object_render_projected(ro, matrix);
            return;
        }
        npc_name_rect(self, matrix, 50.0f, 50.0f, 1);
        if (g_session1.effectOn == 0)
            render_object_render_shadow((struct RenderObject *)self, matrix, self->zoom * 3.0f);
        return;
    case 0x68:
    case 0x8b:
        return;
    }
    if (NPC_IS_MERCENARY(self)) {
        if (!self->visible)
            return;
        if ((self->argb & 0xff000000) == 0xff000000)
            render_object_render_shadow((struct RenderObject *)self, matrix, self->zoom);
        if (self->revealed)
            render_object_render_mercenary((struct RenderObject *)ro, matrix, game_actor_get_job((struct GameActor *)self));
        else
            render_object_render_projected(ro, matrix);
        return;
    }
    if (own_states_find(0xb8) && self->object_type == 5 && !(self->visible && self->revealed)) {
        unsigned char flag;

        render_object_set_argb(ro, -1, 0, 0, 0);
        job = game_actor_get_job((struct GameActor *)self);
        flag = (job > 0x3e8 && job < 0xf9f) || (unsigned)(job - 0x4e35) <= 0x2eca;
        render_object_render_sprite((struct RenderObject *)ro, matrix, flag, 0.0f);
        return;
    }
    if (!self->visible)
        return;
    if ((self->argb & 0xff000000) == 0xff000000)
        render_object_render_shadow((struct RenderObject *)self, matrix, self->zoom);
    if (self->revealed)
        render_object_render_sprite((struct RenderObject *)ro, matrix,
                                   job_is_monster(game_actor_get_job((struct GameActor *)self)),
                                   0.0f);
    else
        render_object_render_projected(ro, matrix);
}

static void npc_drop_state2(struct Npc *self)
{
    if (self->body_state == 2) {
        render_object_send(self, NULL, 0x3b, 0, 0, 0, 0);
        render_object_send(self, NULL, 0x3e, 0, 0, 0, 0);
    }
}

static void npc_idle_talk(struct Npc *self, int state)
{
    int hunger = session_pet_hunger_level(g_session1.petFullness);
    int talk;

    switch (self->pk_mode - 1) {
    case 0:
        talk = session_make_pet_talk_id(game_actor_get_job((struct GameActor *)self), 7, hunger);
        render_object_set_action((struct RenderObject *)self, 0x28, 1);
        break;
    case 1:
        talk = session_make_pet_talk_id(game_actor_get_job((struct GameActor *)self), 8, hunger);
        render_object_set_action((struct RenderObject *)self, 0x30, 1);
        break;
    case 2:
        talk = session_make_pet_talk_id(game_actor_get_job((struct GameActor *)self), 9, hunger);
        render_object_set_action((struct RenderObject *)self, 0x38, 1);
        break;
    case 3:
        talk = session_make_pet_talk_id(game_actor_get_job((struct GameActor *)self), 5, hunger);
        render_object_set_action((struct RenderObject *)self, 0x40, 1);
        break;
    default:
        render_object_set_action((struct RenderObject *)self, 0x28, 1);
        talk = state;
        break;
    }
    if (self->aid == g_session1.aid) {
        game_send_message(0x94, talk, 0, 0, 0);
    }
    self->pk_mode = 0;
    self->motion_finished = 0;
    actor_set_unit_cell(self);
}

static int npc_set_state_by_job(struct Npc *self)
{
    switch (game_actor_get_job((struct GameActor *)self)) {
    case 0x2d: {
        float half_w, half_h;

        if (self->ysize <= 10)
            self->ysize = 10;
        if (self->xsize <= 10)
            self->xsize = 10;
        half_w = (float)(self->xsize / 2);
        half_h = (float)(self->ysize / 2);
        self->area[0] = self->pos.x - half_w;
        self->area[2] = half_w + self->pos.x;
        self->area[1] = self->pos.z - half_h;
        self->area[3] = half_h + self->pos.z;
        if (self->area_effect_launched == 0 && self->revealed) {
            self->area_effect_launched = 1;
            npc_launch(self, 0x141);
        }
        return 1;
    }
    case 0x68:
        self->pos.y = self->pos.y - (float)(self->ysize * 5);
        return 1;
    case 0x6f:
        self->xsize += self->xsize;
        self->ysize += self->ysize;
        return 1;
    case 0x8b:
        return 1;
    }
    return 0;
}

static void npc_set_state_general(struct Npc *self, int state)
{
    struct GameActor *actor = (struct GameActor *)self;
    unsigned char owner_sits;
    void *owner;

    owner_sits = 0;
    owner = session_get_actor_by_aid(&g_session1, self->aid);
    if (owner && ((struct GameActor *)owner)->object_type == 0xc)
        owner_sits = 1;
    if (session_is_npc(&g_session1, self->aid, game_actor_get_job(actor)) == 1 &&
        game_actor_get_job(actor) != 0x235 && !owner_sits) {
        render_object_set_action((struct RenderObject *)self, 0, 0);
        actor_set_unit_cell(self);
        return;
    }
    if (self->state == 4) {
        self->pos.x = self->slide_x;
        self->pos.z = self->slide_z;
    }
    switch (state) {
    case 0:
        self->slant_depth = 0;
        self->pickable = 1;
        render_object_set_action((struct RenderObject *)self, 0, 0);
        if (self->pk_mode) {
            npc_idle_talk(self, state);
        } else {
            if (self->state != 0)
                self->pet_talk_tick = timeGetTime();
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
            actor_set_unit_cell(self);
        }
        break;
    case 1:
        if (self->body_state != 1 && self->body_state != 2)
            self->motion_held = 0;
        render_object_set_action((struct RenderObject *)self, 8, 0);
        actor_clear_unit_cell(self);
        break;
    case 2: {
        unsigned char kind;
        int job;

        if (self->body_state != 1 && self->body_state != 2)
            self->motion_held = 0;
        kind = self->object_type;
        job = game_actor_get_job(actor);
        if (kind == 8) {
            if (job == 0x17a4)
                render_object_set_action((struct RenderObject *)self, 0x10, 1);
            else if (rand() % 2 == 0)
                render_object_set_action((struct RenderObject *)self, 0x30, 1);
            else
                render_object_set_action((struct RenderObject *)self, 0x10, 1);
        } else if (job >= 0x1781 && game_actor_get_job(actor) <= 0x179e) {
            render_object_set_action((struct RenderObject *)self, 0x50, 1);
        } else {
            render_object_set_action((struct RenderObject *)self, 0x10, 1);
        }
        self->attack_motion_count = (float)actor_get_attack_motion(self);
        actor_set_unit_cell(self);
        game_actor_launch_ranged_attack(actor, self->attack_target_aid, self->motion_delay * self->attack_motion_count);
        break;
    }
    case 3:
        self->script_range_on = 0;
        self->script_steps_on = 0;
        self->script_pingpong_on = 0;
        self->motion_held = 0;
        npc_drop_state2(self);
        self->slant_depth = 1;
        self->motion_held = 0;
        self->pickable = 0;
        if (NPC_IS_MERCENARY(self))
            render_object_set_action((struct RenderObject *)self, 0x40, 1);
        else
            render_object_set_action((struct RenderObject *)self, 0x20, 1);
        actor_clear_unit_cell(self);
        break;
    case 4:
        if (self->script_range_on || self->script_steps_on || self->script_pingpong_on)
            return;
        if (NPC_IS_MERCENARY(self)) {
            render_object_set_action((struct RenderObject *)self, 0x30, 1);
        } else if (self->action < 0x28) {
            render_object_set_action((struct RenderObject *)self, 0x18, 1);
            actor_set_unit_cell(self);
        } else {
            render_object_update_motion((struct RenderObject *)self);
            return;
        }
        break;
    case 7:
        switch (self->object_type) {
        case 8: case 10: case 13: case 14:
            render_object_set_action((struct RenderObject *)self, 0x28, 1);
            break;
        case 9:
            render_object_set_action((struct RenderObject *)self, 0x10, 1);
            break;
        }
        if (NPC_IS_MERCENARY(self))
            render_object_set_action((struct RenderObject *)self, 0x60, 1);
        break;
    case 10:
        npc_drop_state2(self);
        self->slant_depth = 0;
        self->motion_finished = 1;
        self->motion_held = 1;
        render_object_set_action((struct RenderObject *)self, 0, 1);
        break;
    }
}

void npc_set_state(struct GameActor * base_actor, int state, unsigned char snap)
{
    struct Npc * self = (struct Npc *)base_actor;

    if (state == -1)
        return;
    if (!npc_set_state_by_job(self))
        npc_set_state_general(self, state);
    self->action = game_actor_dir_of((struct GameObject *)self, self->roty) + self->base_action;
    self->state = state;
    self->start_tick = timeGetTime();
}

void npc_set_unit_cell(struct GameActor * base_actor)
{
    struct Npc * self = (struct Npc *)base_actor;

    struct GameMode *mode = game_current_mode();
    int cx, cy, job;

    game_mode_world_to_cell(mode, self->pos.x, self->pos.z, &cx, &cy);
    job = game_actor_get_job((struct GameActor *)self);
    if (job == 0x2d || job == 0x6f || job == 0x8b)
        return;
    MODE_SEND(mode, 0xc, self->aid, cx, cy, 0);
}

void npc_clear_unit_cell(struct GameActor * base_actor)
{
    struct Npc * self = (struct Npc *)base_actor;

    switch (game_actor_get_job((struct GameActor *)self)) {
    case 0x2d: case 0x68: case 0x6f: case 0x8b:
        return;
    }
    game_send_message(0xd, self->aid, 0, 0, 0);
}

void npc_set_job_and_sex(struct GameActor * base_actor, int job, int sex)
{
    struct Npc * self = (struct Npc *)base_actor;

    char name[64];

    game_actor_set_job((struct GameActor *)self, job);
    game_actor_set_sex((struct GameActor *)self, sex);
    self->body_palette = 0;
    if (session_named_body_palettes_enabled(&g_session1)) {
        self->body_palette = 1;
        string_assign_cstr(&self->body_pal_name, name);
    }
}

void npc_set_spr_job_(struct GameActor * base_actor, int job)
{
    struct Npc * self = (struct Npc *)base_actor;

    char spr[128], act[128];
    const char *spr_fmt, *act_fmt;

    self->monster_job = 0;
    self->shadow_scale = session_get_npc_location_radius(&g_session1, job, 0);
    if (session_get_pet_acc_act_name(&g_session1, job) == 1) {
        self->monster_job = job;
        act_fmt = "NPC\\%s.act";
        spr_fmt = "NPC\\%s.spr";
    } else if ((unsigned)(job - 0x1781) <= 0x1d) {
        act_fmt = "인간족\\몸통\\%s.act";
        spr_fmt = "인간족\\몸통\\%s.spr";
    } else if ((unsigned)(job - 0x1771) <= 0x33) {
        act_fmt = "homun\\%s.act";
        spr_fmt = "homun\\%s.spr";
    } else {
        self->monster_job = job;
        act_fmt = monster_act_fmt;
        spr_fmt = monster_spr_fmt;
    }
    sprintf(spr, spr_fmt, session_get_job_name(&g_session1, job, -1));
    sprintf(act, act_fmt, session_get_job_name(&g_session1, job, -1));
    render_object_load_spr_res((struct RenderObject *)self, spr);
    render_object_load_act_res((struct RenderObject *)self, act);
}
