#ifndef ACTOR_LAYER_ORDER_H
#define ACTOR_LAYER_ORDER_H

struct ImfRes;

int imf_layer_at(const struct ImfRes *imf, int order, int base_action, int action, int frame);

#endif
