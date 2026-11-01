#include <string.h>
#include "resources/model_res.h"

struct ModelResNode *model_res_find_node(const struct C3dModelRes *model, const char *name)
{
    size_t i, length = strlen(name);
    for (i = 0; i < model->nodeCount; ++i) {
        struct ModelResNode *node = model->nodes[i];
        if (node && node->name.size == length && !memcmp(owned_text_cstr(&node->name), name, length))
            return node;
    }
    return NULL;
}
