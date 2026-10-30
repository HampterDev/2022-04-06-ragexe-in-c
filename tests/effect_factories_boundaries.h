#include <stdarg.h>
struct Session g_session1;

int client_lua_call(struct ClientLua *lua, const char *function, const char *format, ...)
{
    va_list args;
    const char *table;
    int *result;
    assert(!strcmp(function, "GetTableSizeForC") && !strcmp(format, "s>d"));
    va_start(args, format);
    table = va_arg(args, const char *);
    result = va_arg(args, int *);
    va_end(args);
    assert(!strcmp(table, "hatEffectTable"));
    *result = hat_count;
    ++lua_calls;
    return 1;
}
#ifdef EFFECT_FACTORIES_REFERENCE
void *ez2str_effect_mgr_instance(void) { return NULL; }
#else
struct Ez2StrEffectMgr *ez2str_effect_mgr_instance(void) { return NULL; }
#endif
struct Vec *ez2_str_effect_mgr_get_effect_ids(struct Ez2StrEffectMgr *self, struct Vec *ids)
{
    int *values = mem_alloc((extra_count + 1) * sizeof *values);
    memcpy(values, extra_ids, extra_count * sizeof *values);
    ids->first = values;
    ids->last = values + extra_count;
    ids->end = values + extra_count + 1;
    return ids;
}

void abstract_method_called(void) { abort(); }

struct C3dAttr;
struct Effect;
struct GameActor;
struct GameMode;
struct Player;
struct Skill;
struct World;
struct List;
struct ListNode;
struct String;
struct Tree;
struct TreeNode;
struct Vec;
float c3d_attr_get_height(struct C3dAttr *self, float x, float z) { abort(); }
unsigned char effect_remove_if_dead(struct Effect *self) { abort(); }
void effect_render_if_visible(struct Effect *self) { abort(); }
void effect_set_owner(struct Effect *self, int aid) { abort(); }
void effect_set_target(struct Effect *self, int aid) { abort(); }
void effect_step(struct Effect *self, int owner_aid) { abort(); }
void game_actor_stop_aura_effects(struct GameActor *actor) { abort(); }
struct Skill *world_get_skill_by_aid(struct World *world, int aid) { abort(); }
struct GameActor *world_get_game_actor_by_aid(struct World *world, unsigned aid) { abort(); }
void game_mode_cell_to_world(struct GameMode *mode, int cell_x, int cell_y, float *x, float *z) { abort(); }
__declspec(noreturn) void fatal_error(const char *message) { abort(); }
struct C3dAttr *game_current_map_attr(void) { abort(); }
struct GameMode *game_current_mode(void) { abort(); }
struct Player *game_current_player(void) { abort(); }
struct World *game_current_world(void) { abort(); }
void string_assign(struct String *s, const char *text, size_t len) { abort(); }
void string_copy(struct String *s, const struct String *src) { abort(); }
void string_free(struct String *s) { abort(); }
int string_compare(const struct String *a, const struct String *b) { abort(); }
