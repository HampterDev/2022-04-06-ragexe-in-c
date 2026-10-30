#include "../src/web/curl.c"

void http_verify_api_signatures(struct HttpClient *api)
{
    api->global_init = curl_global_init;
    api->global_cleanup = curl_global_cleanup;
    api->slist_append = curl_slist_append;
    api->formadd = curl_formadd;
    api->easy_init = curl_easy_init;
    api->easy_setopt = curl_easy_setopt;
    api->easy_perform = curl_easy_perform;
    api->easy_cleanup = curl_easy_cleanup;
    api->formfree = curl_formfree;
    api->slist_free_all = curl_slist_free_all;
    api->easy_getinfo = curl_easy_getinfo;
}
