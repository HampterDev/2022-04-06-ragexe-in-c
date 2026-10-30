#ifndef UI_CHAT_READABLE_LINKS_H
#define UI_CHAT_READABLE_LINKS_H

#include "std/string.h"
struct UIWindow;
struct ItemInfo;

void item_link_caption(struct UIWindow *window, struct String *out, struct TextView name);
struct String *ui_chat_his_box_readable_item_links(struct UIWindow *window,
                                                struct String *out, struct TextView text);
struct String *ui_new_chat_wnd_readable_links(struct UIWindow *window,
                                            struct String *out, struct TextView text);

typedef void (*ItemLinkLineCallback)(void *context, const struct ItemInfo *item,
                                    const char *name, const char *caption, const char *prefix);

int item_link_build_line(struct UIWindow *window, struct String *out,
                         struct String *rest, const char *line,
                         ItemLinkLineCallback on_item, void *context);

#endif
