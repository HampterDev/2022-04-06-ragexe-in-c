#if UI_DROP_OWNER == 2
void * mem_alloc(size_t arg0) { abort(); }
void mem_free(void * arg0) { abort(); }
void string_assign(struct String *arg0, const char *arg1, size_t arg2) { abort(); }
void string_copy(struct String *arg0, const struct String *arg1) { abort(); }
void string_assign_cstr(struct String *arg0, const char *arg1) { abort(); }
void string_format(struct String *arg0, const char *arg1, ...) { abort(); }
struct String string_from_cstr(const char *arg0) { abort(); }
int crt_sprintf(char * arg0, const char * arg1, ...) { abort(); }
struct Renderer * g_renderer;
void ui_window_fill_rect(struct UIWindow * arg0, int arg1, int arg2, int arg3, int arg4, COLORREF arg5) { abort(); }
void ui_window_draw_bitmap(struct UIWindow * arg0, int arg1, int arg2, struct BitmapRes * arg3, int arg4) { abort(); }
int ui_window_draw_title_bar(struct UIWindow * arg0, int arg1, const char * arg2, int arg3) { abort(); }
void ui_window_text_out_centred(struct UIWindow * arg0, int arg1, int arg2, const char * arg3, int arg4, int arg5, int arg6, COLORREF arg7, int arg8) { abort(); }
void ui_window_text_out(struct UIWindow * arg0, int arg1, int arg2, const char * arg3, int arg4, int arg5, int arg6, COLORREF arg7, int arg8, int arg9) { abort(); }
void ui_window_text_out_right(struct UIWindow * arg0, int arg1, int arg2, const char * arg3, int arg4, int arg5, int arg6, COLORREF arg7, int arg8) { abort(); }
int ui_window_text_height(struct UIWindow * arg0, const char * arg1, int arg2, int arg3, int arg4, int arg5, int arg6) { abort(); }
int ui_window_text_width(struct UIWindow * arg0, const char * arg1, int arg2, int arg3, int arg4, int arg5, int arg6) { abort(); }
struct String * ui_window_fit_text(struct UIWindow * arg0, struct String * arg1, int arg2, const char * arg3, int arg4, int arg5, int arg6) { abort(); }

const char * item_info_get_name(const struct ItemInfo * arg0) { abort(); }
struct String * item_info_get_display_name(const struct ItemInfo * arg0, struct String * arg1) { abort(); }
struct ItemInfo * item_info_construct(struct ItemInfo * arg0) { abort(); }
struct ItemInfo * item_info_copy(struct ItemInfo * arg0, const struct ItemInfo * arg1) { abort(); }
struct ItemInfo * item_info_assign(struct ItemInfo * arg0, const struct ItemInfo * arg1) { abort(); }
struct ItemInfo * item_info_assign_from_record_a(struct ItemInfo * arg0, const struct ItemRecordA * arg1) { abort(); }
struct ItemInfo * item_info_assign_from_drag(struct ItemInfo * arg0, const struct DragInfo * arg1) { abort(); }
void item_info_set_item_id(struct ItemInfo * arg0, int arg1) { abort(); }
const char * item_info_get_res_name(const struct ItemInfo * arg0) { abort(); }
void item_info_destroy(void *item) { abort(); }
const char * session_character_name(const struct Session * arg0) { abort(); }
struct Session g_session1;
void tree_free(struct Tree * arg0, void (*arg1)(void *)) { abort(); }
int crt_sprintf_s(char * arg0, size_t arg1, const char * arg2, ...) { abort(); }
struct StdBitIter * std_vector_bool_erase(struct StdBitVector * arg0, struct StdBitIter * arg1, unsigned int * arg2, unsigned int arg3, unsigned int * arg4, unsigned int arg5) { abort(); }
struct StdBitIter * std_vector_bool_insert(struct StdBitVector * arg0, struct StdBitIter * arg1, unsigned int * arg2, unsigned int arg3, size_t arg4, const BYTE * arg5) { abort(); }
struct UIWindow * ui_window_construct(struct UIWindow * arg0, int arg1) { abort(); }
void ui_window_create(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_add_child(struct UIWindow * arg0, struct UIWindow * arg1) { abort(); }

void ui_window_invalidate_base(struct UIWindow * arg0) { abort(); }
struct UIFrameWnd * ui_frame_wnd_construct(struct UIFrameWnd * arg0, int arg1) { abort(); }
void ui_frame_wnd_destruct(struct UIFrameWnd * arg0) { abort(); }
void ui_frame_wnd_move(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_scroll_bar_resize(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_frame_wnd_save_pos(struct UIWindow *arg0) { abort(); }
void ui_frame_wnd_move_delta(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_frame_wnd_on_l_btn_down(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_frame_wnd_on_mouse_move(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_frame_wnd_on_mouse_hover(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_frame_wnd_on_l_btn_up(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_frame_wnd_snap(struct UIWindow *arg0) { abort(); }

char * msg_str(int arg0) { abort(); }
BYTE ui_window_is_focusable_base(struct UIWindow * arg0) { abort(); }
BYTE ui_window_get_backdrop_box_base(struct UIWindow * arg0, struct UIBackdropBox * arg1) { abort(); }
BYTE ui_window_is_click_through_base(struct UIWindow * arg0) { abort(); }
BYTE ui_window_is_hit_testable_base(struct UIWindow * arg0) { abort(); }
void ui_window_store_info_base(struct UIWindow * arg0) { abort(); }
void ui_window_save_pos_base(struct UIWindow * arg0) { abort(); }
void ui_window_move_delta_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_set_show_base(struct UIWindow * arg0, int arg1) { abort(); }
void ui_window_on_create_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_destroy_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_process_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_focus_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_focus_lost_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_l_btn_down_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_l_btn_dbl_clk_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_mouse_move_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_mouse_hover_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_track_mouse_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_l_btn_up_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_r_btn_down_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_r_btn_up_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_wheel_base(struct UIWindow * arg0, int arg1) { abort(); }
void ui_window_snap_base(struct UIWindow * arg0) { abort(); }
void ui_window_draw_backdrop_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_hover_enter_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_hover_leave_base(struct UIWindow * arg0) { abort(); }
BYTE ui_window_render_base(struct UIWindow * arg0, int arg1) { abort(); }
struct TexDrawPrim * ui_window_make_quad_base(struct UIWindow * arg0) { abort(); }
struct UIWindow * ui_window_mgr_get_focus_edit(struct UIWindowMgr * arg0) { abort(); }
struct UIWindow * ui_window_find(int arg0) { abort(); }
struct UIWindow * ui_window_open(int arg0) { abort(); }
intptr_t ui_window_send(struct UIWindow * arg0, struct UIWindow * arg1, int arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6) { abort(); }
struct Mode * game_current_base_mode(void) { abort(); }
struct GameMode *game_current_mode(void) { abort(); }
struct DragInfo * game_current_drag(void) { abort(); }
int ui_window_mgr_message_box(struct UIWindowMgr * arg0, const char * arg1, int arg2, int arg3, int arg4, int arg5, const char * arg6, int arg7, int arg8, int arg9) { abort(); }
intptr_t ui_window_mgr_send_msg_(struct UIWindowMgr * arg0, int arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5) { abort(); }
BYTE ui_window_mgr_show_option_window(struct UIWindowMgr * arg0, int arg1, int arg2) { abort(); }
void ui_window_mgr_remove_from_name_waiting_list(struct UIWindowMgr * arg0, struct UIWindow * arg1) { abort(); }
void ui_window_mgr_forget_window(struct UIWindowMgr * arg0, const struct UIWindow * arg1) { abort(); }

void ui_window_mgr_set_focus_edit(struct UIWindowMgr * arg0, struct UIWindow * arg1) { abort(); }
struct UIWindow * ui_window_mgr_get_capture(struct UIWindowMgr * arg0) { abort(); }
void ui_window_mgr_set_capture(struct UIWindowMgr * arg0, struct UIWindow * arg1) { abort(); }
void ui_window_mgr_release_capture(struct UIWindowMgr * arg0) { abort(); }
void ui_window_mgr_update_snap_rect(struct UIWindowMgr * arg0, struct UIWindow * arg1) { abort(); }
int client_lua_call(struct ClientLua * arg0, const char * arg1, const char * arg2, ...) { abort(); }
void mode_show_tooltip(struct Mode * arg0, const char * arg1, int arg2, int arg3, DWORD arg4, BYTE arg5) { abort(); }
void session_clear_lists(struct Session * arg0) { abort(); }
struct DcBitmap * dc_bitmap_create(int arg0, int arg1) { abort(); }
void dc_bitmap_destroy(struct DcBitmap * arg0) { abort(); }
void dc_bitmap_fill(struct DcBitmap * arg0, const RECT * arg1, DWORD arg2) { abort(); }
int ui_window_draw_button_bar(struct UIWindow * arg0, int arg1, int arg2, int arg3) { abort(); }
int ui_window_vertical_text_length(struct UIWindow * arg0, const char * arg1, int arg2, int arg3, int arg4, int arg5) { abort(); }
char * format_money(int arg0, char * arg1) { abort(); }
void ui_window_text_out_down(struct UIWindow * arg0, int arg1, int arg2, const char * arg3, int arg4, int arg5, int arg6, COLORREF arg7) { abort(); }
const char * ui_window_shorten_text(struct UIWindow * arg0, struct String * arg1, const char * arg2, int arg3, int arg4, int arg5, int arg6) { abort(); }
struct ResMgr * g_resMgr(void) { abort(); }
void * res_mgr_get(struct ResMgr * arg0, const char * arg1) { abort(); }
void * res_mgr_get_string(struct ResMgr * arg0, struct String arg1) { abort(); }
struct SkinMgr g_skinMgr;
void ui_skin_color_chip(struct SkinMgr * arg0, int arg1, int arg2, int * arg3, int * arg4, int * arg5) { abort(); }
const char * ui_skin_path(const char * arg0) { abort(); }
void ui_bmp_text_button_set_text_style(struct UIBmpTextButton * arg0, int arg1, int arg2, COLORREF arg3, char arg4, char arg5, int arg6, int arg7) { abort(); }
void ui_bmp_text_button_on_draw(struct UIWindow *arg0) { abort(); }
struct UIWindow * ui_static_text_construct(struct UIWindow * arg0) { abort(); }
struct UIStaticBitmap * ui_static_bitmap_construct(struct UIStaticBitmap * arg0) { abort(); }
void ui_static_bitmap_set_bitmap(struct UIStaticBitmap * arg0, const char * arg1) { abort(); }
void ui_static_text_set_text_fit(struct UIWindow * arg0, const char * arg1) { abort(); }
struct UIWindow * ui_edit_ctrl2_construct(struct UIWindow * arg0) { abort(); }
const char * ui_edit_ctrl_get_text(struct UIWindow * arg0) { abort(); }
void ui_window_mgr_focus_edit_lost(struct UIWindowMgr * arg0) { abort(); }
void ui_scroll_bar_set_range(struct UIScrollBar * arg0, int arg1, int arg2, int arg3) { abort(); }
void ui_scroll_bar_set_pos(struct UIScrollBar * arg0, int arg1) { abort(); }
void ui_window_draw_item_win_mid(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_draw_item_icon(struct UIWindow * arg0, int arg1, int arg2, const struct ItemInfo * arg3, BYTE arg4) { abort(); }
void ui_window_draw_item_icon_count(struct UIWindow * arg0, int arg1, int arg2, const struct ItemInfo * arg3, BYTE arg4, BYTE arg5) { abort(); }
void ui_window_draw_item_icon_count_grey(struct UIWindow * arg0, int arg1, int arg2, int arg3, const struct ItemInfo * arg4, BYTE arg5, BYTE arg6) { abort(); }
int ui_window_draw_item_name_short(struct UIWindow * arg0, int arg1, int arg2, int arg3, const struct ItemInfo * arg4, int arg5) { abort(); }
COLORREF session_get_item_dis_color(const struct ItemInfo * arg0) { abort(); }
void ui_refining_wnd_reset(struct UIRefiningWnd * arg0) { abort(); }
struct ItemInfo * session_get_inventory_item_by_id(struct Session * arg0, struct ItemInfo * arg1, int arg2, BYTE arg3) { abort(); }
int session_get_shop_buy_total(struct Session * arg0) { abort(); }
int session_get_shop_buy_list_size(struct Session * arg0) { abort(); }
struct ItemInfo * session_get_shop_buy_item_at(struct Session * arg0, struct ItemInfo * arg1, int arg2) { abort(); }
int session_get_shop_buy_count_by_name(struct Session * arg0, const char * arg1) { abort(); }
void session_add_shop_buy_item(struct Session * arg0, const struct ItemInfo * arg1) { abort(); }
void item_info_vec_free(struct Vec * arg0) { abort(); }
void game_mode_drag_item(struct GameMode *arg0, int arg1, const struct ItemInfo *arg2, int arg3, int arg4) { abort(); }
#endif

#if UI_DROP_OWNER == 2
int session_is_pet_egg_id(struct Session * arg0, int arg1) { abort(); }
void ui_refining_notice_set_text(struct UIRefining_Notice * arg0, const char * arg1, int arg2, BYTE arg3) { abort(); }
int refining_selected_zeny(struct UIRefiningWnd * arg0) { abort(); }
int refining_carried(int arg0) { abort(); }
#endif

#if UI_DROP_OWNER == 4
struct StdBitIter * std_vector_bool_erase_at(struct StdBitVector * arg0, struct StdBitIter * arg1, unsigned int * arg2, unsigned int arg3) { abort(); }
#endif

#if UI_DROP_OWNER == 8
void ui_window_on_draw_base(struct UIWindow * arg0) { abort(); }
void surface_destroy(struct Surface * arg0) { abort(); }
#endif

#if UI_DROP_OWNER == 9

#endif

#if UI_DROP_OWNER == 10
void * object_as_type(void * arg0, enum ObjectType arg1) { abort(); }
int ui_item_base_wnd_index_at(struct UIItemBaseWnd * arg0, int arg1, int arg2) { abort(); }
struct ItemInfo * ui_item_base_wnd_get_item(struct UIItemBaseWnd * arg0, struct ItemInfo * arg1, int arg2) { abort(); }
struct ItemInfo * ui_item_base_wnd_item_at(struct UIItemBaseWnd * arg0, struct ItemInfo * arg1, int arg2, int arg3) { abort(); }
struct UIItemShopWnd * ui_item_shop_wnd_construct(struct UIItemShopWnd * arg0) { abort(); }
void ui_item_shop_wnd_destruct(struct UIItemShopWnd * arg0) { abort(); }
struct UIItemPurchaseWnd * ui_item_purchase_wnd_construct(struct UIItemPurchaseWnd * arg0) { abort(); }
void ui_item_purchase_wnd_destruct(struct UIItemPurchaseWnd * arg0) { abort(); }
struct UIBitmapButton * shop_add_button(struct UIWindow * arg0, const char *const arg1[3], int arg2, int arg3, int arg4) { abort(); }
void shop_draw_frame(struct UIItemBaseWnd * arg0, int arg1, int arg2) { abort(); }
void shop_draw_cells(struct UIWindow * arg0, int arg1) { abort(); }
void shop_fill_names(struct UIWindow * arg0, int arg1) { abort(); }
void shop_resize(struct UIItemShopWnd2 * arg0, int arg1, int arg2, int arg3) { abort(); }
int shop_info_usable(struct WindowInfo * arg0) { abort(); }
void shop_info_default(struct WindowInfo * arg0) { abort(); }
struct UIWindow * shop_add_title(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void shop_add_resizer(struct UIItemBaseWnd * arg0, int arg1) { abort(); }
void shop_add_scroll_bar(struct UIItemBaseWnd * arg0, int arg1) { abort(); }
void shop_tooltip_cell(struct UIWindow * arg0, int arg1, int arg2, int * arg3, int * arg4) { abort(); }
int purchase_info_usable(struct WindowInfo * arg0) { abort(); }
void purchase_info_default(struct WindowInfo * arg0) { abort(); }
int session_get_shop_buy_count(struct Session * arg0, int arg1) { abort(); }
int session_get_shop_goods_count_by_id(struct Session * arg0, const struct ItemInfo * arg1) { abort(); }
int session_get_shop_goods_count_by_index(struct Session * arg0, const struct ItemInfo * arg1) { abort(); }
WORD session_get_shop_goods_weight_by_id(struct Session * arg0, const struct ItemInfo * arg1) { abort(); }
WORD session_get_shop_goods_weight_by_index(struct Session * arg0, const struct ItemInfo * arg1) { abort(); }
WORD session_shop_buy_weight_by_id(struct Session * arg0) { abort(); }
WORD session_shop_buy_weight_by_index(struct Session * arg0) { abort(); }
void session_remove_counted_shop_goods(struct Session * arg0, const struct ItemInfo * arg1) { abort(); }
void session_remove_shop_goods_by_id(struct Session * arg0, const struct ItemInfo * arg1) { abort(); }
void session_add_shop_buy_item_keyed(struct Session * arg0, const struct ItemInfo * arg1) { abort(); }
void session_remove_shop_buy_item_keyed(struct Session * arg0, const struct ItemInfo * arg1) { abort(); }
void session_add_shop_goods_by_name(struct Session * arg0, const struct ItemInfo * arg1) { abort(); }
int session_sellable_count(struct Session * arg0) { abort(); }
struct ItemInfo * session_sellable_at(struct Session * arg0, struct ItemInfo * arg1, int arg2) { abort(); }
void session_sellable_add(struct Session * arg0, const struct ItemInfo * arg1) { abort(); }
void session_purchase_list_remove(struct Session * arg0, const struct ItemInfo * arg1) { abort(); }
void ui_item_shop_wnd_store_info(struct UIWindow *arg0) { abort(); }
void ui_item_shop_wnd_on_create(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_item_shop_wnd_on_l_btn_down(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_item_shop_wnd_on_l_btn_dbl_clk(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_item_shop_wnd_on_mouse_hover(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_item_shop_wnd_on_r_btn_down(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_item_purchase_wnd_store_info(struct UIWindow *arg0) { abort(); }
void ui_item_purchase_wnd_on_create(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_item_purchase_wnd_on_draw(struct UIWindow *arg0) { abort(); }
void ui_item_purchase_wnd_on_l_btn_down(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_item_purchase_wnd_on_mouse_hover(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_item_purchase_wnd_on_r_btn_down(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
#endif
