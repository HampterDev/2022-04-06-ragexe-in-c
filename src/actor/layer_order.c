#include "actor/layer_order.h"
#include "resources/imf.h"

int imf_layer_at(const struct ImfRes *imf, int order, int base_action, int action, int frame)
{
    static const signed char toward[9] = {-1, -2, 2, 3, 4, 8, 5, 6, 7};
    static const signed char away[9] = {7, -1, -2, 2, 3, 4, 8, 5, 6};
    unsigned direction = (unsigned)action - (unsigned)base_action;
    unsigned slot = 8u - (unsigned)order;
    int layer;
    if (direction > 7 || slot > 8)
        return 9;
    layer = (direction >= 2 && direction <= 5) ? away[slot] : toward[slot];
    if (layer < 0)
        layer = imf_resource_layer(imf, layer == -1 ? 1 : 0, action, frame);
    return (unsigned)layer < 9 ? layer : 9;
}
