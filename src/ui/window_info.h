#ifndef UI_WINDOW_INFO_H
#define UI_WINDOW_INFO_H

#include <windows.h>
#include "base.h"

struct WindowInfo {
    int x, y;
    int width, height;
};

struct WndInfoShow {
    struct WindowInfo rect;
    int show;
};

struct BasicInfoWndInfo {
    struct WindowInfo rect;
    union {
        int show;
        struct {
            BYTE expanded;
            BYTE show_byte1;
            BYTE tall;
        };
    };
};

struct WndInfoTall {
    struct WindowInfo rect;
    int org_height;
    int show;
};

struct StatusWndInfo {
    struct WindowInfo rect;
    int org_height;
    int show;
    int expanded;
};

struct MacroReporterInfo {
    int width, height;
    int x, y;
    BYTE open;
    BYTE pick_mode;
    struct Vec players;
};

struct ItemWndInfo {
    struct WindowInfo rect;
    int org_height;
    int cur_tab;
    int cur_radio[3];
    int show;
};

struct QuestWndInfo {
    struct WindowInfo rect;
    int org_height;
    int cur_tab;
    int show;
};

struct MessengerGroupWndInfo {
    struct WindowInfo rect;
    int show;
    int radio;
    int size_mode;
};

struct ItemStoreWndInfo {
    struct WindowInfo rect;
    int sub_wnd;
    int find_wnd;
    struct WindowInfo sub_info[8];
};

struct NavigationWndInfo {
    struct WindowInfo rect;
    int is_screen;
};

struct GuildWndInfo {
    struct WindowInfo frame;
    int tab;
};

struct MiniMapWndInfo {
    int memo_x, memo_y;
    int x, y;
    int width, height;
    int show_quest;
    int show_npc;
    int show_member;
};

#endif
