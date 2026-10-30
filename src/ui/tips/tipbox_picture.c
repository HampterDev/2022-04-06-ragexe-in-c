#include "ui/tips/tipbox_mgr.h"
#include "ui/tips/tipbox_picture.h"
#include "resources/bitmap.h"
#include "resources/sprite.h"
#include "fileio/resource_api.h"
#include <limits.h>

void tipbox_picture_layout(struct TipboxView *view, const char *path, int texture)
{
    int cell = view->tip.image_kind - 1;
    float width, height, size_scale;
    void *resource = res_mgr_get(g_resMgr(), path);
    if (!resource) return;
    if (texture) {
        const struct BitmapRes *bitmap = resource;
        width = (float)bitmap->width;
        height = (float)bitmap->height;
    } else {
        const struct SprRes *sprite = resource;
        const struct SprImg *frame = sprite_resource_image(sprite, SPR_PALETTE_IMAGES, 0);
        if (!frame) return;
        width = (float)frame->width;
        height = (float)frame->height;
    }
    view->scale = (float)view->tip.image_arg * 0.01f;
    view->left = (float)(cell % 3) * 80.0f + 40.0f - width * 0.5f * view->scale;
    view->top = (float)(cell / 3) * 106.666664f + 70.33333f - height * 0.5f * view->scale;
    if (texture) {
        if (view->left < 1.0f) {
            if ((double)(1.0f - view->left) > INT_MAX) return;
            view->tex_arg1 = (int)(1.0f - view->left);
            view->left = 1.0f;
        }
        if (view->top < 17.0f) {
            if ((double)(17.0f - view->top) > INT_MAX) return;
            view->tex_arg2 = (int)(17.0f - view->top);
            view->top = 17.0f;
        }
    } else {
        size_scale = (float)view->tip.image_arg / 100.0f;
        if (width * size_scale + view->left > 240.0f)
            view->left -= width * size_scale + view->left - 240.0f;
        if (height * size_scale + view->top > 320.0f)
            view->top -= height * size_scale + view->top - 320.0f;
    }
    view->right = width * view->scale;
    view->bottom = height * view->scale;
    if (!((double)view->left >= INT_MIN && (double)view->left <= INT_MAX &&
          (double)view->top >= INT_MIN && (double)view->top <= INT_MAX &&
          (double)view->right >= INT_MIN && (double)view->right <= INT_MAX &&
          (double)view->bottom >= INT_MIN && (double)view->bottom <= INT_MAX)) return;
    view->picture = texture ? TIP_PICTURE_TEXTURE : TIP_PICTURE_SPRITE;
}
