#include "session/state.h"
#include "session/chat_macros.h"
#include "session/character_name.h"
#include "json/service_json.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int live, attempts, fail_at;
static void *checked_malloc(size_t size)
{
    unsigned char *memory;
    if (++attempts == fail_at) return NULL;
    assert(size <= SIZE_MAX - 64);
    memory = malloc(size + 64); assert(memory);
    memset(memory, 0xa7, size + 64);
    *(size_t *)memory = size;
    ++live;
    return memory + 32;
}
static void checked_free(void *pointer)
{
    unsigned char *memory;
    size_t size, i;
    if (!pointer) return;
    memory = (unsigned char *)pointer - 32;
    size = *(size_t *)memory;
    for (i = sizeof size; i < 32; ++i) assert(memory[i] == 0xa7);
    for (i = 0; i < 32; ++i) assert(memory[32 + size + i] == 0xa7);
    assert(live > 0); --live;
    free(memory);
}
static void *checked_realloc(void *pointer, size_t size)
{
    void *replacement = checked_malloc(size);
    size_t old_size;
    if (!replacement) return NULL;
    if (pointer) {
        old_size = *(size_t *)((unsigned char *)pointer - 32);
        memcpy(replacement, pointer, old_size < size ? old_size : size);
        checked_free(pointer);
    }
    return replacement;
}

#define malloc checked_malloc
#define realloc checked_realloc
#define free checked_free
#include "../src/core/owned_text.c"
#include "../src/core/text_array.c"
#undef malloc
#undef realloc
#undef free
#include "../src/session/session_chat_macros.c"
#include "../src/session/chat_macro_json.c"
#include "../src/session/character_name.c"

const char *g_registryKeyPath = "Software\\RagexeTest";
static int registry_available = 1, opened, closed;
static LSTATUS WINAPI read_open(HKEY parent, LPCSTR path, DWORD options, REGSAM access, PHKEY key)
{
    assert(parent == HKEY_CURRENT_USER && options == 0 && access == KEY_QUERY_VALUE);
    assert(!strcmp(path, "Software\\RagexeTest\\ShortCutList\\"));
    if (!registry_available) return ERROR_FILE_NOT_FOUND;
    *key = (HKEY)(uintptr_t)42; ++opened;
    return ERROR_SUCCESS;
}
static LSTATUS WINAPI read_value(HKEY key, LPCSTR name, LPDWORD reserved, LPDWORD type, LPBYTE value, LPDWORD size)
{
    const char *text;
    assert(key == (HKEY)(uintptr_t)42 && !reserved && *size == 256);
    *type = REG_SZ;
    switch (atoi(name)) {
    case 0: text = "custom"; break;
    case 1: text = ""; break;
    case 2: text = "/기쁨"; break;
    case 3: *type = REG_BINARY; text = "ignored"; break;
    case 4:
        memcpy(value, "unterminated", 12); *size = 12; return ERROR_SUCCESS;
    case 5: *size = 500; return ERROR_MORE_DATA;
    default: return ERROR_FILE_NOT_FOUND;
    }
    *size = (DWORD)strlen(text) + 1;
    memcpy(value, text, *size);
    return ERROR_SUCCESS;
}
static LSTATUS WINAPI read_close(HKEY key)
{
    assert(key == (HKEY)(uintptr_t)42); ++closed; return ERROR_SUCCESS;
}
char *msg_str(int id)
{
    static char text[32];
    sprintf(text, "default-%x", id);
    return text;
}
#define RegOpenKeyExA read_open
#define RegQueryValueExA read_value
#define RegCloseKey read_close
#include "../src/session/chat_macro_settings.c"
#undef RegOpenKeyExA
#undef RegQueryValueExA
#undef RegCloseKey

static void clear_macros(struct Session *session)
{
    text_array_clear(&session->chatMacros);
    text_array_clear(&session->chatMacrosLoaded);
    assert(!live);
}
static void old_macros(struct Session *session)
{
    int i;
    char text[32];
    for (i = 0; i < CHAT_MACRO_COUNT; ++i) {
        sprintf(text, "old-%d", i);
        assert(session_set_chat_macro(session, i, text_view_cstr(text)));
    }
}
static void test_names(void)
{
    static struct Session first, second;
    static unsigned char before[sizeof first];
    static const unsigned char korean[] = {0xc5,0xd7,0xbd,0xba,0xc6,0xae};
    SYSTEM_INFO system;
    DWORD protection;
    unsigned char *pages;
    const char *retained;
    size_t length, offset = offsetof(struct Session, charName);
    session_set_character_name(&first, text_view_cstr("first"));
    retained = session_character_name(&first);
    session_set_character_name(&second, text_view_cstr("second"));
    assert(retained == first.charName && !strcmp(retained, "first"));
    assert(!strcmp(session_character_name(&second), "second"));
    session_set_character_name(&first, text_view_cstr("테스트"));
    assert(!memcmp(first.charName, korean, sizeof korean) && !first.charName[sizeof korean]);
    session_set_character_name(&first, text_view("abc\0ignored", 11));
    assert(!strcmp(first.charName, "abc"));
    session_set_character_name(&first, text_view(first.charName + 1, 2));
    assert(!strcmp(first.charName, "bc"));
    assert(!*session_character_name(NULL));
    session_set_character_name(NULL, text_view(NULL, 0));
    GetSystemInfo(&system);
    pages = VirtualAlloc(NULL, 2 * system.dwPageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    assert(pages && VirtualProtect(pages + system.dwPageSize, system.dwPageSize, PAGE_NOACCESS, &protection));
    for (length = 0; length <= 64; ++length) {
        char *source = (char *)pages + system.dwPageSize - length;
        size_t kept = length < 64 ? length : 63;
        memset(source, 'x', length);
        memset(&first, 0xa5, sizeof first); memcpy(before, &first, sizeof first);
        session_set_character_name(&first, text_view(source, length));
        assert(strlen(first.charName) == kept);
        assert(!memcmp(first.charName, source, kept));
        assert(!memcmp(before, &first, offset));
        assert(!memcmp(before + offset + 64, (unsigned char *)&first + offset + 64, sizeof first - offset - 64));
    }
    assert(VirtualFree(pages, 0, MEM_RELEASE));
}
static void test_arrays(void)
{
    struct TextArray array = {0};
    struct OwnedText *retained;
    attempts = 0;
    assert(!text_array_resize(&array, SIZE_MAX) && !attempts);
    assert(text_array_resize(&array, 2));
    assert(owned_text_assign(&array.items[0], text_view_cstr("retained")));
    retained = array.items;
    attempts = 0; fail_at = 1;
    assert(!text_array_resize(&array, 5) && array.items == retained && array.size == 2);
    fail_at = 0;
    assert(text_array_resize(&array, 5) && !strcmp(text_array_at(&array, 0).data, "retained"));
    assert(!text_array_at(&array, 4).size && !text_array_at(&array, 5).size);
    assert(text_array_resize(&array, 0));
    assert(text_array_resize(&array, 5) && !text_array_at(&array, 0).size);
    text_array_clear(&array); text_array_clear(&array);
    assert(!live);
}
static int test_json_macros(void)
{
    static struct Session session;
    struct JsonTree root, data, roundtrip, scalar;
    struct OwnedText encoded = {0};
    struct TextArray previous, snapshot;
    char input[4096], large[4097];
    int i, count, failures = 0;
    size_t used = 0;
    assert(session_chat_macros_changed(&session));
    assert(!session_chat_macro(&session, -1).size && !session_chat_macro(&session, 10).size);
    attempts = 0;
    assert(!session_set_chat_macro(&session, -1, text_view_cstr("x")) && !attempts);
    assert(!session_set_chat_macro(&session, 10, text_view_cstr("x")) && !attempts);
    old_macros(&session);
    previous = session.chatMacros;
    attempts = 0; fail_at = 1;
    assert(!session_set_chat_macro(&session, 0, text_view_cstr("replacement")));
    fail_at = 0;
    assert(session.chatMacros.items == previous.items && !strcmp(session_chat_macro(&session, 0).data, "old-0"));
    json_tree_init(&root);
    used += sprintf(input + used, "[");
    for (i = 0; i < 64; ++i) used += sprintf(input + used, "%s\"slot-%d\"", i ? "," : "", i);
    used += sprintf(input + used, "]");
    assert(json_parse_text(&root, input, used));
    attempts = 0;
    assert(session_read_chat_macros(&session, &root));
    count = attempts;
    assert(session.chatMacros.size == 10 && session.chatMacrosLoaded.size == 10);
    assert(!strcmp(session_chat_macro(&session, 9).data, "slot-9") && !session_chat_macros_changed(&session));
    for (i = 1; i <= count; ++i) {
        old_macros(&session);
        previous = session.chatMacros; snapshot = session.chatMacrosLoaded;
        attempts = 0; fail_at = i;
        assert(!session_read_chat_macros(&session, &root));
        fail_at = 0; ++failures;
        assert(previous.items == session.chatMacros.items && snapshot.items == session.chatMacrosLoaded.items);
        assert(!strcmp(session_chat_macro(&session, 0).data, "old-0"));
        assert(!strcmp(text_array_at(&session.chatMacrosLoaded, 9).data, "slot-9"));
    }
    json_tree_free(&root);
    assert(json_parse_text(&root, "[\"head\\u0000tail\",true,false,7,null]", strlen("[\"head\\u0000tail\",true,false,7,null]")));
    assert(session_read_chat_macros(&session, &root));
    assert(session_chat_macro(&session, 0).size == 9 && !memcmp(session_chat_macro(&session, 0).data, "head\0tail", 9));
    assert(!strcmp(session_chat_macro(&session, 1).data, "true"));
    assert(!strcmp(session_chat_macro(&session, 2).data, "false"));
    assert(!session_chat_macro(&session, 3).size && !session_chat_macro(&session, 4).size);
    assert(!strcmp(session_chat_macro(&session, 5).data, "old-5"));
    assert(!text_array_at(&session.chatMacrosLoaded, 5).size && session_chat_macros_changed(&session));
    memset(large, 'L', sizeof large - 1); large[sizeof large - 1] = 0;
    assert(session_set_chat_macro(&session, 9, text_view_cstr(large)));
    assert(session_chat_macro(&session, 9).size == 4096);
    json_tree_init(&data); json_tree_init(&roundtrip);
    session_write_chat_macros(&session, &data);
    assert(json_write_request(&data, &encoded));
    assert(json_parse_text(&roundtrip, owned_text_cstr(&encoded), encoded.size));
    assert(session_read_chat_macros(&session, json_tree_find(&roundtrip, "EmotionHotkey", 13)));
    assert(!session_chat_macros_changed(&session) && session_chat_macro(&session, 9).size == 4096);
    assert(session_chat_macro(&session, 0).size == 9);
    owned_text_clear(&encoded); json_tree_free(&roundtrip); json_tree_free(&data);
    json_value_from_int(&scalar, 4);
    previous = session.chatMacros;
    assert(!session_read_chat_macros(&session, &scalar) && previous.items == session.chatMacros.items);
    json_tree_free(&scalar); json_tree_free(&root);
    assert(session_read_chat_macros(&session, NULL) && session_chat_macros_changed(&session));
    assert(text_array_resize(&session.chatMacrosLoaded, 3) && session_chat_macros_changed(&session));
    clear_macros(&session);
    assert(session_read_chat_macros(&session, NULL) && !session_chat_macros_changed(&session));
    clear_macros(&session);
    return failures;
}
static int test_defaults(void)
{
    static struct Session session;
    struct TextArray previous;
    int count, i;
    old_macros(&session);
    attempts = 0;
    session_init_short_cut_texts(&session);
    count = attempts;
    assert(!strcmp(session_chat_macro(&session, 0).data, "custom"));
    assert(session_chat_macro(&session, 1).size == 0);
    assert(!strcmp(session_chat_macro(&session, 2).data, "default-222"));
    assert(!strcmp(session_chat_macro(&session, 3).data, "default-223"));
    assert(!strcmp(session_chat_macro(&session, 4).data, "unterminated"));
    assert(!strcmp(session_chat_macro(&session, 5).data, "default-226"));
    assert(opened == closed);
    for (i = 1; i <= count; ++i) {
        old_macros(&session); previous = session.chatMacros;
        attempts = 0; fail_at = i;
        session_init_short_cut_texts(&session);
        fail_at = 0;
        assert(previous.items == session.chatMacros.items);
        assert(!strcmp(session_chat_macro(&session, 0).data, "old-0"));
        assert(opened == closed);
    }
    registry_available = 0;
    session_init_short_cut_texts(&session);
    assert(!strcmp(session_chat_macro(&session, 0).data, "default-220"));
    clear_macros(&session);
    return count;
}
int main(void)
{
    int failures;
    test_names(); test_arrays();
    failures = test_json_macros() + test_defaults();
    assert(!live && opened == closed);
    printf("Session text: session-owned names, 65 guarded spans, aliases and Korean bytes; bounded macro JSON, long/counted text, partial defaults, registry errors and %d allocation failures passed\n", failures);
    return 0;
}
