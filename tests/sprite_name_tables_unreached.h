float act_res_get_delay(const struct ActRes *self, unsigned action)
{
    abort();
}
struct Motion * act_res_get_motion(const struct ActRes *self, unsigned action, unsigned motion)
{
    abort();
}
unsigned actres_frame_count(const struct ActRes *self, unsigned action)
{
    abort();
}
void error_msg(const char *text, int flags)
{
    abort();
}
struct GameMode * game_current_mode(void)
{
    abort();
}
struct GameActor * game_find_actor(int aid)
{
    abort();
}
struct String * game_mode_get_actor_name(struct GameMode *self, struct String *out, int aid)
{
    abort();
}
struct ItemInfo * item_info_construct_with_arg(struct ItemInfo *self)
{
    abort();
}
struct ItemInfo * item_info_copy(struct ItemInfo *self, const struct ItemInfo *src)
{
    abort();
}
const char * map_info_display_name(const struct MapInfoMgr *self, const char *map)
{
    abort();
}
int once_begin(INIT_ONCE *once)
{
    abort();
}
void once_complete(INIT_ONCE *once)
{
    abort();
}
int session_get_drawn_job(struct Session *self)
{
    abort();
}
const char * session_get_pet_act_text(int action)
{
    abort();
}
const char * session_get_pet_name(int petJob)
{
    abort();
}
int session_get_sex(struct Session *self)
{
    abort();
}
const char * session_get_weapon_name_by_class(int itemId)
{
    abort();
}
int session_is_admin_option(struct Session *self, int what)
{
    abort();
}
void string_vec_free(struct Vec *v)
{
    abort();
}
void string_vec_resize(struct Vec *v, size_t count)
{
    abort();
}
char * text_decode(char *text)
{
    abort();
}
size_t text_prefix_length(const char *text, size_t length, size_t max_bytes, unsigned code_page)
{
    abort();
}
void text_to_lower(struct String *s)
{
    abort();
}
void tokens_split(struct Vec *tokens, const struct String *line)
{
    abort();
}
size_t tree_erase_key(struct Tree *tree, const void *key, TreeLessFn less, TreeDestroyFn destroy)
{
    abort();
}
void * tree_find(const struct Tree *tree, const void *key, TreeLessFn less)
{
    abort();
}
int tree_less_int(const void *a, const void *b)
{
    abort();
}
int tree_less_int64(const void *a, const void *b)
{
    abort();
}
int tree_less_uint(const void *a, const void *b)
{
    abort();
}
int ui_window_mgr_message_box(struct UIWindowMgr *self, const char *text, int type, int arg3, int arg4, int arg5, const char *title, int width, int height, int arg9)
{
    abort();
}
void vec_free(struct Vec *v)
{
    abort();
}
void * vec_push(struct Vec *v, const void *elem, size_t size)
{
    abort();
}
void vec_resize(struct Vec *v, size_t count, size_t size)
{
    abort();
}
struct XmlElement * xml_find_child(struct XmlElement *self, const char *name)
{
    abort();
}
struct XmlElement * xml_find_next(struct XmlElement *self, const char *name)
{
    abort();
}
struct String * xml_node_text(struct XmlElement *self)
{
    abort();
}
