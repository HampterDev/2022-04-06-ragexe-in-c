void dc_bitmap_copy_rect(struct DcBitmap * arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, struct DcBitmap * arg7) { abort(); }
void dc_bitmap_to_texture(struct DcBitmap * arg0, struct Texture ** arg1) { abort(); }
struct TexDrawPrim * renderer_new_prim(struct Renderer * arg0) { abort(); }
void * res_mgr_get_string(struct ResMgr * arg0, struct String arg1) { abort(); }
void * res_mgr_get(struct ResMgr * arg0, const char * arg1) { abort(); }
const char * ui_skin_path(const char * arg0) { abort(); }
struct UIWindow * ui_window_mgr_get_capture(struct UIWindowMgr * arg0) { abort(); }
void ui_window_mgr_release_capture(struct UIWindowMgr * arg0) { abort(); }
void ui_window_mgr_set_capture(struct UIWindowMgr * arg0, struct UIWindow * arg1) { abort(); }
void ui_window_add_child(struct UIWindow * arg0, struct UIWindow * arg1) { abort(); }
void ui_window_clear_canvas(struct UIWindow * arg0, DWORD arg1) { abort(); }
struct UIWindow * ui_window_construct(struct UIWindow * arg0, int arg1) { abort(); }
void ui_window_destroy_children(struct UIWindow * arg0) { abort(); }
void ui_window_destruct(struct UIWindow * arg0) { abort(); }
void ui_window_drag_and_drop_base(struct UIWindow * arg0, int arg1, int arg2, const struct DragInfo * arg3) { abort(); }
void ui_window_draw_backdrop_base(struct UIWindow * arg0) { abort(); }
void ui_window_draw_bitmap(struct UIWindow * arg0, int arg1, int arg2, struct BitmapRes * arg3, int arg4) { abort(); }
void ui_window_fill_rect(struct UIWindow * arg0, int arg1, int arg2, int arg3, int arg4, COLORREF arg5) { abort(); }
BYTE ui_window_get_backdrop_box_base(struct UIWindow * arg0, struct UIBackdropBox * arg1) { abort(); }
DWORD ui_window_get_color(struct UIWindow * arg0) { abort(); }

struct UIWindow * ui_window_hit_test_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
BYTE ui_window_is_hit_testable_base(struct UIWindow * arg0) { abort(); }
struct TexDrawPrim * ui_window_make_quad_base(struct UIWindow * arg0) { abort(); }
void ui_window_move_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_move_delta_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_draw_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_create_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_destroy_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_l_btn_dbl_clk_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_l_btn_down_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_l_btn_up_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_mouse_hover_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_mouse_move_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_mouse_shape_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_process_base(struct UIWindow * arg0) { abort(); }
void ui_window_remove_child(struct UIWindow * arg0, struct UIWindow * arg1, BYTE arg2) { abort(); }
BYTE ui_window_render_base(struct UIWindow * arg0, int arg1) { abort(); }
void ui_window_save_pos_base(struct UIWindow * arg0) { abort(); }
int ui_window_send_msg_base(struct UIWindow * arg0, struct UIWindow * arg1, int arg2, int arg3, int arg4, int arg5, int arg6) { abort(); }
void ui_window_render_overlay_base(struct UIWindow * arg0) { abort(); }
void ui_window_snap_base(struct UIWindow * arg0) { abort(); }
void ui_window_store_info_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_focus_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_focus_lost_base(struct UIWindow * arg0) { abort(); }
void ui_window_track_mouse_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_r_btn_down_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_r_btn_up_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_hover_enter_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_hover_leave_base(struct UIWindow * arg0) { abort(); }
BYTE ui_window_is_focusable_base(struct UIWindow * arg0) { abort(); }
BYTE ui_window_is_click_through_base(struct UIWindow * arg0) { abort(); }
void dc_bitmap_copy_bitmap(struct DcBitmap * arg0, unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4, struct DcBitmap * arg5) { abort(); }
void fatal_error(const char * arg0) { abort(); }
struct Renderer * g_renderer;
struct ResMgr * g_resMgr(void) { abort(); }
float g_texelOffsetU;
float g_texelOffsetV;
struct UIWindowMgr g_windowMgr;
void * mem_alloc(size_t arg0) { abort(); }
void mem_free(void * arg0) { abort(); }
char render_quads_to_texture(int arg0, int arg1, const struct List * arg2, struct Texture ** arg3) { abort(); }
void string_copy(struct String *arg0, const struct String *arg1) { abort(); }
void string_free(struct String *arg0) { abort(); }
int string_compare(const struct String *arg0, const struct String *arg1) { abort(); }
void ui_window_list_copy(struct List * arg0, const struct List * arg1) { abort(); }
