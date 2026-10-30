#ifndef ASYNCWORK_HTTP_POST_H
#define ASYNCWORK_HTTP_POST_H

#include <stddef.h>

struct HttpPost;

int http_client_start(void);
void http_client_stop(void);

struct HttpPost *http_post_create(const char *url);
void http_post_free(struct HttpPost *post);
int http_post_add_text(struct HttpPost *post, const char *name, const char *value);
int http_post_add_file(struct HttpPost *post, const char *name, const char *path);
int http_post_perform(struct HttpPost *post);
long http_post_status(const struct HttpPost *post);
const char *http_post_body(const struct HttpPost *post, size_t *size);

char *http_text_to_utf8(const char *text);
char *http_text_from_utf8(const char *text);

#endif
