#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "platform/language.h"
#include <curl/curl.h>

UINT g_codePage = 949;
static struct Block { unsigned char *raw; size_t size; } blocks[256];
static int live, attempts, fail_allocation;
static void *allocate(size_t size)
{
    int i;
    if (++attempts == fail_allocation) return NULL;
    assert(size <= SIZE_MAX - 16);
    for (i = 0; i < 256 && blocks[i].raw; ++i) {}
    assert(i < 256);
    blocks[i].raw = malloc(size + 16); assert(blocks[i].raw);
    blocks[i].size = size; ++live;
    memset(blocks[i].raw, 0xa5, size + 16);
    return blocks[i].raw + 8;
}
static int block_index(void *p)
{
    int i;
    for (i = 0; i < 256 && (!blocks[i].raw || blocks[i].raw + 8 != p); ++i) {}
    assert(i < 256); return i;
}
static void release(void *p)
{
    int i, j;
    if (!p) return;
    i = block_index(p);
    for (j = 0; j < 8; ++j) {
        assert(blocks[i].raw[j] == 0xa5);
        assert(blocks[i].raw[8 + blocks[i].size + j] == 0xa5);
    }
    free(blocks[i].raw); blocks[i].raw = NULL; --live;
}
static void *zero_allocate(size_t count, size_t size)
{
    void *p;
    assert(!size || count <= SIZE_MAX / size);
    p = allocate(count * size);
    if (p) memset(p, 0, count * size);
    return p;
}
static void *resize(void *p, size_t size)
{
    void *next = allocate(size);
    if (next && p) {
        size_t old_size = blocks[block_index(p)].size;
        memcpy(next, p, old_size < size ? old_size : size); release(p);
    }
    return next;
}

static int fail_load, missing_export, init_error, easy_error, header_error, form_error, option_error, info_error;
static int loads, unloads, gets, inits, cleanups, easy_live, headers_live, forms_live, performs, options;
static int header_calls, form_calls, conversions, fail_conversion;
static const char *payload = "response";
static size_t payload_size = 8, payload_step = 3;
static char cleanup_order[256];
static int cleanup_count;
struct FakeEasy { curl_write_callback write; void *context; const char *url; struct curl_slist *headers; struct curl_httppost *form; };
struct FakeForm { struct curl_httppost base; CURLformoption option; };
static char *copy_text(const char *text)
{ size_t n = strlen(text) + 1; char *p = malloc(n); assert(p); memcpy(p, text, n); return p; }
static CURLcode fake_global_init(long flags) { assert(flags == CURL_GLOBAL_ALL); ++inits; return init_error ? CURLE_FAILED_INIT : CURLE_OK; }
static void fake_global_cleanup(void) { ++cleanups; cleanup_order[cleanup_count++] = 'g'; }
static CURL *fake_easy_init(void)
{
    struct FakeEasy *easy;
    if (easy_error) return NULL;
    easy = calloc(1, sizeof *easy); assert(easy); ++easy_live; return (CURL *)easy;
}
static void fake_easy_cleanup(CURL *handle)
{ assert(handle && easy_live > 0); --easy_live; free(handle); cleanup_order[cleanup_count++] = 'e'; }
static struct curl_slist *fake_slist_append(struct curl_slist *first, const char *text)
{
    struct curl_slist *next, **tail = &first;
    if (++header_calls == header_error) return NULL;
    next = calloc(1, sizeof *next); assert(next); next->data = copy_text(text); ++headers_live;
    while (*tail) tail = &(*tail)->next;
    *tail = next; return first;
}
static void fake_slist_free_all(struct curl_slist *list)
{
    while (list) { struct curl_slist *next = list->next; free(list->data); free(list); --headers_live; list = next; }
    cleanup_order[cleanup_count++] = 'h';
}
static CURLFORMcode fake_formadd(struct curl_httppost **first, struct curl_httppost **last, ...)
{
    struct FakeForm *form;
    const char *name, *value;
    CURLformoption option;
    va_list args;
    if (++form_calls == form_error) return CURL_FORMADD_MEMORY;
    va_start(args, last); assert(va_arg(args, int) == CURLFORM_COPYNAME);
    name = va_arg(args, const char *); option = (CURLformoption)va_arg(args, int);
    assert(option == CURLFORM_COPYCONTENTS || option == CURLFORM_FILE);
    value = va_arg(args, const char *); assert(va_arg(args, int) == CURLFORM_END); va_end(args);
    form = calloc(1, sizeof *form); assert(form);
    form->base.name = copy_text(name); form->base.contents = copy_text(value); form->option = option;
    if (*last) (*last)->next = &form->base; else *first = &form->base;
    *last = &form->base; ++forms_live; return CURL_FORMADD_OK;
}
static void fake_formfree(struct curl_httppost *form)
{
    assert(cleanup_count && cleanup_order[cleanup_count - 1] == 'e');
    while (form) { struct curl_httppost *next = form->next; free(form->name); free(form->contents); free(form); --forms_live; form = next; }
    cleanup_order[cleanup_count++] = 'f';
}
static CURLcode fake_easy_setopt(CURL *handle, CURLoption option, ...)
{
    struct FakeEasy *easy = (struct FakeEasy *)handle;
    va_list args;
    if (++options == option_error) return CURLE_UNKNOWN_OPTION;
    va_start(args, option);
    switch (option) {
    case CURLOPT_URL: easy->url = va_arg(args, const char *); break;
    case CURLOPT_HTTPHEADER: easy->headers = va_arg(args, struct curl_slist *); break;
    case CURLOPT_POST: assert(va_arg(args, long) == 1L); break;
    case CURLOPT_HTTPPOST: easy->form = va_arg(args, struct curl_httppost *); break;
    case CURLOPT_WRITEFUNCTION: easy->write = va_arg(args, curl_write_callback); break;
    case CURLOPT_WRITEDATA: easy->context = va_arg(args, void *); break;
    case CURLOPT_TIMEOUT: case CURLOPT_CONNECTTIMEOUT: assert(va_arg(args, long) == 3L); break;
    default: abort();
    }
    va_end(args); return CURLE_OK;
}
static CURLcode fake_easy_perform(CURL *handle)
{
    struct FakeEasy *easy = (struct FakeEasy *)handle;
    size_t offset = 0;
    assert(easy->url && easy->write && easy->context && easy->headers); ++performs;
    while (offset < payload_size) {
        size_t n = payload_size - offset;
        if (n > payload_step) n = payload_step;
        if (easy->write((char *)payload + offset, 1, n, easy->context) != n) return CURLE_WRITE_ERROR;
        offset += n;
    }
    return CURLE_OK;
}
static CURLcode fake_easy_getinfo(CURL *handle, CURLINFO info, ...)
{
    va_list args;
    assert(handle && info == CURLINFO_RESPONSE_CODE);
    if (info_error) return CURLE_BAD_FUNCTION_ARGUMENT;
    va_start(args, info); *va_arg(args, long *) = 207L; va_end(args); return CURLE_OK;
}
static HMODULE WINAPI fake_load(LPCSTR path)
{ assert(!strcmp(path, "libcurl.dll")); if (fail_load) return NULL; ++loads; return (HMODULE)(uintptr_t)1; }
static BOOL WINAPI fake_unload(HMODULE module)
{ assert(module == (HMODULE)(uintptr_t)1); ++unloads; cleanup_order[cleanup_count++] = 'u'; return 1; }
static FARPROC WINAPI fake_get(HMODULE module, LPCSTR name)
{
    static const struct Export { const char *name; FARPROC function; } exports[] = {
        {"curl_global_init", (FARPROC)fake_global_init}, {"curl_global_cleanup", (FARPROC)fake_global_cleanup},
        {"curl_slist_append", (FARPROC)fake_slist_append}, {"curl_formadd", (FARPROC)fake_formadd},
        {"curl_easy_init", (FARPROC)fake_easy_init}, {"curl_easy_setopt", (FARPROC)fake_easy_setopt},
        {"curl_easy_perform", (FARPROC)fake_easy_perform}, {"curl_easy_cleanup", (FARPROC)fake_easy_cleanup},
        {"curl_formfree", (FARPROC)fake_formfree}, {"curl_slist_free_all", (FARPROC)fake_slist_free_all},
        {"curl_easy_getinfo", (FARPROC)fake_easy_getinfo}
    };
    size_t i;
    assert(module == (HMODULE)(uintptr_t)1);
    if (++gets == missing_export) return NULL;
    for (i = 0; i < sizeof exports / sizeof exports[0]; ++i)
        if (!strcmp(name, exports[i].name)) return exports[i].function;
    abort();
}
static int WINAPI fake_multibyte(UINT page, DWORD flags, LPCCH text, int count, LPWSTR wide, int capacity)
{ if (++conversions == fail_conversion) return 0; return MultiByteToWideChar(page, flags, text, count, wide, capacity); }
static int WINAPI fake_wide(UINT page, DWORD flags, LPCWCH wide, int count, LPSTR text, int capacity, LPCCH fallback, LPBOOL used)
{ if (++conversions == fail_conversion) return 0; return WideCharToMultiByte(page, flags, wide, count, text, capacity, fallback, used); }

#define LoadLibraryA fake_load
#define FreeLibrary fake_unload
#define GetProcAddress fake_get
#define MultiByteToWideChar fake_multibyte
#define WideCharToMultiByte fake_wide
#define malloc allocate
#define calloc zero_allocate
#define realloc resize
#define free release
#include "../src/web/curl.c"
#undef free
#undef realloc
#undef calloc
#undef malloc
#undef WideCharToMultiByte
#undef MultiByteToWideChar
#undef GetProcAddress
#undef FreeLibrary
#undef LoadLibraryA

static void reset(void)
{
    assert(!active_client && !live && !easy_live && !headers_live && !forms_live);
    assert(loads == unloads);
    fail_load = missing_export = init_error = easy_error = header_error = form_error = option_error = info_error = 0;
    loads = unloads = gets = inits = cleanups = performs = options = header_calls = form_calls = 0;
    attempts = fail_allocation = conversions = fail_conversion = cleanup_count = 0;
    payload = "response"; payload_size = 8; payload_step = 3;
}
static void loader_and_creation(void)
{
    struct HttpPost *post;
    int i;
    reset(); assert(!http_post_create("unused")); http_client_stop();
    fail_allocation = 1; assert(!http_client_start()); reset();
    fail_load = 1; assert(!http_client_start() && !inits && !cleanups); reset();
    for (i = 1; i <= 11; ++i) {
        missing_export = i; assert(!http_client_start());
        assert(gets == 11 && !inits && !cleanups && loads == 1 && unloads == 1); reset();
    }
    init_error = 1; assert(!http_client_start() && inits == 1 && !cleanups); reset();
    for (i = 1; i <= 2; ++i) {
        assert(http_client_start()); fail_allocation = attempts + i;
        assert(!http_post_create("http://test.invalid/form")); assert(live == 1);
        http_client_stop(); assert(cleanups == 1); reset();
    }
    assert(http_client_start()); easy_error = 1;
    assert(!http_post_create("http://test.invalid/form") && live == 1);
    http_client_stop(); reset();
    for (i = 1; i <= 3; ++i) {
        assert(http_client_start()); header_error = i;
        assert(!http_post_create("http://test.invalid/form"));
        assert(live == 1 && !headers_live && !easy_live); http_client_stop(); reset();
    }
    assert(http_client_start() && !http_client_start());
    post = http_post_create("http://test.invalid/form"); assert(post && active_client->references == 2);
    http_client_stop(); assert(!cleanups && !unloads && !active_client);
    assert(http_post_perform(post) == CURLE_OK);
    http_post_free(post); assert(cleanups == 1 && unloads == 1);
    http_client_stop(); http_post_free(NULL); reset();
}
static void requests_and_response(void)
{
    struct HttpPost *post;
    const char *body;
    char url[] = "http://test.invalid/form", name[] = "WorldName", value[] = "borrowed value";
    char data[9000];
    size_t size, old_size;
    int i;
    assert(http_post_perform(NULL) == CURLE_FAILED_INIT && http_post_status(NULL) == 0);
    assert(http_post_add_text(NULL, "x", "y") == CURLE_FAILED_INIT);
    assert(http_post_add_file(NULL, "x", "y") == CURLE_FAILED_INIT);
    assert(!strcmp(http_post_body(NULL, &size), "") && size == 0);
    assert(http_client_start()); post = http_post_create(url); assert(post);
    assert(!http_post_add_text(post, name, value));
    assert(!http_post_add_file(post, "Image", "test-image.bmp"));
    memset(url, 'x', sizeof url - 1); memset(name, 'x', sizeof name - 1); memset(value, 'x', sizeof value - 1);
    assert(!strcmp(post->url, "http://test.invalid/form"));
    assert(!strcmp(post->form->name, "WorldName") && !strcmp(post->form->contents, "borrowed value"));
    assert(((struct FakeForm *)post->last)->option == CURLFORM_FILE);
    assert(!strcmp(post->headers->data, "Content-Type: multipart/form-data"));
    assert(!strcmp(post->headers->next->data, "Expect: 100-continue"));
    assert(!strcmp(post->headers->next->next->data, "Connection: keep-alive"));
    memset(data, 'z', sizeof data); data[2] = 0; payload = data; payload_size = sizeof data; payload_step = 701;
    assert(http_post_perform(post) == CURLE_OK && options == 8 && performs == 1);
    body = http_post_body(post, &size); assert(size == sizeof data && !memcmp(body, data, size) && body[size] == 0);
    assert(http_post_status(post) == 207); info_error = 1; assert(http_post_status(post) == 0); info_error = 0;
    assert(!http_receive_body(data, SIZE_MAX, 2, post));
    old_size = post->size; post->size = SIZE_MAX - 1; assert(!http_receive_body(data, 1, 1, post)); post->size = old_size;
    assert(!http_receive_body(NULL, 1, 1, post));
    assert(!http_receive_body(data, 0, SIZE_MAX, post));
    payload = "new"; payload_size = 3; assert(!http_post_perform(post));
    assert(!strcmp(http_post_body(post, &size), "new") && size == 3);
    http_post_free(post); http_client_stop(); reset();
    for (i = 1; i <= 8; ++i) {
        assert(http_client_start()); post = http_post_create("url"); option_error = i;
        assert(http_post_perform(post) == CURLE_UNKNOWN_OPTION && !performs && options == i);
        http_post_free(post); http_client_stop(); reset();
    }
    assert(http_client_start()); post = http_post_create("url");
    assert(!http_post_add_text(post, "first", "value")); form_error = 2;
    assert(http_post_add_text(post, "failed", "value") == CURLE_OUT_OF_MEMORY);
    assert(http_post_add_text(post, "later", "value") == CURLE_OUT_OF_MEMORY && form_calls == 2);
    assert(http_post_perform(post) == CURLE_OUT_OF_MEMORY && !performs);
    http_post_free(post); http_client_stop(); reset();
    assert(http_client_start()); post = http_post_create("url");
    assert(http_post_add_text(post, "WorldName", NULL) == CURLE_BAD_FUNCTION_ARGUMENT);
    assert(http_post_perform(post) == CURLE_BAD_FUNCTION_ARGUMENT && !performs);
    http_post_free(post); http_client_stop(); reset();
    assert(http_client_start()); post = http_post_create("url"); fail_allocation = attempts + 1;
    assert(http_post_perform(post) == CURLE_WRITE_ERROR); assert(!post->body && !post->size);
    fail_allocation = 0; assert(!http_post_perform(post));
    old_size = post->size; fail_allocation = attempts + 1;
    assert(!http_receive_body(data, 1, sizeof data, post) && post->size == old_size && !memcmp(post->body, "response", 8));
    http_post_free(post); http_client_stop(); reset();
}
static void text_conversion(void)
{
    static const char korean[] = "\xc7\xd1\xb1\xdb";
    static const char utf8[] = "\xed\x95\x9c\xea\xb8\x80";
    char *text, *back, *large;
    int i;
    assert(!http_text_to_utf8(NULL) && !http_text_from_utf8(NULL));
    text = http_text_to_utf8(""); assert(text && !*text); release(text);
    text = http_text_to_utf8(korean); assert(text && !strcmp(text, utf8));
    back = http_text_from_utf8(text); assert(back && !strcmp(back, korean)); release(back); release(text);
    large = malloc(40001); assert(large);
    for (i = 0; i < 10000; ++i) memcpy(large + 4 * i, korean, 4);
    large[40000] = 0; text = http_text_to_utf8(large); assert(text && strlen(text) == 60000);
    back = http_text_from_utf8(text); assert(back && !strcmp(back, large)); release(back); release(text); free(large);
    for (i = 1; i <= 2; ++i) {
        fail_allocation = attempts + i; assert(!http_text_to_utf8(korean) && !live); fail_allocation = 0;
        fail_allocation = attempts + i; assert(!http_text_from_utf8(utf8) && !live); fail_allocation = 0;
    }
    for (i = 1; i <= 4; ++i) {
        conversions = 0; fail_conversion = i; assert(!http_text_to_utf8(korean) && !live);
        conversions = 0; assert(!http_text_from_utf8(utf8) && !live);
    }
    fail_conversion = 0; reset();
}
int main(void)
{
    loader_and_creation(); requests_and_response(); text_conversion();
    puts("HTTP ownership: official API types, 11 missing exports, failed initialization, retained DLLs, request rollback, sticky form errors, eight option failures, guarded binary responses and Korean conversion failures passed");
    return 0;
}
