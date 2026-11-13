int game_actor_dir_of(struct GameObject *object, float roty)
{
    abort();
}
void render_object_update_motion_base(struct RenderObject *object)
{
    abort();
}
void game_actor_set_action(struct RenderObject *object, int action, int type)
{
    abort();
}
void skill_render_body(struct RenderObject *object, const struct ScreenBounds *bounds, float x, float y)
{
    abort();
}
int game_actor_get_effect_state(struct RenderObject *object)
{
    abort();
}
void game_actor_clear_unit_cell(struct GameActor *actor)
{
    abort();
}
int game_actor_get_attack_motion(struct GameActor *actor)
{
    abort();
}
void game_actor_make_corpse(struct GameActor *actor)
{
    abort();
}
unsigned char game_actor_process_state(struct GameActor *actor)
{
    abort();
}
void game_actor_set_attack_speed(struct GameActor *actor, int value0)
{
    abort();
}
void game_actor_set_mounted(struct GameActor *actor, unsigned char value0)
{
    abort();
}
void game_actor_set_state(struct GameActor *actor, int value0, unsigned char value1)
{
    abort();
}
void game_actor_set_unit_cell(struct GameActor *actor)
{
    abort();
}
void game_actor_walk_motion(struct GameActor *actor)
{
    abort();
}
struct ActorHeadExtent game_actor_get_head_extent(struct GameActor *actor)
{
    abort();
}
void actor_default_bool(struct GameActor *actor, unsigned char value0)
{
    abort();
}
unsigned char actor_default_false(struct GameActor *actor)
{
    abort();
}
void actor_default_int(struct GameActor *actor, int value0)
{
    abort();
}
void actor_default_int_bool(struct GameActor *actor, int value0, unsigned char value1)
{
    abort();
}
void actor_default_int_flags(struct GameActor *actor, int value0, unsigned int value1)
{
    abort();
}
void actor_default_look(struct GameActor *actor, const struct ActorLook *value0)
{
    abort();
}
void actor_default_look_state(struct GameActor *actor, const struct ActorLook *value0, int value1)
{
    abort();
}
int64_t actor_default_no_weapon(struct GameActor *actor)
{
    abort();
}
void actor_default_noop(struct GameActor *actor)
{
    abort();
}
void actor_default_pair(struct GameActor *actor, int value0, int value1)
{
    abort();
}
void actor_default_weapon(struct GameActor *actor, int64_t value0)
{
    abort();
}
int actor_default_zero(struct GameActor *actor)
{
    abort();
}
void * mem_try_alloc(size_t bytes)
{
    abort();
}
void mem_free(void *memory)
{
    abort();
}
void game_actor_send_msg(struct GameObject *object, struct GameObject *source, RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    abort();
}
void surface_destroy(struct Surface *surface)
{
    abort();
}
float float_sin(float x)
{
    abort();
}
float float_cos(float x)
{
    abort();
}
void c3d_ground15_draw_attr_quad(struct C3dGround15 *self, const float *view, int ax, int ay, DWORD colour, struct Texture *texture, const float *uv)
{
    abort();
}
struct RagEffect * game_actor_launch_effect(struct GameActor *self, int effect_id, Vec3 offset, float rotation)
{
    abort();
}
struct ListNode * list_erase(struct List *list, struct ListNode *node, ListDestroyFn destroy)
{
    abort();
}
struct C3dGround15 * game_current_ground(void)
{
    abort();
}
void skill_destruct(struct Skill *self)
{
    abort();
}
void game_mode_cell_to_world(struct GameMode *self, int x, int y, float *world_x, float *world_z)
{
    abort();
}
int game_actor_get_job(struct GameActor *actor)
{
    abort();
}
struct Skill * world_get_skill_by_aid(struct World *world, int aid)
{
    abort();
}
float c3d_attr_get_height(struct C3dAttr *self, float x, float z)
{
    abort();
}
struct GameActor * world_get_game_actor_by_aid(struct World *world, unsigned aid)
{
    abort();
}
void * vec_push(struct Vec *v, const void *elem, size_t size)
{
    abort();
}
struct Pc * pc_construct(struct Pc *self)
{
    abort();
}
void actor_list_push_back(struct List *list, void *actor)
{
    abort();
}
struct ActorEntryInfo * actor_entry_info_construct(struct ActorEntryInfo *info)
{
    abort();
}
int session_get_drawn_job(struct Session *self)
{
    abort();
}
int session_get_sex(struct Session *self)
{
    abort();
}
BYTE world_remove_skill(struct World *self, int aid)
{
    abort();
}
