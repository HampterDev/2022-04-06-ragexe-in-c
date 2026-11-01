#include "render/texture_cache.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "render/render_api.h"
#include "contents/client_lua.h"
#include <lauxlib.h>
#include <lualib.h>

static int allocations, fail_after = -1, allocation_calls;
static int file_calls, fail_file, diagnostics, draws, scales, missing_texture;
static const char *script;
static char texture_path[512];
static struct Texture texture;
static struct Renderer renderer;
struct Renderer *g_renderer = &renderer;
struct TexCache g_textureCache;
struct Session g_session1;

static void *checked_malloc(size_t size)
{
    unsigned char *raw;
    ++allocation_calls;
    if (fail_after == 0) return NULL;
    if (fail_after > 0) --fail_after;
    assert(size < 20000000);
    raw = malloc(size + 24); assert(raw);
    *(size_t *)raw = size;
    memset(raw + 8, 0x63, 8); memset(raw + 16 + size, 0x57, 8);
    memset(raw + 16, 0xcc, size);
    ++allocations;
    return raw + 16;
}

static void checked_free(void *pointer)
{
    unsigned char *raw;
    size_t size, i;
    if (!pointer) return;
    raw = (unsigned char *)pointer - 16; size = *(size_t *)raw;
    for (i = 0; i < 8; ++i) { assert(raw[8+i] == 0x63); assert(raw[16+size+i] == 0x57); }
    memset(raw, 0xee, size + 24); free(raw); --allocations;
}

static void *checked_realloc(void *pointer, size_t size)
{
    void *next = checked_malloc(size);
    if (next && pointer) {
        size_t old = *(size_t *)((unsigned char *)pointer - 16);
        memcpy(next, pointer, old < size ? old : size);
        checked_free(pointer);
    }
    return next;
}

#define malloc checked_malloc
#define realloc checked_realloc
#define free checked_free
#include "../src/ui/quests/world_view.c"
#undef malloc
#undef realloc
#undef free

char *msg_str(int id) { assert(id == 0xbb); return "unknown map"; }
const char *session_map_display_name(const char *name)
{
    return strcmp(name, "missing.rsw") == 0 ? "unknown map" : "known map";
}
int session_scale_rect_maps(struct Session *session)
{
    assert(session == &g_session1 && session->rectMapsScaled == 1);
    ++scales;
    return 1;
}
struct Texture *texture_cache_get(struct TexCache *cache, const char *name, int a, int b)
{
    assert(cache == &g_textureCache && !a && b == 1);
    strcpy_s(texture_path, sizeof texture_path, name);
    return missing_texture ? NULL : &texture;
}
void texture_draw_surface_stretch(struct Texture *image, int x, int y, int width, int height)
{
    assert(image == &texture && x == 0 && y == WORLD_VIEW_TOP && width > 0 && height > 0);
    ++draws;
}

void client_lua_error_box(struct ClientLua *lua, const char *caption, const char *format, ...)
{
    assert(lua && lua->L && caption && format);
    ++diagnostics;
}
char client_lua_load_file(struct ClientLua *lua, const char *name, char via_cfile, char disk_only)
{
    static const char *const suffix[] = {"Language", "list", "table", "f", "Info"};
    char path[128];
    int index = file_calls++ % 5;
    assert(via_cfile == 1 && !disk_only);
    sprintf_s(path, sizeof path, "Lua Files\\worldviewdata\\worldviewdata_%s", suffix[index]);
    assert(strcmp(name, path) == 0);
    if (fail_file == index + 1) return 0;
    if (!index && script)
        return luaL_loadbuffer(lua->L, script, strlen(script), "world-view-fixture") == 0 && lua_pcall(lua->L, 0, 0, 0) == 0;
    return 1;
}

static const char basic[] =
    "function queryWorld_List(n) collectgarbage('collect'); if n>2 then return end; "
    " return '세계 '..n, 'maps '..n, 'fields '..n, 'world.bmp' end\n"
    "function querymaptable_load(t,n) collectgarbage('collect'); "
    " if t=='maps 2' then if n==1 then return 2,'group.rsw',10,20,30,40,'two','second world' end; return end; "
    " assert(t=='maps 1'); if n==1 then return 1,'town.rsw',0,0,100,100,'도시',string.rep('x',5000) end; "
    " if n==2 then return 1,'field.rsw',100,100,200,200,'field','level' end; "
    " if n==3 then return 100,'group.rsw',200,200,300,300,'group','boss' end; "
    " if n==4 then return 3,'missing.rsw',0,0,0,0,'missing','' end end\n"
    "function queryfieldtable_load(t,n) collectgarbage('collect'); "
    " assert(t=='fields 1' or t=='fields 2'); if t=='fields 1' and n==1 then return 1,5,6,205,206,'지역','던전' end end\n"
    "function Get_Worldview_info_Dungeon() return 1,2,3,4 end\n"
    "function Get_Worldview_info_Dungeon_FontColor() return 1,2,3,4,5,6 end\n"
    "function Get_Worldview_info_Dungeon_FontSize() return 10,11,12,13 end\n"
    "function Get_Worldview_info_Dungeon_SelectColor() return 1,2,3 end\n"
    "function Get_Worldview_info_Mob_Color() return 1,2,3,4,5,6 end\n"
    "function Get_Worldview_info_Mob_font() return 20,21,22,23 end\n"
    "function Get_Worldview_info_MobLv_Color() return 1,2,3,4,5,6 end\n"
    "function Get_Worldview_info_MobLv_font() return 30,31,32,33 end\n"
    "function Get_Worldview_info_BoxLine_Color() return 1,2,3 end\n"
    "function Get_Worldview_info_Sreach_Alpha() return 10,20,30,40,50,60,70 end\n"
    "function Get_Worldview_info_Sreach_Color(n) return 1,2,3 end\n"
    "function Get_Worldview_info_Run() return 1,2,3,4 end\n"
    "function Get_Worldview_info_Run_fontColor() return 1,2,3,4,5,6 end\n"
    "function Get_Worldview_info_Run_font() return 40,41,42,43 end\n"
    "function Get_Worldview_info_SelectBox_Color() return 1,2,3,4 end\n";

static void run(lua_State *L, const char *source)
{
    assert(luaL_loadbuffer(L, source, strlen(source), "test") == 0);
    assert(lua_pcall(L, 0, 0, 0) == 0);
}
static int load(struct WorldView *view, struct ClientLua *lua)
{
    int base = lua_gettop(lua->L), ok;
    file_calls = 0;
    ok = world_view_load(view, lua);
    assert(lua_gettop(lua->L) == base);
    return ok;
}

static void verify_basic(const struct WorldView *view)
{
    const struct WorldViewWorld *world = world_view_world_at(view, 0);
    const struct WorldViewArea *area;
    const struct WorldViewMap *map;
    int index = -1;
    assert(view->world_count == 2 && world && world->area_count == 2);
    assert(strcmp(world->name, "세계 1") == 0);
    assert(!world_view_world_at(view, 2));
    area = &world->areas[0];
    assert(area->id == 1 && area->map_count == 2);
    assert(strcmp(area->name, "지역") == 0 && strcmp(area->text, "던전") == 0);
    assert(area->rect.left == 5 && area->rect.bottom == 206);
    map = world_view_find_map(world, "town.rsw");
    assert(map && strcmp(map->text_a, "도시") == 0 && strlen(map->text_b) == 5000);
    assert(!world_view_find_map(world, "missing.rsw"));
    assert(world_view_map_at(world, &index, 50, 50) == map && index == 0);
    assert(!world_view_map_at(world, &index, 0, 50));
    assert(!world_view_map_at(world, &index, 150, 150));
    assert(world_view_field_at(world, &index, 50, 50) == area);
    assert(!world_view_field_at(world, &index, 250, 250));
    assert(world_view_map_at(world, &index, 250, 250) == &world->areas[1].maps[0] && index == 1);
    assert(view->dungeon_colour == 0x04010203 && view->dungeon_alpha == 0x04ffffff);
    assert(view->dungeon_font_colour[0] == 0x030201 && view->dungeon_font_colour[1] == 0x060504);
    assert(view->dungeon_select_colour[0] == 0x010203 && view->dungeon_select_colour[1] == 0x030201);
    assert(view->dungeon_font_size[3] == 13 && view->mob_font_size[2] == 22 && view->mob_level_font_size[1] == 31);
    assert(view->mob_colour[0] == 0x030201 && view->mob_level_colour[1] == 0x060504);
    assert(view->box_line_colour == 0x010203 && view->run_colour == 0x04010203 && view->select_box_colour == 0x04010203);
    assert(view->run_font_colour[1] == 0x060504 && view->run_font_size[0] == 40);
    assert(view->search_colour[0] == 0x0a010203 && view->search_colour[10] == 0x46010203);
    assert(view->search_colour[11] == view->search_colour[1] && view->search_colour[15] == view->search_colour[5]);
    assert(view->search_colour[6] == (uint32_t)(0xeeffffffu + 0x010203u));
    assert(strcmp(view->search_icon[1], "유저인터페이스\\worldview_interface\\wv_SreachMob1.bmp") == 0);
}

static void test_marks_and_layout(struct WorldView *view)
{
    const struct WorldViewRect *rect;
    struct WorldViewRect once;
    int icon, old, i;
    const int levels[] = {0,1,4,5,19,20,49,50,99,100,10000,10001,10005,10020,10050,10100};
    const int icons[] = {0,1,1,2,2,3,3,4,4,5,10,11,12,13,14,15};
    assert(world_view_select(view, -1) == 0 && world_view_select(view, INT_MAX) == 1);
    assert(!world_view_current_world(&(struct WorldView){0}));
    assert(!world_view_find_map(NULL, "town.rsw") && !world_view_map_at(NULL, &icon, 1, 1));
    assert(world_view_select_for_map(view, "town.rsw") == 0);
    assert(world_view_set_current(view, "group.rsw") && view->current.world == 1);
    fail_after = 0;
    assert(!world_view_add_mark(view, "town.rsw") && !view->mark_count && !view->marks);
    assert(!world_view_add_level_mark(view, "town.rsw", 20) && !view->level_mark_count && !view->level_marks);
    fail_after = -1;
    assert(world_view_add_mark(view, "town.rsw") && world_view_add_mark(view, "town.rsw") && view->mark_count == 1);
    assert(world_view_add_mark(view, "group.rsw") && view->mark_count == 2 && view->marks[1].world == 1);
    assert(!world_view_add_mark(view, NULL) && !world_view_add_mark(view, "missing.rsw"));
    assert(!world_view_mark(view, -1, 0) && !world_view_mark(view, 2, 0));
    assert(world_view_mark(view, 0, 1) && !world_view_mark(view, 1, 1));
    for (i = 0; i < (int)(sizeof levels / sizeof levels[0]); ++i) {
        world_view_clear_level_marks(view);
        assert(world_view_add_level_mark(view, "town.rsw", levels[i]));
        icon = 0; assert(world_view_level_mark_rect(view, &icon, 0, 1) && icon == icons[i]);
    }
    world_view_clear_level_marks(view);
    assert(world_view_add_level_mark(view, "town.rsw", 20));
    assert(world_view_add_level_mark(view, "town.rsw", 10) && view->level_marks[0].level == 20);
    assert(world_view_add_level_mark(view, "town.rsw", 10000) && view->level_marks[0].level == 10020);
    assert(world_view_add_level_mark(view, "town.rsw", 50) && view->level_marks[0].level == 10050);
    old = view->level_marks[0].level;
    assert(!world_view_add_level_mark(view, "town.rsw", INT_MAX) && view->level_marks[0].level == old);
    icon = 3; assert(!world_view_level_mark_rect(view, &icon, -1, 0) && icon == 3);
    texture.reqWidth = 400; texture.reqHeight = 400;
    renderer.width = 800; renderer.height = 817;
    world_view_prepare(view);
    assert(scales == 1 && view->resolution == 1);
    assert(strcmp(texture_path, "유저인터페이스\\world.bmp") == 0);
    rect = world_view_mark_rect(view, 0, 1);
    assert(rect && rect->left == 0 && rect->top == 17 && rect->right == 200 && rect->bottom == 217);
    assert(view->current.rect.left == 20 && view->current.rect.top == 57);
    once = *rect; world_view_prepare(view);
    assert(scales == 1 && memcmp(rect, &once, sizeof once) == 0);
    renderer.width = 640; renderer.height = 417;
    world_view_prepare(view); assert(view->resolution == 0 && rect->right == 160 && rect->bottom == 117);
    renderer.width = 1024; world_view_prepare(view); assert(view->resolution == 2);
    renderer.width = 1200; world_view_prepare(view); assert(view->resolution == 3);
    once = *rect; texture.reqWidth = 0; world_view_prepare(view); assert(memcmp(rect, &once, sizeof once) == 0);
    texture.reqWidth = 400;
    view->worlds[0].areas[0].maps[0].source_rect.right = INT_MAX;
    world_view_layout(view, INT_MAX, 817); assert(memcmp(rect, &once, sizeof once) == 0);
    view->worlds[0].areas[0].maps[0].source_rect.right = 100;
    renderer.width = 800; renderer.height = 817;
    world_view_draw(view, 0, 17, 640, 400); assert(draws == 1);
    assert(view->resolution == 1 && rect->right == 200 && rect->bottom == 217);
    missing_texture = 1; world_view_draw(view, 0, 17, 640, 400); assert(draws == 1);
    missing_texture = 0;
    texture.reqWidth = 0; world_view_draw(view, 0, 17, 640, 400); assert(draws == 1);
    texture.reqWidth = 800;
    world_view_draw(view, 0, 17, 640, 400); assert(draws == 2);
    assert(rect->right == 100 && rect->bottom == 217);
    texture.reqWidth = 400;
    world_view_draw(view, 0, 17, 0, 400); assert(draws == 2);
    assert(!world_view_set_current(view, "missing.rsw") && !*view->current.name);
    world_view_clear_marks(view); world_view_clear_level_marks(view);
    assert(!view->mark_count && !view->level_mark_count);
}

static void test_failure(struct WorldView *view, struct ClientLua *lua, const char *source)
{
    struct WorldViewWorld *original = view->worlds;
    int live = allocations;
    script = NULL; run(lua->L, source);
    assert(!load(view, lua));
    assert(view->worlds == original && allocations == live);
    script = basic; assert(load(view, lua)); verify_basic(view);
}

static void test_failures(struct WorldView *view, struct ClientLua *lua)
{
    int n, attempts, live;
    struct WorldViewWorld *original;
    allocation_calls = 0; script = basic; assert(load(view, lua)); attempts = allocation_calls;
    assert(attempts > 20);
    original = view->worlds; live = allocations;
    for (n = 0; n < attempts; ++n) {
        fail_after = n;
        assert(!load(view, lua));
        assert(view->worlds == original && allocations == live);
    }
    fail_after = -1;
    for (n = 1; n <= 5; ++n) { fail_file = n; assert(!load(view, lua)); assert(view->worlds == original && allocations == live); }
    fail_file = 0;
    test_failure(view, lua, "function queryWorld_List(n) error({}) end");
    test_failure(view, lua, "function queryWorld_List(n) return 'world',nil,nil,nil end");
    test_failure(view, lua, "function querymaptable_load(t,n) error('query failed') end");
    test_failure(view, lua, "function querymaptable_load(t,n) return 0/0,'bad',1,2,3,4 end");
    test_failure(view, lua, "function querymaptable_load(t,n) return 1,'bad',1/0,2,3,4 end");
    test_failure(view, lua, "function queryfieldtable_load(t,n) error('field failed') end");
    test_failure(view, lua, "function Get_Worldview_info_Dungeon() error({}) end");
    test_failure(view, lua, "function Get_Worldview_info_Dungeon_FontSize() return 'bad',2,3,4 end");
    assert(diagnostics >= 4);
}

struct LuaFailure {
    lua_Alloc allocate;
    void *context;
    int remaining;
};
static void *fail_lua_allocation(void *context, void *pointer, size_t old_size, size_t new_size)
{
    struct LuaFailure *failure = context;
    if (new_size > old_size) {
        if (!failure->remaining) return NULL;
        --failure->remaining;
    }
    return failure->allocate(failure->context, pointer, old_size, new_size);
}
static void test_lua_failures(struct WorldView *view, struct ClientLua *lua)
{
    struct LuaFailure failure;
    struct WorldViewWorld *original = view->worlds;
    int n, live = allocations, succeeded = 0;
    failure.allocate = lua_getallocf(lua->L, &failure.context);
    script = basic;
    for (n = 0; n < 2048; ++n) {
        lua_gc(lua->L, LUA_GCCOLLECT, 0);
        failure.remaining = n;
        lua_setallocf(lua->L, fail_lua_allocation, &failure);
        succeeded = load(view, lua);
        lua_setallocf(lua->L, failure.allocate, failure.context);
        if (succeeded) break;
        assert(view->worlds == original && allocations == live);
    }
    assert(succeeded && n > 20 && n < 2048);
    verify_basic(view);
}

static void test_large(struct WorldView *view, struct ClientLua *lua)
{
    int cycle, i;
    char name[64];
    script = NULL;
    run(lua->L,
        "function queryWorld_List(n) if n>20 then return end; return 'world'..n,tostring(n),nil,'world.bmp' end "
        "function querymaptable_load(t,n) if n>200 then return end; return n%90,t..'_'..n..'.rsw',n,n,n+1,n+1,'title','text' end");
    for (cycle = 0; cycle < 3; ++cycle) {
        assert(load(view, lua) && view->world_count == 20);
        for (i = 1; i <= 200; ++i) {
            sprintf_s(name, sizeof name, "20_%d.rsw", i);
            assert(world_view_add_mark(view, name));
        }
        assert(view->mark_count == 200);
        world_view_clear(view); assert(!allocations);
    }
    run(lua->L, "function queryWorld_List(n) assert(n<400); return 'world',nil,nil,'world.bmp' end");
    assert(load(view, lua) && view->world_count == 399);
    world_view_clear(view); assert(!allocations);
    run(lua->L,
        "function queryWorld_List(n) if n==1 then return 'world','maps',nil,'world.bmp' end end "
        "function querymaptable_load(t,n) assert(n<400); return 1,'map'..n,0,0,10,10 end");
    assert(load(view, lua) && view->worlds[0].areas[0].map_count == 399);
}

int main(void)
{
    struct ClientLua lua;
    struct WorldView view;
    lua.L = luaL_newstate(); assert(lua.L); luaL_openlibs(lua.L);
    world_view_initialize(&view);
    lua_pushliteral(lua.L, "keep this stack value");
    script = basic; assert(load(&view, &lua) && file_calls == 5);
    assert(strcmp(lua_tostring(lua.L, 1), "keep this stack value") == 0);
    verify_basic(&view);
    test_marks_and_layout(&view);
    test_failures(&view, &lua);
    test_lua_failures(&view, &lua);
    script = NULL;
    run(lua.L, "for k in pairs(_G) do if string.sub(k,1,19)=='Get_Worldview_info_' then _G[k]=nil end end");
    assert(load(&view, &lua));
    assert(view.dungeon_colour == 0 && view.dungeon_alpha == 0);
    assert(view.dungeon_font_colour[0] == 0xffffffff && view.dungeon_font_size[3] == 10);
    assert(view.search_colour[0] == 0xeeffffff && view.run_font_size[0] == 10);
    test_large(&view, &lua);
    script = NULL; run(lua.L, "function queryWorld_List(n) return end");
    assert(load(&view, &lua) && !view.world_count && !world_view_current_world(&view));
    assert(!world_view_set_current(&view, "town.rsw") && !world_view_add_mark(&view, "town.rsw"));
    world_view_draw(&view, 0, 17, 640, 400); assert(draws == 2);
    world_view_clear(&view); world_view_clear(&view); assert(!allocations);
    script = basic; assert(load(&view, &lua));
    lua_close(lua.L); lua.L = NULL;
    verify_basic(&view); assert(!world_view_load(&view, &lua));
    world_view_clear(&view); assert(!allocations);
    puts("World view: real Lua, owned data, transactional reload, marks, scaling, empty states, rendering and guarded cleanup passed");
    return 0;
}
