#include "contents/body_layer.h"
#include "std/tree.h"

struct LayerSprite *body_layer_sprite_slot(struct BodyLayerDraw *self, int layer)
{
    return tree_get(&self->sprites, &layer, sizeof layer, sizeof(struct LayerSprite), tree_less_int);
}

int *body_layer_view_slot(struct BodyLayerDraw *self, int layer)
{
    struct LayerView *view = tree_get(&self->views, &layer, sizeof layer, sizeof *view, tree_less_int);

    return &view->view;
}
