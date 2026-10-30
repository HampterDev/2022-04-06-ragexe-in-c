#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "fileio/file.h"
#include "fileio/file_api.h"
#include "owned_storage_test_allocator.h"

static void *checked_calloc(size_t count, size_t size)
{
    void *data;
    assert(!size || count <= SIZE_MAX / size);
    data = checked_malloc(count * size);
    if (data) memset(data, 0, count * size);
    return data;
}
static void *checked_realloc(void *old, size_t size)
{
    void *data = checked_malloc(size);
    if (data && old) {
        size_t count = ((struct Allocation *)old - 1)->size;
        memcpy(data, old, count < size ? count : size);
        checked_free(old);
    }
    return data;
}
#define malloc checked_malloc
#define calloc checked_calloc
#define realloc checked_realloc
#define free checked_free
#include "../src/resources/rma.c"
#undef malloc
#undef calloc
#undef realloc
#undef free

struct GameFileMgr g_fileMgr;
static BYTE *input;
static int input_size, present = 1, borrowed;
static unsigned errors, duplicate_errors, prefixes, failed_allocations;
static const char fixture[] = "// comment\r\n#rsxname 한글.rsx\r\n#action 2 4 10 20 1 3 5\n"
    "#interiorzoom 1.5\n#interioroffsetx -4\n#interioroffsety 9\n"
    "#interiorframes 2 7\n#shadowzoom 0.75\n#rsxname next.rsx\n#action 0 5 11 21";

struct Res *res_construct(struct Res *base)
{ memset(base,0,sizeof *base); base->typeIndex = -1; return base; }
void error_msg(const char *text, int flags)
{
    assert(!flags);
    if (!strcmp(text,"중복되는 액션이 있음")) ++duplicate_errors;
    else { assert(!strcmp(text,"Syntax Error in Rma")); ++errors; }
}
BYTE *game_file_mgr_get_data(struct GameFileMgr *manager, const char *name, int *size, BYTE disk)
{
    assert(manager == &g_fileMgr && name && !disk && !borrowed);
    if (!present) { *size = 0; return NULL; }
    *size = input_size; ++borrowed; return input;
}
void game_file_mgr_release_data(struct GameFileMgr *manager, void *data)
{ assert(manager == &g_fileMgr && data == input && borrowed == 1); --borrowed; }
BYTE game_file_mgr_is_file_exist(struct GameFileMgr *manager, const char *name, BYTE disk)
{ (void)manager; (void)name; (void)disk; return (BYTE)present; }
static void set_input(const void *data, size_t size)
{ assert(size <= INT_MAX && !borrowed); input = (BYTE *)data; input_size = (int)size; }
static int load_text(struct Res *base, const char *text)
{ set_input(text,strlen(text)); return resource_load(base,"fixture.rma"); }
static void clean(void) { assert(!outstanding && !borrowed); }
static void verify_fixture(struct Res *base)
{
    struct RmaData *data = &((struct RmaRes *)base)->data;
    struct RmaAction *a = rma_find_action(data,0), *b = rma_find_action(data,2);
    assert(data->action_count == 2 && data->name_count == 2);
    assert(data->interior_zoom == 1.5f && data->shadow_zoom == 0.75f);
    assert(data->interior_offset_x == -4 && data->interior_offset_y == 9);
    assert(data->interior_frames[0] == 2 && data->interior_frames[1] == 7);
    assert(!strcmp(data->names[0],"한글.rsx") && !strcmp(data->names[1],"next.rsx"));
    assert(a && a->a == 11 && a->b == 21 && a->c == 5 && a->name_index == 1 && !a->frame_count);
    assert(b && b->a == 10 && b->b == 20 && b->c == 4 && b->name_index == 0 && b->frame_count == 3);
    assert(b->frames[0] == 1 && b->frames[1] == 3 && b->frames[2] == 5 && !rma_find_action(data,1));
}
static void test_replacement(void)
{
    char buffer[sizeof fixture]; unsigned before;
    struct Res *base = rma_resource_create(); struct RmaData *data = &((struct RmaRes *)base)->data;
    struct RmaRes saved;
    assert(base && base->ops == &rma_resource_ops);
    assert(data->interior_zoom == 1.0f && data->shadow_zoom == 1.0f && !data->text);
    memcpy(buffer,fixture,sizeof fixture); set_input(buffer,sizeof buffer-1);
    assert(resource_load(base,"sample.rma")); memset(buffer,'x',sizeof buffer); verify_fixture(base);
    saved = *(struct RmaRes *)base; before = errors;
    present = 0; assert(!resource_load(base,"missing.rma")); present = 1;
    assert(!resource_load(base,NULL) && errors == before && !memcmp(base,&saved,sizeof saved));
    set_input(fixture,sizeof fixture-1); input_size = -1;
    assert(!resource_load(base,"negative-size.rma") && !memcmp(base,&saved,sizeof saved));
    assert(load_text(base,fixture)); verify_fixture(base);
    assert(load_text(base,""));
    assert(!data->action_count && !data->name_count && data->interior_zoom == 1.0f && data->shadow_zoom == 1.0f);
    assert(!data->interior_offset_x && !data->interior_offset_y && !data->interior_frames[0] && !data->interior_frames[1]);
    rma_data_clear(data); rma_data_clear(data); assert(outstanding == 1);
    assert(load_text(base,fixture)); verify_fixture(base); resource_destroy(base); clean();
}
static void test_invalid(void)
{
    static const char *invalid[] = {
        "#rsxname", "#action 0 1 2 3", "#interiorframes 1", "#unknown 1",
        "#rsxname a\n#action -1 1 2 3", "#rsxname a\n#action 9999999999999 1 2 3",
        "#rsxname a\n#action 1x 1 2 3", "#rsxname a\n#action 0 1 2",
        "#rsxname a\n#action 0 4 1 2 broken", "#rsxname a\n#action 0 4 1 2 3 2147483648",
        "#interiorzoom NaN", "#shadowzoom inf", "#interiorzoom 1e39", "#shadowzoom 1e9999",
        "#interiorzoom word", "#shadowzoom 2x", "#interioroffsetx 2147483648",
        "#interioroffsety -2147483649", "#interiorframes 1 nope", "#rsxname a//discarded",
        "#rsxname a\n#action 0 4 1x 2", "#rsxname a\n#action 0 2147483648 1 2"
    };
    struct Res *base = rma_resource_create(); struct RmaRes saved; unsigned held, before, i;
    assert(load_text(base,fixture)); saved = *(struct RmaRes *)base; held = outstanding;
    for (i = 0; i < sizeof invalid/sizeof *invalid; ++i) {
        before = errors;
        assert(!load_text(base,invalid[i]) && errors == before+1 && !borrowed);
        assert(!memcmp(base,&saved,sizeof saved) && outstanding == held); verify_fixture(base);
    }
    resource_destroy(base); clean();
}
static void test_directives(void)
{
    struct Res *base = rma_resource_create(); struct RmaData *data = &((struct RmaRes *)base)->data;
    struct RmaAction *a; unsigned before = duplicate_errors;
    char long_name[5100];
    const char nul[] = "#shadowzoom -2\0#unknown 1";
    assert(load_text(base,"\n\r\n// ignored\n#rsxname one\n#action 2 4 10 20 1 3 5\n"
        "#rsxname two\n#action 2 7 8 9 // keep frames\n#action 3 4 -1 20\n#action 3 4 12 30\n"));
    a = rma_find_action(data,2);
    assert(duplicate_errors == before+1 && a && a->a == 8 && a->b == 9 && a->c == 7);
    assert(a->name_index == 1 && a->frame_count == 3 && a->frames[2] == 5);
    assert(load_text(base,"#rsxname one\n#action 1 2 3 4 1 2 3\n#action 1 6 7 8 -4 99"));
    a = rma_find_action(data,1); assert(a && a->frame_count == 2 && a->frames[0] == -4 && a->frames[1] == 99);
    assert(load_text(base,"#rsxname a\n#action 2147483647 -2147483648 2147483647 -2147483648 -2147483648 2147483647\n"
        "#interiorzoom 0\n#shadowzoom -1\n#interioroffsetx -2147483648\n#interioroffsety 2147483647"));
    a = rma_find_action(data,INT_MAX);
    assert(data->action_count == 1 && a && a->c == INT_MIN && a->a == INT_MAX && a->b == INT_MIN);
    assert(a->frames[0] == INT_MIN && a->frames[1] == INT_MAX);
    assert(data->interior_zoom == 0 && data->shadow_zoom == -1);
    assert(data->interior_offset_x == INT_MIN && data->interior_offset_y == INT_MAX);
    assert(load_text(base,"#rsxname a extra ignored\n#action 0 4 1 2 3 discarded//ignored\n#shadowzoom 2 trailing"));
    a = rma_find_action(data,0); assert(a && a->frame_count == 1 && a->frames[0] == 3 && data->shadow_zoom == 2);
    set_input(nul,sizeof nul); assert(resource_load(base,"nul.rma") && data->shadow_zoom == -2);
    memcpy(long_name,"#rsxname ",9); memset(long_name+9,'x',5000); memcpy(long_name+5009,"\n#shadowzoom 2",14);
    set_input(long_name,5023); assert(resource_load(base,"long.rma"));
    assert(data->name_count == 1 && strlen(data->names[0]) == 5000 && data->shadow_zoom == 2.0f);
    resource_destroy(base); clean();
}
static void test_prefixes(void)
{
    SYSTEM_INFO sys; DWORD old; BYTE *pages,*guard; size_t n;
    GetSystemInfo(&sys); pages = VirtualAlloc(NULL,sys.dwPageSize*2,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    assert(pages); guard = pages+sys.dwPageSize; assert(VirtualProtect(guard,sys.dwPageSize,PAGE_NOACCESS,&old));
    for (n = 0; n < sizeof fixture; ++n) {
        struct Res *base = rma_resource_create(); struct RmaRes saved; unsigned held;
        assert(load_text(base,fixture)); saved = *(struct RmaRes *)base; held = outstanding;
        memcpy(guard-n,fixture,n); set_input(guard-n,n);
        if (!resource_load(base,"prefix.rma")) {
            assert(!memcmp(base,&saved,sizeof saved) && outstanding == held); verify_fixture(base);
        }
        assert(!borrowed); resource_destroy(base); clean(); ++prefixes;
    }
    assert(VirtualFree(pages,0,MEM_RELEASE));
}
static void test_failure_points(const char *text)
{
    struct Res *base; unsigned count,start,point;
    base = rma_resource_create(); start = allocation_calls;
    assert(load_text(base,text)); count = allocation_calls-start; resource_destroy(base); clean();
    for (point = 1; point <= count; ++point) {
        struct RmaRes saved; unsigned held;
        base = rma_resource_create(); assert(load_text(base,fixture)); saved = *(struct RmaRes *)base;
        held = outstanding; fail_at = allocation_calls+point;
        assert(!load_text(base,text)); fail_at = 0;
        assert(!memcmp(base,&saved,sizeof saved) && outstanding == held && !borrowed);
        verify_fixture(base); resource_destroy(base); clean(); ++failed_allocations;
    }
}
static void test_allocation_failures(void)
{
    char grown[12000]; size_t used = 0; unsigned i,j;
    fail_at = allocation_calls+1; assert(!rma_resource_create()); fail_at = 0; clean();
    test_failure_points(fixture);
    for (i = 0; i < 10; ++i) {
        used += (size_t)sprintf(grown+used,"#rsxname name%u\n#action %u 4 1 2",i,i);
        for (j = 0; j < 25; ++j) used += (size_t)sprintf(grown+used," %u",j);
        grown[used++] = '\n';
    }
    grown[used] = 0; assert(used < sizeof grown); test_failure_points(grown);
}
int main(void)
{
    test_replacement(); test_invalid(); test_directives(); test_prefixes(); test_allocation_failures();
    printf("RMA owner: %u guarded prefixes, %u allocation failure points, sparse IDs, Korean names, duplicate directives, bounded numbers and transactional replacement pass.\n",prefixes,failed_allocations);
    return 0;
}
