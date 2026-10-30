#include <string.h>
#include "skills/world_video.h"
#include "game/context.h"
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

void model_res_place_videos(const struct C3dModelRes *model, Vec3 position)
{
    size_t i;
    struct WorldVideoList *list = game_current_video_list();
    if (!list) return;
    for (i = 0; i < model->videoNames.size; ++i) {
        struct WorldVideo *video = world_video_list_open(list, owned_text_cstr(&model->videoNames.items[i]), 1);
        if (video && !world_video_add_position(video, position)) {
            error_msg("Unable to allocate world video positions.", 0);
            return;
        }
    }
}
