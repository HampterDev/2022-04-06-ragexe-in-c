#include "render/sprite_draw.h"
#include "render/math3d.h"
#include "render/rp_pool.h"
#include "render/sprite_atlas.h"
#include "render/texture_types.h"
#include "resources/animation.h"
#include "resources/sprite.h"

#define MOTION_CLIP_COUNT(m) ((m)->clip_count)
#define SPR_PALETTE(spr) ((spr)->palette)

void screen_sprite_frame(struct Renderer *renderer, int x, int y, struct ActRes *act, struct SprRes *spr,
                                 int action, int motion, float scale, float angle, DWORD colour, int flags)
{
    struct SpriteRegion uv = {0};
    struct Motion *mot;
    int i;

    mot = act_res_get_motion(act, action, motion);
    for (i = 0; i < MOTION_CLIP_COUNT(mot); i++) {
        struct TexDrawPrim *prim;
        struct SprClip *clip = motion_get_clip(mot, i);
        struct TexDrawVertex *v;
        struct SprImg *img;
        struct Texture *tex;
        float zoomX, x0s, x1s, X0, X1, Y0, Y1;
        float v0x, v0y, v1x, v1y, v2x, v2y, v3x, v3y;
        int x1, y1;

        if (clip->sprIndex == -1)
            continue;
        zoomX = clip->zoomX;
        img = sprite_resource_image(spr, clip->clipType, clip->sprIndex);
        if (!img) continue;
        x1 = img->width * (img->widthHalved + 1) - 1 + clip->x;
        y1 = img->height * (img->heightHalved + 1) - 1 + clip->y;
        x0s = (float)clip->x * scale;
        Y0 = (float)clip->y * scale * clip->zoomY;
        x1s = (float)x1 * scale;
        X0 = zoomX * x0s;
        X1 = zoomX * x1s;
        Y1 = (float)y1 * scale * clip->zoomY;
        if (clip->clipType == 0) {
            tex = sprite_atlas_resolve(renderer, img, SPR_PALETTE(spr), &uv);
        } else {
            tex = img->texture;
            if (tex) {
                uv.u = uv.v = 0.0f;
                uv.u1 = (float)tex->reqWidth / (float)tex->base.width;
                uv.v1 = (float)tex->reqHeight / (float)tex->base.height;
            }
        }
        if (!tex) continue;
        prim = renderer_new_prim(renderer);
        prim->base.texture = tex;
        if (angle != 0.0f) {
            int deg = (int)angle;
            float c = get_cos_deg(deg), s = get_sin_deg(deg);
            float cX0 = c * X0, cX1 = c * X1, cY0 = c * Y0, cY1 = c * Y1;
            float sX0 = s * X0, sX1 = s * X1, sY0 = s * Y0, sY1 = s * Y1;
            float fx = (float)x, fy = (float)y;

            v0x = cX0 - sY0 + fx;
            v0y = sX0 + cY0 + fy;
            v1x = cX1 - sY0 + fx;
            v1y = sX1 + cY0 + fy;
            v2x = cX1 - sY1 + fx;
            v2y = cY1 + sX1 + fy;
            v3x = cX0 - sY1 + fx;
            v3y = cY1 + sX0 + fy;
        } else {
            float a, b, fx = (float)x, fy = (float)y;

            if (clip->flags & 1) {
                a = X1;
                b = zoomX * x0s;
            } else {
                a = X0;
                b = zoomX * x1s;
            }
            v0x = fx + a;
            v1x = b + fx;
            v2x = fx + a;
            v3x = b + fx;
            v0y = fy + Y0;
            v1y = fy + Y0;
            v2y = fy + Y1;
            v3y = fy + Y1;
        }
        v = prim->quad;
        v[0].x = v0x + 0.5f;
        v[0].y = v0y + 0.5f;
        v[1].x = v1x + 0.5f;
        v[1].y = v1y + 0.5f;
        v[2].x = v2x + 0.5f;
        v[2].y = v2y + 0.5f;
        v[3].x = v3x + 0.5f;
        v[3].y = v3y + 0.5f;
        for (x1 = 0; x1 < 4; x1++) {
            *(DWORD *)&v[x1].z = 0x358637bd;
            *(DWORD *)&v[x1].rhw = 0x3f7fffef;
            v[x1].diffuse = colour;
        }
        v[0].u = uv.u;
        v[0].v = uv.v;
        v[1].u = uv.u1;
        v[1].v = uv.v;
        v[2].u = uv.u;
        v[2].v = uv.v1;
        v[3].u = uv.u1;
        v[3].v = uv.v1;
        renderer_add_prim(renderer, &prim->base, flags);
    }
}
