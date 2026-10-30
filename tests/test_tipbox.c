#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mbctype.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "../src/ui/tips/tipbox_picture.c"

static int fail_after = -1, allocations, errors, loads, fail_state, resource_mode;
static const char *script, *fallback;
static char last_error[1024], resource_path[256];
static struct BitmapRes bitmap;
static struct SprRes sprite;
static struct SprImg frame;

static void *checked_malloc(size_t size)
{
    unsigned char *raw;
    if (fail_after == 0) return NULL;
    if (fail_after > 0) --fail_after;
    assert(size < 1000000);
    raw = malloc(size + 24); assert(raw);
    *(size_t *)raw = size;
    memset(raw + 8, 0x63, 8); memset(raw + 16 + size, 0x57, 8);
    ++allocations;
    return raw + 16;
}
static void checked_free(void *p)
{
    unsigned char *raw;
    size_t size, i;
    if (!p) return;
    raw = (unsigned char *)p - 16; size = *(size_t *)raw;
    for (i = 0; i < 8; ++i) { assert(raw[8 + i] == 0x63); assert(raw[16 + size + i] == 0x57); }
    memset(raw, 0xee, size + 24); free(raw); --allocations;
}
static void *checked_realloc(void *p, size_t size)
{
    void *next = checked_malloc(size);
    if (next && p) {
        size_t old = *(size_t *)((unsigned char *)p - 16);
        memcpy(next, p, old < size ? old : size);
        checked_free(p);
    }
    return next;
}
static int WINAPI error_box(HWND window, LPCSTR text, LPCSTR caption, UINT type)
{
    assert(!window && text && strcmp(caption, "TipBox") == 0 && !type);
    strncpy_s(last_error, sizeof last_error, text, _TRUNCATE);
    ++errors;
    return 0;
}
static lua_State *new_state(void)
{
    return fail_state ? NULL : luaL_newstate();
}
static int load_file(lua_State *L, const char *name)
{
    const char *source;
    ++loads;
    if (strcmp(name, "System\\tipbox.lua") == 0) source = script;
    else { assert(strcmp(name, "System\\tipbox.lub") == 0); source = fallback; }
    if (!source) { lua_pushstring(L, "missing test script"); return LUA_ERRFILE; }
    return luaL_loadbuffer(L, source, strlen(source), name);
}
#define malloc checked_malloc
#define realloc checked_realloc
#define free checked_free
#define MessageBoxA error_box
#define luaL_loadfile load_file
#define luaL_newstate new_state
#include "../src/ui/tips/tipbox_mgr.c"
#undef malloc
#undef realloc
#undef free
#undef MessageBoxA
#undef luaL_loadfile
#undef luaL_newstate

void client_lua_loadfile_hook(const char *filename) { (void)filename; abort(); }
struct ResMgr *g_resMgr(void) { return (struct ResMgr *)1; }
void *res_mgr_get(struct ResMgr *manager, const char *name)
{
    assert(manager == (struct ResMgr *)1);
    strcpy_s(resource_path, sizeof resource_path, name);
    if (resource_mode == 1) return NULL;
    if (strstr(name, ".bmp")) return &bitmap;
    sprite.images[0].items = resource_mode == 3 ? NULL : &frame;
    sprite.images[0].count = resource_mode == 2 ? 0 : 1;
    return &sprite;
}

static const char normal[] =
    "function main() "
    " AddPage(2, 'first\\\\n한글'); AddImgcoord(2,5,50); "
    " AddTip(10,'other',1,''); AddTip(2,'old',1,'old'); "
    " AddTip(-2,'lower',1,''); AddTip(1000,'limit',1,''); AddTip(20,'hidden',2,''); "
    " AddTip(2,'한글 title#extra',1,'test'); "
    " AddPage(2,string.rep('x',5000)..'끝'); "
    " AddPageEx(2,6,1,10,20,30,40,5); AddPageEx(2,3,0,4,5,6,7,2.5); "
    " AddPageEx(2,-1,0,0,0,0,0,0); AddPageEx(0,6,0,0,0,0,0,0); "
    " collectgarbage('collect'); return true,'good' end";

static void check_normal(void)
{
    const struct TipboxView *v;
    const struct TipEffect *e;
    assert(strcmp(tipbox_title(2), "한글 title#extra") == 0);
    assert(strcmp(tipbox_title(999), "") == 0);
    assert(tipbox_search("") == 4);
    assert(tipbox_search_id(0) == -2 && tipbox_search_id(1) == 2 && tipbox_search_id(2) == 10 && tipbox_search_id(3) == 1000);
    assert(tipbox_search_id(4) == 0 && tipbox_search_id((size_t)-1) == 0);
    assert(tipbox_search("한글") == 1 && tipbox_search_id(0) == 2);
    assert(tipbox_search("TITLE") == 0 && tipbox_search("first") == 0);
    assert(tipbox_search(NULL) == 0);
    tipbox_show(2); v = tipbox_view();
    assert(v->tip.page_count == 2 && v->tip.effect_count == 2);
    assert(strcmp(tipbox_page_text(), "first\n한글") == 0 && !tipbox_on_last_page());
    e = v->tip.effects;
    assert(e[0].kind == 6 && e[0].blinks == 1 && e[0].x0 == 10 && e[0].y0 == 20 && e[0].x1 == 30 && e[0].y1 == 40 && e[0].duration == 5);
    assert(e[1].kind == 3 && !e[1].blinks && e[1].duration == 2.5f);
    assert(v->picture == TIP_PICTURE_SPRITE && strcmp(v->sprite,"sprite\\test") == 0 && strcmp(resource_path,"sprite\\test.spr") == 0);
    assert(fabs(v->left - 95) < .001 && fabs(v->top - 162) < .001 && v->right == 50 && v->bottom == 30 && v->scale == .5f);
    tipbox_set_page(INT_MAX); assert(tipbox_view()->page == 1 && tipbox_on_last_page());
    assert(strlen(tipbox_page_text()) == 5002 && strcmp(tipbox_page_text()+5000,"끝") == 0);
    tipbox_set_page(INT_MIN); assert(tipbox_view()->page == 0);
    tipbox_set_page(1); tipbox_show(999); assert(tipbox_view()->page == 0 && strcmp(tipbox_view()->tip.title,"한글 title#extra") == 0);
}

int main(void)
{
    const struct TipboxView *v;
    const char *title;
    size_t i;
    int fail, old_allocations;
    const char *bad[] = {
        "AddTip(3,'partial',1,''); error('chunk failed')",
        "function main() AddTip(3,'partial',1,'');error({}) end",
        "function main() AddTip(3,'partial',1,'');return false,'bad data' end",
        "function main() AddTip(3,'partial',1,'');return true,{} end",
        "AddTip(3,'partial',1,'')"
    };
    _setmbcp(949);
    bitmap.width = 500; bitmap.height = 600; frame.width = 100; frame.height = 60;
    assert(!tipbox_pick_random()->page_count && !strcmp(tipbox_page_text(),"") && tipbox_on_last_page());
    script = normal; assert(tipbox_load() && loads == 1); check_normal();
    for (i = 0; i < 100; ++i) {
        const struct TipboxTip *tip = tipbox_pick_random();
        assert(!strcmp(tip->title,"lower") || !strcmp(tip->title,"other") || !strcmp(tip->title,"한글 title#extra"));
        assert(tipbox_search_count() == 3);
    }
    tipbox_show(2); tipbox_set_page(1); title = tipbox_view()->tip.title;
    old_allocations = allocations;
    for (i = 0; i < sizeof bad / sizeof bad[0]; ++i) {
        script = bad[i]; assert(!tipbox_load());
        assert(tipbox_view()->tip.title == title && tipbox_view()->page == 1 && allocations == old_allocations);
    }
    assert(errors == 5);
    script = "function main() local p=newproxy(true);getmetatable(p).__gc=function() AddTip(3,'finalizer',1,'') end;keep=p;return true,'good' end";
    fail_after = 0; assert(!tipbox_load() && tipbox_view()->tip.title == title && allocations == old_allocations); fail_after = -1;
    fail_state = 1; assert(!tipbox_load() && tipbox_view()->tip.title == title); fail_state = 0;
    script = NULL; fallback = normal; loads = 0; assert(tipbox_load() && loads == 2); check_normal();
    script = "syntax !"; loads = 0; assert(tipbox_load() && loads == 2);
    fallback = NULL; assert(!tipbox_load());
    script = normal; tipbox_show(2); title = tipbox_view()->tip.title; old_allocations = allocations;
    for (fail = 0; fail < 100; ++fail) {
        fail_after = fail;
        if (tipbox_load()) break;
        assert(tipbox_view()->tip.title == title && allocations == old_allocations);
    }
    assert(fail > 10 && fail < 100); fail_after = -1; check_normal();
    resource_mode = 1; tipbox_show(2); assert(tipbox_view()->picture == TIP_PICTURE_NONE);
    resource_mode = 2; tipbox_show(2); assert(tipbox_view()->picture == TIP_PICTURE_NONE);
    resource_mode = 3; tipbox_show(2); assert(tipbox_view()->picture == TIP_PICTURE_NONE); resource_mode = 0;
    for (fail = 0; fail < 2; ++fail) {
        fail_after = fail; tipbox_show(2); assert(tipbox_view()->picture == TIP_PICTURE_NONE);
    }
    fail_after = -1;
    script = "function main() AddTip(1,'bitmap',1,'유저인터페이스\\\\test');AddImgcoord(1,1,100);return 0,123 end";
    assert(tipbox_load()); tipbox_show(1); v = tipbox_view();
    assert(v->picture == TIP_PICTURE_TEXTURE && v->left == 1 && v->top == 17 && v->right == 500 && v->bottom == 600);
    assert(v->tex_arg1 == 211 && v->tex_arg2 == 246 && strcmp(v->sprite,"유저인터페이스\\test.bmp") == 0);
    assert(!strcmp(tipbox_page_text(),"") && tipbox_on_last_page()); tipbox_set_page(INT_MAX); assert(v->page == 0);
    script = "function main() "
        "assert(not AddTip(0,'x',1,'')); assert(not AddTip(1/0,'x',1,'')); assert(not AddTip(0/0,'x',1,'')); "
        "assert(not AddTip(2,'x',{},'')); assert(not AddPage(1,{})); assert(not AddImgcoord(1,-2147483648,1)); "
        "assert(not AddPageEx(1,6,0,0,0,0,0,2147483647)); assert(not AddPageEx(1,1,0,0,0,0,0,1/0)); "
        "for i=200,1,-1 do AddTip(i,'title'..i,1,''); for j=1,10 do AddPage(i,'page'..j);AddPageEx(i,1,0,j,j,j,j,1) end end "
        "return true,'good' end";
    for (i = 0; i < 10; ++i) {
        assert(tipbox_load()); assert(tipbox_search("") == 200 && tipbox_search_id(199) == 200);
        tipbox_show(100); assert(tipbox_view()->tip.page_count == 10 && tipbox_view()->tip.effect_count == 10);
        tipbox_set_page(9); assert(strcmp(tipbox_page_text(),"page10") == 0);
    }
    script = "function main() AddTip(1000,'limit',1,'');return true,'good' end";
    assert(tipbox_load() && !tipbox_pick_random()->page_count && !strcmp(tipbox_view()->tip.title,""));
    script = "function main() return true,'empty' end"; assert(tipbox_load());
    tipbox_clear(); tipbox_clear(); assert(allocations == 0 && !tipbox_search_count());
    puts("Tipbox: real Lua callbacks, C ownership, sorted search, paging, pictures, allocation rollback and repeated cleanup passed");
    return 0;
}
