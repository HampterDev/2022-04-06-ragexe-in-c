#ifndef CORE_WINDOW_EVENTS_H
#define CORE_WINDOW_EVENTS_H

void window_events_language_changed(void);
void window_events_activate(int active);
void window_events_close(void);
void window_events_key_down(int vk, int lparam);
void window_events_key_up(int vk, int lparam);
void window_events_char(int ch);

#endif
