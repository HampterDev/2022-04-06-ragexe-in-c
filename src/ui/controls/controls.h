#ifndef UI_CONTROLS_H
#define UI_CONTROLS_H

#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include <string.h>

#include "ui/controls/buttons.h"
#include "ui/controls/check_radio.h"
#include "ui/controls/static.h"
#include "ui/controls/edit.h"
#include "ui/controls/resizer.h"
#include "ui/controls/tabs.h"
#include "ui/controls/scroll.h"
#include "ui/controls/listbox.h"
#include "ui/controls/text_viewer.h"
#include "ui/controls/combo_box.h"
#include "ui/controls/popups.h"
#include "ui/controls/item_ctrls.h"

void ui_window_on_draw_base(struct UIWindow *self);

#endif
