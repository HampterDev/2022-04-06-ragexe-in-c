#ifndef UI_SOCIAL_GUILD_WINDOWS_H
#define UI_SOCIAL_GUILD_WINDOWS_H

#include "ui/social/guild_wnd.h"
#include "ui/controls/agit.h"

#define GUILD_MEMBER_ROWS 6
#define GUILD_POSITION_ROWS 11

struct UIGuildInfoManageWnd {
    struct UIGuildWnd base;
    struct UIBitmapButton *emblem_button;
    struct UIBitmapButton *disband_button;
    struct List emblem_files;
    struct Vec menu;
    BYTE *emblem_data;
    unsigned emblem_size;
    int guild_id;
    int ally_selected;
    int enemy_selected;
    struct UIListBox *agit_list;
    struct UIListBoxAgitMenu *agit_menu;
};

struct UIGuildInfoManageWnd *ui_guild_info_manage_wnd_construct(struct UIGuildInfoManageWnd *self);
void ui_guild_info_manage_wnd_destruct(struct UIGuildInfoManageWnd *self);
void ui_guild_info_manage_wnd_destroy(struct UIWindow *window);
void ui_guild_info_manage_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_guild_info_manage_wnd_on_process(struct UIWindow *self);
void ui_guild_info_manage_wnd_on_draw(struct UIWindow *self);
void ui_guild_info_manage_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_guild_info_manage_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
void ui_guild_info_manage_wnd_on_r_btn_up(struct UIWindow *self, int x, int y);
intptr_t ui_guild_info_manage_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);

struct UIGuildMemberManageWnd {
    struct UIGuildWnd base;
    struct UIWindow *col_position[GUILD_MEMBER_ROWS];
    struct UIWindow *col_name[GUILD_MEMBER_ROWS];
    struct UIWindow *col_2[GUILD_MEMBER_ROWS];
    struct UIWindow *col_3[GUILD_MEMBER_ROWS];
    struct UIBitmapButton *pos_buttons[GUILD_MEMBER_ROWS];
    struct UIScrollBar *bar;
    struct UIBitmapButton *ok_button;
    struct Vec menu;
    int top;
    int selected;
    int per_row;
    int rows;
    int menu_row;
    int scrollable;
    int row_gap;
    struct List members;
    struct Tree changes;
    int sort_column;
    struct Tree sort_dirs;
    struct UICheckBox *sort_check;
    DWORD ok_time;
};

struct UIGuildMemberManageWnd *ui_guild_member_manage_wnd_construct(struct UIGuildMemberManageWnd *self);
void ui_guild_member_manage_wnd_destruct(struct UIGuildMemberManageWnd *self);
void ui_guild_member_manage_wnd_destroy(struct UIWindow *window);
void ui_guild_member_manage_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_guild_member_manage_wnd_on_process(struct UIWindow *self);
void ui_guild_member_manage_wnd_on_draw(struct UIWindow *self);
void ui_guild_member_manage_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_guild_member_manage_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
void ui_guild_member_manage_wnd_on_r_btn_up(struct UIWindow *self, int x, int y);
void ui_guild_member_manage_wnd_on_wheel(struct UIWindow *self, int delta);
intptr_t ui_guild_member_manage_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);

struct UIGuildPositionManageWnd {
    struct UIGuildWnd base;
    struct UICheckBox *invite_checks[GUILD_POSITION_ROWS];
    struct UICheckBox *expel_checks[GUILD_POSITION_ROWS];
    struct UICheckBox *storage_checks[GUILD_POSITION_ROWS];
    struct UIWindow *tax_edits[GUILD_POSITION_ROWS];
    struct UIWindow *name_edits[GUILD_POSITION_ROWS];
    struct UIWindow *tax_texts[GUILD_POSITION_ROWS];
    struct UIWindow *name_texts[GUILD_POSITION_ROWS];
    struct UIScrollBar *bar;
    struct UIBitmapButton *ok_button;
    int top;
    int selected;
    int per_row;
    int rows;
    int scrollable;
    struct List positions;
    struct Tree changes;
    DWORD ok_time;
};

struct UIGuildPositionManageWnd *ui_guild_position_manage_wnd_construct(struct UIGuildPositionManageWnd *self);
void ui_guild_position_manage_wnd_destruct(struct UIGuildPositionManageWnd *self);
void ui_guild_position_manage_wnd_destroy(struct UIWindow *window);
void ui_guild_position_manage_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_guild_position_manage_wnd_on_process(struct UIWindow *self);
void ui_guild_position_manage_wnd_on_draw(struct UIWindow *self);
void ui_guild_position_manage_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_guild_position_manage_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
void ui_guild_position_manage_wnd_on_wheel(struct UIWindow *self, int delta);
intptr_t ui_guild_position_manage_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
void ui_guild_position_manage_wnd_invalidate(struct UIWindow *self);

#endif
