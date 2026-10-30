#ifndef RESOURCES_IMF_H
#define RESOURCES_IMF_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

struct ImfRes;

int imf_resource_layer(const struct ImfRes *resource, int order, int action, int frame);
POINT imf_resource_position(const struct ImfRes *resource, int layer, int action, int frame);

#endif
