#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct { void *pointer; size_t size; } blocks[20000];
static size_t live;
static int fail_after = -1;
static void *tracked_malloc(size_t size)
{
    void *p;
    if (fail_after == 0) return NULL;
    if (fail_after > 0) --fail_after;
    p = malloc(size); assert(p && live < 20000);
    blocks[live].pointer = p; blocks[live++].size = size;
    return p;
}
static void tracked_free(void *p)
{
    size_t i;
    if (!p) return;
    for (i=0; i<live && blocks[i].pointer!=p; ++i) {}
    assert(i<live);
    memset(p, 0xdd, blocks[i].size); free(p); blocks[i]=blocks[--live];
}
static void *tracked_calloc(size_t count, size_t size)
{
    void *p = tracked_malloc(count*size);
    if (p) memset(p, 0, count*size);
    return p;
}
static void *tracked_realloc(void *p, size_t size)
{
    size_t i;
    void *next=tracked_malloc(size);
    if (!next) return NULL;
    if (p) {
        for (i=0; i<live && blocks[i].pointer!=p; ++i) {}
        assert(i<live);
        memcpy(next,p,blocks[i].size<size?blocks[i].size:size);
        tracked_free(p);
    }
    return next;
}
#define malloc tracked_malloc
#define calloc tracked_calloc
#define realloc tracked_realloc
#define free tracked_free
#include "../src/contents/map_info.c"
#undef malloc
#undef calloc
#undef realloc
#undef free

static char default_name[64]="Unknown map";
static char shown_line1[2048], shown_line2[2048], shown_ornament[2048];
static int banners, released, rendered, banner_finished, fail_banner;
char *msg_str(int id) { assert(id==0xbb); return default_name; }
struct MapNameDisplay *map_name_display_construct(struct MapNameDisplay *s,
    const char *line1, const char *line2, const char *ornament)
{
    memset(s,0,sizeof *s);
    if (fail_banner) return NULL;
    strcpy(shown_line1,line1); strcpy(shown_line2,line2); strcpy(shown_ornament,ornament);
    ++banners; return s;
}
void map_name_display_destruct(struct MapNameDisplay *s) { assert(s); ++released; }
char map_name_display_render(struct MapNameDisplay *s)
{ assert(s); ++rendered; return !banner_finished; }
static void reset(void)
{
    fail_after=-1; map_info_destroy(); map_info_destroy(); assert(!live && !g_mapInfo);
    assert(banners==released); banner_finished=fail_banner=0;
    assert(map_info_create()); assert(!map_info_create()); assert(!map_info_load(g_mapInfo));
}
static void test_failures(void)
{
    int success;
    fail_after=0; assert(!map_info_create() && !g_mapInfo && !live); fail_after=-1;
    for (int existing=0;existing<2;++existing) {
        success=0;
        for (int n=0;n<6;++n) {
            size_t before;
            reset();
            if (existing) assert(map_info_set_display_name(g_mapInfo,"map","original",1));
            before=live; fail_after=n; success=map_info_set_display_name(g_mapInfo,"map","changed",1);
            fail_after=-1;
            if (success) break;
            assert(live==before);
            assert(!strcmp(map_info_display_name(g_mapInfo,"map"),existing?"original":"Unknown map"));
        }
        assert(success);
    }
    reset(); fail_after=0; strcpy(default_name,"New default");
    assert(!map_info_load(g_mapInfo)); fail_after=-1;
    assert(!strcmp(map_info_display_name(g_mapInfo,"missing"),"Unknown map"));
    strcpy(default_name,"Unknown map");
}
static void test_storage(void)
{
    const char *borrowed;
    char key[64],value[64],input[32]="copied";
    reset(); assert(map_info_set_display_name(g_mapInfo,"stable",input,0));
    borrowed=map_info_display_name(g_mapInfo,"stable"); strcpy(input,"mutated");
    for (int i=2047;i>=0;--i) {
        sprintf(key,"map%04d.rsw",i); sprintf(value,"name-%04d",i);
        assert(map_info_set_display_name(g_mapInfo,key,value,0));
    }
    for (int i=0;i<2048;++i) {
        sprintf(key,"map%04d.rsw",i); sprintf(value,"name-%04d",i);
        assert(!strcmp(map_info_display_name(g_mapInfo,key),value));
    }
    assert(borrowed==map_info_display_name(g_mapInfo,"stable") && !strcmp(borrowed,"copied"));
    assert(map_info_set_display_name(g_mapInfo,"stable",borrowed,0));
    assert(map_info_set_display_name(g_mapInfo,"stable","",0));
    assert(!*map_info_display_name(g_mapInfo,"stable"));
}
static void test_instances(void)
{
    const char *cases[]={"","@","a@","ab@","abc@","abcd@","abcde@map",
        "1234@map.rsw","prefix1234@map.rsw","1234@xx5678@map.rsw","a@b@c",
        "long_map_name_without_an_at.rsw","\xb0\xa1\xb0\xa2@map.rsw"};
    char normalized[1024], name[64];
    reset();
    for (size_t i=0;i<sizeof cases/sizeof cases[0];++i) {
        char *at;
        strcpy(normalized,cases[i]); at=strrchr(normalized,'@');
        if (at && at-normalized>=4) memmove(at-4,at-1,strlen(at-1)+1);
        sprintf(name,"value%u",(unsigned)i);
        assert(map_info_set_display_name(g_mapInfo,normalized,name,1));
        assert(!strcmp(map_info_display_name(g_mapInfo,cases[i]),name));
    }
    assert(map_info_set_display_name(g_mapInfo,"case","lower",0));
    assert(map_info_set_display_name(g_mapInfo,"CASE","upper",0));
    assert(!strcmp(map_info_display_name(g_mapInfo,"case"),"lower"));
    assert(!strcmp(map_info_display_name(g_mapInfo,"CASE"),"upper"));
}
static void test_banners(void)
{
    int before;
    reset(); assert(map_info_set_display_name(g_mapInfo,"map","Map",1));
    map_info_show_banner(g_mapInfo,"map");
    assert(!*shown_line1 && !strcmp(shown_line2,"Unknown map") && !strcmp(shown_ornament,"noname"));
    before=banners; fail_after=0; map_info_show_banner(g_mapInfo,"map"); fail_after=-1;
    assert(banners==before && banners-released==1);
    fail_banner=1; map_info_show_banner(g_mapInfo,"map"); fail_banner=0;
    assert(banners==before && banners-released==1);
    map_info_show_banner(g_mapInfo,"missing"); assert(banners==before);
    assert(map_info_set_display_name(g_mapInfo,"map","Map",0)); map_info_show_banner(g_mapInfo,"map");
    assert(banners==before); map_info_render_banner(g_mapInfo); assert(banners-released==1);
    banner_finished=1; map_info_render_banner(g_mapInfo); assert(banners==released);
    before=rendered; map_info_render_banner(g_mapInfo); assert(rendered==before);
}
static void test_load(void)
{
    reset(); strcpy(default_name,"Loaded default");
    assert(!map_info_load(g_mapInfo));
    assert(!strcmp(map_info_display_name(g_mapInfo,"missing"),"Loaded default"));
    map_info_show_banner(g_mapInfo,"missing"); assert(banners==released);
    strcpy(default_name,"Unknown map");
}
int main(void)
{
    test_failures(); test_storage(); test_instances(); test_banners(); test_load();
    fail_after=-1; map_info_destroy(); map_info_destroy(); assert(!live && banners==released);
    assert(!*map_info_display_name(NULL,"x"));
    map_info_show_banner(NULL,"x"); map_info_render_banner(NULL);
    assert(!map_info_load(NULL) && !map_info_set_display_name(NULL,"x","y",0));
    puts("Map metadata: C ownership, moving storage, stable text, atomic failures, instance names, no mapInfo script and banner lifetime passed.");
    return 0;
}
