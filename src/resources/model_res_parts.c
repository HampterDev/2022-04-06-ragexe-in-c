#include "render/texture_cache.h"
#include <string.h>
#include <stdlib.h>
#include "render/render_api.h"
#include "resources/model_res.h"
#include "render/keyframes.h"

static void model_res_node_dispose(struct ModelResNode *node)
{
    size_t i;
    for (i = 0; i < node->textureCount; ++i) {
        if (node->textures[i]) {
            texture_unlock(node->textures[i]);
            ui_tex_cache_release(&g_textureCache, node->textures[i]);
        }
    }
    anim_mesh_destroy(node->mesh);
    free(node->textures);
    free(node->children);
    owned_text_clear(&node->name);
    owned_text_clear(&node->parentName);
}

void model_res_node_v1_destroy(struct ModelResNode *n)
{
    struct ModelResNodeV1 *node = (struct ModelResNodeV1 *)n;
    free(node->rotKeys);
    model_res_node_dispose(n);
    free(node);
}

void model_res_node_v2_destroy(struct ModelResNode *node)
{
    struct ModelResNodeV2 *n = (struct ModelResNodeV2 *)node;
    key_uv_clear(&n->uvKeys);
    free(n->posKeys);
    free(n->rotKeys);
    free(n->scaleKeys);
    model_res_node_dispose(&n->base);
    free(n);
}

struct ModelResNode *model_res_node_create(int version)
{
    size_t size = version == 1 ? sizeof(struct ModelResNodeV1) :
                  version == 2 ? sizeof(struct ModelResNodeV2) : 0;
    struct ModelResNode *node;
    if (!size) return NULL;
    node = calloc(1, size);
    if (!node) return NULL;
    node->alpha = 255;
    node->destroy = version == 1 ? model_res_node_v1_destroy : model_res_node_v2_destroy;
    return node;
}
