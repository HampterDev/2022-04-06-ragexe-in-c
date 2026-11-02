#if CONTAINER_MEMORY_PROBE == 2
struct UIWindowMgr g_windowMgr;
struct UIWindow * ui_window_find(int arg0) { abort(); }
struct UIWindow * ui_window_open(int arg0) { abort(); }
intptr_t ui_window_send(struct UIWindow * arg0, struct UIWindow * arg1, int arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6) { abort(); }
void ui_window_refresh(struct UIWindow * arg0) { abort(); }
int ui_window_mgr_message_box(struct UIWindowMgr * arg0, const char * arg1, int arg2, int arg3, int arg4, int arg5, const char * arg6, int arg7, int arg8, int arg9) { abort(); }
BYTE ui_window_mgr_post_quit1(struct UIWindowMgr * arg0, int arg1) { abort(); }
intptr_t ui_window_mgr_send_msg_(struct UIWindowMgr * arg0, int arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5) { abort(); }
struct String * item_info_get_display_name(const struct ItemInfo * arg0, struct String * arg1) { abort(); }
struct ItemInfo * item_info_construct(struct ItemInfo * arg0) { abort(); }
struct ItemInfo * item_info_copy(struct ItemInfo * arg0, const struct ItemInfo * arg1) { abort(); }
void item_info_set_item_id(struct ItemInfo * arg0, int arg1) { abort(); }
struct Session g_session1;
char * msg_str(int arg0) { abort(); }
void * tree_find(const struct Tree * arg0, const void * arg1, TreeLessFn arg2) { abort(); }
void * tree_add(struct Tree * arg0, const void * arg1, size_t arg2, TreeLessFn arg3, int * arg4) { abort(); }
struct TreeNode * tree_erase(struct Tree * arg0, struct TreeNode * arg1, TreeDestroyFn arg2) { abort(); }
int mail_id_greater(const void * arg0, const void * arg1) { abort(); }
void mail_pair_destroy(void * arg0) { abort(); }
struct MailPair * mail_pair_construct(struct MailPair * arg0, const long long * arg1, const struct Mail * arg2) { abort(); }
void session_clear_mail_item_counts(struct Session * arg0) { abort(); }
void session_put_mail_item(struct Session * arg0, struct ItemInfo arg1) { abort(); }
unsigned char session_is_account_char_name(struct Session * arg0, struct String arg1) { abort(); }
unsigned char session_is_known_name(struct Session * arg0, struct String arg1) { abort(); }
void rodex_inbox_wnd_remove(struct UIRodexInboxWnd * arg0, int arg1) { abort(); }
void ui_rodex_inbox_wnd_rebuild_lists(struct UIRodexInboxWnd * arg0) { abort(); }
void ui_rodex_inbox_wnd_set_buttons_disabled(struct UIRodexInboxWnd * arg0, BYTE arg1) { abort(); }
void ui_rodex_inbox_wnd_next_page(struct UIRodexInboxWnd * arg0) { abort(); }
void rodex_contents_wnd_set_zeny_flag(struct UIRodexContentsWnd * arg0, BYTE arg1) { abort(); }
void rodex_contents_wnd_set_items_flag(struct UIRodexContentsWnd * arg0, BYTE arg1) { abort(); }
void rodex_contents_wnd_zeny_taken(struct UIRodexContentsWnd * arg0) { abort(); }
void rodex_contents_wnd_items_taken(struct UIRodexContentsWnd * arg0) { abort(); }
void ui_rodex_compose_wnd_set_receiver(struct UIRodexComposeWnd * arg0, int arg1, WORD arg2, WORD arg3, const char * arg4) { abort(); }
#endif

#if CONTAINER_MEMORY_PROBE == 3
char * std_copy_memmove(const char * arg1, const char * arg2, char * arg3) { abort(); }

void std_vector_free(struct Vec * arg0, size_t arg1) { abort(); }
void store_search_items_destroy(struct StoreSearchItem * arg0, struct StoreSearchItem * arg1) { abort(); }
struct StoreSearchItem * store_search_item_copy(struct StoreSearchItem * arg0, const struct StoreSearchItem * arg1) { abort(); }
#endif

#if CONTAINER_MEMORY_PROBE == 4
#endif

#if CONTAINER_MEMORY_PROBE == 5

#endif
