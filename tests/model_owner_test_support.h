#ifndef MODEL_OWNER_TEST_SUPPORT_H
#define MODEL_OWNER_TEST_SUPPORT_H

#ifndef MODEL_TEST_FREE
#define MODEL_TEST_FREE checked_free
#endif

static void *model_test_calloc(size_t count, size_t size)
{
    void *p;
    assert(!size || count <= (size_t)-1 / size);
    p = checked_malloc(count * size);
    if (p) memset(p, 0, count * size);
    return p;
}

static void *model_test_realloc(void *old, size_t size)
{
    void *p = checked_malloc(size);
    size_t before = old ? ((struct Allocation *)old - 1)->size : 0;
    if (!p) return NULL;
    if (before) memcpy(p, old, before < size ? before : size);
    MODEL_TEST_FREE(old);
    return p;
}

#define malloc checked_malloc
#define calloc model_test_calloc
#define realloc model_test_realloc
#define free MODEL_TEST_FREE
#include "../src/core/owned_text.c"
#include "../src/core/text_array.c"
#include "../src/render/keyframes.c"
#include "../src/resources/model_res_parts.c"
#include "../src/resources/model_res_base.c"
#ifdef TEST_RESOURCE_LOAD
#include "../src/resources/model_res_load.c"
#endif
#ifdef TEST_RENDER_NODES
#include "../src/world/model_node.c"
/* a node's mesh is built on its first draw, which these tests do not reach */
void gfx_mesh_destroy(gfx_mesh *mesh) { (void)mesh; abort(); }
#endif
#ifdef TEST_MODEL_ACTORS
#define xform actor_xform
#include "../src/world/model_actor.c"
#undef xform
#endif
#undef malloc
#undef calloc
#undef realloc
#undef free
#undef MODEL_TEST_FREE

#endif
