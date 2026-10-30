#ifndef SESSION_SERVICE_ADDRESS_H
#define SESSION_SERVICE_ADDRESS_H

#include "core/owned_text.h"
struct Session;
int session_assist_addr(struct Session *self, struct OwnedText *out);
int session_service_url(struct Session *self, struct OwnedText *out, const char *path);
int session_service_urls(struct Session *self,
    struct OwnedText *first, const char *first_path,
    struct OwnedText *second, const char *second_path);

#endif
