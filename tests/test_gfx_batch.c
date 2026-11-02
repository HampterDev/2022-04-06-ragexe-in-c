#include <assert.h>
#include <stdio.h>
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
    BYTE vertices[4 * 32] = {0};
    assert(batch_add(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, vertices, 4, NULL, 0));
}

int main(void)
{
    HANDLE thread;

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
    puts("Draw batch: another thread neither draws nor drops the render thread's batch passed");
    return 0;
}
