#include "web/http_post.h"
#include "platform/language.h"
#include <curl/curl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

struct HttpClient {
    HMODULE module;
    LONG references;
    CURLcode (*global_init)(long);
    void (*global_cleanup)(void);
    struct curl_slist *(*slist_append)(struct curl_slist *, const char *);
    CURLFORMcode (*formadd)(struct curl_httppost **, struct curl_httppost **, ...);
    CURL *(*easy_init)(void);
    CURLcode (*easy_setopt)(CURL *, CURLoption, ...);
    CURLcode (*easy_perform)(CURL *);
    void (*easy_cleanup)(CURL *);
    void (*formfree)(struct curl_httppost *);
    void (*slist_free_all)(struct curl_slist *);
    CURLcode (*easy_getinfo)(CURL *, CURLINFO, ...);
};

struct HttpPost {
    struct HttpClient *client;
    CURL *easy;
    struct curl_slist *headers;
    struct curl_httppost *form, *last;
    char *url, *body;
    size_t size, capacity;
    CURLcode error;
};

static struct HttpClient *active_client;

static int http_load_functions(struct HttpClient *client)
{
    client->global_init = (CURLcode (*)(long))GetProcAddress(client->module, "curl_global_init");
    client->global_cleanup = (void (*)(void))GetProcAddress(client->module, "curl_global_cleanup");
    client->slist_append = (struct curl_slist *(*)(struct curl_slist *, const char *))GetProcAddress(client->module, "curl_slist_append");
    client->formadd = (CURLFORMcode (*)(struct curl_httppost **, struct curl_httppost **, ...))GetProcAddress(client->module, "curl_formadd");
    client->easy_init = (CURL *(*)(void))GetProcAddress(client->module, "curl_easy_init");
    client->easy_setopt = (CURLcode (*)(CURL *, CURLoption, ...))GetProcAddress(client->module, "curl_easy_setopt");
    client->easy_perform = (CURLcode (*)(CURL *))GetProcAddress(client->module, "curl_easy_perform");
    client->easy_cleanup = (void (*)(CURL *))GetProcAddress(client->module, "curl_easy_cleanup");
    client->formfree = (void (*)(struct curl_httppost *))GetProcAddress(client->module, "curl_formfree");
    client->slist_free_all = (void (*)(struct curl_slist *))GetProcAddress(client->module, "curl_slist_free_all");
    client->easy_getinfo = (CURLcode (*)(CURL *, CURLINFO, ...))GetProcAddress(client->module, "curl_easy_getinfo");
    return client->global_init && client->global_cleanup && client->slist_append &&
        client->formadd && client->easy_init && client->easy_setopt &&
        client->easy_perform && client->easy_cleanup && client->formfree &&
        client->slist_free_all && client->easy_getinfo;
}

static void http_client_release(struct HttpClient *client)
{
    if (InterlockedDecrement(&client->references) == 0) {
        client->global_cleanup();
        FreeLibrary(client->module);
        free(client);
    }
}

int http_client_start(void)
{
    struct HttpClient *client;
    if (active_client) return 0;
    client = calloc(1, sizeof *client);
    if (!client) return 0;
    client->module = LoadLibraryA("libcurl.dll");
    if (!client->module || !http_load_functions(client) ||
        client->global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
        if (client->module) FreeLibrary(client->module);
        free(client);
        return 0;
    }
    client->references = 1;
    active_client = client;
    return 1;
}

void http_client_stop(void)
{
    struct HttpClient *client = active_client;
    active_client = NULL;
    if (client) http_client_release(client);
}

static char *http_convert_text(const char *text, UINT from, UINT to)
{
    wchar_t *wide;
    char *result;
    int count, bytes;
    if (!text) return NULL;
    count = MultiByteToWideChar(from, 0, text, -1, NULL, 0);
    if (!count || (size_t)count > SIZE_MAX / sizeof *wide) return NULL;
    wide = malloc((size_t)count * sizeof *wide);
    if (!wide) return NULL;
    if (MultiByteToWideChar(from, 0, text, -1, wide, count) != count) {
        free(wide);
        return NULL;
    }
    bytes = WideCharToMultiByte(to, 0, wide, -1, NULL, 0, NULL, NULL);
    result = bytes ? malloc((size_t)bytes) : NULL;
    if (result && WideCharToMultiByte(to, 0, wide, -1, result, bytes, NULL, NULL) != bytes) {
        free(result);
        result = NULL;
    }
    free(wide);
    return result;
}

char *http_text_to_utf8(const char *text)
{
    return http_convert_text(text, g_codePage, CP_UTF8);
}

char *http_text_from_utf8(const char *text)
{
    return http_convert_text(text, CP_UTF8, g_codePage);
}

struct HttpPost *http_post_create(const char *url)
{
    static const char *const headers[] = {
        "Content-Type: multipart/form-data", "Expect: 100-continue", "Connection: keep-alive"
    };
    struct HttpPost *post;
    size_t length, i;
    if (!active_client || !url) return NULL;
    post = calloc(1, sizeof *post);
    if (!post) return NULL;
    post->client = active_client;
    InterlockedIncrement(&post->client->references);
    length = strlen(url);
    if (length == SIZE_MAX) goto fail;
    post->url = malloc(length + 1);
    if (!post->url) goto fail;
    memcpy(post->url, url, length + 1);
    post->easy = post->client->easy_init();
    if (!post->easy) goto fail;
    for (i = 0; i < sizeof headers / sizeof headers[0]; ++i) {
        struct curl_slist *next = post->client->slist_append(post->headers, headers[i]);
        if (!next) goto fail;
        post->headers = next;
    }
    return post;
fail:
    http_post_free(post);
    return NULL;
}

void http_post_free(struct HttpPost *post)
{
    if (!post) return;
    if (post->easy) post->client->easy_cleanup(post->easy);
    if (post->form) post->client->formfree(post->form);
    if (post->headers) post->client->slist_free_all(post->headers);
    free(post->body);
    free(post->url);
    http_client_release(post->client);
    free(post);
}

static int http_add_field(struct HttpPost *post, const char *name, const char *value, CURLformoption option)
{
    CURLFORMcode code;
    if (!post) return CURLE_FAILED_INIT;
    if (post->error) return post->error;
    if (!name || !value) return post->error = CURLE_BAD_FUNCTION_ARGUMENT;
    code = post->client->formadd(&post->form, &post->last,
        CURLFORM_COPYNAME, name, option, value, CURLFORM_END);
    if (code != CURL_FORMADD_OK)
        post->error = code == CURL_FORMADD_MEMORY ? CURLE_OUT_OF_MEMORY : CURLE_HTTP_POST_ERROR;
    return post->error;
}

int http_post_add_text(struct HttpPost *post, const char *name, const char *value)
{
    return http_add_field(post, name, value, CURLFORM_COPYCONTENTS);
}

int http_post_add_file(struct HttpPost *post, const char *name, const char *path)
{
    return http_add_field(post, name, path, CURLFORM_FILE);
}

static size_t http_receive_body(char *data, size_t size, size_t count, void *context)
{
    struct HttpPost *post = context;
    size_t bytes, needed, capacity;
    char *body;
    if (!post || (size && count > SIZE_MAX / size)) return 0;
    bytes = size * count;
    if (!bytes) return 0;
    if (!data || post->size == SIZE_MAX || bytes > SIZE_MAX - post->size - 1) return 0;
    needed = post->size + bytes + 1;
    if (needed > post->capacity) {
        capacity = post->capacity ? post->capacity : 1024;
        while (capacity < needed) {
            if (capacity > SIZE_MAX / 2) { capacity = needed; break; }
            capacity *= 2;
        }
        body = realloc(post->body, capacity);
        if (!body) return 0;
        post->body = body;
        post->capacity = capacity;
    }
    memcpy(post->body + post->size, data, bytes);
    post->size += bytes;
    post->body[post->size] = '\0';
    return bytes;
}

int http_post_perform(struct HttpPost *post)
{
    struct HttpClient *client;
    CURLcode code;
    curl_write_callback write_body = http_receive_body;
    if (!post) return CURLE_FAILED_INIT;
    if (post->error) return post->error;
    client = post->client;
    post->size = 0;
    if (post->body) post->body[0] = '\0';
    code = client->easy_setopt(post->easy, CURLOPT_URL, post->url);
    if (code == CURLE_OK) code = client->easy_setopt(post->easy, CURLOPT_HTTPHEADER, post->headers);
    if (code == CURLE_OK) code = client->easy_setopt(post->easy, CURLOPT_POST, 1L);
    if (code == CURLE_OK) code = client->easy_setopt(post->easy, CURLOPT_HTTPPOST, post->form);
    if (code == CURLE_OK) code = client->easy_setopt(post->easy, CURLOPT_WRITEFUNCTION, write_body);
    if (code == CURLE_OK) code = client->easy_setopt(post->easy, CURLOPT_WRITEDATA, post);
    if (code == CURLE_OK) code = client->easy_setopt(post->easy, CURLOPT_TIMEOUT, 3L);
    if (code == CURLE_OK) code = client->easy_setopt(post->easy, CURLOPT_CONNECTTIMEOUT, 3L);
    return code == CURLE_OK ? client->easy_perform(post->easy) : code;
}

long http_post_status(const struct HttpPost *post)
{
    long status = 0;
    if (!post || post->client->easy_getinfo(post->easy, CURLINFO_RESPONSE_CODE, &status) != CURLE_OK)
        return 0;
    return status;
}

const char *http_post_body(const struct HttpPost *post, size_t *size)
{
    if (size) *size = post ? post->size : 0;
    return post && post->body ? post->body : "";
}
