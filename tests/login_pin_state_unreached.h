struct Mode *client_current_mode(void) { abort(); }
int connection_is_open(const struct Connection *arg0) { (void)arg0; abort(); }
int connection_recv_bytes(struct Connection *arg0, void *arg1, int arg2, char arg3) { (void)arg0; (void)arg1; (void)arg2; (void)arg3; abort(); }
void login_mode_enter_zone(struct LoginMode *arg0, DWORD arg1, struct String arg2, DWORD arg3, DWORD arg4, struct String arg5) { (void)arg0; (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5; abort(); }
void login_mode_on_accept_enter(struct LoginMode *arg0, const BYTE *arg1) { (void)arg0; (void)arg1; abort(); }
void login_mode_on_accept_login(struct LoginMode *arg0, const BYTE *arg1, int arg2) { (void)arg0; (void)arg1; (void)arg2; abort(); }
void login_mode_on_accept_make_char(struct LoginMode *arg0, const struct CharInfo *arg1) { (void)arg0; (void)arg1; abort(); }
void login_mode_on_auth_result(struct LoginMode *arg0, const BYTE *arg1) { (void)arg0; (void)arg1; abort(); }
void login_mode_on_auth_result2(struct LoginMode *arg0, const BYTE *arg1) { (void)arg0; (void)arg1; abort(); }
void login_mode_on_char_list(struct LoginMode *arg0, const BYTE *arg1) { (void)arg0; (void)arg1; abort(); }
void login_mode_on_char_list_header(struct LoginMode *arg0, const BYTE *arg1) { (void)arg0; (void)arg1; abort(); }
void login_mode_on_char_list_page(struct LoginMode *arg0, const BYTE *arg1) { (void)arg0; (void)arg1; abort(); }
void login_mode_on_disconnect_reason(struct LoginMode *arg0, const BYTE *arg1) { (void)arg0; (void)arg1; abort(); }
void login_mode_on_hash_key(struct LoginMode *arg0, const BYTE *arg1) { (void)arg0; (void)arg1; abort(); }
void login_mode_on_refuse_byte(struct LoginMode *arg0, const BYTE *arg1) { (void)arg0; (void)arg1; abort(); }
void login_mode_on_zone_accept(struct LoginMode *arg0, const BYTE *arg1) { (void)arg0; (void)arg1; abort(); }
void login_mode_on_zone_accept2(const BYTE *arg0) { (void)arg0; abort(); }
void login_mode_refuse_login(struct LoginMode *arg0, int arg1, const char *arg2) { (void)arg0; (void)arg1; (void)arg2; abort(); }
void login_mode_remove_deleted_char(struct LoginMode *arg0) { (void)arg0; abort(); }
int mode_send_message(struct Mode *arg0, int arg1, int arg2, int arg3, int arg4, int arg5) { (void)arg0; (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5; abort(); }
char *msg_str(int arg0) { (void)arg0; abort(); }
int rag_connection_get_packet_length(struct RagConnection *arg0, int arg1) { (void)arg0; (void)arg1; abort(); }
char rag_connection_send_packet(struct RagConnection *arg0, int arg1, const void *arg2) { (void)arg0; (void)arg1; (void)arg2; abort(); }
const char *session_get_rsw_name(const char *arg0) { (void)arg0; abort(); }
int session_set_blocked_characters(struct Session *arg0, const void *arg1, size_t arg2) { (void)arg0; (void)arg1; (void)arg2; abort(); }
const char *thread_text_format(const char *arg0, ...) { (void)arg0; abort(); }
BYTE ui_window_mgr_post_quit1(struct UIWindowMgr *arg0, int arg1) { (void)arg0; (void)arg1; abort(); }
int ui_window_send(struct UIWindow *arg0, struct UIWindow *arg1, int arg2, int arg3, int arg4, int arg5, int arg6) { (void)arg0; (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6; abort(); }
void string_assign(struct String *arg0, const char *arg1, size_t arg2) { (void)arg0; (void)arg1; (void)arg2; abort(); }
void string_assign_cstr(struct String *arg0, const char *arg1) { (void)arg0; (void)arg1; abort(); }
void string_free(struct String *arg0) { (void)arg0; abort(); }
struct String string_from_cstr(const char *arg0) { (void)arg0; abort(); }
void string_replace(struct String *arg0, size_t arg1, size_t arg2, const char *arg3, size_t arg4) { (void)arg0; (void)arg1; (void)arg2; (void)arg3; (void)arg4; abort(); }
void *vec_push(struct Vec *arg0, const void *arg1, size_t arg2) { (void)arg0; (void)arg1; (void)arg2; abort(); }
