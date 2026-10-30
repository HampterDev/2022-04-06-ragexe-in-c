#ifndef UI_ACTOR_LABELS_H
#define UI_ACTOR_LABELS_H
#include <stddef.h>
struct UIWindow;
struct ChatRoomInfo;

const struct ChatRoomInfo *ui_chat_room_title_info(const struct UIWindow *window);
const char *ui_merchant_shop_title_text(const struct UIWindow *window, size_t *length);
#endif
