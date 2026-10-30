#ifndef RESOURCES_RESOURCE_REGISTRY_H
#define RESOURCES_RESOURCE_REGISTRY_H

struct Res;
enum { RESOURCE_TYPE_COUNT = 20 };

int resource_type_index(const char *extension);
const char *resource_type_directory(int type);
struct Res *resource_create(int type);

#endif
