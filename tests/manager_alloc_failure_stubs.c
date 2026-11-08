#include <windows.h>
#include <lua.h>
#include <stdio.h>
#include <stdlib.h>
#include "contents/lua_script.h"
#include "contents/repute_api.h"
#include "contents/repute_mgr.h"
#include "contents/item_reform.h"
#include "contents/enchant_mgr.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "platform/error_message.h"
#include "std/string_vec.h"
#include "ui/window_access.h"

int alloc_failure_owns(const void *p);
int alloc_failure_window_sends, alloc_failure_window_opens, alloc_failure_script_runs, alloc_failure_repute_loads;
static struct UIWindow *const fake_window = (struct UIWindow *)(UINT_PTR)0x4000;

static void unreached(const char *name)
{
    printf("manager allocation failure: %s reached\n", name);
    exit(1);
}

static void check_script(const struct LuaScript *self, const char *name)
{
    if (!alloc_failure_owns(self)) {
        printf("manager allocation failure: %s got the script of a NULL manager (%p)\n", name, (const void *)self);
        exit(1);
    }
}

struct LuaScript *lua_script_construct(struct LuaScript *self) { check_script(self, "lua_script_construct"); return self; }
void lua_script_destruct(struct LuaScript *self) { check_script(self, "lua_script_destruct"); }
void lua_script_set_number(struct LuaScript *self, const char *name, double value)
{ (void)name; (void)value; check_script(self, "lua_script_set_number"); }
void lua_script_set_boolean(struct LuaScript *self, const char *name, char value)
{ (void)name; (void)value; check_script(self, "lua_script_set_boolean"); }
void lua_script_register(struct LuaScript *self, const char *name, int (*fn)(struct lua_State *))
{ (void)name; (void)fn; check_script(self, "lua_script_register"); }
char lua_script_run_file(struct LuaScript *self, const char *path)
{
    (void)path;
    check_script(self, "lua_script_run_file");
    alloc_failure_script_runs++;
    return 0;
}

char repute_mgr_load(struct ReputeMgr *self)
{
    if (!alloc_failure_owns(self)) {
        puts("manager allocation failure: repute_mgr_load was called with NULL");
        exit(1);
    }
    alloc_failure_repute_loads++;
    return 0;
}

struct UIWindow *ui_window_open(int id) { (void)id; alloc_failure_window_opens++; return fake_window; }
struct UIWindow *ui_window_find(int id) { (void)id; return fake_window; }
int ui_window_send(struct UIWindow *window, struct UIWindow *sender, int message, int a, int b, int c, int d)
{
    (void)sender; (void)message; (void)a; (void)b; (void)c; (void)d;
    if (window != fake_window)
        unreached("ui_window_send");
    alloc_failure_window_sends++;
    return 0;
}
char *msg_str(int id) { static char text[] = "changed"; (void)id; return text; }
unsigned int g_codePage = 65001;

struct String *text_utf8_to_codepage(struct String *out, const struct String *utf8, UINT code_page)
{ (void)utf8; (void)code_page; unreached("text_utf8_to_codepage"); return out; }
struct String *string_vec_push_copy(struct Vec *v, const struct String *s) { (void)v; (void)s; unreached("string_vec_push_copy"); return NULL; }
void repute_group_entry_destroy(void *value) { (void)value; unreached("repute_group_entry_destroy"); }
void repute_info_entry_destroy(void *value) { (void)value; unreached("repute_info_entry_destroy"); }
void reform_info_init(struct ReformInfo *info) { (void)info; unreached("reform_info_init"); }
void reform_info_destroy(struct ReformInfo *info) { (void)info; unreached("reform_info_destroy"); }
void error_msg(const char *text, int flags) { (void)text; (void)flags; unreached("error_msg"); }
struct EnchantList *enchant_list_at(struct Tree *lists, unsigned __int64 id) { (void)lists; (void)id; unreached("enchant_list_at"); return NULL; }
void enchant_list_entry_destroy(void *value) { (void)value; unreached("enchant_list_entry_destroy"); }
struct Tree *enchant_set_at(struct Tree *sets, unsigned grade) { (void)sets; (void)grade; unreached("enchant_set_at"); return NULL; }
struct EnchantSlot *enchant_slot_at(struct Tree *slots, unsigned index) { (void)slots; (void)index; unreached("enchant_slot_at"); return NULL; }
struct PerfectEnchant *perfect_enchant_at(struct Tree *perfects, int item_id) { (void)perfects; (void)item_id; unreached("perfect_enchant_at"); return NULL; }
struct UpgradeEnchant *upgrade_enchant_at(struct Tree *upgrades, int item_id) { (void)upgrades; (void)item_id; unreached("upgrade_enchant_at"); return NULL; }
#define ENCHANT_CALLBACK(name) int name(struct lua_State *L) { (void)L; unreached(#name); return 0; }
ENCHANT_CALLBACK(enchant_lua_set_slot_order)
ENCHANT_CALLBACK(enchant_lua_add_target_item)
ENCHANT_CALLBACK(enchant_lua_set_condition)
ENCHANT_CALLBACK(enchant_lua_approve_random_option)
ENCHANT_CALLBACK(enchant_lua_set_reset)
ENCHANT_CALLBACK(enchant_lua_set_caution)
ENCHANT_CALLBACK(enchant_lua_set_require)
ENCHANT_CALLBACK(enchant_lua_set_success_rate)
ENCHANT_CALLBACK(enchant_lua_set_grade_bonus)
ENCHANT_CALLBACK(enchant_lua_set_enchant)
ENCHANT_CALLBACK(enchant_lua_add_perfect_enchant)
ENCHANT_CALLBACK(enchant_lua_add_upgrade_enchant)
ENCHANT_CALLBACK(enchant_lua_get_slot_count)

int lua_gettop(lua_State *L) { (void)L; unreached("lua_gettop"); return 0; }
void lua_settop(lua_State *L, int idx) { (void)L; (void)idx; unreached("lua_settop"); }
int lua_isnumber(lua_State *L, int idx) { (void)L; (void)idx; unreached("lua_isnumber"); return 0; }
int lua_isstring(lua_State *L, int idx) { (void)L; (void)idx; unreached("lua_isstring"); return 0; }
int lua_type(lua_State *L, int idx) { (void)L; (void)idx; unreached("lua_type"); return 0; }
lua_Number lua_tonumber(lua_State *L, int idx) { (void)L; (void)idx; unreached("lua_tonumber"); return 0; }
lua_Integer lua_tointeger(lua_State *L, int idx) { (void)L; (void)idx; unreached("lua_tointeger"); return 0; }
int lua_toboolean(lua_State *L, int idx) { (void)L; (void)idx; unreached("lua_toboolean"); return 0; }
const char *lua_tolstring(lua_State *L, int idx, size_t *len) { (void)L; (void)idx; (void)len; unreached("lua_tolstring"); return NULL; }
void *lua_touserdata(lua_State *L, int idx) { (void)L; (void)idx; unreached("lua_touserdata"); return NULL; }
void lua_pushnil(lua_State *L) { (void)L; unreached("lua_pushnil"); }
void lua_pushstring(lua_State *L, const char *s) { (void)L; (void)s; unreached("lua_pushstring"); }
void lua_pushboolean(lua_State *L, int b) { (void)L; (void)b; unreached("lua_pushboolean"); }
void lua_gettable(lua_State *L, int idx) { (void)L; (void)idx; unreached("lua_gettable"); }
void lua_getfield(lua_State *L, int idx, const char *k) { (void)L; (void)idx; (void)k; unreached("lua_getfield"); }
void lua_call(lua_State *L, int nargs, int nresults) { (void)L; (void)nargs; (void)nresults; unreached("lua_call"); }
int lua_pcall(lua_State *L, int nargs, int nresults, int errfunc) { (void)L; (void)nargs; (void)nresults; (void)errfunc; unreached("lua_pcall"); return 0; }
int lua_cpcall(lua_State *L, lua_CFunction func, void *ud) { (void)L; (void)func; (void)ud; unreached("lua_cpcall"); return 0; }
int lua_next(lua_State *L, int idx) { (void)L; (void)idx; unreached("lua_next"); return 0; }
