#ifndef UI_CHARACTER_JOB_ICON_BUTTON_H
#define UI_CHARACTER_JOB_ICON_BUTTON_H

#include "ui/controls/buttons.h"

struct UIJobIconButton {
    struct UIBitmapButton bmp;
    int draw_mode;
};

struct UIJobIconButton *ui_job_icon_button_construct(struct UIJobIconButton *self);

#endif
