#include "resources/model_res.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void set_name(struct OwnedText *out, const char *name)
{
    out->data = (char *)name; out->size = strlen(name);
}

int main(void)
{
    struct C3dModelRes res = {0};
    struct ModelResNodeV1 a = {0};
    struct ModelResNodeV2 b = {0};
    struct ModelResNode *nodes[] = {&a.base, &b.base};
    assert(!model_res_find_node(&res, "missing"));
    res.nodes = nodes; res.nodeCount = 2;
    set_name(&a.base.name, "기둥"); set_name(&b.base.name, "UpperNode");
    assert(model_res_find_node(&res, "기둥") == &a.base);
    assert(model_res_find_node(&res, "UpperNode") == &b.base);
    assert(!model_res_find_node(&res, "uppernode"));
    assert(!model_res_find_node(&res, "Upper"));

    puts("model resource types: node lookup, Korean names and exact matching passed");
    return 0;
}
