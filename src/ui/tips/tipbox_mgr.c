#include "ui/tips/tipbox_mgr.h"
#include "ui/tips/tipbox_picture.h"
#include <windows.h>
#include <float.h>
#include <limits.h>
#include <mbstring.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

struct TipRow {
    int id, listed;
    char *title, *image;
    char **pages;
    size_t page_count, page_capacity;
    struct TipEffect *effects;
    size_t effect_count, effect_capacity;
    int image_kind, image_arg;
};

struct TipTable {
    struct TipRow *rows;
    size_t count, capacity;
    int failed;
};

static struct TipTable table;
static int *found;
static size_t found_count;
static char *picture_path;
static struct TipboxView view = {{"", ""}, TIP_PICTURE_NONE, 0, 0, 0, 0, 0, 0, 0, "", 0};

static void free_table(struct TipTable *data)
{
    size_t i, j;
    for (i = 0; i < data->count; ++i) {
        struct TipRow *row = &data->rows[i];
        free(row->title);
        free(row->image);
        for (j = 0; j < row->page_count; ++j) free(row->pages[j]);
        free(row->pages);
        free(row->effects);
    }
    free(data->rows);
    memset(data, 0, sizeof *data);
}

static void empty_view(void)
{
    free(picture_path);
    picture_path = NULL;
    memset(&view, 0, sizeof view);
    view.tip.title = view.tip.image = view.sprite = "";
    view.picture = TIP_PICTURE_NONE;
}

void tipbox_clear(void)
{
    empty_view();
    free_table(&table);
    free(found);
    found = NULL;
    found_count = 0;
}

const struct TipboxView *tipbox_view(void)
{
    return &view;
}

static void *grow(void *data, size_t *capacity, size_t count, size_t size)
{
    size_t next;
    void *result;
    if (count <= *capacity) return data;
    if (count > (size_t)-1 / size) return NULL;
    next = *capacity > (size_t)-1 / 2 ? count : *capacity * 2;
    if (next < 8) next = 8;
    if (next < count || next > (size_t)-1 / size) next = count;
    result = realloc(data, next * size);
    if (result) *capacity = next;
    return result;
}

static char *join(const char *first, const char *last)
{
    size_t a = strlen(first), b = strlen(last);
    char *text;
    if (a > (size_t)-1 - b - 1) return NULL;
    text = malloc(a + b + 1);
    if (text) {
        memcpy(text, first, a);
        memcpy(text + a, last, b + 1);
    }
    return text;
}

static size_t lower_bound(const struct TipTable *data, int id)
{
    size_t lo = 0, hi = data->count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (data->rows[mid].id < id) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}

static struct TipRow *get_row(struct TipTable *data, int id)
{
    size_t at = lower_bound(data, id);
    struct TipRow *rows;
    if (at < data->count && data->rows[at].id == id) return &data->rows[at];
    rows = grow(data->rows, &data->capacity, data->count + 1, sizeof *rows);
    if (!rows) { data->failed = 1; return NULL; }
    data->rows = rows;
    memmove(rows + at + 1, rows + at, (data->count - at) * sizeof *rows);
    memset(rows + at, 0, sizeof *rows);
    rows[at].id = id;
    ++data->count;
    return rows + at;
}

static void layout(void)
{
    int texture;
    char *resource_name;
    free(picture_path);
    picture_path = NULL;
    view.sprite = "";
    view.picture = TIP_PICTURE_NONE;
    view.left = view.top = view.right = view.bottom = view.scale = 0;
    view.tex_arg1 = view.tex_arg2 = 0;
    if (!view.tip.image[0]) return;
    texture = strstr(view.tip.image, "유저인터페이스\\") != NULL;
    picture_path = texture ? join(view.tip.image, ".bmp") : join("sprite\\", view.tip.image);
    if (!picture_path) return;
    view.sprite = picture_path;
    if (texture) tipbox_picture_layout(&view, picture_path, 1);
    else {
        resource_name = join(picture_path, ".spr");
        if (!resource_name) return;
        tipbox_picture_layout(&view, resource_name, 0);
        free(resource_name);
    }
}

void tipbox_show(int id)
{
    size_t at = lower_bound(&table, id);
    if (at < table.count && table.rows[at].id == id) {
        const struct TipRow *row = &table.rows[at];
        view.tip.title = row->title ? row->title : "";
        view.tip.image = row->image ? row->image : "";
        view.tip.pages = (const char *const *)row->pages;
        view.tip.page_count = row->page_count;
        view.tip.effects = row->effects;
        view.tip.effect_count = row->effect_count;
        view.tip.image_kind = row->image_kind;
        view.tip.image_arg = row->image_arg;
    }
    view.page = 0;
    layout();
}

const struct TipboxTip *tipbox_pick_random(void)
{
    size_t i;
    found_count = 0;
    for (i = 0; i < table.count; ++i)
        if (table.rows[i].id < 1000 && table.rows[i].listed)
            found[found_count++] = table.rows[i].id;
    if (found_count) tipbox_show(found[(unsigned)rand() % found_count]);
    else empty_view();
    return &view.tip;
}

const char *tipbox_title(int id)
{
    size_t at = lower_bound(&table, id);
    return at < table.count && table.rows[at].id == id && table.rows[at].title
        ? table.rows[at].title : "";
}

size_t tipbox_search(const char *words)
{
    size_t i;
    found_count = 0;
    if (!words) return 0;
    for (i = 0; i < table.count; ++i) {
        const struct TipRow *row = &table.rows[i];
        if (row->listed && _mbsstr((const unsigned char *)(row->title ? row->title : ""),
                                 (const unsigned char *)words))
            found[found_count++] = row->id;
    }
    return found_count;
}

size_t tipbox_search_count(void)
{
    return found_count;
}

int tipbox_search_id(size_t index)
{
    return index < found_count ? found[index] : 0;
}

void tipbox_set_page(int page)
{
    if (page < 0 || !view.tip.page_count) page = 0;
    else if ((size_t)page >= view.tip.page_count) page = (int)view.tip.page_count - 1;
    view.page = page;
}

const char *tipbox_page_text(void)
{
    return (size_t)view.page < view.tip.page_count ? view.tip.pages[view.page] : "";
}

int tipbox_on_last_page(void)
{
    return !view.tip.page_count || (size_t)view.page + 1 >= view.tip.page_count;
}

static int lua_answer(lua_State *L, int ok, const char *message)
{
    lua_pushboolean(L, ok);
    lua_pushstring(L, message);
    return 2;
}

static struct TipTable *builder(lua_State *L)
{
    return lua_touserdata(L, lua_upvalueindex(1));
}

static int allocation_failed(lua_State *L)
{
    builder(L)->failed = 1;
    return lua_answer(L, 0, "not enough memory for tips");
}

static int integer(lua_State *L, int index, int *out)
{
    double n;
    if (!lua_isnumber(L, index)) return 0;
    n = lua_tonumber(L, index);
    if (!(n >= INT_MIN && n <= INT_MAX)) return 0;
    *out = (int)n;
    return 1;
}

static int lua_add_tip(lua_State *L)
{
    struct TipRow *row;
    char *title, *image;
    int id, flag;
    if (!lua_isstring(L, -1)) return lua_answer(L, 0, "4th argment is must string");
    if (!integer(L, -2, &flag)) return lua_answer(L, 0, "3nd argment is must number");
    if (!lua_isstring(L, -3)) return lua_answer(L, 0, "2st argment is must string");
    if (!integer(L, -4, &id) || lua_tonumber(L, -4) == 0)
        return lua_answer(L, 0, "1st argment is must number");
    lua_tolstring(L, -3, NULL);
    lua_tolstring(L, -1, NULL);
    row = get_row(builder(L), id);
    if (!row) return allocation_failed(L);
    title = join(lua_tostring(L, -3), "");
    image = join(lua_tostring(L, -1), "");
    if (!title || !image) {
        free(title); free(image);
        return allocation_failed(L);
    }
    free(row->title); free(row->image);
    row->title = title; row->image = image; row->listed = flag == 1;
    return lua_answer(L, 1, "good");
}

static int lua_add_page(lua_State *L)
{
    struct TipRow *row;
    char **pages, *page, *read, *write;
    int id;
    if (!integer(L, -2, &id)) return lua_answer(L, 0, "2st argment is must number");
    if (!lua_isstring(L, -1)) return lua_answer(L, 0, "1th argment is must string");
    lua_tolstring(L, -1, NULL);
    row = get_row(builder(L), id);
    if (!row || row->page_count == INT_MAX) return allocation_failed(L);
    pages = grow(row->pages, &row->page_capacity, row->page_count + 1, sizeof *pages);
    if (!pages) return allocation_failed(L);
    row->pages = pages;
    page = join(lua_tostring(L, -1), "");
    if (!page) return allocation_failed(L);
    for (read = write = page; *read; ++read) {
        if (read[0] == '\\' && read[1] == 'n') { *write++ = '\n'; ++read; }
        else *write++ = *read;
    }
    *write = 0;
    row->pages[row->page_count++] = page;
    return lua_answer(L, 1, "good");
}

static int lua_add_img_coord(lua_State *L)
{
    struct TipRow *row;
    int id, kind, arg;
    if (!integer(L, -1, &arg)) return lua_answer(L, 0, "3nd argment is must number");
    if (!integer(L, -2, &kind) || kind == INT_MIN) return lua_answer(L, 0, "2nd argment is must number");
    if (!integer(L, -3, &id)) return lua_answer(L, 0, "1st argment is must number");
    row = get_row(builder(L), id);
    if (!row) return allocation_failed(L);
    row->image_kind = kind; row->image_arg = arg;
    return lua_answer(L, 1, "good");
}

static int lua_add_page_ex(lua_State *L)
{
    static const char *const errors[] = {
        "1st argment is must number", "2st argment is must number", "3st argment is must number",
        "4st argment is must number", "5st argment is must number", "6st argment is must number",
        "7nd argment is must number", "8nd argment is must number"
    };
    struct TipEffect effect, *effects;
    struct TipRow *row;
    int v[7], i;
    double duration;
    for (i = 6; i >= 0; --i)
        if (!integer(L, i - 8, &v[i])) return lua_answer(L, 0, errors[i]);
    if (!lua_isnumber(L, -1)) return lua_answer(L, 0, errors[7]);
    duration = lua_tonumber(L, -1);
    if (!(duration >= -FLT_MAX && duration <= FLT_MAX) ||
        (v[1] == 6 && !((float)duration >= INT_MIN && (double)(float)duration <= INT_MAX)))
        return lua_answer(L, 0, errors[7]);
    if (v[0] <= 0 || v[1] == -1) return lua_answer(L, 1, "good");
    row = get_row(builder(L), v[0]);
    if (!row) return allocation_failed(L);
    effects = grow(row->effects, &row->effect_capacity, row->effect_count + 1, sizeof *effects);
    if (!effects) return allocation_failed(L);
    row->effects = effects;
    effect.kind = v[1]; effect.blinks = v[2];
    effect.x0 = v[3]; effect.y0 = v[4]; effect.x1 = v[5]; effect.y1 = v[6];
    effect.duration = (float)duration;
    row->effects[row->effect_count++] = effect;
    return lua_answer(L, 1, "good");
}

static int load_script(lua_State *L)
{
    static const struct { const char *name; lua_CFunction callback; } callbacks[] = {
        {"AddTip", lua_add_tip}, {"AddPage", lua_add_page},
        {"AddImgcoord", lua_add_img_coord}, {"AddPageEx", lua_add_page_ex}
    };
    struct TipTable *data = lua_touserdata(L, 1);
    size_t i;
    luaL_openlibs(L);
    for (i = 0; i < sizeof callbacks / sizeof callbacks[0]; ++i) {
        lua_pushlightuserdata(L, data);
        lua_pushcclosure(L, callbacks[i].callback, 1);
        lua_setglobal(L, callbacks[i].name);
    }
    if (luaL_loadfile(L, "System\\tipbox.lua") != 0) {
        lua_pop(L, 1);
        if (luaL_loadfile(L, "System\\tipbox.lub") != 0) return lua_error(L);
    }
    lua_call(L, 0, 0);
    lua_getglobal(L, "main");
    lua_call(L, 0, 2);
    if (!lua_isstring(L, -1)) return luaL_error(L, "second return value is not string...");
    if (!lua_toboolean(L, -2)) return luaL_error(L, "error found :%s", lua_tostring(L, -1));
    if (data->failed) return luaL_error(L, "not enough memory for tips");
    return 0;
}

int tipbox_load(void)
{
    struct TipTable next = {0};
    lua_State *L = luaL_newstate();
    int *ids = NULL, ok;
    if (!L) return 0;
    ok = lua_cpcall(L, load_script, &next) == 0;
    if (!ok) {
        const char *error = lua_type(L, -1) == LUA_TSTRING ? lua_tostring(L, -1) : NULL;
        MessageBoxA(NULL, error ? error : "TipBox script failed", "TipBox", 0);
    }
    lua_close(L);
    if (next.failed) ok = 0;
    if (ok && next.count) {
        if (next.count <= (size_t)-1 / sizeof *ids) ids = malloc(next.count * sizeof *ids);
        if (!ids) ok = 0;
    }
    if (!ok) { free_table(&next); return 0; }
    tipbox_clear();
    table = next;
    found = ids;
    return 1;
}
