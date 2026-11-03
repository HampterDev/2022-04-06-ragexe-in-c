#include "ui/window_access.h"
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <imm.h>

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
static WCHAR ime_text[256];
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
    assert(imc==(HIMC)1 && (kind==GCS_COMPSTR || kind==GCS_RESULTSTR));
    if (ime_bytes>=0 && (DWORD)ime_bytes<=size) memcpy(out,ime_text,ime_bytes);
    return ime_bytes;
}
static LONG test_composition_a(HIMC imc,DWORD kind,LPVOID out,DWORD size)
{ assert(imc==(HIMC)1 && kind==GCS_CURSORPOS && !size && !out); return ime_cursor; }
static DWORD candidate_storage[256],candidate_query,candidate_reply;
static DWORD test_candidate_list(HIMC imc,DWORD list,LPCANDIDATELIST out,DWORD size)
{
    assert(imc==(HIMC)1 && list==0);
    if (!out) { assert(!size);return candidate_query; }
    assert(size==candidate_query);
    if (candidate_reply<=size) memcpy(out,candidate_storage,candidate_reply);
    return candidate_reply;
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
    assert(format==CF_TEXT && block==(HANDLE)clip_pages && !locked);
    if (clipboard_failure==5) return NULL;
    assert(clip_bytes[clip_size-1]==0 && strlen((char*)clip_bytes)==clip_size-1);
    strcpy(clipboard_text,(char*)clip_bytes); clip_system_owned=1; ++published; return block;
}

#define GetKeyState test_key_state
#define GetKeyboardLayout test_keyboard
#define GetUserDefaultLangID test_user_language
#define ImmGetContext test_get_context
#define ImmReleaseContext test_release_context
#define ImmNotifyIME test_notify
#define ImmGetCompositionStringW test_composition_w
#define ImmGetCompositionStringA test_composition_a
#define ImmGetCandidateListW test_candidate_list
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
#undef ImmGetCompositionStringA
#undef ImmGetCandidateListW
#undef OpenClipboard
#undef CloseClipboard
#undef GlobalAlloc
#undef GlobalLock
#undef GlobalUnlock
#undef GlobalFree
#undef EmptyClipboard
#undef SetClipboardData

HWND g_hMainWnd;
BYTE g_useCharsetTag;
UINT g_codePage=949;
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
    language_cancel_input(&s); type_char(&s,0xa1b0); assert(s.input_length==2);
    language_caret_back(&s); assert(s.caret==0); language_caret_forward(&s); assert(s.caret==2);
    language_cancel_input(&s);language_replace_selection(&s,"abcd"); language_set_selection(&s,2,2);
    strcpy(s.composition,"XY");s.composition_length=2;assert(language_get_text(&s,&out));
    assert(!strcmp(out.data,"abXYcd") && out.size==6);
    language_destruct(&s); assert(!heap_live);
}
static void test_ime(void)
{
    struct Language s; BYTE handled;
    language_construct(&s); language_replace_selection(&s,"test"); language_set_selection(&s,1,3);
    assert(!language_on_ime_message(&s,NULL,WM_IME_STARTCOMPOSITION,0,0,&handled) && handled);
    expect_text(&s,"tt",1,1);
    ime_text[0]='A';ime_text[1]='B';ime_bytes=4;
    language_on_ime_message(&s,NULL,WM_IME_COMPOSITION,0,GCS_COMPSTR|GCS_CURSORPOS,&handled);
    assert(handled && !strcmp(s.composition,"AB") && s.composition_cursor==2);
    language_on_ime_message(&s,NULL,WM_IME_COMPOSITION,0,GCS_RESULTSTR,&handled);expect_text(&s,"tABt",3,3);
    ime_cursor=-1;language_read_cursor_pos(&s);assert(s.composition_cursor==2);ime_cursor=7;
    language_read_cursor_pos(&s);assert(s.composition_cursor==7);ime_cursor=2;
    ime_bytes=-1;language_read_composition(&s);assert(!strcmp(s.composition,"AB"));
    ime_bytes=513;language_read_composition(&s);assert(!strcmp(s.composition,"AB"));
    language_on_ime_message(&s,NULL,WM_IME_ENDCOMPOSITION,0,0,&handled);
    assert(handled && !s.composition_length);
    language_on_ime_message(&s,NULL,WM_INPUTLANGCHANGE,0,0x412,&handled);
    assert(handled && s.keyboard_lang==0x412);
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
    clipboard_failure=0;language_copy(&s);assert(!strcmp(clipboard_text,"**") && clip_size==3 && clip_system_owned);
    clip_system_owned=0;test_global_free((HGLOBAL)clip_pages);
    language_set_masked(&s,0);language_set_selection(&s,8,3);language_copy(&s);
    assert(!strcmp(clipboard_text,"34567") && clip_size==6 && clip_system_owned);
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

static void test_charset_tags(void)
{
    struct Language a,b;
    char saved_tag[20];
    language_construct(&a);language_construct(&b);
    g_useCharsetTag=1;strcpy(saved_tag,language_charset_tag(&a,1));assert(!strcmp(saved_tag,"|00"));
    language_charset_tag(&b,0);assert(!strcmp(a.charset_tag,saved_tag));g_useCharsetTag=0;
    assert(!strcmp(language_charset_tag(&a,1),""));
    language_destruct(&a);language_destruct(&b);assert(!heap_live);
}

static CANDIDATELIST *candidate_fixture(void)
{
    CANDIDATELIST *list=(CANDIDATELIST*)candidate_storage;
    static const WCHAR first[]={ 'a','l','p','h','a',0 };
    static const WCHAR second[]={0xac00,0};
    static const WCHAR third[]={ 'z',0 };
    size_t next=offsetof(CANDIDATELIST,dwOffset)+3*sizeof(DWORD);
    memset(candidate_storage,0,sizeof candidate_storage);list->dwCount=3;list->dwSelection=1;list->dwPageSize=20;list->dwStyle=IME_CAND_READ;
    list->dwOffset[0]=(DWORD)next;memcpy((BYTE*)list+next,first,sizeof first);next+=sizeof first;
    list->dwOffset[1]=(DWORD)next;memcpy((BYTE*)list+next,second,sizeof second);next+=sizeof second;
    list->dwOffset[2]=(DWORD)next;memcpy((BYTE*)list+next,third,sizeof third);next+=sizeof third;
    list->dwSize=(DWORD)next;candidate_query=candidate_reply=(DWORD)next;return list;
}
static void test_candidates(void)
{
    struct Language s;
    CANDIDATELIST *list,*saved;
    DWORD full;
    int releases,calls;
    BYTE handled;
    language_construct(&s);candidate_fixture();
    language_on_ime_message(&s,NULL,WM_IME_NOTIFY,IMN_OPENCANDIDATE,0,&handled);
    assert(handled && s.candidate_count==3 && s.candidate_selection==1 && s.candidate_page==9);
    assert(!strcmp(language_candidate_text(&s,0),"alpha"));
    assert(!strcmp(language_candidate_text(&s,1),"가"));
    assert(!strcmp(language_candidate_text(&s,2),"z"));
    assert(!strcmp(language_candidate_text(&s,-1),"") && !strcmp(language_candidate_text(&s,3),""));
    saved=s.candidates;fail_after=0;releases=ime_releases;language_read_candidates(&s);
    assert(!s.candidate_count && s.candidates==saved && ime_releases==releases+1);fail_after=-1;
    full=candidate_reply;
    for (DWORD bytes=0;bytes<full;++bytes) {
        candidate_fixture();candidate_reply=bytes;releases=ime_releases;
        language_read_candidates(&s);assert(!s.candidate_count && ime_releases==releases+1 && s.candidates==saved);
    }
    for (int fault=0;fault<10;++fault) {
        list=candidate_fixture();
        switch (fault) {
        case 0:list->dwSize=4;break;
        case 1:list->dwCount=0xffffffffu;break;
        case 2:list->dwSelection=3;break;
        case 3:list->dwOffset[0]=1;break;
        case 4:list->dwOffset[0]=24;break;
        case 5:list->dwOffset[0]=list->dwSize;break;
        case 6:((WCHAR*)((BYTE*)list+list->dwOffset[2]))[1]='x';break;
        case 7:list->dwStyle=IME_CAND_CODE;list->dwCount=1;list->dwSelection=0;break;
        case 8:candidate_reply=candidate_query+1;break;
        case 9:candidate_query=0xffffffffu;break;
        }
        language_read_candidates(&s);assert(!s.candidate_count && s.candidates==saved && !strcmp(language_candidate_text(&s,0),""));
    }
    candidate_fixture();language_read_candidates(&s);assert(s.candidate_count==3);
    calls=heap_calls;ime_missing=1;releases=ime_releases;language_read_candidates(&s);
    assert(!s.candidate_count && heap_calls==calls && releases==ime_releases);ime_missing=0;
    list=candidate_fixture();list->dwCount=0;list->dwSelection=0;list->dwSize=24;candidate_reply=candidate_query=24;
    language_read_candidates(&s);assert(!s.candidate_count);
    candidate_fixture();language_read_candidates(&s);assert(s.candidate_count==3);
    language_on_ime_message(&s,NULL,WM_IME_NOTIFY,IMN_CLOSECANDIDATE,0,&handled);assert(handled && !s.candidate_count);
    language_destruct(&s);language_destruct(&s);assert(!heap_live);candidate_query=candidate_reply=0;
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
    test_input_ownership();test_charset_tags();test_candidates();test_edit_sequence();
    puts("Language input: C ownership, atomic edits, aliasing, reusable storage, bounded IME candidates, charset tags, DBCS and clipboard failures passed.");
    return 0;
}
