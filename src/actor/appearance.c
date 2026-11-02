#include "actor/actor_ops.h"
#include "actor/appearance.h"
#include "actor/game_actor.h"
#include "session/state.h"
#include "game/actor_entry.h"
#include "session/session_names.h"
#include "session/session_skills.h"

int actor_get_screen_position(const struct GameActor *actor, struct ActorScreenPosition *position)
{
    if (!actor)
        return 0;
    position->x = actor->screen_x;
    position->y = actor->screen_y;
    position->scale = actor->scale;
    return 1;
}

int player_get_appearance(struct Player *player, struct PlayerAppearance *appearance)
{
    if (!player)
        return 0;
    appearance->job = game_actor_get_job((struct GameActor *)player);
    appearance->look_job = player->look_job;
    appearance->sex = player->sex;
    appearance->head = player->head;
    appearance->body_palette = player->body_palette;
    appearance->head_palette = player->head_palette;
    appearance->accessory[0] = player->accessory[0];
    appearance->accessory[1] = player->accessory[1];
    appearance->accessory[2] = player->accessory[2];
    appearance->robe = player->robe;
    appearance->body_style = player->body_style;
    appearance->mounted = player->mounted;
    return 1;
}

int player_state(const struct Player *player)
{
    return player ? player->state : -1;
}

struct ActorLook actor_look_from_entry(const struct ActorEntryInfo *entry)
{
    struct ActorLook look = {
        entry->job, entry->sex, entry->head,
        actor_weapon_pair(entry->weapon[0], entry->weapon[1]),
        { entry->accessory, entry->accessory2, entry->accessory3 },
        entry->shield, entry->head_palette, entry->body_palette
    };
    return look;
}

struct ActorLook actor_look_from_move_entry(const struct ActorMoveEntryInfo *entry)
{
    struct ActorLook look = {
        entry->job, entry->sex, entry->head,
        actor_weapon_pair(entry->weapon[0], entry->weapon[1]),
        { entry->accessory, entry->accessory2, entry->accessory3 },
        entry->shield, entry->head_palette, entry->body_palette
    };
    return look;
}

void actor_set_look(void *actor, const struct ActorLook *look)
{
    struct GameActor *object = actor;
    actor_ops_of(object)->set_look(object, look);
}

void actor_set_look_with_state(void *actor, const struct ActorLook *look, int effect_state)
{
    struct GameActor *object = actor;
    actor_ops_of(object)->set_look_with_state(object, look, effect_state);
}

static int pc_drawn_job(struct Pc *self, int job, int st)
{
    if (effect_state_bit12(st))
        return 0x16;
    if (effect_state_bit16(st) && !effect_state_bit15(st))
        return 0x1a;
    if (effect_state_bit18(st) && !effect_state_bit15(st))
        return 0x1b;
    if (effect_state_bit27(st) && !effect_state_bit15(st))
        return 0x1c;
    if (effect_state_bit28(st) && !effect_state_bit15(st))
        return 0x1d;
    if (self->mount_look == 1 && !effect_state_bit15(self->effect_state))
        return 0x1e;
    if (self->mounted)
        return session_get_mounted_job(job);
    return job;
}

void pc_set_spr_act(struct GameActor * actor, const struct ActorLook * look)
{
    struct Pc *self = (struct Pc *)actor;

    self->look_job = look->job;
    game_actor_set_sex((struct GameActor *)self, look->sex);
    self->head = look->head;
    self->weapon = look->weapon;
    self->accessory[0] = look->accessory[0];
    self->accessory[1] = look->accessory[1];
    self->accessory[2] = look->accessory[2];
    self->shield = look->shield;
    self->head_palette = look->head_palette;
    self->body_palette = look->body_palette;
    game_actor_set_job((struct GameActor *)self, look->job);
    game_actor_set_job((struct GameActor *)self, pc_drawn_job(self, look->job, self->effect_state));
    actor_set_head(self, self->head);
    actor_set_weapon(self, self->weapon);
    actor_set_accessory(self, self->accessory[0]);
    actor_set_accessory2(self, self->accessory[1]);
    actor_set_accessory3(self, self->accessory[2]);
    actor_set_shield(self, self->shield);
    actor_set_imf_name(self);
    actor_set_head_pal_name(self);
    actor_set_body_palette(self, self->body_palette);
    actor_set_body_pal_name(self);
}

static int pc_plain_look(struct Pc *self, int st)
{
    return !effect_state_bit12(st) && !effect_state_bit16(st) && !effect_state_bit18(st) && !effect_state_bit27(st)
           && !effect_state_bit28(st) && self->mount_look != 1 && !self->mounted;
}

void pc_set_spr_act_with_state(struct GameActor * actor, const struct ActorLook * look, int st)
{
    struct Pc *self = (struct Pc *)actor;
    int weapon = (int)look->weapon;
    int weapon2 = (int)((uint64_t)look->weapon >> 32);

    self->look_job = look->job;
    game_actor_set_sex((struct GameActor *)self, look->sex);
    self->head = look->head;
    self->weapon = look->weapon;
    self->accessory[0] = look->accessory[0];
    self->accessory[1] = look->accessory[1];
    self->accessory[2] = look->accessory[2];
    self->shield = session_get_weapon_view_id(weapon2) < 0 ? weapon2 : 0;
    self->head_palette = look->head_palette;
    self->body_palette = look->body_palette;
    if (look->body_palette < 2 && !self->body_style) {
        self->body_palette = 0;
        if ((unsigned)(look->job - 0xfa1) <= 0x15 && look->job <= 0xfa7)
            self->body_palette = 1;
    }
    game_actor_set_job((struct GameActor *)self, pc_drawn_job(self, look->job, st));
    actor_set_head(self, self->head);
    if (pc_plain_look(self, st)) {
        __int64 w;

        switch (game_actor_get_job((struct GameActor *)self)) {
        case 0xc: case 0xfad: case 0xfc3: case 0xfdb: case 0xfe1: case 0x1005:
            w = look->weapon;
            break;
        default:
            w = weapon;
            break;
        }
        actor_set_weapon_look(self, w);
    }
    actor_set_accessory(self, self->accessory[0]);
    actor_set_accessory2(self, self->accessory[1]);
    actor_set_accessory3(self, self->accessory[2]);
    if (pc_plain_look(self, st))
        actor_set_shield(self, self->shield);
    actor_set_imf_name(self);
    actor_set_head_pal_name(self);
    actor_set_body_pal_name(self);
}
