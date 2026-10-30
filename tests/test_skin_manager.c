#include "std/string.h"
#include "fileio/resource_api.h"
#include "ui/skin.h"
#include "ui/ui_std.h"
#include "core/fatal.h"
#include "std/std.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char g_clientDir[] = "?:\\skin-test-no-files";
static unsigned allocations, releases, cache_purges;
static int resource_manager;

const char *ui_skin_path(const char *path) { abort(); }
void *res_mgr_get(struct ResMgr *self, const char *path) { abort(); }
char *str_find_last(const char *text, int c) { abort(); }
DWORD bitmap_res_get_pixel(const struct BitmapRes *self, int x, int y) { abort(); }
void text_to_lower(struct String *s) { abort(); }
void *std_allocate(size_t n, size_t size, size_t limit) { abort(); }

void *std_allocate_bytes(size_t bytes)
{
    void *p = malloc(bytes);
    assert(p);
    ++allocations;
    return p;
}

void *mem_alloc(size_t bytes)
{
    return std_allocate_bytes(bytes);
}

void mem_free(void *p)
{
    assert(p);
    ++releases;
    free(p);
}

__declspec(noreturn) void fatal_error(const char *message)
{
    (void)message;
    abort();
}

int crt_sprintf(char *out, const char *format, ...)
{
    int result;
    va_list args;
    va_start(args, format);
    result = vsprintf(out, format, args);
    va_end(args);
    return result;
}

void dir_remove_tree(const char *path)
{
    (void)path;
    assert(!"No temporary skin directory exists in this fixture");
}

struct ResMgr *g_resMgr(void)
{
    return (struct ResMgr *)&resource_manager;
}

void res_mgr_purge_by_extension(struct ResMgr *manager, const char *extension)
{
    assert(manager == g_resMgr() && strcmp(extension, "bmp") == 0);
    ++cache_purges;
}

static void strings(struct Vec *v, const char *first, const char *second)
{
    struct String *values = std_allocate_bytes(2 * sizeof *values);
    string_init(&values[0]);
    string_init(&values[1]);
    string_assign_cstr(&values[0], first);
    string_assign_cstr(&values[1], second);
    v->first = values;
    v->last = v->end = values + 2;
}

static void flags(struct Vec *v)
{
    int *values = std_allocate_bytes(2 * sizeof *values);
    values[0] = values[1] = 0;
    v->first = values;
    v->last = v->end = values + 2;
}

static void verify_shutdown(void)
{
    assert(allocations >= 7 && releases == allocations);
    assert(!g_skinMgr.names.first && !g_skinMgr.paths.first);
    assert(!g_skinMgr.extracted.first && !g_skinMgr.is_zip.first);
    assert(g_skinMgr.name.size == 0 && !g_skinMgr.name.data);
    puts("skin manager: startup order, selection, cache invalidation and owned shutdown passed");
}

int main(void)
{
    const char requested[] = "An explicitly requested skin with a heap-owned name";
    assert(g_skinMgr.name.size == 0 && !g_skinMgr.name.data);
    assert(g_skinMgr.current == 0);
    assert(atexit(verify_shutdown) == 0);
    skin_name_init();
    basic_skin_name_init();
    assert(strcmp(string_cstr(&g_skinMgr.name), "") == 0);
    assert(strcmp(skin_mgr_get_name(&g_skinMgr, -1), "<Basic Skin>") == 0);
    string_assign_cstr(&g_skinMgr.name, requested);
    strings(&g_skinMgr.names, "Short", "A second skin with a longer name");
    strings(&g_skinMgr.paths, "skin\\short\\", "skin\\a-directory-with-a-long-name\\");
    flags(&g_skinMgr.extracted);
    flags(&g_skinMgr.is_zip);
    assert(strcmp(skin_mgr_get_current_name(&g_skinMgr), "Short") == 0);
    assert(strcmp(skin_mgr_get_current_path(&g_skinMgr), "skin\\short\\") == 0);
    assert(strcmp(skin_mgr_get_name(&g_skinMgr, 1), "A second skin with a longer name") == 0);
    skin_mgr_set_current_skin_num(&g_skinMgr, 0);
    assert(cache_purges == 0);
    skin_mgr_set_current_skin_num(&g_skinMgr, 1);
    assert(cache_purges == 1);
    assert(strcmp(skin_mgr_get_current_path(&g_skinMgr), "skin\\a-directory-with-a-long-name\\") == 0);
    skin_mgr_set_current_skin_num(&g_skinMgr, 1);
    assert(cache_purges == 1);
    skin_mgr_set_current_skin_num(&g_skinMgr, -1);
    assert(cache_purges == 2);
    assert(strcmp(skin_mgr_get_current_name(&g_skinMgr), "<Basic Skin>") == 0);
    assert(strcmp(string_cstr(&g_skinMgr.name), requested) == 0);
    return 0;
}
