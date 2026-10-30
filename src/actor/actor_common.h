#ifndef ACTOR_ACTOR_COMMON_H
#define ACTOR_ACTOR_COMMON_H

#include "session/skill_commands.h"
#include "session/party_queries.h"
#include "game/actor_marks.h"
#include "game/actor_names.h"
#include "game/cells.h"
#include "game/mode_misc.h"
#include "game/mode_util.h"
#include "game/pickup_requests.h"
#include "game/quest_marks.h"
#include "skills/skill_action.h"
#include "game/world_objects.h"
#include "actor/render_ops.h"
#include "session/abr.h"
#include "ui/chat/readable_links.h"
#include "ui/chat/navi_text.h"
#include "std/string.h"
#include "render/rp_pool.h"
#include "resources/imf.h"
#include "render/texture_cache.h"
#include "session/job_sounds.h"
#include "render/sprite_atlas.h"
#include "resources/sprite.h"
#include "skills/skill_bundle.h"
#include "skills/skill_info.h"
#include "ui/window_messages.h"
#include "contents/client_lua_calls.h"
#include "contents/guild_emblem_api.h"
#include "actor/job.h"
#include "world/object_lookup.h"
#include "replay/replay.h"
#include "effects/effect_mgr.h"
#include "effects/effect_controls.h"
#include "ui/chat/balloon.h"
#include "render/projection.h"

#include "render/math3d.h"

#include "actor/render_object.h"
#include "world/ground_api.h"
#include "resources/attr_queries.h"
#include "world/pathfinder_state.h"
#include "world/path_info.h"
#include "navi/navi_manager.h"
#include "world/pick_node.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/memory.h"
#include "ui/window_access.h"
#include "skills/attack_range.h"
#include "world/actor_lookup.h"
#include "contents/connection_api.h"

#include "fileio/resource_api.h"

#include "std/tree.h"

#define WIN32_LEAN_AND_MEAN
#include <stdlib.h>
#include <windows.h>
#include "render/renderer.h"
#include <mmsystem.h>
#include <math.h>
#include "base.h"
#include "std/std.h"
#include "core/client.h"
#include "fileio/res_mgr.h"
#include "resources/animation.h"

#include "render/vec3.h"

struct UIWindow;
struct RenderObject;
struct SkillActionMgr;
struct RPMesh;
struct SprImg;
struct GameObject;
struct GameActor;
struct RagEffect;

struct MercenaryAI;

struct Session;
struct World;

struct ReplayRecorder;
extern const char monster_spr_fmt[], monster_act_fmt[];
extern int g_maxAttackDelayMs;

const char *res_get_name(struct Res *res);

unsigned char able_to_make_effect_is_effect_exist(void *actor, int effect_id);

struct Pc;
void game_actor_set_hat_effect(struct GameActor *self, int item, unsigned char on);
struct Pc *pc_construct(struct Pc *self);
void game_actor_set_job(struct GameActor *self, int job);
struct RenderObject *hallu_pc_construct(void *mem, struct RenderObject *actor);

struct C3dDevice;

int game_actor_transform_action_of(int kind);
int game_actor_action_of(struct GameActor *self, unsigned motion);
void render_object_play_motion_steps(struct RenderObject *self, unsigned char a0, unsigned char m0,
                           unsigned char a1, unsigned char m1, unsigned char a2, unsigned char m2,
                           unsigned char a3, unsigned char m3, unsigned char a4, unsigned char m4, int timed,
                           int duration, int loop);
void render_object_play_motion_range(struct RenderObject *self, unsigned char action,
                           unsigned char motion, unsigned char last, unsigned char close_action,
                           unsigned char close_motion, int timed, int duration, int loop);

struct GameMode;
struct ActorNameCard;

unsigned char job_is_monster(int job);
struct RenderObject;

#include "effects/rag/rag_effect_types.h"
#include "effects/render_object.h"

struct GameActor;
unsigned char able_to_make_effect_has_effect(struct GameActor *self,
                                                    struct RagEffect *effect);

struct MsgEffect;
struct Texture;
struct ChampionMonsterMgr;
struct RPQuad;
struct TexDrawVertex;
struct QuestMarkValue;
struct SkillInfo;
struct TexDrawPrim;
struct Texture;

#include "actor/liveness.h"

void actor_footprints(struct RenderObject *actor, int kind);

#endif
