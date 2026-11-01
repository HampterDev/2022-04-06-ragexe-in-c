#include "world/mode.h"
#include "fileio/res_mgr.h"
#include "render/texture_cache.h"
#include "contents/mouse.h"
struct AccountUserDataMgr;
struct AdventurerAgencyMgr;
struct ActRes;
struct BitmapRes;
struct ModeMgr;
struct Motion;
struct Mouse;
struct RagConnection;
struct Renderer;
struct ResMgr;
struct Session;
struct SprClip;
struct Texture;
struct CharUserDataMgr;
struct TexCache;
struct ItemInfo;
struct RPQuad;
struct RenewQuestB;
struct SprImg;
struct SpriteRegion;
struct String;
struct Tree;
struct TreeNode;
struct Vec;
struct TexDrawPrim;
struct TextureUV;

struct Motion *act_res_get_motion(const struct ActRes *arg0, unsigned int arg1, unsigned int arg2)
{
    abort();
}

float act_res_get_delay(const struct ActRes *arg0, unsigned int arg1)
{
    abort();
}

struct SprClip *motion_get_clip(const struct Motion *arg0, unsigned int arg1)
{
    abort();
}

char rag_connection_send_packet(struct RagConnection *arg0, int arg1, const void *arg2)
{
    abort();
}

void renderer_add_prim(struct Renderer *arg0, struct RPQuad *arg1, int arg2)
{
    abort();
}

struct TexDrawPrim *renderer_new_prim(struct Renderer *arg0)
{
    abort();
}

void renderer_restore_frame(struct Renderer *arg0)
{
    abort();
}

void renderer_save_frame(struct Renderer *arg0)
{
    abort();
}

char renderer_screenshot(struct Renderer *arg0, struct String arg1)
{
    abort();
}

int renderer_text_out(struct Renderer *arg0, int arg1, int arg2, const char *arg3, COLORREF arg4)
{
    abort();
}

int renderer_draw_scene(struct Renderer *arg0)
{
    abort();
}

void renderer_flip(struct Renderer *arg0, char arg1)
{
    abort();
}

void *res_mgr_get(struct ResMgr *arg0, const char *arg1)
{
    abort();
}

void draw_box_screen(int arg0, int arg1, int arg2, int arg3, DWORD arg4)
{
    abort();
}

float get_cos_deg(int arg0)
{
    abort();
}

float get_sin_deg(int arg0)
{
    abort();
}

struct ItemInfo *item_info_copy(struct ItemInfo *arg0, const struct ItemInfo *arg1)
{
    abort();
}

struct RenewQuestB *quest_hunt_construct(struct RenewQuestB *arg0)
{
    abort();
}

struct RagConnection *rag_connection_instance(void)
{
    abort();
}

void store_search_item_copy(void *arg0, const void *arg1)
{
    abort();
}

void store_search_items_destroy(void *arg0, void *arg1)
{
    abort();
}

struct Texture *texture_cache_get(struct TexCache *arg0, const char *arg1, int arg2, int arg3)
{
    abort();
}

void ui_window_mgr_draw_picture(struct UIWindowMgr *arg0, int arg1, int arg2, struct BitmapRes *arg3)
{
    abort();
}

void account_user_data_save(struct AccountUserDataMgr *arg0)
{
    abort();
}

unsigned int actres_frame_count(const struct ActRes *arg0, unsigned int arg1)
{
    abort();
}

void adventurer_agency_cancel_own_booking(struct AdventurerAgencyMgr *arg0)
{
    abort();
}

int char_user_data_save(struct CharUserDataMgr *arg0)
{
    abort();
}

int crt_sprintf(char *arg0, const char *arg1, ...)
{
    abort();
}

void device_flip_to_gdi(void)
{
    abort();
}

struct AccountUserDataMgr *g_accountUserDataMgr;

struct AdventurerAgencyMgr *g_adventurerAgency;

struct CharUserDataMgr *g_charUserDataMgr;

char g_clientDir[128];

const struct TextureUV g_fullUV;

HWND g_hMainWnd;

struct ModeMgr g_modeMgr;

struct Mouse g_mouse;

struct Renderer *g_renderer;

struct ResMgr *g_resMgr(void)
{
    abort();
}

int g_serviceType;

struct Session g_session1;

struct TexCache g_textureCache;

void screen_texture_rect(struct Renderer *arg0, const RECT *arg1, int arg2, float arg3, DWORD arg4,
                         struct Texture *arg5, struct TextureUV arg6)
{
    abort();
}

void sound_bgm_fade_in(void)
{
    abort();
}

void sound_bgm_fade_out(void)
{
    abort();
}

struct Texture *sprite_atlas_resolve(struct Renderer *arg0, const struct SprImg *arg1,
                                      const DWORD *arg2, struct SpriteRegion *arg3)
{
    abort();
}

char *std_copy_memmove(const char *arg0, const char *arg1, char *arg2)
{
    abort();
}
