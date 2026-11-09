#define malloc checked_malloc
#define free checked_free
#include "../src/actor/granny_projection.c"
#undef malloc
#undef free

static void granny_projection_ownership(void)
{
    struct Renderer renderer = {0};
    struct GrannyProjection *projection;
    struct TexDrawVertex *vertices;
    struct TexDrawVertex first_vertices[6], second_vertices[6];
    struct RPQuad *first, *second;
    struct Texture *texture;
    size_t pass;
    unsigned before, i;

    before = allocation_calls;
    assert(!granny_projection_create(0) && !granny_projection_create(-1));
    assert(!granny_projection_create(INT_MAX) && !granny_projection_create(INT_MAX / 3));
    assert(allocation_calls == before);
    fail_at = allocation_calls + 1;
    assert(!granny_projection_create(2));
    fail_at = 0;
    assert(!outstanding);
    projection = granny_projection_create(2);
    assert(projection);
    vertices = granny_projection_vertices(projection, 2);
    assert(vertices && !granny_projection_vertices(projection, 3));
    assert(!granny_projection_vertices(projection, 0) && !granny_projection_vertices(projection, -1));
    assert(!granny_projection_vertices(NULL, 1));
    for (i = 0; i < 6; ++i) {
        struct TexDrawVertex expected = {0};
        expected.specular = 0xff000000;
        assert(!memcmp(&vertices[i], &expected, sizeof expected));
        vertices[i].x = (float)i + 1;
        vertices[i].y = (float)i - 20;
        vertices[i].z = 0.25f;
        vertices[i].rhw = 0.00125f;
        vertices[i].diffuse = 0xff123456 + i;
        vertices[i].u = (float)i / 8;
        vertices[i].v = (float)i / 16;
    }
    memcpy(first_vertices, vertices, sizeof first_vertices);
    assert(renderer_construct(&renderer)); g_renderer = &renderer;
    texture = texture_create(1, 1, GFX_FMT_8888);
    granny_projection_submit(&renderer, projection, texture, 2, 0);
    assert(renderer.queues[RENDER_OPAQUE_CLAMP].count == 1);
    first = renderer.queues[RENDER_OPAQUE_CLAMP].items[0].primitive;
    assert(first->verts != vertices && first->nverts == 6 && first->alpha_mode == 1 && !first->world);
    assert(first->prim == GFX_PT_TRIANGLELIST && !first->indices && !first->nindices);
    assert(!first->srcBlend && !first->destBlend && !first->stageMode);
    assert(!memcmp(first->verts, first_vertices, sizeof first_vertices));
    for (i = 0; i < 6; ++i) vertices[i].x += 100;
    memcpy(second_vertices, vertices, sizeof second_vertices);
    granny_projection_submit(&renderer, projection, texture, 2, 0);
    second = renderer.queues[RENDER_OPAQUE_CLAMP].items[1].primitive;
    assert(second != first && second->verts != first->verts && second->verts != vertices);
    assert(!memcmp(first->verts, first_vertices, sizeof first_vertices));
    assert(!memcmp(second->verts, second_vertices, sizeof second_vertices));
    before = allocation_calls;
    granny_projection_submit(&renderer, projection, texture, 0, 0);
    granny_projection_submit(&renderer, projection, texture, 3, 0);
    granny_projection_submit(&renderer, projection, NULL, 2, 0);
    granny_projection_submit(&renderer, NULL, texture, 2, 0);
    assert(allocation_calls == before && renderer.queues[RENDER_OPAQUE_CLAMP].count == 2);
    granny_projection_destroy(projection);
    texture_destroy(texture);
    projection_draw_count = 0; capture_projection = 1;
    renderer_draw_lists(&renderer);
    capture_projection = 0;
    assert(projection_draw_count == 2);
    assert(!memcmp(projection_draws[0], first_vertices, sizeof first_vertices));
    assert(!memcmp(projection_draws[1], second_vertices, sizeof second_vertices));
    renderer_destroy_all_rp_list(&renderer);
    assert(!live_surfaces);

    projection = granny_projection_create(1);
    vertices = granny_projection_vertices(projection, 1);
    texture = texture_create(1, 1, GFX_FMT_8888);
    renderer.fogVertexCaps = 1;
    for (i = 0; i < 3; ++i) vertices[i].rhw = 0.00125f;
    for (i = 0; i < 8300; ++i) {
        vertices[0].x = (float)i;
        granny_projection_submit(&renderer, projection, texture, 1, 0);
    }
    assert(renderer.queues[RENDER_OPAQUE_CLAMP].count == 8300);
    first = renderer.queues[RENDER_OPAQUE_CLAMP].items[0].primitive;
    second = renderer.queues[RENDER_OPAQUE_CLAMP].items[8299].primitive;
    assert(first->verts[0].x == 0 && second->verts[0].x == 8299);
    assert(first->verts[0].specular == (DWORD)rp_vertex_fog_alpha(0.00125f) << 24);
    granny_projection_destroy(projection); texture_destroy(texture);
    renderer_destroy_all_rp_list(&renderer); renderer_destroy_all_rp_list(&renderer);
    projection = granny_projection_create(1);
    texture = texture_create(1, 1, GFX_FMT_8888);
    vertices = granny_projection_vertices(projection, 1);
    for (i = 0; i < 3; ++i) vertices[i].rhw = 0.00125f;
    before = allocation_calls;
    granny_projection_submit(&renderer, projection, texture, 1, 0);
    assert(allocation_calls == before);
    granny_projection_destroy(projection); granny_projection_destroy(NULL); texture_destroy(texture);
    for (pass = 1; pass < RENDER_PASS_COUNT; ++pass) assert(!renderer.queues[pass].count);
    renderer_destruct(&renderer); renderer_destruct(&renderer);
    assert(!outstanding && !live_surfaces && !g_renderer);
    puts("Granny projections: C ownership, bounded sizes, cached redraw, independent queued vertices, actual draws after destruction, fog, 8300 submissions, skipped frames and reuse passed.");
}
