#ifndef UI_NAVIGATION_VIEW_H
#define UI_NAVIGATION_VIEW_H
#define NAVI_WINDOW_ID 0xcb
enum NavigationMapLayers {
    NAVI_MAP_GUIDED = 1,
    NAVI_MAP_RESULTS = 2,
    NAVI_MAP_ROUTE = 4
};
int ui_navigation_is_open(void);
void ui_navigation_open(void);
void ui_navigation_relayout(void);
void ui_navigation_invalidate(void);
void ui_navigation_show_search(void);
void ui_navigation_show_map(void);
void ui_navigation_refresh_map(unsigned layers);
void ui_navigation_chat(const char *text);
void ui_navigation_message_box(const char *text);
#endif
