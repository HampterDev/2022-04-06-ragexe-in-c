#include "actor/actor_ops.h"
#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#ifndef ACTOR_GAME_ACTOR_H
#define ACTOR_GAME_ACTOR_H
#include "actor/appearance.h"

#include "actor/render_object.h"
#include "actor/actor_path.h"
#include "skills/skill_action_registry.h"

#include "actor/actor.h"
#include "actor/actor_common.h"

struct GameMode;

struct SkillActionMgr;
struct SqQuad;

void actor_effect_state(struct GameActor *self, struct GameMode *mode, long long par1, long long par2, long long par3, long long par4);

void game_actor_msg_attack(struct GameActor *self, const struct ActorAttack *attack);

void game_actor_process_body_state(struct GameActor *self);
void game_actor_process_chat_balloon_(struct GameActor *self);
void game_actor_process_effect_state(struct GameActor *self);
void game_actor_process_will_be_attacked(struct GameActor *self);
void game_actor_place_hp_gage(struct GameActor *self);

unsigned char game_actor_process_fade_out(struct GameActor *self);

void game_actor_set_attr_state(struct GameActor *self, int body, int effect, int health, unsigned char sound);

void game_actor_stop_aura_effects(struct GameActor *self);
void game_actor_drop_siege_window(struct GameActor *self);
void game_actor_forget_handle(struct GameActor *self, int handle);
int game_actor_get_transform_job(struct GameActor *self);
void game_actor_launch_ranged_attack(struct GameActor *self, int id, float height);
void game_actor_raise_body_quarter_height(struct GameActor *self, int value);
void game_actor_set_pk_mode(struct GameActor *self, int value);
void game_actor_set_sex(struct GameActor *self, int value);
#include "actor/tetra_ball.h"
void game_actor_set_transform_job(struct GameActor *self, int value, int index);
void game_actor_set_transform_job_override(struct GameActor *self, int value);
void game_actor_show_level_aura(struct GameActor *self);
int game_actor_state_job(int job, int state);

void game_object_destruct(struct GameObject *self);
const char *render_object_get_act_name(struct RenderObject *self);
void render_object_modify_argb(struct RenderObject *self, int da, int dr, int dg, int db);

void game_actor_am_groundskill(struct GameActor *self, const struct GroundSkillInfo *rec);

void game_actor_set_hat_effect_by_aid(struct GameActor *self, int aid, int item, unsigned char on);
void game_actor_show_party_mark(struct GameActor *self, int id);
void game_actor_show_siege_emblem(struct GameActor *self);

void render_object_set_script_step_frames(struct RenderObject *self, unsigned char frames);

float render_object_calc_dir(float x1, float z1, float x2, float z2);
void render_object_render_projected(struct RenderObject *self, const void *view);
struct View;
void view_set_up(struct View *view, const Vec3 *v);

void game_actor_show_number(struct GameActor *self, int value, unsigned argb, int style);

void game_actor_skill_on_target(struct GameActor *self, const struct ActorSkillUse *rec);

void render_object_submit_spr_quad(struct RenderObject *self, struct SqQuad *quad, const void *palette, int flags, float x, float y);

int game_actor_get_height_offset(struct GameActor *self);
unsigned char game_actor_has_pending_hits(struct GameActor *self);
int game_actor_is_pc_job(int job);
void game_actor_set_clan(struct GameActor *self, unsigned char a, int b);
void game_actor_set_mark_listed(struct GameActor *self, int value);
void game_actor_show_orbs(struct GameActor *self, int kind, int count);
int game_actor_get_mount_height(struct GameActor *self);
float game_actor_get_mount_lift(struct GameActor *self);
void render_object_apply_shrink_scale(struct GameActor *self, float *scale);

#endif
