#ifndef UI_CHAT_REPORTER_CONTROLS_H
#define UI_CHAT_REPORTER_CONTROLS_H

#include "std/vec.h"

struct UIBitmapButton;
struct UIWindow;

struct ReporterRow {
    struct UIBitmapButton *remove;
    struct UIWindow *name;
};

#endif
