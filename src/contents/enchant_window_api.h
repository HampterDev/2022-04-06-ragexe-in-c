#ifndef CONTENTS_ENCHANT_WINDOW_API_H
#define CONTENTS_ENCHANT_WINDOW_API_H

#include <stdint.h>
struct UIEnchantWnd;
void enchant_wnd_open(struct UIEnchantWnd *self, uint64_t table_id);
void enchant_wnd_result(struct UIEnchantWnd *self, int result, int item_id);

#endif
