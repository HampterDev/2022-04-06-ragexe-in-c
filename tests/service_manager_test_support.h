#ifndef SERVICE_MANAGER_TEST_SUPPORT_H
#define SERVICE_MANAGER_TEST_SUPPORT_H
#include <stddef.h>
#include <stdlib.h>

void *service_test_calloc(size_t count, size_t size);
void service_test_free(void *memory);
void service_test_account_state(void);
void service_test_character_state(void);
void service_test_agency_state(void);

void *service_text_malloc(size_t size);
void service_text_free(void *memory);
void service_test_media_state(void);
extern int service_urls_missing;

#endif
