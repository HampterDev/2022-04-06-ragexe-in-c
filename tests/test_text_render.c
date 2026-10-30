#include "render/cdc.h"
#include "render/device.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "render/textdc.h"
#include "render/fonts.h"
#include "platform/language.h"
#include "diag/perf.h"

struct FontCache g_fontCache;
struct C3dDevice g_3dDevice;
struct Language *g_language;
struct perf_counters g_perf;
int g_languageIndex, g_serviceType;
unsigned int g_codePage = 949;
unsigned char g_useCharsetTag;
static int live_blocks, allocations, fail_alloc, fail_convert, fail_extent, fail_abc, fail_font, fail_select;
static int acquired, released, deny_dc;
static int conversions, extents, abc_calls, font_calls, selections, current_charset, timer_depth;
static const char *input_start;
static size_t input_length;
struct Draw { int x, y, charset, length; WCHAR text[4096]; };
static struct Draw actual[128], expected[128];
static int draws, expected_draws;
struct Block { size_t size; };
static void *test_malloc(size_t n)
{
    struct Block *p;
    ++allocations;
    if (fail_alloc) return NULL;
    p = malloc(sizeof(*p) + n + 16); assert(p);
    p->size = n; memset(p + 1, 0xa5, n + 16); ++live_blocks;
    return p + 1;
}
static void test_free(void *data)
{
    size_t i; struct Block *p;
    if (!data) return;
    p = (struct Block *)data - 1;
    for (i = 0; i < 16; ++i) assert(((unsigned char *)data)[p->size+i] == 0xa5);
    assert(live_blocks > 0); --live_blocks; memset(data, 0xdd, p->size); free(p);
}
static int convert(UINT cp, DWORD flags, LPCCH text, int bytes, LPWSTR out, int capacity)
{
    ++conversions; assert(!flags && bytes > 0);
#ifndef TEXT_RENDER_REFERENCE
    assert(capacity == bytes);
#endif
    if (input_start) {
        assert(text >= input_start && text <= input_start + input_length);
        assert((size_t)bytes <= (size_t)(input_start + input_length - text));
    }
    if (fail_convert) return 0;
    return MultiByteToWideChar(cp, flags, text, bytes, out, capacity);
}
HFONT font_cache_get_font(struct FontCache *cache, int face, int size, int charset, int bold, int italic)
{
    assert(cache == &g_fontCache); (void)face; (void)size; (void)bold; (void)italic;
    ++font_calls;
    return fail_font ? NULL : (HFONT)(uintptr_t)(charset + 1);
}
static HGDIOBJ select_font(HDC dc, HGDIOBJ font)
{
    assert(dc == (HDC)1); ++selections;
    if (fail_select) return NULL;
    current_charset = (int)(uintptr_t)font - 1;
    return (HGDIOBJ)1;
}
static BOOL get_extent(HDC dc, LPCWSTR text, int count, LPSIZE out)
{
    assert(dc == (HDC)1 && text && count > 0); ++extents;
    if (fail_extent) return FALSE;
    out->cx = count * (3 + current_charset % 5);
    out->cy = 10 + current_charset % 7;
    return TRUE;
}
static BOOL get_abc(HDC dc, UINT first, UINT last, LPABC abc)
{
    assert(dc == (HDC)1 && first == last); ++abc_calls;
    if (fail_abc) return FALSE;
    abc->abcA = (int)(first % 3) - 1; abc->abcB = 5; abc->abcC = 7;
    return TRUE;
}
static BOOL draw_text(HDC dc, int x, int y, LPCWSTR text, int count)
{
    struct Draw *d;
    assert(dc == (HDC)1 && count > 0 && count <= 4096 && draws < 128);
    d = &actual[draws++]; d->x = x; d->y = y; d->charset = current_charset; d->length = count;
    memcpy(d->text, text, (size_t)count * sizeof(WCHAR));
    return TRUE;
}
static HGDIOBJ current_object(HDC dc, UINT kind)
{ assert(dc==(HDC)1 && kind==OBJ_FONT); return (HGDIOBJ)1; }
static int background_mode(HDC dc, int mode)
{ assert(dc==(HDC)1 && mode==TRANSPARENT); return TRANSPARENT; }
char dc_bitmap_get_dc(struct DcBitmap *canvas,HDC *out)
{ assert(out); *out=NULL; if(!canvas || deny_dc)return 0; *out=(HDC)1; ++acquired; return 1; }
static int surface_refs, surface_dcs, surface_step;
static gfx_surface *const test_surface=(gfx_surface *)(uintptr_t)2;
void gfx_addref(gfx_surface *surface)
{ assert(surface==test_surface && surface_step==0); ++surface_refs; surface_step=1; }
int gfx_get_dc(gfx_surface *surface,HDC *out)
{
    assert(surface==test_surface && surface_refs==1 && surface_step==1);
    surface_step=2; *out=(HDC)1;
    if(deny_dc) return -1;
    ++surface_dcs; return 0;
}
void gfx_release_dc(gfx_surface *surface,HDC dc)
{
    assert(surface==test_surface && dc==(HDC)1 && surface_refs==1 && surface_dcs==1);
    assert(surface_step==2 && current_charset==0);
    --surface_dcs; ++released; surface_step=3;
}
void gfx_release(gfx_surface *surface)
{
    assert(surface==test_surface && surface_refs==1 && !surface_dcs);
    assert(surface_step==(deny_dc?2:3)); --surface_refs; surface_step=0;
}
void perf_begin(int phase) { assert(phase == PERF_TEXT && !timer_depth); ++timer_depth; }
void perf_end(int phase) { assert(phase == PERF_TEXT && timer_depth == 1); --timer_depth; }
#define malloc test_malloc
#define free test_free
#define MultiByteToWideChar convert
#define SelectObject select_font
#define GetTextExtentPoint32W get_extent
#define GetCharABCWidthsW get_abc
#define TextOutW draw_text
#define GetCurrentObject current_object
#define SetBkMode background_mode
#include "../src/render/text_unicode.c"
#include "../src/render/ctextdc.c"
#include "../src/render/textdc.c"
#undef malloc
#undef free
#undef MultiByteToWideChar
#undef SelectObject
#undef GetTextExtentPoint32W
#undef GetCharABCWidthsW
#undef TextOutW
#undef GetCurrentObject
#undef SetBkMode

static struct TextDC context(void)
{
    struct TextDC dc = {0}; dc.hdc = (HDC)1; dc.face = 2; dc.size = 12; dc.charset = HANGUL_CHARSET;
    return dc;
}
static void reset(void)
{
    assert(!live_blocks && !timer_depth);
    memset(extent_cache, 0, sizeof(extent_cache)); extent_tick = 0;
    memset(&g_perf, 0, sizeof(g_perf));
    conversions = extents = abc_calls = font_calls = selections = allocations = draws = expected_draws = 0;
    fail_alloc = fail_convert = fail_extent = fail_abc = fail_font = fail_select = 0;
    input_start = NULL; input_length = 0;
}
static int digit(unsigned char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
static int expected_piece(const char *s, size_t len, int charset, int x, int y, int abc, SIZE *total)
{
    struct Draw *d;
    int i, width;
    if (!len) return x;
    assert(expected_draws < 128);
    d = &expected[expected_draws++]; d->x = x; d->y = y; d->charset = charset;
    d->length = MultiByteToWideChar(get_code_page_from_charset(charset), 0, s, (int)len, d->text, 4096);
    assert(d->length > 0);
    width = d->length * (3 + charset % 5);
    if (abc) { width = 0; for (i=0; i<d->length; ++i) width += 4 + d->text[i] % 3; }
    total->cx += width;
    if (total->cy < 10 + charset % 7) total->cy = 10 + charset % 7;
    return x + width;
}
static void check_runs(const char *s, size_t len, int tags, int abc, int trace)
{
    struct TextDC dc = context();
    SIZE want = {0}, got = {-1,-1};
    size_t i = 0, start = 0;
    int charset = dc.charset, parse = tags, x = 10, j;
    reset(); g_useCharsetTag = (unsigned char)tags; dc.italic = (BYTE)abc;
    input_start = s; input_length = len;
    while (i < len) {
        if (!s[i]) parse = 0;
        if (parse && s[i] == '|' && len-i >= 3 && digit((unsigned char)s[i+1]) >= 0 && digit((unsigned char)s[i+2]) >= 0) {
            x = expected_piece(s+start, i-start, charset, x, 20, abc, &want);
            charset = digit((unsigned char)s[i+1]) * 16 + digit((unsigned char)s[i+2]);
            i += 3; start = i;
        } else ++i;
    }
    expected_piece(s+start, len-start, charset, x, 20, abc, &want);
    text_dc_text_extent(&dc, s, (int)len, &got);
    assert(got.cx == want.cx && got.cy == want.cy);
    text_dc_text_out(&dc, 10, 20, s, (int)len);
    assert(draws == expected_draws);
    for (j=0; j<draws; ++j) {
        struct Draw *a=&actual[j], *b=&expected[j];
        assert(a->x==b->x && a->y==b->y && a->charset==b->charset && a->length==b->length);
        assert(memcmp(a->text,b->text,(size_t)a->length*sizeof(WCHAR))==0);
    }
    assert(!live_blocks && !timer_depth);
    if (trace) {
        printf("%ld %ld %d",got.cx,got.cy,draws);
        for (j=0;j<draws;++j) {
            int n; struct Draw *d=&actual[j]; printf(" %d,%d,%d,%d:",d->x,d->y,d->charset,d->length);
            for(n=0;n<d->length;++n) printf("%04x",d->text[n]);
        }
        putchar('\n');
    }
}
static void lifetime(void)
{
    struct DcBitmap canvas={0};
    struct TextDC dc;
    reset(); acquired=released=deny_dc=0;
    memset(&dc,0xa5,sizeof dc);
    assert(text_dc_construct(&dc,&canvas)==&dc);
    assert(acquired==1 && dc.hdc==(HDC)1 && !dc.surface && !dc.bold && !dc.italic);
    text_dc_destruct(&dc); assert(!released && !dc.surface && !dc.hdc);
    text_dc_destruct(&dc); assert(!released);
    deny_dc=1; text_dc_construct(&dc,&canvas); assert(!dc.hdc);
    text_dc_destruct(&dc); assert(!released); deny_dc=0;
    text_dc_construct(&dc,NULL); assert(!dc.hdc); text_dc_destruct(&dc);
    assert(text_dc_from_surface(&dc,test_surface)==&dc);
    assert(dc.hdc==(HDC)1 && dc.surface==test_surface && surface_refs==1 && surface_dcs==1);
    current_charset=129;
    text_dc_destruct(&dc); assert(released==1 && !dc.surface && !dc.hdc && !surface_refs && !surface_dcs);
    text_dc_destruct(&dc); assert(released==1);
    deny_dc=1; text_dc_from_surface(&dc,test_surface); assert(!dc.hdc && surface_refs==1);
    text_dc_destruct(&dc); assert(released==1 && !surface_refs); deny_dc=0;
    text_dc_from_surface(&dc,NULL); assert(!dc.hdc && !dc.surface);
    text_dc_destruct(&dc); assert(released==1 && !surface_refs && !surface_dcs);
}
static void cache_and_failures(void)
{
    struct TextDC dc = context(); SIZE size; int calls;
    char large[1025]; memset(large,'a',sizeof(large));
    reset();
    text_dc_measure_in_charset(&dc,&size,"cache",5,129); assert(size.cx==35 && conversions==1);
    calls=selections; text_dc_measure_in_charset(&dc,&size,"cache",5,129);
    assert(conversions==1 && selections==calls+1 && g_perf.textHits==1);
    dc.italic=1; text_dc_measure_in_charset(&dc,&size,"cache",5,129); assert(conversions==2 && abc_calls==5);
    dc.italic=0; text_dc_measure_in_charset(&dc,&size,"cache",5,128); assert(conversions==3);
    reset(); fail_convert=1; text_dc_measure_in_charset(&dc,&size,"retry",5,129);
    assert(size.cx==0 && size.cy==0 && !extents);
    fail_convert=0; text_dc_measure_in_charset(&dc,&size,"retry",5,129); assert(size.cx==35 && conversions==2);
    reset(); fail_extent=1; text_dc_measure_in_charset(&dc,&size,"retry",5,129); assert(!size.cx && !size.cy);
    fail_extent=0; text_dc_measure_in_charset(&dc,&size,"retry",5,129); assert(size.cx==35 && extents==2);
    reset(); dc.italic=1; fail_abc=1; text_dc_measure_in_charset(&dc,&size,"retry",5,129); assert(!size.cx && !size.cy);
    fail_abc=0; text_dc_measure_in_charset(&dc,&size,"retry",5,129); assert(size.cx>0 && extents==2);
    dc.italic=0;
    reset(); textdc_text_out(&dc,0,0,large,512,129); assert(draws==1 && !allocations);
    reset(); textdc_text_out(&dc,0,0,large,513,129); assert(draws==1 && allocations==1 && !live_blocks);
    reset(); fail_alloc=1; textdc_text_out(&dc,0,0,large,513,129); assert(!draws && !conversions && !live_blocks);
    reset(); fail_convert=1; textdc_text_out(&dc,0,0,large,513,129); assert(!draws && !live_blocks && !font_calls);
    reset(); fail_font=1; textdc_text_out(&dc,0,0,large,513,129); assert(!draws && !live_blocks && !selections);
    reset(); fail_select=1; textdc_text_out(&dc,0,0,large,513,129); assert(!draws && !live_blocks);
    reset(); fail_alloc=1; text_dc_measure_in_charset(&dc,&size,large,513,129); assert(!size.cx && !size.cy && !live_blocks);
    reset(); fail_extent=1; text_dc_measure_in_charset(&dc,&size,large,513,129); assert(!size.cx && !size.cy && !live_blocks);
    reset(); fail_font=1; text_dc_measure_in_charset(&dc,&size,"retry",5,129); assert(!size.cx && !conversions);
    reset(); fail_select=1; text_dc_measure_in_charset(&dc,&size,"retry",5,129); assert(!size.cx && !conversions);
    reset(); text_dc_text_extent(&dc,NULL,5,&size); assert(!size.cx && !size.cy);
    text_dc_text_out(&dc,0,0,NULL,5); text_dc_text_out(&dc,0,0,"x",-1);
    assert(!draws && !conversions);
}
static void guarded(void)
{
    const char *values[]={"before|81한국|00after", "|81a|80b|81c", "|8", "a\0|81b"};
    const size_t lengths[]={sizeof("before|81한국|00after")-1,12,2,6};
    SYSTEM_INFO si; BYTE *pages; DWORD old; size_t i,n; int enabled;
    GetSystemInfo(&si); pages=VirtualAlloc(NULL,2*si.dwPageSize,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE); assert(pages);
    assert(VirtualProtect(pages+si.dwPageSize,si.dwPageSize,PAGE_NOACCESS,&old));
    for(i=0;i<sizeof(values)/sizeof(values[0]);++i) for(n=0;n<=lengths[i];++n) {
        char *s=(char *)pages+si.dwPageSize-n; memcpy(s,values[i],n);
        for(enabled=0;enabled<2;++enabled) check_runs(s,n,enabled,0,0);
    }
    assert(VirtualFree(pages,0,MEM_RELEASE));
}
static void generated(int trace)
{
    unsigned seed=0xa11378u,sample;
    for(sample=0;sample<3000;++sample) {
        char text[2048]; size_t n=0,i; int tags=(sample%3)!=0,abc=sample%2;
        seed=seed*1664525u+1013904223u;
        for(i=0;i<(seed>>24);++i) text[n++]=(char)('a'+i%26);
        if(sample%2) { static const char *const codes[]={"|81","|80","|00","|cc"}; memcpy(text+n,codes[(sample/2)%4],3);n+=3; }
        seed=seed*1664525u+1013904223u;
        for(i=0;i<(seed>>24);++i) text[n++]=(char)('A'+i%26);
        if(sample%7==0) { memcpy(text+n,"한국",sizeof("한국")-1);n+=sizeof("한국")-1; }
        if(!trace) { memcpy(text+n,"|80日本|00tail|81한국",sizeof("|80日本|00tail|81한국")-1);n+=sizeof("|80日本|00tail|81한국")-1; }
        text[n]=0; check_runs(text,n,tags,abc,trace);
    }
}
int main(int argc,char **argv)
{
    int trace=argc==2 && strcmp(argv[1],"--trace")==0;
    if(!trace) {
        check_runs("prefix|81한국|80ab|00tail",sizeof("prefix|81한국|80ab|00tail")-1,1,0,0);
        check_runs("|81|80x|00",10,1,1,0);
        check_runs("bad|g0x|8",9,1,0,0);
        check_runs("a\0|81b",6,1,0,0);
        lifetime(); cache_and_failures(); guarded();
    }
    generated(trace);
    assert(!live_blocks && !timer_depth);
    if(!trace) puts("Text rendering: shared bounded runs, UTF-16 ownership, Korean/counts, protected pages, GDI/allocator failure retry, cache and 3000 generated cases passed.");
    return 0;
}
