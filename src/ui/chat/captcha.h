#ifndef UI_CHAT_CAPTCHA_H
#define UI_CHAT_CAPTCHA_H
#include <stddef.h>
#include "platform/win_types.h"

struct UIWindow;

struct UIWindow *captcha_preview_create(void);
struct UIWindow *captcha_detector_create(void);

int captcha_preview_begin(struct UIWindow *window, const void *key, size_t size);
void captcha_preview_append(struct UIWindow *window, const void *key, const void *chunk, size_t size);
int captcha_detector_begin(struct UIWindow *window, const void *key, size_t size);
void captcha_detector_append(struct UIWindow *window, const void *key, const void *chunk, size_t size);
void captcha_detector_set_timer(struct UIWindow *window, unsigned char count, DWORD limit_ms);
struct UIWindow *captcha_detector_edit(struct UIWindow *window);

#endif
