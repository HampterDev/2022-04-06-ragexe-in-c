#include "render/renderer.h"
#include "render/texture_quad.h"
#include "render/texture.h"
#include "render/screen_prim.h"
#include "render/math3d.h"

void screen_texture_rect(struct Renderer *renderer, const RECT *rect, int angle,
                         float rhw, DWORD colour, struct Texture *texture,
                         struct TextureUV uv)
{
    struct TexDrawPrim *prim;
    struct TexDrawVertex *v;
    float z, su, sv;
    int i;

    if (!renderer || !rect || !texture || texture->base.width <= 0 || texture->base.height <= 0)
        return;
    prim = renderer_new_prim(renderer);
    if (!prim)
        return;
    prim->base.texture = texture;
    v = prim->base.verts;
    if (angle) {
        float c = get_cos_deg(angle), s = get_sin_deg(angle);
        float hw = (float)(rect->right - rect->left) * 0.5f;
        float hh = (float)(rect->bottom - rect->top) * 0.5f;
        float cx = (float)rect->left + hw, cy = (float)rect->top + hh;

        v[0].x = -hw * c + hh * s + cx;
        v[0].y = -hh * c - hw * s + cy;
        v[1].x = hw * c + hh * s + cx;
        v[1].y = hw * s - hh * c + cy;
        v[2].x = -hw * c - hh * s + cx;
        v[2].y = hh * c - hw * s + cy;
        v[3].x = hw * c - hh * s + cx;
        v[3].y = hh * c + hw * s + cy;
    } else {
        v[0].x = (float)rect->left;
        v[0].y = (float)rect->top;
        v[1].x = (float)rect->right;
        v[1].y = (float)rect->top;
        v[2].x = (float)rect->left;
        v[2].y = (float)rect->bottom;
        v[3].x = (float)rect->right;
        v[3].y = (float)rect->bottom;
    }
    z = renderer_project_depth(rhw);
    for (i = 0; i < 4; ++i) {
        v[i].z = z;
        v[i].rhw = rhw;
        v[i].diffuse = colour;
    }
    su = (float)texture->reqWidth / (float)texture->base.width;
    sv = (float)texture->reqHeight / (float)texture->base.height;
    v[0].u = v[2].u = uv.l * su;
    v[1].u = v[3].u = uv.r * su;
    v[0].v = v[1].v = uv.t * sv;
    v[2].v = v[3].v = uv.b * sv;
    renderer_add_prim(renderer, &prim->base, SCREEN_LIST);
}
