#include "render/texture_cache.h"
#include "ui/quests/world_view.h"
#include "render/cdc.h"
#include "session/session_state.h"
#include <stdlib.h>
#include <string.h>

static struct Texture *world_texture(struct WorldViewWorld *world)
{
    static const char directory[] = "유저인터페이스\\";
    size_t length = strlen(world->texture_name);
    char *path;
    if (length > (size_t)-1 - sizeof directory)
        return NULL;
    path = malloc(sizeof directory + length);
    if (!path)
        return NULL;
    memcpy(path, directory, sizeof directory - 1);
    memcpy(path + sizeof directory - 1, world->texture_name, length + 1);
    world->texture = texture_cache_get(&g_textureCache, path, 0, 1);
    free(path);
    world->image_width = world->texture ? world->texture->reqWidth : 0;
    world->image_height = world->texture ? world->texture->reqHeight : 0;
    return world->texture;
}

void world_view_prepare(struct WorldView *view)
{
    size_t i;
    if (!g_renderer)
        return;
    for (i = 0; i < view->world_count; ++i)
        world_texture(&view->worlds[i]);
    world_view_layout(view, g_renderer->width, g_renderer->height);
    if (!g_session1.rectMapsScaled) {
        g_session1.rectMapsScaled = 1;
        session_scale_rect_maps(&g_session1);
    }
}

void world_view_draw(struct WorldView *view, int x, int y, int width, int height)
{
    struct Texture *texture;
    struct WorldViewWorld *world;
    int old_width, old_height;
    if (g_renderer && (view->layout_width != g_renderer->width || view->layout_height != g_renderer->height))
        world_view_prepare(view);
    world_view_select(view, view->current_world);
    if (!view->world_count || width <= 0 || height <= 0)
        return;
    world = &view->worlds[view->current_world];
    old_width = world->image_width;
    old_height = world->image_height;
    texture = world_texture(world);
    if (g_renderer && (world->image_width != old_width || world->image_height != old_height))
        world_view_layout(view, g_renderer->width, g_renderer->height);
    if (texture && texture->reqWidth > 0 && texture->reqHeight > 0)
        texture_draw_surface_stretch(texture, x, y, width, height);
}
