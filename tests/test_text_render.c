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
#include "utf8_text.h"

struct FontCache g_fontCache;
struct C3dDevice g_3dDevice;
struct Language *g_language;
struct perf_counters g_perf;
static int live_blocks, allocations, fail_alloc, fail_convert, fail_extent, fail_abc, fail_font, fail_select;
static int released, deny_dc;
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
static int convert_utf16(unsigned code_page, const char *text, int bytes, uint16_t *out, int capacity)
{
    return convert(code_page, 0, text, bytes, out, capacity);
}
/* The selected face's font is charset + 1 and a script's fallback font is FALLBACK_FONT,
   so that the font's "charset", its handle minus one, sets the mock widths. */
#define FALLBACK_FONT(script) (2001 + (script))
#define FACE_FONT(charset) ((HFONT)(uintptr_t)((charset) + 1))
static unsigned covered, no_fallback;
static int fail_ascent, cover_calls, script_font_calls, last_bold, last_italic;
HFONT font_cache_get_font(struct FontCache *cache, int face, int size, int charset, int bold, int italic)
{
    assert(cache == &g_fontCache); (void)face; (void)size; (void)bold; (void)italic;
    ++font_calls;
    return fail_font ? NULL : (HFONT)(uintptr_t)(charset + 1);
}
int font_cache_covers(struct FontCache *cache, HDC dc, HFONT font, int script)
{
    assert(cache == &g_fontCache && dc == (HDC)1 && (uintptr_t)font < 1000);
    assert(script > TEXT_SCRIPT_DEFAULT && script < TEXT_SCRIPT_COUNT);
    ++cover_calls;
    return (covered >> script) & 1;
}
HFONT font_cache_get_script_font(struct FontCache *cache, int script, int size, int bold, int italic)
{
    assert(cache == &g_fontCache && size == 12);
    assert(script > TEXT_SCRIPT_DEFAULT && script < TEXT_SCRIPT_COUNT);
    ++script_font_calls; last_bold = bold; last_italic = italic;
    return (no_fallback >> script) & 1 ? NULL : (HFONT)(uintptr_t)FALLBACK_FONT(script);
}
static int ascent_of(uintptr_t font) { return font < 1000 ? 12 : 10 + (int)(font % 3); }
int font_cache_ascent(struct FontCache *cache, HDC dc, HFONT font)
{
    assert(cache == &g_fontCache && dc == (HDC)1);
    return fail_ascent ? 0 : ascent_of((uintptr_t)font);
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
#define platform_code_page_to_utf16 convert_utf16
#define SelectObject select_font
#define GetTextExtentPoint32W get_extent
#define GetCharABCWidthsW get_abc
#define TextOutW draw_text
#define GetCurrentObject current_object
#define SetBkMode background_mode
#include "../src/render/text_unicode.c"
#include "../src/render/ctextdc.c"
#include "../src/render/textdc.c"
#include "../src/render/text_gdi.c"
#undef malloc
#undef free
#undef platform_code_page_to_utf16
#undef SelectObject
#undef GetTextExtentPoint32W
#undef GetCharABCWidthsW
#undef TextOutW
#undef GetCurrentObject
#undef SetBkMode

static struct TextDC context(void)
{
    struct TextDC dc = {0}; dc.state.hdc = (HDC)1; dc.face = 2; dc.size = 12; dc.charset = HANGUL_CHARSET;
    return dc;
}
static void reset(void)
{
    assert(!live_blocks && !timer_depth);
    memset(extent_cache, 0, sizeof(extent_cache)); extent_tick = 0;
    memset(&g_perf, 0, sizeof(g_perf));
    conversions = extents = abc_calls = font_calls = selections = allocations = draws = expected_draws = 0;
    cover_calls = script_font_calls = 0;
    fail_alloc = fail_convert = fail_extent = fail_abc = fail_font = fail_select = 0;
    input_start = NULL; input_length = 0;
}
static int expected_piece(const char *s, size_t len, int charset, int x, int y, int abc, SIZE *total)
{
    struct Draw *d;
    int i, width;
    if (!len) return x;
    assert(expected_draws < 128);
    d = &expected[expected_draws++]; d->x = x; d->y = y; d->charset = charset;
    d->length = MultiByteToWideChar(CP_UTF8, 0, s, (int)len, d->text, 4096);
    assert(d->length > 0);
    width = d->length * (3 + charset % 5);
    if (abc) {
        /* Italic widths add each character's A and B widths; a surrogate pair is one
           character, measured whole. */
        for (width = 0, i = 0; i < d->length; ++i)
            if (IS_HIGH_SURROGATE(d->text[i]) && i + 1 < d->length && IS_LOW_SURROGATE(d->text[i + 1]))
                width += 2 * (3 + charset % 5), ++i;
            else
                width += 4 + d->text[i] % 3;
    }
    total->cx += width;
    if (total->cy < 10 + charset % 7) total->cy = 10 + charset % 7;
    return x + width;
}
/* The font the client should pick for a run of script: the face's unless the face lacks
   the script and the script has a fallback. */
static uintptr_t expected_font(int charset, int script)
{
    if (script == TEXT_SCRIPT_DEFAULT || ((covered | no_fallback) >> script & 1))
        return (uintptr_t)charset + 1;
    return FALLBACK_FONT(script);
}
/* The text is drawn run by run, each run in its script's font, on the face's baseline;
   "|xx" is ordinary text. */
static void check_runs(const char *s, size_t len, int abc, int trace)
{
    struct TextDC dc = context();
    SIZE want = {0}, got = {-1,-1};
    size_t at = 0;
    int j, x = 10;
    reset(); dc.italic = (BYTE)abc;
    input_start = s; input_length = len;
    while (at < len) {
        enum TextScript script;
        size_t run = text_script_run(s + at, len - at, &script);
        uintptr_t font = expected_font(dc.charset, script);
        int y = 20 + (fail_ascent ? 0 : ascent_of((uintptr_t)dc.charset + 1) - ascent_of(font));
        x = expected_piece(s + at, run, (int)font - 1, x, y, abc, &want);
        at += run;
    }
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
static void decoded(const char *s, const WCHAR *expected_text, int count, int runs)
{
    WCHAR all[64];
    int j, n = 0;
    check_runs(s, strlen(s), 0, 0);
    assert(draws == runs);
    for (j = 0; j < draws; ++j) {
        assert(n + actual[j].length <= 64);
        memcpy(all + n, actual[j].text, (size_t)actual[j].length * sizeof(WCHAR)); n += actual[j].length;
    }
    assert(n == count && !memcmp(all, expected_text, (size_t)count * sizeof(WCHAR)));
}
/* Hand-computed: the face's font is 130 ("charset" 129: 7 pixels a unit, 13 high, ascent
   12) and Hangul's fallback is 2002 ("charset" 2001: 4 pixels, 16 high, ascent 11). */
static void fallbacks(void)
{
    struct TextDC dc = context(); SIZE size;
    reset(); dc.bold = 1; dc.italic = 0;
    text_dc_text_extent(&dc, "ab" U8_HAN "c", 6, &size);
    assert(size.cx == 14 + 4 + 7 && size.cy == 16 && font_calls == 1 && cover_calls == 1 && script_font_calls == 1);
    assert(last_bold == 1 && !last_italic);
    text_dc_text_out(&dc, 10, 20, "ab" U8_HAN "c", 6);
    assert(draws == 3 && font_calls == 2);
    assert(actual[0].charset == 129 && actual[0].x == 10 && actual[0].y == 20 && actual[0].length == 2);
    assert(actual[1].charset == 2001 && actual[1].x == 24 && actual[1].y == 21 && actual[1].text[0] == 0xd55c);
    assert(actual[2].charset == 129 && actual[2].x == 28 && actual[2].y == 20 && actual[2].text[0] == 'c');
    covered = 1u << TEXT_SCRIPT_HANGUL;
    reset(); text_dc_text_out(&dc, 10, 20, "ab" U8_HAN "c", 6);
    assert(draws == 3 && !script_font_calls && actual[1].charset == 129 && actual[1].y == 20 && actual[2].x == 31);
    covered = 0; no_fallback = 1u << TEXT_SCRIPT_HANGUL;
    reset(); text_dc_text_out(&dc, 10, 20, "ab" U8_HAN "c", 6);
    assert(draws == 3 && script_font_calls == 1 && actual[1].charset == 129 && actual[2].x == 31);
    no_fallback = 0; fail_ascent = 1;
    reset(); text_dc_text_out(&dc, 10, 20, "ab" U8_HAN "c", 6);
    assert(draws == 3 && actual[1].charset == 2001 && actual[1].y == 20);
    fail_ascent = 0;
    /* Spaces join the run before them; the selected face is asked once per draw. */
    reset(); text_dc_text_out(&dc, 0, 0, U8_HAN " " U8_GUK " x", 9);
    assert(draws == 2 && font_calls == 1 && actual[0].length == 4 && actual[1].length == 1);
    /* A surrogate pair in an italic run is measured whole, not as two halves. */
    reset(); dc.italic = 1;
    text_dc_text_extent(&dc, "a\xf0\x9d\x90\x80" "b", 6, &size);
    assert(abc_calls == 2 && extents == 2 && size.cx == 5 + 14 + 6);
    reset(); fail_font = 1; text_dc_text_extent(&dc, "ab" U8_HAN, 5, &size);
    assert(!size.cx && !size.cy && !conversions && !cover_calls);
}
static void lifetime(void)
{
    struct DcBitmap canvas={0};
    struct TextDC dc;
    reset(); released=deny_dc=0;
    memset(&dc,0xa5,sizeof dc);
    canvas.text.hdc=(HDC)1;
    assert(text_dc_construct(&dc,&canvas)==&dc);
    assert(canvas.changed && dc.state.hdc==(HDC)1 && !dc.surface && !dc.bold && !dc.italic);
    text_dc_destruct(&dc); assert(!released && !dc.surface && !dc.state.hdc);
    text_dc_destruct(&dc); assert(!released);
    canvas.text.hdc=NULL; canvas.changed=0; text_dc_construct(&dc,&canvas); assert(!dc.state.hdc && !canvas.changed);
    text_dc_destruct(&dc); assert(!released);
    text_dc_construct(&dc,NULL); assert(!dc.state.hdc); text_dc_destruct(&dc);
    assert(text_dc_from_surface(&dc,test_surface)==&dc);
    assert(dc.state.hdc==(HDC)1 && dc.surface==test_surface && surface_refs==1 && surface_dcs==1);
    current_charset=129;
    text_dc_destruct(&dc); assert(released==1 && !dc.surface && !dc.state.hdc && !surface_refs && !surface_dcs);
    text_dc_destruct(&dc); assert(released==1);
    deny_dc=1; text_dc_from_surface(&dc,test_surface); assert(!dc.state.hdc && surface_refs==1);
    text_dc_destruct(&dc); assert(released==1 && !surface_refs); deny_dc=0;
    text_dc_from_surface(&dc,NULL); assert(!dc.state.hdc && !dc.surface);
    text_dc_destruct(&dc); assert(released==1 && !surface_refs && !surface_dcs);
}
static void cache_and_failures(void)
{
    struct TextDC dc = context(); SIZE size; int calls;
    char large[1025]; memset(large,'a',sizeof(large));
    reset();
    text_dc_measure_font(&dc,&size,"cache",5,FACE_FONT(129)); assert(size.cx==35 && conversions==1);
    calls=selections; text_dc_measure_font(&dc,&size,"cache",5,FACE_FONT(129));
    assert(conversions==1 && selections==calls+1 && g_perf.textHits==1);
    dc.italic=1; text_dc_measure_font(&dc,&size,"cache",5,FACE_FONT(129)); assert(conversions==2 && abc_calls==5);
    dc.italic=0; text_dc_measure_font(&dc,&size,"cache",5,FACE_FONT(128)); assert(conversions==3);
    reset(); fail_convert=1; text_dc_measure_font(&dc,&size,"retry",5,FACE_FONT(129));
    assert(size.cx==0 && size.cy==0 && !extents);
    fail_convert=0; text_dc_measure_font(&dc,&size,"retry",5,FACE_FONT(129)); assert(size.cx==35 && conversions==2);
    reset(); fail_extent=1; text_dc_measure_font(&dc,&size,"retry",5,FACE_FONT(129)); assert(!size.cx && !size.cy);
    fail_extent=0; text_dc_measure_font(&dc,&size,"retry",5,FACE_FONT(129)); assert(size.cx==35 && extents==2);
    reset(); dc.italic=1; fail_abc=1; text_dc_measure_font(&dc,&size,"retry",5,FACE_FONT(129)); assert(!size.cx && !size.cy);
    fail_abc=0; text_dc_measure_font(&dc,&size,"retry",5,FACE_FONT(129)); assert(size.cx>0 && extents==2);
    dc.italic=0;
    reset(); text_dc_text_out(&dc,0,0,large,512); assert(draws==1 && !allocations);
    reset(); text_dc_text_out(&dc,0,0,large,513); assert(draws==1 && allocations==1 && !live_blocks);
    reset(); fail_alloc=1; text_dc_text_out(&dc,0,0,large,513); assert(!draws && !conversions && !live_blocks);
    reset(); fail_convert=1; text_dc_text_out(&dc,0,0,large,513); assert(!draws && !live_blocks && !font_calls);
    reset(); fail_font=1; text_dc_text_out(&dc,0,0,large,513); assert(!draws && !live_blocks && !selections);
    reset(); fail_select=1; text_dc_text_out(&dc,0,0,large,513); assert(!draws && !live_blocks);
    reset(); fail_alloc=1; text_dc_measure_font(&dc,&size,large,513,FACE_FONT(129)); assert(!size.cx && !size.cy && !live_blocks);
    reset(); fail_extent=1; text_dc_measure_font(&dc,&size,large,513,FACE_FONT(129)); assert(!size.cx && !size.cy && !live_blocks);
    reset(); text_dc_measure_font(&dc,&size,"retry",5,NULL); assert(!size.cx && !conversions && !selections);
    reset(); fail_select=1; text_dc_measure_font(&dc,&size,"retry",5,FACE_FONT(129)); assert(!size.cx && !conversions);
    reset(); text_dc_text_extent(&dc,NULL,5,&size); assert(!size.cx && !size.cy);
    text_dc_text_out(&dc,0,0,NULL,5); text_dc_text_out(&dc,0,0,"x",-1);
    assert(!draws && !conversions);
}
static void guarded(void)
{
    static const char *const values[]={"before|81" U8_HAN U8_GUK "|00after", U8_ZHE "a" U8_GRIN, "\xf0\x9f\x98", "a\0" U8_EURO "b"};
    const size_t lengths[]={sizeof("before|81" U8_HAN U8_GUK "|00after")-1,7,3,6};
    SYSTEM_INFO si; BYTE *pages; DWORD old; size_t i,n;
    GetSystemInfo(&si); pages=VirtualAlloc(NULL,2*si.dwPageSize,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE); assert(pages);
    assert(VirtualProtect(pages+si.dwPageSize,si.dwPageSize,PAGE_NOACCESS,&old));
    for(i=0;i<sizeof(values)/sizeof(values[0]);++i) for(n=0;n<=lengths[i];++n) {
        char *s=(char *)pages+si.dwPageSize-n; memcpy(s,values[i],n);
        check_runs(s,n,0,0); check_runs(s,n,1,0);
    }
    assert(VirtualFree(pages,0,MEM_RELEASE));
}
static void generated(int trace)
{
    static const char *const pieces[]={U8_HAN U8_GUK, U8_ZHE, U8_GRIN, "\xe2\x82", "\xff", "|81", U8_EURO,
        "\xe2\x80\xbb", "\xe3\x81\x82", " " U8_HAN " ", "\xe4\xb8\xad", "\xe0\xb8\x81", "e\xcc\x81"};
    const unsigned count=sizeof(pieces)/sizeof(pieces[0]);
    unsigned seed=0xa11378u,sample;
    for(sample=0;sample<3000;++sample) {
        char text[2048]; size_t n=0,i; int abc=sample%2;
        covered=sample%3==0 ? 1u<<TEXT_SCRIPT_HANGUL : 0;
        no_fallback=sample%5==0 ? 1u<<TEXT_SCRIPT_EMOJI : 0;
        fail_ascent=sample%11==0;
        seed=seed*1664525u+1013904223u;
        for(i=0;i<(seed>>24);++i) text[n++]=(char)('a'+i%26);
        if(sample%2) { const char *piece=pieces[(sample/2)%count]; memcpy(text+n,piece,strlen(piece));n+=strlen(piece); }
        seed=seed*1664525u+1013904223u;
        for(i=0;i<(seed>>24);++i) text[n++]=(char)('A'+i%26);
        if(sample%7==0) { memcpy(text+n,U8_HAN U8_GUK,6);n+=6; }
        text[n]=0; check_runs(text,n,abc,trace);
    }
    covered=no_fallback=0; fail_ascent=0;
}
int main(int argc,char **argv)
{
    static const WCHAR korean[]={0xd55c,0xad6d}, mixed[]={'|','8','1',0x416,0xd83d,0xde00,0x20ac};
    static const WCHAR broken[]={'a',0xfffd,'b'};
    int trace=argc==2 && strcmp(argv[1],"--trace")==0;
    if(!trace) {
        decoded(U8_HAN U8_GUK, korean, 2, 1);
        decoded("|81" U8_ZHE U8_GRIN U8_EURO, mixed, 7, 3);
        decoded("a\xe2\x82" "b", broken, 3, 1);
        check_runs("prefix|81" U8_HAN U8_GUK "|80ab|00tail",sizeof("prefix|81" U8_HAN U8_GUK "|80ab|00tail")-1,0,0);
        check_runs("|81|80x|00",10,1,0);
        check_runs("a\0|81b",6,0,0);
        lifetime(); cache_and_failures(); fallbacks(); guarded();
    }
    generated(trace);
    assert(!live_blocks && !timer_depth);
    if(!trace) puts("Text rendering: UTF-8 runs by script with fallback fonts on the face's baseline, UTF-16 ownership, italic surrogate pairs and invalid bytes, protected pages, GDI/allocator failure retry, cache and 3000 generated cases passed.");
    return 0;
}
