#include "render/screen_bounds.h"
#include "render/rp_pool.h"
#include "actor/layer_order.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "contents/client_lua_calls.h"
#include "resources/attr_queries.h"
#include "session/state.h"
#ifndef UI_RENDER_CHARACTER_RENDER_H
#define UI_RENDER_CHARACTER_RENDER_H

#include "render/projection.h"

#include "render/math3d.h"

#include "ui/render/render.h"
#include "ui/character/character.h"
#include "ui/skin.h"
#include "ui/game.h"
#include "std/list.h"

#define CHR_LAYER_BODY 0
#define CHR_LAYER_HEAD 1
#define CHR_LAYER_TOP 2
#define CHR_LAYER_MID 3
#define CHR_LAYER_BOTTOM 4
#define CHR_LAYER_ROBE 8
#define CHR_LAYERS 9

#define CHR_JOB_WEDDING 0x16

struct CharacterRender {
    float pos[3];
    int job;
    int job2;
    int sex;
    int head;
    int body_palette;
    int head_palette;
    int accessory_top;
    int accessory_mid;
    int accessory_bottom;
    int head_low_location, head_top_location, head_mid_location;
    int action;
    int motion;
    BYTE riding;
    int body_style;
    float centre_dx, centre_dy;
    int body_flags;
    int body_x, body_y;
    int view_width, view_height;
    float distance;
    float pitch;
    float yaw;
    float up[3];
    struct ScreenBounds bbox;
    float screen_x, screen_y;
    BYTE shadow;
    struct Tree layers;
    int robe;
    BYTE is_helm_robe;
};

struct ChrSpriteRect {
    float top, left, bottom, right;
    float rhw_bottom;
    float rhw_top;
    DWORD colour;
    float angle;
    struct SprImg *img;
};

struct ChrQuadPair {
    int flags;
    struct TexDrawPrim *quad;
};

struct CharacterRender *character_render_create(int view_width, int view_height);
void character_render_destroy(struct CharacterRender *self);
void character_render_set_robe(struct CharacterRender *self, int robe);
struct List *character_render_collect_world_quads(struct CharacterRender *self,
                                                     struct List *out, BYTE use_bbox,
                                                     BYTE *drawn);
struct TexDrawPrim *character_render_shadow_quad(struct CharacterRender *self, const float *m,
                                                 float scale);

extern int g_chrHeadDx, g_chrHeadDy;
extern int g_chrWorldHeadDx, g_chrWorldHeadDy;

#endif
