#include "render/texture_image.h"
#include "render/render_api.h"
int texture_image_width(const struct Texture *texture) { return texture ? texture->reqWidth : 0; }
