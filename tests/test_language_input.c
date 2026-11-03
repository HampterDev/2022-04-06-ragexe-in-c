#include "ui/window_access.h"
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <imm.h>
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
static int shift_down,control_down,ime_missing,ime_releases,ime_cancels;
static LANGID user_language=LANG_GERMAN;
static WCHAR ime_text[1024];
static LONG ime_bytes,ime_cursor=2;
static SHORT test_key_state(int key)
{ return (key==VK_SHIFT?shift_down:key==VK_CONTROL?control_down:0)?(SHORT)0x8000:0; }
static HKL test_keyboard(DWORD id) { (void)id; return (HKL)(uintptr_t)0x409; }
static LANGID test_user_language(void) { return user_language; }
static HIMC test_get_context(HWND window) { (void)window; return ime_missing?NULL:(HIMC)1; }
static BOOL test_release_context(HWND window,HIMC imc)
{ (void)window; assert(imc==(HIMC)1); ++ime_releases; return TRUE; }
static BOOL test_notify(HIMC imc,DWORD action,DWORD index,DWORD value)
{ assert(imc==(HIMC)1 && action==NI_COMPOSITIONSTR && index==CPS_CANCEL && !value); ++ime_cancels; return TRUE; }
static LONG test_composition_w(HIMC imc,DWORD kind,LPVOID out,DWORD size)
{
    assert(imc==(HIMC)1 && (kind==GCS_COMPSTR || kind==GCS_RESULTSTR || kind==GCS_CURSORPOS));
    if (kind==GCS_CURSORPOS) { assert(!size && !out); return ime_cursor; }
    if (ime_bytes>=0 && (DWORD)ime_bytes<=size) memcpy(out,ime_text,ime_bytes);
    return ime_bytes;
}
static CANDIDATEFORM candidate_forms[4];
static int candidate_calls;
static BOOL test_candidate_window(HIMC imc,LPCANDIDATEFORM form)
{
    assert(imc==(HIMC)1 && form && candidate_calls<4);
    candidate_forms[candidate_calls++]=*form;
    return TRUE;
}

static int clipboard_failure,opened,closed,allocated,freed,published,locked;
static unsigned char *clip_pages,*clip_bytes;
static size_t clip_size;
static int clip_system_owned;
static char clipboard_text[256];
static BOOL test_open(HWND window) { (void)window; ++opened; return clipboard_failure!=1; }
static BOOL test_close(void) { ++closed; return TRUE; }
static HGLOBAL test_global_alloc(UINT flags,SIZE_T size)
{
    DWORD old;
    assert(flags==GMEM_MOVEABLE && size && size<=4096 && !clip_pages);
    if (clipboard_failure==2) return NULL;
    clip_pages=VirtualAlloc(NULL,8192,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE); assert(clip_pages);
    assert(VirtualProtect(clip_pages+4096,4096,PAGE_NOACCESS,&old));
    clip_size=size; clip_bytes=clip_pages+4096-size; memset(clip_bytes,0xcc,size); ++allocated;
    return (HGLOBAL)clip_pages;
}
static LPVOID test_global_lock(HGLOBAL block)
{ assert(block==(HGLOBAL)clip_pages && !locked); if (clipboard_failure==3) return NULL; locked=1; return clip_bytes; }
static BOOL test_global_unlock(HGLOBAL block)
{ assert(block==(HGLOBAL)clip_pages && locked); locked=0; return FALSE; }
static HGLOBAL test_global_free(HGLOBAL block)
{
    assert(block==(HGLOBAL)clip_pages && !locked && !clip_system_owned);
    assert(VirtualFree(clip_pages,0,MEM_RELEASE)); clip_pages=clip_bytes=NULL; ++freed; return NULL;
}
static BOOL test_empty(void) { assert(!locked); return clipboard_failure!=4; }
static HANDLE test_set_clipboard(UINT format,HANDLE block)
{
    const WCHAR *wide=(const WCHAR*)clip_bytes;
    assert(format==CF_UNICODETEXT && block==(HANDLE)clip_pages && !locked);
    if (clipboard_failure==5) return NULL;
    assert(clip_size%sizeof(WCHAR)==0 && !wide[clip_size/sizeof(WCHAR)-1] && wcslen(wide)==clip_size/sizeof(WCHAR)-1);
    assert(WideCharToMultiByte(CP_UTF8,0,wide,-1,clipboard_text,sizeof clipboard_text,NULL,NULL));
    clip_system_owned=1; ++published; return block;
}

#define GetKeyState test_key_state
#define GetKeyboardLayout test_keyboard
#define GetUserDefaultLangID test_user_language
#define ImmGetContext test_get_context
#define ImmReleaseContext test_release_context
#define ImmNotifyIME test_notify
#define ImmGetCompositionStringW test_composition_w
#define ImmSetCandidateWindow test_candidate_window
#define OpenClipboard test_open
#define CloseClipboard test_close
#define GlobalAlloc test_global_alloc
#define GlobalLock test_global_lock
#define GlobalUnlock test_global_unlock
#define GlobalFree test_global_free
#define EmptyClipboard test_empty
#define SetClipboardData test_set_clipboard
#define malloc test_malloc
#define realloc test_realloc
#define free test_free
#include "../src/platform/language.c"
#undef malloc
#undef realloc
#undef free
#undef GetKeyState
#undef GetKeyboardLayout
#undef GetUserDefaultLangID
#undef ImmGetContext
#undef ImmReleaseContext
#undef ImmNotifyIME
#undef ImmGetCompositionStringW
#undef ImmSetCandidateWindow
#undef OpenClipboard
#undef CloseClipboard
#undef GlobalAlloc
#undef GlobalLock
#undef GlobalUnlock
#undef GlobalFree
#undef EmptyClipboard
#undef SetClipboardData

HWND g_hMainWnd;
UINT g_codePage=CP_UTF8;
struct UIWindowMgr g_windowMgr;
struct UIWindow *ui_window_mgr_get_focus_edit(struct UIWindowMgr *mgr) { (void)mgr; return NULL; }

static void type_char(struct Language *s,int ch)
{
    language_on_char(s,ch);
}
static void press_key(struct Language *s,int key)
{
    language_on_key_down(s,key);
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
    shift_down=1; press_key(&s,VK_HOME); expect_text(&s,"abf",2,0);
    shift_down=0; type_char(&s,'Z'); expect_text(&s,"Zf",1,1);
    press_key(&s,VK_END); type_char(&s,'!'); expect_text(&s,"Zf!",3,3);
    type_char(&s,'\n'); type_char(&s,'\t'); expect_text(&s,"Zf!",3,3);
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
    type_char(&s,0xd55c); type_char(&s,0x416); type_char(&s,0xe9); expect_text(&s,U8_HAN U8_ZHE U8_E_ACUTE,7,7);
    type_char(&s,0xd83d); expect_text(&s,U8_HAN U8_ZHE U8_E_ACUTE,7,7);
    type_char(&s,0xde00); expect_text(&s,U8_HAN U8_ZHE U8_E_ACUTE U8_GRIN,11,11);
    type_char(&s,0xde00); type_char(&s,0xd83d); type_char(&s,'x');
    expect_text(&s,U8_HAN U8_ZHE U8_E_ACUTE U8_GRIN "\xef\xbf\xbd\xef\xbf\xbdx",18,18);
    type_char(&s,0xd83d); type_char(&s,VK_BACK); type_char(&s,0xde00);
    expect_text(&s,U8_HAN U8_ZHE U8_E_ACUTE U8_GRIN "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd",20,20);
    type_char(&s,VK_BACK); type_char(&s,VK_BACK); type_char(&s,VK_BACK); type_char(&s,VK_BACK);
    expect_text(&s,U8_HAN U8_ZHE U8_E_ACUTE,7,7);
    language_cancel_input(&s);language_replace_selection(&s,"abcd"); language_set_selection(&s,2,2);
    strcpy(s.composition,"XY");s.composition_length=2;assert(language_get_text(&s,&out));
    assert(!strcmp(out.data,"abXYcd") && out.size==6);
    language_destruct(&s); assert(!heap_live);
}
static void test_ime(void)
{
    struct Language s; BYTE handled; int i;
    language_construct(&s); language_replace_selection(&s,"test"); language_set_selection(&s,1,3);
    assert(!language_on_ime_message(&s,NULL,WM_IME_STARTCOMPOSITION,0,0,&handled) && handled);
    expect_text(&s,"tt",1,1);
    ime_text[0]='A';ime_text[1]='B';ime_bytes=4;
    language_on_ime_message(&s,NULL,WM_IME_COMPOSITION,0,GCS_COMPSTR|GCS_CURSORPOS,&handled);
    assert(handled && !strcmp(s.composition,"AB") && s.composition_cursor==2);
    language_on_ime_message(&s,NULL,WM_IME_COMPOSITION,0,GCS_RESULTSTR,&handled);expect_text(&s,"tABt",3,3);
    ime_cursor=-1;language_read_cursor_pos(&s);assert(s.composition_cursor==2);ime_cursor=7;
    language_read_cursor_pos(&s);assert(s.composition_cursor==2);
    ime_text[0]=0xac00;ime_text[1]=0xd83d;ime_text[2]=0xde00;ime_text[3]='B';ime_bytes=8;ime_cursor=1;
    language_on_ime_message(&s,NULL,WM_IME_COMPOSITION,0,GCS_COMPSTR|GCS_CURSORPOS,&handled);
    assert(!strcmp(s.composition,U8_GA U8_GRIN "B") && s.composition_length==8 && s.composition_cursor==3);
    ime_cursor=2;language_read_cursor_pos(&s);assert(s.composition_cursor==7);
    ime_cursor=3;language_read_cursor_pos(&s);assert(s.composition_cursor==7);
    ime_cursor=4;language_read_cursor_pos(&s);assert(s.composition_cursor==8);
    for (i=0;i<600;++i) ime_text[i]=0xd55c;
    ime_bytes=1200;language_read_composition(&s);
    assert(s.composition_length==768 && strlen(s.composition)==768);
    language_cancel_input(&s);
    language_on_ime_message(&s,NULL,WM_IME_COMPOSITION,0,GCS_RESULTSTR,&handled);
    assert(s.input_length==1800 && s.caret==1800 && !memcmp(s.input,U8_HAN U8_HAN,6));
    language_cancel_input(&s);language_replace_selection(&s,"tABt");
    ime_text[0]='A';ime_text[1]='B';ime_bytes=4;ime_cursor=2;language_read_composition(&s);
    ime_bytes=-1;language_read_composition(&s);assert(!strcmp(s.composition,"AB"));
    ime_bytes=513;language_read_composition(&s);assert(!strcmp(s.composition,"AB"));
    language_on_ime_message(&s,NULL,WM_IME_ENDCOMPOSITION,0,0,&handled);
    assert(handled && !s.composition_length);
    language_on_ime_message(&s,NULL,WM_INPUTLANGCHANGE,0,0x412,&handled);
    assert(handled);
    language_on_ime_message(&s,NULL,WM_USER,0,0,&handled); assert(!handled);
    ime_missing=1;strcpy(s.composition,"old");s.composition_length=3;language_read_composition(&s);assert(!s.composition_length);ime_missing=0;
    language_destruct(&s);assert(!heap_live && ime_releases>0 && ime_cancels>0);
}
static void test_clipboard(void)
{
    struct Language s;int before;
    language_construct(&s);language_replace_selection(&s,"0123456789");language_set_selection(&s,2,4);language_set_masked(&s,1);
    for (clipboard_failure=1;clipboard_failure<=5;++clipboard_failure) {
        before=closed;language_copy(&s);assert(!clip_pages && !clip_system_owned && !published && !locked);
        assert(closed==before+(clipboard_failure!=1));
    }
    clipboard_failure=0;language_copy(&s);assert(!strcmp(clipboard_text,"**") && clip_size==6 && clip_system_owned);
    clip_system_owned=0;test_global_free((HGLOBAL)clip_pages);
    language_set_masked(&s,0);language_set_selection(&s,8,3);language_copy(&s);
    assert(!strcmp(clipboard_text,"34567") && clip_size==12 && clip_system_owned);
    clip_system_owned=0;test_global_free((HGLOBAL)clip_pages);
    language_cancel_input(&s);language_replace_selection(&s,"a" U8_HAN U8_GRIN "\xff" U8_ZHE);
    language_set_selection(&s,0,11);language_copy(&s);
    assert(!strcmp(clipboard_text,"a" U8_HAN U8_GRIN "\xef\xbf\xbd" U8_ZHE) && clip_size==14 && clip_system_owned);
    clip_system_owned=0;test_global_free((HGLOBAL)clip_pages);
    assert(language_copy_text("C-123",5) && !strcmp(clipboard_text,"C-123") && clip_size==12);
    clip_system_owned=0;test_global_free((HGLOBAL)clip_pages);
    before=opened;language_set_selection(&s,3,3);language_copy(&s);assert(opened==before);
    language_destruct(&s);assert(!heap_live && allocated==freed && !locked);
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

static void expect_candidate_forms(int x,int top,int bottom)
{
    const CANDIDATEFORM *pos=&candidate_forms[0],*area=&candidate_forms[1];
    assert(candidate_calls==2);
    assert(pos->dwIndex==0 && pos->dwStyle==CFS_CANDIDATEPOS && pos->ptCurrentPos.x==x && pos->ptCurrentPos.y==bottom);
    assert(area->dwIndex==0 && area->dwStyle==CFS_EXCLUDE && area->ptCurrentPos.x==x && area->ptCurrentPos.y==top);
    assert(area->rcArea.left==x && area->rcArea.top==top && area->rcArea.right==x+1 && area->rcArea.bottom==bottom);
    candidate_calls=0;
}
static void test_candidates(void)
{
    struct Language s;
    BYTE handled;
    int releases;
    language_construct(&s);
    /* The IME draws its own candidate list, so its notifications go on to the default window procedure. */
    handled=1;language_on_ime_message(&s,NULL,WM_IME_NOTIFY,IMN_OPENCANDIDATE,0,&handled);assert(!handled);
    handled=1;language_on_ime_message(&s,NULL,WM_IME_NOTIFY,IMN_CHANGECANDIDATE,0,&handled);assert(!handled);
    handled=1;language_on_ime_message(&s,NULL,WM_IME_NOTIFY,IMN_CLOSECANDIDATE,0,&handled);assert(!handled);
    candidate_calls=0;releases=ime_releases;
    language_place_candidates(&s,120,300,318);expect_candidate_forms(120,300,318);assert(ime_releases==releases+1);
    language_place_candidates(&s,120,300,318);assert(!candidate_calls && ime_releases==releases+1);
    language_place_candidates(&s,126,300,318);expect_candidate_forms(126,300,318);
    language_place_candidates(&s,126,40,58);expect_candidate_forms(126,40,58);
    /* A new composition places the list again, even at the same spot. */
    language_on_ime_message(&s,NULL,WM_IME_STARTCOMPOSITION,0,0,&handled);assert(handled);
    language_place_candidates(&s,126,40,58);expect_candidate_forms(126,40,58);
    ime_missing=1;releases=ime_releases;language_on_ime_message(&s,NULL,WM_IME_STARTCOMPOSITION,0,0,&handled);
    language_place_candidates(&s,10,20,30);assert(!candidate_calls && ime_releases==releases);ime_missing=0;
    language_place_candidates(&s,10,20,30);expect_candidate_forms(10,20,30);
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
        case 4:shift_down=1;press_key(&s,(random&0x100)?VK_LEFT:VK_RIGHT);shift_down=0;break;
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
    puts("Language input: C ownership, atomic edits, aliasing, reusable storage, IME candidate placement, UTF-16 typing and surrogate pairs, UTF-8 caret steps, Unicode clipboard and its failures passed.");
    return 0;
}
