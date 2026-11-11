#include "ui/window_access.h"
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <SDL3/SDL.h>
#include "utf8_text.h"

static int heap_live,heap_calls,fail_after=-1;
struct TestAllocation { size_t size; unsigned magic; };
static void *test_malloc(size_t size)
{
    struct TestAllocation *p;
    ++heap_calls;
    if (!fail_after) return NULL;
    if (fail_after>0) --fail_after;
    p=malloc(sizeof *p+size+16); assert(p);
    p->size=size;p->magic=0x8137abcdu;memset(p+1,0xcd,size+16);++heap_live;
    return p+1;
}
static void test_free(void *data)
{
    struct TestAllocation *p;
    if (!data) return;
    p=(struct TestAllocation*)data-1;assert(p->magic==0x8137abcdu && heap_live>0);
    for (size_t i=0;i<16;++i) assert(((unsigned char*)data)[p->size+i]==0xcd);
    memset(data,0xdd,p->size);p->magic=0;--heap_live;free(p);
}
static void *test_realloc(void *data,size_t size)
{
    struct TestAllocation *p;
    void *copy=test_malloc(size);
    if (!copy) return NULL;
    if (data) { p=(struct TestAllocation*)data-1;memcpy(copy,data,p->size<size?p->size:size);test_free(data); }
    return copy;
}
static int trace_steps;
static LANGID user_language=LANG_GERMAN;
static LANGID test_user_language(void) { return user_language; }

/* SDL as platform/window_sdl.c calls it for the editor: the clipboard, and the IME's composition
   that language_cancel_input clears. The rest of SDL is linked as it is. */
static char clipboard_text[256];
static int clipboard_fail,clipboard_sets,compositions_cleared;
bool test_set_clipboard(const char *text)
{
    ++clipboard_sets;
    if (clipboard_fail) return false;
    assert(strlen(text)<sizeof clipboard_text);
    strcpy(clipboard_text,text);
    return true;
}
char *test_get_clipboard(void)
{
    char *copy=malloc(strlen(clipboard_text)+1); assert(copy);
    strcpy(copy,clipboard_text);
    return copy;
}
void test_sdl_free(void *data) { free(data); }
bool test_clear_composition(SDL_Window *window)
{
    (void)window;
    ++compositions_cleared;
    return true;
}

#define GetUserDefaultLangID test_user_language
#define SDL_SetClipboardText test_set_clipboard
#define SDL_GetClipboardText test_get_clipboard
#define SDL_free test_sdl_free
#define SDL_ClearComposition test_clear_composition
#define malloc test_malloc
#define realloc test_realloc
#define free test_free
#include "../src/platform/system_win32.c"
#include "../src/platform/language.c"
#include "../src/platform/window_sdl.c"
#undef malloc
#undef realloc
#undef free
#undef GetUserDefaultLangID
#undef SDL_SetClipboardText
#undef SDL_GetClipboardText
#undef SDL_free
#undef SDL_ClearComposition

PlatformWindow g_hMainWnd=(PlatformWindow)(uintptr_t)0x10;
UINT g_codePage=CP_UTF8;
struct UIWindowMgr g_windowMgr;
static int edit_focused;
struct UIWindow *ui_window_mgr_get_focus_edit(struct UIWindowMgr *mgr)
{ (void)mgr; return edit_focused?(struct UIWindow *)&g_windowMgr:NULL; }

static void type_char(struct Language *s,int ch)
{
    language_on_char(s,ch);
}
static void press_key(struct Language *s,int key)
{
    language_on_key_down(s,key);
}
static void hold(int key,int down)
{
    platform_note_key(key,down);
}
static void expect_text(struct Language *s,const char *text,int anchor,int caret)
{ assert(!strcmp(language_input_text(s),text) && s->anchor==anchor && s->caret==caret); }
static void test_editing(void)
{
    struct Language s;
    struct TextView out;
    language_construct(&s); assert(s.folder_lang==LANG_GERMAN);
    language_replace_selection(&s,"abcdef"); expect_text(&s,"abcdef",6,6);
    language_set_selection(&s,4,2); language_replace_selection(&s,"X"); expect_text(&s,"abXef",3,3);
    type_char(&s,VK_BACK); expect_text(&s,"abef",2,2);
    press_key(&s,VK_DELETE); expect_text(&s,"abf",2,2);
    hold(VK_SHIFT,1); press_key(&s,VK_HOME); expect_text(&s,"abf",2,0);
    hold(VK_SHIFT,0); type_char(&s,'Z'); expect_text(&s,"Zf",1,1);
    press_key(&s,VK_END); type_char(&s,'!'); expect_text(&s,"Zf!",3,3);
    /* Only Backspace and printable ASCII come as characters; other text is committed. */
    type_char(&s,'\n'); type_char(&s,'\t'); type_char(&s,0x7f); type_char(&s,0xe9); type_char(&s,0xd55c);
    expect_text(&s,"Zf!",3,3);
    language_set_selection(&s,-1,100); assert(s.anchor==0 && s.caret==3);
    language_cancel_input(&s); language_replace_selection(&s,"a" U8_GA U8_GRIN "\xff" U8_ZHE);
    assert(s.input_length==11 && s.caret==11);
    language_caret_back(&s); assert(s.caret==9); language_caret_back(&s); assert(s.caret==8);
    language_caret_back(&s); assert(s.caret==4); language_caret_back(&s); assert(s.caret==1);
    language_caret_back(&s); assert(s.caret==0); language_caret_back(&s); assert(s.caret==0);
    language_caret_forward(&s); assert(s.caret==1); language_caret_forward(&s); assert(s.caret==4);
    language_caret_forward(&s); assert(s.caret==8); language_caret_forward(&s); assert(s.caret==9);
    language_caret_forward(&s); assert(s.caret==11); language_caret_forward(&s); assert(s.caret==11);
    language_set_selection(&s,1,1); press_key(&s,VK_DELETE); expect_text(&s,"a" U8_GRIN "\xff" U8_ZHE,1,1);
    language_set_selection(&s,5,5); type_char(&s,VK_BACK); expect_text(&s,"a\xff" U8_ZHE,1,1);
    language_set_selection(&s,4,4); type_char(&s,VK_BACK); expect_text(&s,"a\xff",2,2);
    language_set_selection(&s,2,2); press_key(&s,VK_LEFT); assert(s.caret==1 && s.anchor==1);
    language_cancel_input(&s);
    language_commit(&s,U8_HAN U8_ZHE U8_E_ACUTE); expect_text(&s,U8_HAN U8_ZHE U8_E_ACUTE,7,7);
    language_commit(&s,U8_GRIN); expect_text(&s,U8_HAN U8_ZHE U8_E_ACUTE U8_GRIN,11,11);
    type_char(&s,VK_BACK); type_char(&s,VK_BACK); expect_text(&s,U8_HAN U8_ZHE,5,5);
    language_cancel_input(&s);language_replace_selection(&s,"abcd"); language_set_selection(&s,2,2);
    strcpy(s.composition,"XY");s.composition_length=2;assert(language_get_text(&s,&out));
    assert(!strcmp(out.data,"abXYcd") && out.size==6);
    language_destruct(&s); assert(!heap_live);
}
static void test_ime(void)
{
    struct Language s; char text[601*3]; int i, cleared;
    language_construct(&s); language_replace_selection(&s,"test"); language_set_selection(&s,1,3);
    /* core/app_sdl.c deletes the selection as a composition starts. */
    language_delete_selection(&s); expect_text(&s,"tt",1,1);
    language_set_composition(&s,"AB",-1);
    assert(!strcmp(s.composition,"AB") && s.composition_length==2 && s.composition_cursor==2);
    language_set_composition(&s,"AB",1); assert(s.composition_cursor==1);
    language_set_composition(&s,"AB",7); assert(s.composition_cursor==2);
    language_commit(&s,"AB"); expect_text(&s,"tABt",3,3); assert(!s.composition_length && !s.composition[0]);
    /* SDL counts the cursor in characters, the editor in bytes. */
    language_set_composition(&s,U8_GA U8_GRIN "B",1);
    assert(!strcmp(s.composition,U8_GA U8_GRIN "B") && s.composition_length==8 && s.composition_cursor==3);
    language_set_composition(&s,U8_GA U8_GRIN "B",2); assert(s.composition_cursor==7);
    language_set_composition(&s,U8_GA U8_GRIN "B",3); assert(s.composition_cursor==8);
    language_set_composition(&s,U8_GA U8_GRIN "B",0); assert(s.composition_cursor==0);
    /* A composition that does not fit is cut on a character boundary. */
    for (i=0;i<600;++i) memcpy(text+i*3,U8_HAN,3);
    text[1800]=0;
    language_set_composition(&s,text,-1);
    assert(s.composition_length==768 && strlen(s.composition)==768 && s.composition_cursor==768);
    cleared=compositions_cleared;
    language_cancel_input(&s);
    assert(compositions_cleared==cleared+1 && !s.composition_length && !s.input_length);
    language_commit(&s,text);
    assert(s.input_length==1800 && s.caret==1800 && !memcmp(s.input,U8_HAN U8_HAN,6));
    language_set_composition(&s,"AB",-1); language_set_composition(&s,"",0);
    assert(!s.composition_length && !s.composition[0]);
    language_destruct(&s);assert(!heap_live);
}
static void test_clipboard(void)
{
    struct Language s;int sets;
    language_construct(&s);language_replace_selection(&s,"0123456789");language_set_selection(&s,2,4);language_set_masked(&s,1);
    clipboard_fail=1;sets=clipboard_sets;language_copy(&s);assert(clipboard_sets==sets+1 && !clipboard_text[0]);
    clipboard_fail=0;language_copy(&s);assert(!strcmp(clipboard_text,"**"));
    language_set_masked(&s,0);language_set_selection(&s,8,3);language_copy(&s);
    assert(!strcmp(clipboard_text,"34567"));
    /* The clipboard takes UTF-8, as SDL's does: a byte that is not UTF-8 becomes U+FFFD. */
    language_cancel_input(&s);language_replace_selection(&s,"a" U8_HAN U8_GRIN "\xff" U8_ZHE);
    language_set_selection(&s,0,11);language_copy(&s);
    assert(!strcmp(clipboard_text,"a" U8_HAN U8_GRIN "\xef\xbf\xbd" U8_ZHE));
    assert(language_copy_text("C-123",5) && !strcmp(clipboard_text,"C-123"));
    sets=clipboard_sets;language_set_selection(&s,3,3);language_copy(&s);assert(clipboard_sets==sets);
    /* Pasting needs a focused edit box and replaces the selection. */
    strcpy(clipboard_text,U8_GA U8_NA);
    language_cancel_input(&s);language_replace_selection(&s,"xyz");language_set_selection(&s,1,2);
    language_paste(&s);expect_text(&s,"xyz",1,2);
    edit_focused=1;language_paste(&s);expect_text(&s,"x" U8_GA U8_NA "z",7,7);edit_focused=0;
    language_destruct(&s);assert(!heap_live);
}

static void test_input_ownership(void)
{
    struct Language s;
    struct TextView out = {"sentinel", 8};
    const char *saved;
    char large[4097];
    int calls;
    memset(large,'Q',sizeof large-1);large[sizeof large-1]=0;
    language_construct(&s);calls=heap_calls;
    assert(!s.input && !strcmp(language_input_text(&s),"") && language_get_text(&s,&out));
    assert(out.size==0 && !strcmp(out.data,"") && heap_calls==calls);
    fail_after=0;assert(!language_replace_selection(&s,"abc"));expect_text(&s,"",0,0);fail_after=-1;
    assert(language_replace_selection(&s,"abcdef"));language_set_selection(&s,1,4);saved=s.input;
    fail_after=0;assert(!language_replace_selection(&s,s.input+2));
    assert(s.input==saved);expect_text(&s,"abcdef",1,4);fail_after=-1;
    assert(language_replace_selection(&s,s.input+2));expect_text(&s,"acdefef",5,5);
    language_set_selection(&s,2,5);saved=s.input;
    fail_after=0;assert(!language_replace_selection(&s,large));
    assert(s.input==saved);expect_text(&s,"acdefef",2,5);fail_after=-1;
    assert(language_replace_selection(&s,large));assert(s.input_length==4100 && s.caret==4098 && s.anchor==4098);
    assert(s.input[0]=='a' && s.input[1]=='c' && !memcmp(s.input+2,large,4096) && !strcmp(s.input+4098,"ef"));
    language_set_selection(&s,0,(int)s.input_length);assert(language_replace_selection(&s,"abc"));
    language_set_selection(&s,1,1);strcpy(s.composition,"XY");s.composition_length=2;
    out=text_view("sentinel",8);fail_after=0;assert(!language_get_text(&s,&out));
    assert(!strcmp(out.data,"sentinel") && out.size==8);fail_after=-1;
    assert(language_get_text(&s,&out) && out.size==5 && !strcmp(out.data,"aXYbc"));
    saved=out.data;calls=heap_calls;
    assert(language_get_text(&s,&out) && out.data==saved && heap_calls==calls);
    memset(s.composition,'X',200);s.composition[200]=0;s.composition_length=200;
    out=text_view("untouched",9);fail_after=0;assert(!language_get_text(&s,&out));
    assert(!strcmp(out.data,"untouched") && out.size==9 && !strcmp(saved,"aXYbc"));fail_after=-1;
    assert(language_get_text(&s,&out) && out.size==203);
    language_destruct(&s);language_destruct(&s);assert(!heap_live);
    language_construct(&s);calls=heap_calls;
    for (int i=0;i<5000;++i) type_char(&s,'a');
    assert(s.input_length==5000 && s.caret==5000 && heap_calls-calls<10);
    language_cancel_input(&s);assert(!s.input_length && !s.input[0]);
    calls=heap_calls;assert(language_replace_selection(&s,"reuse"));assert(heap_calls==calls);
    language_destruct(&s);assert(!heap_live);
}

/* The edit box puts the IME's candidate list below the composition's line; core/app_sdl.c hands
   the place to SDL's text input area while a composition lasts. */
static void test_candidates(void)
{
    struct Language s;
    language_construct(&s);
    assert(!s.candidate_pos.placed);
    language_place_candidates(&s,120,300,318);
    assert(s.candidate_pos.placed && s.candidate_pos.x==120 && s.candidate_pos.top==300 && s.candidate_pos.bottom==318);
    language_place_candidates(&s,126,40,58);
    assert(s.candidate_pos.placed && s.candidate_pos.x==126 && s.candidate_pos.top==40 && s.candidate_pos.bottom==58);
    /* A composition in progress keeps the place; its end lets the next one place the list again. */
    language_set_composition(&s,U8_GA,-1); assert(s.candidate_pos.placed);
    language_set_composition(&s,"",0); assert(!s.candidate_pos.placed);
    language_destruct(&s);assert(!heap_live);
}

static void test_edit_sequence(void)
{
    struct Language s;
    uint32_t random=0x82973615u;
    language_construct(&s);
    for (unsigned i=0;i<5000;++i) {
        const char *bytes;
        size_t size;
        random=random*1664525u+1013904223u;
        size=s.input_length;
        if (size>256 || i%113==0) language_cancel_input(&s);
        switch (random%10) {
        case 0:language_replace_selection(&s,"ab");break;
        case 1:type_char(&s,'A'+(random>>16)%26);break;
        case 2:type_char(&s,VK_BACK);break;
        case 3:press_key(&s,VK_DELETE);break;
        case 4:hold(VK_SHIFT,1);press_key(&s,(random&0x100)?VK_LEFT:VK_RIGHT);hold(VK_SHIFT,0);break;
        case 5:size=s.input_length;language_set_selection(&s,(random>>8)%(size+1),(random>>20)%(size+1));break;
        case 6:press_key(&s,VK_HOME);break;
        case 7:press_key(&s,VK_END);break;
        case 8:language_replace_selection(&s,"");break;
        case 9:type_char(&s,'\n');break;
        }
        size=s.input_length;bytes=language_input_text(&s);
        assert(s.caret>=0 && s.anchor>=0 && (size_t)s.caret<=size && (size_t)s.anchor<=size && strlen(bytes)==size);
        if (trace_steps) {
            printf("edit %u %d %d %u ",i,s.anchor,s.caret,(unsigned)size);
            for (size_t j=0;j<size;++j) printf("%02x",(unsigned char)bytes[j]);
            putchar('\n');
        }
    }
    language_destruct(&s);assert(!heap_live);
}

int main(int argc,char **argv)
{
    (void)argv; trace_steps=argc>1;
    test_editing();test_ime();
    test_clipboard();
    test_input_ownership();test_candidates();test_edit_sequence();
    puts("Language input: C ownership, atomic edits, aliasing, reusable storage, SDL composition and commits, candidate placement, ASCII typing, UTF-8 caret steps, UTF-8 clipboard and its failures passed.");
    return 0;
}
