#include "ui/window_access.h"
#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "skills/skill_info.h"
#include "ui/window_messages.h"

#if WINDOW_WRITER_OWNER == 1
void std_vector_bytes_resize_fill(struct Vec *arg0, size_t arg1, const char *arg3)
{
    abort();
}
int crt_sprintf(char *arg0, const char *arg1, ...) { abort(); }
void ui_scroll_bar_resize(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
int ui_tab_send_msg(struct UIWindow *arg0, struct UIWindow *arg1, int arg2, int arg3, int arg4, int arg5, int arg6) { abort(); }
struct Renderer *g_renderer;
struct Session g_session1;
int crt_snprintf_s(char *arg0, size_t arg1, size_t arg2, const char *arg3, ...) { abort(); }
const char *thread_text_format(const char *arg0, ...) { abort(); }
void ui_menu_entry_vec_free(struct Vec *arg0) { abort(); }
struct UIWindow *ui_window_construct(struct UIWindow *arg0, int arg1) { abort(); }
void ui_window_create(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_window_add_child(struct UIWindow *arg0, struct UIWindow *arg1) { abort(); }
void ui_window_invalidate_children(struct UIWindow *arg0) { abort(); }
void ui_window_destruct(struct UIWindow *arg0) { abort(); }
void ui_window_resize_base(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_window_move_base(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_window_drag_and_drop_base(struct UIWindow *arg0, int arg1, int arg2, const struct DragInfo *arg3) { abort(); }
void ui_window_on_mouse_shape_base(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
int ui_window_send_msg_base(struct UIWindow *arg0, struct UIWindow *arg1, int arg2, int arg3, int arg4, int arg5,
                            int arg6)
{
    abort();
}
DWORD ui_window_get_color(struct UIWindow *arg0) { abort(); }
struct UIWindow *ui_window_hit_test_base(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_window_render_overlay_base(struct UIWindow *arg0) { abort(); }
void ui_window_to_screen(struct UIWindow *arg0, int *arg1, int *arg2) { abort(); }
struct UIFrameWnd *ui_frame_wnd_construct(struct UIFrameWnd *arg0, int arg1) { abort(); }
void ui_frame_wnd_destruct(struct UIFrameWnd *arg0) { abort(); }
int ui_frame_wnd_send_msg(struct UIWindow *arg0, struct UIWindow *arg1, int arg2, int arg3, int arg4, int arg5,
                          int arg6)
{
    abort();
}
void ui_frame_wnd_move(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_frame_wnd_save_pos(struct UIWindow *arg0) { abort(); }
void ui_frame_wnd_move_delta(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_frame_wnd_on_l_btn_down(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_frame_wnd_on_mouse_move(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_frame_wnd_on_mouse_hover(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_frame_wnd_on_l_btn_up(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_frame_wnd_snap(struct UIWindow *arg0) { abort(); }
void ui_window_record_command(struct UIWindow *arg0, int arg1, int arg2, int arg3, int arg4) { abort(); }
int g_replayPlaying;
void ui_window_clear_canvas(struct UIWindow *arg0, DWORD arg1) { abort(); }
BYTE ui_window_render_base(struct UIWindow *arg0, int arg1) { abort(); }
struct TexDrawPrim *ui_window_make_quad_base(struct UIWindow *arg0) { abort(); }
struct WordFilter g_nameFilter;
int word_filter_contains(const struct WordFilter *arg0, const char *arg1) { abort(); }
void word_filter_censor(const struct WordFilter *arg0, char *arg1) { abort(); }
#ifdef WINDOW_WRITER_ORACLE
const void *ui_chat_room_wnd_ops[48];
#else
const struct UIWindowOps ui_chat_room_wnd_ops;
#endif
#ifdef WINDOW_WRITER_ORACLE
const void *ui_edit_ctrl_wheel_ops[51];
#else
const struct UIEditCtrlOps ui_edit_ctrl_wheel_ops;
#endif
struct Mode *game_current_base_mode(void) { abort(); }
struct DragInfo *game_current_drag(void) { abort(); }
struct UIWindow *ui_chat_window_edit(struct UIWindow *arg0) { abort(); }
struct String *game_current_trade_partner(void) { abort(); }
void game_mark_npc_dialog_open(void) { abort(); }
int game_send_message(int arg0, int arg1, int arg2, int arg3, int arg4) { abort(); }
struct UIWindowMgr g_windowMgr;
void ui_window_fill_rect(struct UIWindow *arg0, int arg1, int arg2, int arg3, int arg4, COLORREF arg5)
{
    abort();
}
void ui_window_fill_round_rect(struct UIWindow *arg0, int arg1, int arg2, int arg3, int arg4, COLORREF arg5)
{
    abort();
}
void ui_window_draw_round_frame(struct UIWindow *arg0, int arg1, int arg2, int arg3, int arg4, COLORREF arg5)
{
    abort();
}
void ui_window_draw_bitmap(struct UIWindow *arg0, int arg1, int arg2, struct BitmapRes *arg3, int arg4)
{
    abort();
}
int ui_window_draw_title_bar(struct UIWindow *arg0, int arg1, const char *arg2, int arg3) { abort(); }
void ui_window_draw_body(struct UIWindow *arg0, int arg1, int arg2, const char *arg3) { abort(); }
int ui_window_draw_button_bar(struct UIWindow *arg0, int arg1, int arg2, int arg3) { abort(); }
void ui_window_text_out(struct UIWindow *arg0, int arg1, int arg2, const char *arg3, int arg4, int arg5,
                      int arg6, COLORREF arg7, int arg8, int arg9)
{
    abort();
}
void ui_window_text_out_centred(struct UIWindow *arg0, int arg1, int arg2, const char *arg3, int arg4, int arg5,
                             int arg6, COLORREF arg7, int arg8)
{
    abort();
}
void ui_window_text_out_right(struct UIWindow *arg0, int arg1, int arg2, const char *arg3, int arg4, int arg5,
                           int arg6, COLORREF arg7, int arg8)
{
    abort();
}
void ui_window_text_out_outlined(struct UIWindow *arg0, int arg1, int arg2, const char *arg3, int arg4,
                              COLORREF arg5, COLORREF arg6, int arg7, int arg8, int arg9, int arg10)
{
    abort();
}
void ui_window_draw_shadow_text(struct UIWindow *arg0, int arg1, int arg2, const char *arg3, int arg4,
                             COLORREF arg5, COLORREF arg6, int arg7, int arg8, int arg9)
{
    abort();
}
int ui_window_text_width(struct UIWindow *arg0, const char *arg1, int arg2, int arg3, int arg4, int arg5,
                         int arg6)
{
    abort();
}
char *format_money(int arg0, char *arg1) { abort(); }
char *format_money64(long long arg0, char *arg1, int arg2) { abort(); }
struct AmountColours *ui_window_amount_colours(struct UIWindow *arg0, struct AmountColours *arg1,
                                             long long arg2)
{
    abort();
}
void ui_window_text_out_price(struct UIWindow *arg0, int arg1, struct String arg2, int arg3, int arg4)
{
    abort();
}
struct UIWindow *ui_window_add_close_button(struct UIWindow *arg0, const char *arg1, int arg2) { abort(); }
struct ResMgr *g_resMgr(void) { abort(); }
void *res_mgr_get(struct ResMgr *arg0, const char *arg1) { abort(); }
struct SkinMgr g_skinMgr;
void ui_skin_color_chip(struct SkinMgr *arg0, int arg1, int arg2, int *arg3, int *arg4, int *arg5) { abort(); }
const char *ui_skin_path(const char *arg0) { abort(); }
int g_serviceType;
struct UIBitmapButton *ui_bitmap_button_construct(struct UIBitmapButton *arg0) { abort(); }
BYTE ui_bitmap_button_set_bitmap(struct UIBitmapButton *arg0, const char *arg1, int arg2, int arg3) { abort(); }
void ui_bitmap_button_set_tool_tip(struct UIBitmapButton *arg0, const char *arg1) { abort(); }
struct UIBasicButton *ui_basic_button_construct(struct UIBasicButton *arg0) { abort(); }
BYTE ui_basic_button_set_label(struct UIWindow *arg0, int arg1, const char *arg2) { abort(); }
struct UICheckBox *ui_check_box_construct(struct UIWindow *arg0) { abort(); }
void ui_check_box_set_bitmaps(struct UIWindow *arg0, const char *arg1, const char *arg2, const char *arg3)
{
    abort();
}
void ui_check_box_set_check(struct UIWindow *arg0, int arg1) { abort(); }
void ui_check_box_set_tool_tip(struct UIWindow *arg0, const char *arg1) { abort(); }
void ui_check_box_fit_to_label(struct UICheckBox *arg0) { abort(); }
void ui_check_box_set_bitmaps_fit(struct UICheckBox *arg0, const char *arg1, const char *arg2) { abort(); }
struct UIWindow *ui_static_text_construct(struct UIWindow *arg0) { abort(); }
void ui_static_text_set_text(struct UIWindow *arg0, const char *arg1, int arg2) { abort(); }
void ui_static_text_set_text_ellipsis(struct UIWindow *arg0, const char *arg1) { abort(); }
void ui_static_text_set_text_fit(struct UIWindow *arg0, const char *arg1) { abort(); }
struct UIWindow *ui_edit_ctrl_construct(struct UIWindow *arg0) { abort(); }
int ui_edit_ctrl_send_msg(struct UIWindow *arg0, struct UIWindow *arg1, int arg2, int arg3, int arg4, int arg5,
                          int arg6)
{
    abort();
}
void ui_edit_ctrl_on_focus(struct UIWindow *arg0) { abort(); }
void ui_edit_ctrl_on_focus_lost(struct UIWindow *arg0) { abort(); }
#ifdef WINDOW_WRITER_ORACLE
void UIEditCtrl_SetTextSlot(struct UIEditCtrl *arg0, const char *arg1) { abort(); }
#else
void ui_edit_ctrl_set_text_default(struct UIEditCtrl *arg0, const char *arg1) { abort(); }
#endif
const char *ui_edit_ctrl_get_text(struct UIWindow *arg0) { abort(); }
int ui_edit_ctrl_get_text_size(struct UIWindow *arg0) { abort(); }
void ui_edit_ctrl_select_all(struct UIWindow *arg0) { abort(); }
void ui_edit_ctrl_set_caret(struct UIWindow *arg0, int arg1) { abort(); }
void ui_edit_ctrl_set_back_colour(struct UIWindow *arg0, int arg1, int arg2, int arg3) { abort(); }
void ui_edit_ctrl_set_text_colour(struct UIWindow *arg0, int arg1, int arg2, int arg3, int arg4) { abort(); }
void ui_window_text_out_doubled(struct UIWindow *arg0, int arg1, int arg2, const char *arg3, COLORREF arg4,
                             COLORREF arg5, int arg6, int arg7, int arg8)
{
    abort();
}
void ui_edit_ctrl_on_l_btn_down(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_edit_ctrl_on_mouse_hover(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
#ifdef WINDOW_WRITER_ORACLE
void ui_edit_ctrl_draw_text(struct UIEditCtrl *arg0) { abort(); }
#else
void ui_edit_ctrl_draw_text_default(struct UIEditCtrl *arg0) { abort(); }
#endif
#ifdef WINDOW_WRITER_ORACLE
void ui_edit_ctrl_truncate(struct UIEditCtrl *arg0) { abort(); }
#else
void ui_edit_ctrl_truncate_default(struct UIEditCtrl *arg0) { abort(); }
#endif
void ui_edit_ctrl_make_select(struct UIWindow *arg0) { abort(); }
void ui_bitmap_edit_ctrl_destroy(struct UIWindow *window) { abort(); }
void ui_bitmap_edit_ctrl_set_bitmap(struct UIBitmapEditCtrl *arg0, const char *arg1) { abort(); }
void ui_bitmap_edit_ctrl_on_draw(struct UIWindow *arg0) { abort(); }
struct UIScrollBar *ui_scroll_bar_construct(struct UIScrollBar *arg0) { abort(); }
void ui_scroll_bar_create(struct UIScrollBar *arg0, int arg1, int arg2, int arg3, int arg4) { abort(); }
void ui_scroll_bar_set_range(struct UIScrollBar *arg0, int arg1, int arg2, int arg3) { abort(); }
void ui_scroll_bar_set_pos(struct UIScrollBar *arg0, int arg1) { abort(); }
struct UIListBox *ui_list_box_construct(struct UIListBox *arg0) { abort(); }
int ui_list_box_count(struct UIListBox *arg0) { abort(); }
struct UITab *ui_tab_construct(struct UITab *arg0) { abort(); }
struct UIWindow *ui_text_viewer_construct(struct UIWindow *arg0) { abort(); }
void ui_combo_box_def2_wnd_add_item(struct UIWindow *arg0, struct String arg1, int arg2) { abort(); }
void ui_combo_box_def_wnd_select(struct UIWindow *arg0, int arg1) { abort(); }
#ifdef WINDOW_WRITER_ORACLE
struct UIWindow *ui_window_mgr_make_window(struct UIWindowMgr *arg0, int arg1) { abort(); }
#endif
struct UIWindow *ui_window_find(int arg0) { abort(); }
struct UIWindow *ui_window_open(int arg0) { abort(); }
int ui_window_send(struct UIWindow *arg0, struct UIWindow *arg1, int arg2, int arg3, int arg4, int arg5,
                   int arg6)
{
    abort();
}
void ui_window_show(struct UIWindow *arg0, int arg1) { abort(); }
BYTE ui_window_mgr_post_quit1(struct UIWindowMgr *arg0, int arg1) { abort(); }
BYTE ui_window_mgr_show_option_window(struct UIWindowMgr *arg0, int arg1, int arg2) { abort(); }
int ui_window_mgr_send_msg_(struct UIWindowMgr *arg0, int arg1, int arg2, int arg3, int arg4, int arg5)
{
    abort();
}
void ui_window_mgr_set_focus_edit(struct UIWindowMgr *arg0, struct UIWindow *arg1) { abort(); }
struct UIWindow *ui_window_mgr_get_focus_edit(struct UIWindowMgr *arg0) { abort(); }
int ui_window_mgr_message_box(struct UIWindowMgr *arg0, const char *arg1, int arg2, int arg3, int arg4, int arg5,
                           const char *arg6, int arg7, int arg8, int arg9)
{
    abort();
}
void ui_window_mgr_update_snap_rect(struct UIWindowMgr *arg0, struct UIWindow *arg1) { abort(); }
struct ReplayRecorder *replay_recorder_instance(void) { abort(); }
BYTE replay_recorder_write_packet(struct ReplayRecorder *arg0, unsigned short arg1, const void *arg2, int arg3,
                                DWORD arg4)
{
    abort();
}
struct ModeMgr g_modeMgr;
struct Mode *client_current_mode(void) { abort(); }
int mode_send_message(struct Mode *arg0, int arg1, int arg2, int arg3, int arg4, int arg5) { abort(); }
void mode_set_cursor_action(struct Mode *arg0, int arg1) { abort(); }
void mode_show_tooltip(struct Mode *arg0, const char *arg1, int arg2, int arg3, DWORD arg4, BYTE arg5)
{
    abort();
}
struct String *session_scramble_number(struct Session *arg0, struct String *arg1, unsigned arg2)
{
    abort();
}
struct SkillInfo *skill_info_construct(struct SkillInfo *arg0) { abort(); }
void skill_info_destruct(struct SkillInfo *arg0) { abort(); }
const char *skill_info_get_id_name(struct SkillInfo *arg0) { abort(); }
const char *skill_name(int arg0) { abort(); }
void session_format_item_bitmap_path(const char *arg1, char *arg2) { abort(); }
void session_clear_lists(struct Session *arg0) { abort(); }
char game_mode_refuse_held_item_drag(struct GameMode *arg0) { abort(); }
BYTE session_is_number_text(struct Session *arg0, char *arg1) { abort(); }
char g_clientDir[0x88];
int ui_chat_his_box_save_to_file(struct UIChatHisBox *arg0, const char *arg1) { abort(); }
void tree_free(struct Tree *arg0, void (*arg1)(void *)) { abort(); }
void chat_room_member_destroy(void *arg0) { abort(); }
struct ItemInfo *item_info_construct(struct ItemInfo *arg0) { abort(); }
struct ItemInfo *item_info_construct_with_arg(struct ItemInfo *arg0) { abort(); }
struct ItemInfo *item_info_assign(struct ItemInfo *arg0, const struct ItemInfo *arg1) { abort(); }
struct ItemInfo *item_info_copy(struct ItemInfo *arg0, const struct ItemInfo *arg1) { abort(); }
void item_info_set_item_id(struct ItemInfo *arg0, int arg1) { abort(); }
int item_info_get_item_id(struct ItemInfo *arg0) { abort(); }
struct String *item_info_get_display_name(const struct ItemInfo *arg0, struct String *arg1) { abort(); }
void item_info_destroy(void *item) { abort(); }
struct ItemInfo *session_get_inventory_item(struct Session *arg0, struct ItemInfo *arg1, int arg2)
{
    abort();
}
void session_remove_inventory_item(struct Session *arg0, int arg1, int arg2) { abort(); }
void session_add_inventory_item(struct Session *arg0, const struct ItemInfo *arg1, int arg2) { abort(); }
COLORREF session_get_item_dis_color(const struct ItemInfo *arg1) { abort(); }
int session_purchase_count(struct Session *arg0) { abort(); }
struct ItemInfo *session_purchase_item(struct Session *arg0, struct ItemInfo *arg1, int arg2) { abort(); }
void session_clear_purchase_lists(struct Session *arg0) { abort(); }
void ui_window_draw_item_win_mid(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_window_draw_item_icon(struct UIWindow *arg0, int arg1, int arg2, const struct ItemInfo *arg3, BYTE arg4)
{
    abort();
}
void ui_window_draw_item_icon_count(struct UIWindow *arg0, int arg1, int arg2, const struct ItemInfo *arg3,
                                BYTE arg4)
{
    abort();
}
int ui_window_draw_item_name(struct UIWindow *arg0, int arg1, int arg2, const struct ItemInfo *arg3,
                          COLORREF arg4, int arg7)
{
    abort();
}
int ui_window_draw_item_name_short(struct UIWindow *arg0, int arg1, int arg2, int arg3,
                               const struct ItemInfo *arg4, int arg7)
{
    abort();
}
const char *item_info_get_name(const struct ItemInfo *arg0) { abort(); }
void ui_chat_edit_ctrl_add_item_link(struct UIWindow *arg0, const struct ItemInfo *arg1) { abort(); }
void ui_rodex_compose_wnd_add_item_link(struct UIWindow *arg0, const struct ItemInfo *arg1) { abort(); }
struct ItemInfo *session_get_store_setup_item_at(struct Session *arg0, struct ItemInfo *arg1, int arg2) { abort(); }
struct ItemInfo *session_get_store_setup_item_by_index(struct Session *arg0, struct ItemInfo *arg1, int arg2)
{
    abort();
}
void session_clear_store_setup(struct Session *arg0) { abort(); }
int session_get_store_setup_count(struct Session *arg0, int arg1) { abort(); }
BYTE session_mirror_remove(struct Session *arg0, const struct ItemInfo *arg1, int arg2, BYTE arg3)
{
    abort();
}
int zeny_fee_percent(long long arg0) { abort(); }
int session_get_shop_sell_list_size(struct Session *arg0) { abort(); }
struct ItemInfo *session_get_shop_sell_item_at(struct Session *arg0, struct ItemInfo *arg1, int arg2) { abort(); }
void session_add_shop_sell_item(struct Session *arg0, const struct ItemInfo *arg1) { abort(); }
BYTE session_has_shop_goods(struct Session *arg0, int arg1) { abort(); }
void session_remove_shop_goods(struct Session *arg0, const struct ItemInfo *arg1) { abort(); }
int session_get_shop_buy_total(struct Session *arg0) { abort(); }
int session_get_shop_buy_list_size(struct Session *arg0) { abort(); }
struct ItemInfo *session_get_shop_buy_item_at(struct Session *arg0, struct ItemInfo *arg1, int arg2) { abort(); }
int session_get_shop_buy_count_by_name(struct Session *arg0, const char *arg1) { abort(); }
void session_add_shop_buy_item(struct Session *arg0, const struct ItemInfo *arg1) { abort(); }
int session_has_shop_sell_items(struct Session *arg0) { abort(); }
void ui_window_invalidate_base(struct UIWindow *arg0) { abort(); }
#endif

#if WINDOW_WRITER_OWNER == 4
const struct ItemTextLines *metal_process_get(int arg0) { abort(); }
void item_info_clear(struct ItemInfo *arg0) { abort(); }
int session_is_pet_egg_id(struct Session *arg0, int arg1) { abort(); }
#ifdef WINDOW_WRITER_ORACLE
void game_mode_item_identify_list(struct Mode *arg0, const char *arg1) { abort(); }
void game_mode_item_composition_list(struct Mode *arg0, const char *arg1) { abort(); }
#else
void game_mode_item_identify_list(struct GameMode *arg0, const unsigned char *arg1) { abort(); }
void game_mode_item_composition_list(struct GameMode *arg0, const unsigned char *arg1) { abort(); }
#endif
#endif

#if WINDOW_WRITER_OWNER == 6
void ui_window_mgr_refresh_item_windows(struct UIWindowMgr *arg0) { abort(); }
unsigned char item_info_is_element_stone_or_star_crumb(const struct ItemInfo *arg0) { abort(); }
unsigned char item_info_is_element_stone(const struct ItemInfo *arg0) { abort(); }
#endif

#if WINDOW_WRITER_OWNER == 8
void megaphone_wnd_add_item_link(struct UIWindow *arg0, const struct ItemInfo *arg1) { abort(); }
void game_mode_drag_item(struct GameMode *arg0, int arg1, const struct ItemInfo *arg2, int arg3, int arg4)
{
    abort();
}
struct UIItemBaseWnd *ui_item_base_wnd_construct(struct UIItemBaseWnd *arg0) { abort(); }
void ui_item_base_wnd_destruct(struct UIItemBaseWnd *arg0) { abort(); }
int ui_item_base_wnd_send_msg(struct UIWindow *arg0, struct UIWindow *arg1, int arg2, int arg3, int arg4,
                          int arg5, int arg6)
{
    abort();
}
void ui_item_base_wnd_update_scroll(struct UIItemBaseWnd *arg0) { abort(); }
void ui_item_base_wnd_on_draw(struct UIWindow *arg0) { abort(); }
struct ItemInfo *item_info_assign_from_drag(struct ItemInfo *arg0, const struct DragInfo *arg1)
{
    abort();
}
struct String *game_mode_get_actor_name(struct GameMode *arg0, struct String *arg1, int arg2) { abort(); }
void ui_window_mgr_add_to_name_waiting_list(struct UIWindowMgr *arg0, struct UIWindow *arg1) { abort(); }
int ui_item_base_wnd_index_at(struct UIItemBaseWnd *arg0, int arg1, int arg2) { abort(); }
struct ItemInfo *ui_item_base_wnd_get_item(struct UIItemBaseWnd *arg0, struct ItemInfo *arg1, int arg2)
{
    abort();
}
struct ItemInfo *ui_item_base_wnd_item_at(struct UIItemBaseWnd *arg0, struct ItemInfo *arg1, int arg2, int arg3)
{
    abort();
}
struct UIBitmapButton *shop_add_button(struct UIWindow *arg0, const char *const arg1[3], int arg2, int arg3,
                                       int arg4)
{
    abort();
}
const char *const g_shopBuyButton[3];
const char *const g_shopCancelButton[3];
void shop_draw_frame(struct UIItemBaseWnd *arg0, int arg1, int arg2) { abort(); }
void shop_draw_cells(struct UIWindow *arg0, int arg1) { abort(); }
void shop_fill_names(struct UIWindow *arg0, int arg1) { abort(); }
struct UIBitmapButton *shop_place_button(struct UIWindow *arg0, struct UIBitmapButton *arg1,
                                         const char *const arg2[3], int arg3, int arg4, int arg5)
{
    abort();
}
int shop_info_usable(struct WindowInfo *arg0) { abort(); }
void shop_info_default(struct WindowInfo *arg0) { abort(); }
struct UIWindow *shop_add_title(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void shop_add_resizer(struct UIItemBaseWnd *arg0, int arg1) { abort(); }
void shop_add_scroll_bar(struct UIItemBaseWnd *arg0, int arg1) { abort(); }
void shop_tooltip_cell(struct UIWindow *arg0, int arg1, int arg2, int *arg3, int *arg4) { abort(); }
struct ItemInfo *session_cart_item_by_index(struct Session *arg0, struct ItemInfo *arg1, int arg2) { abort(); }
struct ItemInfo *session_mirror_item_by_index(struct Session *arg0, struct ItemInfo *arg1, int arg2)
{
    abort();
}
int session_mirror_count(struct Session *arg0) { abort(); }
struct ItemInfo *session_mirror_item_at(struct Session *arg0, struct ItemInfo *arg1, int arg2) { abort(); }
void session_mirror_from_cart(struct Session *arg0) { abort(); }
void session_fill_buying_store_list(struct Session *arg0) { abort(); }
void session_fill_buying_store_list_priced(struct Session *arg0) { abort(); }
BYTE session_mirror_add(struct Session *arg0, const struct ItemInfo *arg1, int arg2, BYTE arg3) { abort(); }
BYTE session_remove_store_setup_item(struct Session *arg0, const struct ItemInfo *arg1, BYTE arg2) { abort(); }
void session_remove_store_purchase_item(struct Session *arg0, const struct ItemInfo *arg1) { abort(); }
void session_clear_purchase_list(struct Session *arg0) { abort(); }
int session_get_store_purchase_count(struct Session *arg0, int arg1) { abort(); }
struct ItemInfo *session_purchase_item_by_id(struct Session *arg0, struct ItemInfo *arg1, const char *arg2)
{
    abort();
}
void session_purchase_add(struct Session *arg0, const struct ItemInfo *arg1) { abort(); }
long long session_purchase_total(struct Session *arg0) { abort(); }
int session_shop_list_count(struct Session *arg0) { abort(); }
struct ItemInfo *session_shop_list_item(struct Session *arg0, struct ItemInfo *arg1, int arg2) { abort(); }
void ui_merchant_item_purchase_msg_wnd_set_purchase(struct UIMerchantItemPurchaseMSGWnd *arg0, int arg1, int arg2) { abort(); }
void ui_merchant_item_wnd_on_r_btn_down(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_item_base_wnd_on_wheel(struct UIWindow *arg0, int arg1) { abort(); }
int ui_merchant_item_wnd_send_msg(struct UIWindow *arg0, struct UIWindow *arg1, int arg2, int arg3,
                              int arg4, int arg5, int arg6)
{
    abort();
}
void ui_merchant_mirror_item_wnd_on_r_btn_down(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
struct UIItemTagOnMegaPhone *ui_item_tag_on_mega_phone_construct(struct UIItemTagOnMegaPhone *arg0) { abort(); }
BYTE ui_item_tag_ctrl_can_add(struct UIItemTagCtrl *arg0, struct TextView arg1, int arg2) { abort(); }
struct String *ui_item_tag_ctrl_append_tag(struct UIItemTagCtrl *arg0, struct String *arg1, struct TextView arg2)
{
    abort();
}
void ui_item_tag_ctrl_add_item(struct UIItemTagCtrl *arg0, const struct ItemInfo *arg1, int arg2) { abort(); }
#endif

#if WINDOW_WRITER_OWNER == 9
void ui_merchant_item_purchase_wnd_on_r_btn_down(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
#endif

#if WINDOW_WRITER_OWNER == 10
void ui_merchant_item_shop_wnd_drag_and_drop(struct UIWindow *window, int arg1, int arg2, const struct DragInfo *arg3)
{
    abort();
}
void ui_merchant_item_shop_wnd_on_r_btn_down(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
int ui_merchant_item_shop_wnd_send_msg(struct UIWindow *arg0, struct UIWindow *arg1, int arg2,
                                  int arg3, int arg4, int arg5, int arg6)
{
    abort();
}
void ui_item_base_wnd_drag_and_drop(struct UIWindow *window, int arg1, int arg2, const struct DragInfo *arg3) { abort(); }
void ui_merchant_item_my_shop_wnd_on_r_btn_down(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
int ui_merchant_item_my_shop_wnd_send_msg(struct UIWindow *arg0, struct UIWindow *arg1, int arg2,
                                    int arg3, int arg4, int arg5, int arg6)
{
    abort();
}
#endif

#if WINDOW_WRITER_OWNER == 11
int ui_merchant_shop_make_wnd_send_msg(struct UIWindow *arg0, struct UIWindow *arg1, int arg2,
                                  int arg3, int arg4, int arg5, int arg6)
{
    abort();
}
#endif

#if WINDOW_WRITER_OWNER == 12
struct ItemInfo *session_find_purchase_item(struct Session *arg0, struct ItemInfo *arg1, const char *arg2)
{
    abort();
}
int purchase_info_usable(struct WindowInfo *arg0) { abort(); }
int sell_info_usable(struct WindowInfo *arg0) { abort(); }
void purchase_info_default(struct WindowInfo *arg0) { abort(); }
void sell_info_default(struct WindowInfo *arg0) { abort(); }
void npc_shop_price_unit_long(char *arg0, int arg1) { abort(); }
void npc_shop_show_price_tooltip(struct UIItemBaseWnd *arg0, int arg1, int arg2, const char *arg3, int arg4,
                                 int arg5)
{
    abort();
}
void shop_draw_backdrop(struct UIItemBaseWnd *arg0, int arg1, int arg2) { abort(); }
void shop_draw_lines(struct UIItemBaseWnd *arg0, int arg1, int arg2, const char *arg3) { abort(); }
void npc_shop_store_info(struct UIWindow *arg0, struct WindowInfo *arg1) { abort(); }
void npc_shop_drop_or_count(struct UIItemBaseWnd *arg0, int arg1, int arg2, int arg3) { abort(); }
void npc_shop_pass_on(struct UIItemBaseWnd *arg0, int arg1, int arg2, struct UIWindow *arg3, int arg4)
{
    abort();
}
void npc_shop_list_press(struct UIItemBaseWnd *arg0, int *arg1, int arg2, int arg3, int arg4) { abort(); }
void npc_shop_describe_hovered(struct UIItemBaseWnd *arg0, int *arg1, int arg2, int arg3) { abort(); }
void npc_shop_describe_at(struct UIItemBaseWnd *arg0, int arg1, int arg2, int arg3) { abort(); }
int npc_shop_refill(struct UIItemBaseWnd *arg0, int arg1,
                    struct ItemInfo *(*arg2)(struct Session *, struct ItemInfo *, int))
{
    abort();
}
void npc_shop_list_resize(struct UIItemBaseWnd *arg0, int arg1, int arg2, int arg3) { abort(); }
void npc_shop_list_place_buttons(struct UIItemBaseWnd *arg0) { abort(); }
void npc_shop_list_set_info(struct UIItemBaseWnd *arg0, struct WindowInfo *arg1, int arg2,
                            int (*arg3)(struct WindowInfo *), void (*arg4)(struct WindowInfo *))
{
    abort();
}
int npc_shop_records_reach(const char *arg0, unsigned int arg1, short arg2, short arg3) { abort(); }
void npc_shop_open_shop_list(int arg0, int arg1) { abort(); }
#endif

#if WINDOW_WRITER_OWNER == 14
void storage_draw_items(struct UIItemBaseWnd *arg0, int arg1, int arg2) { abort(); }
void storage_on_lbtn_down(struct UIItemBaseWnd *arg0, int arg1, int arg2, int arg3, int arg4) { abort(); }
void storage_on_mouse_hover(struct UIItemBaseWnd *arg0, int arg1, int arg2, int arg3) { abort(); }
void storage_on_r_btn_down(struct UIItemBaseWnd *arg0, int arg1, int arg2, int arg3) { abort(); }
void storage_drop(struct UIItemBaseWnd *arg0, int arg1, int arg2, const struct DragInfo *arg3) { abort(); }
void storage_add_dragged(void) { abort(); }
void storage_store_info(struct UIWindow *arg0, struct WindowInfo *arg1) { abort(); }
void storage_fill(struct UIItemBaseWnd *arg0, int arg1) { abort(); }
int storage_sort(struct UIItemBaseWnd *arg0, int arg1) { abort(); }
int storage_filter(struct UIItemBaseWnd *arg0, const struct String *arg1, int arg2) { abort(); }
void storage_sort_chosen(struct UIItemBaseWnd *arg0) { abort(); }
void storage_find_set_search(struct UIItemStoreFindWnd *arg0, struct String arg1) { abort(); }
struct UIItemStoreComboBoxWnd *ui_item_store_combo_box_wnd_construct(struct UIItemStoreComboBoxWnd *arg0) { abort(); }
#endif

#if WINDOW_WRITER_OWNER == 15
struct ItemInfo *session_get_my_trade_item(struct Session *arg0, struct ItemInfo *arg1, int arg2) { abort(); }
struct ItemInfo *session_get_partner_trade_item(struct Session *arg0, struct ItemInfo *arg1, int arg2)
{
    abort();
}
#endif
