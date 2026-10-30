#ifndef WORLD_TOOLTIP_H
#define WORLD_TOOLTIP_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

struct Mode;

void mode_show_tooltip(struct Mode *mode, const char *text, int x, int y,
    DWORD colour, BYTE flag);
void mode_hide_help_balloon(struct Mode *mode);
void mode_process_help_balloon(struct Mode *mode);

#endif
