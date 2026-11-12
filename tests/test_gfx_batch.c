#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "render/gfx.h"
#include "render/gfx_batch.h"

static int draws, drawn_vertices, drawn_indices;

static void count_draw(int prim, int vertexKind, const void *verts, int nverts, const WORD *indices, int nindices)
{
    (void)prim;
    (void)vertexKind;
    (void)verts;
    (void)indices;
    ++draws;
    drawn_vertices = nverts;
    drawn_indices = nindices;
}

static DWORD WINAPI other_thread(void *context)
{
    (void)context;
    batch_flush();
    batch_drop();
    return 0;
}

static void add_quad(void)
{
    BYTE vertices[4 * GFX_VTX_TEX1_SIZE] = {0};
    assert(batch_add(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, vertices, 4, NULL, 0));
}

/* A strip's triangles in one order for the batch and for the device's own strips (a blit, a clear,
   a strip too long for the batch): the odd ones (t + 1, t, t + 2). */
static void strip_order(void)
{
    static const WORD plain[12] = {0, 1, 2, 2, 1, 3, 2, 3, 4, 4, 3, 5};
    static const WORD indices[6] = {9, 8, 7, 6, 5, 4};
    static const WORD indexed[12] = {109, 108, 107, 107, 108, 106, 107, 106, 105, 105, 106, 104};
    WORD out[16];
    int i;
    memset(out, 0xee, sizeof out);
    batch_strip_list(out, 6, NULL, 0);
    for (i = 0; i < 12; i++)
        assert(out[i] == plain[i]);
    assert(out[12] == 0xeeee);
    batch_strip_list(out, 6, indices, 100);
    for (i = 0; i < 12; i++)
        assert(out[i] == indexed[i]);
    memset(out, 0xee, sizeof out);
    batch_strip_list(out, 2, NULL, 0);
    batch_strip_list(out, 0, NULL, 0);
    assert(out[0] == 0xeeee);
}

int main(void)
{
    HANDLE thread;

    strip_order();
    batch_init(count_draw);
    add_quad();
    add_quad();
    thread = CreateThread(NULL, 0, other_thread, NULL, 0, NULL);
    assert(thread);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    assert(draws == 0);
    batch_flush();
    assert(draws == 1 && drawn_vertices == 8 && drawn_indices == 12);
    add_quad();
    batch_drop();
    batch_flush();
    assert(draws == 1);
    puts("Draw batch: strips' triangles in the device's order, and another thread neither draws nor drops the render "
         "thread's batch passed");
    return 0;
}
