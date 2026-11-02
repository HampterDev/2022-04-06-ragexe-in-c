/* Windows scene runs: Granny's clock is the scene clock. The client times its 3D models with
   granny2.dll's _GrannyGetSystemSeconds and _GrannyGetSecondsElapsed; their import slots are
   pointed at the virtual clock, so the models animate alike in every run and on every platform
   (the POSIX runtime, src/resources/granny_gltf.c, already uses platform_perf_counter). */
#include "platform/win_types.h"
#include "platform/clock.h"
#include "resources/granny_sdk.h"
#include "scene.h"
#include <string.h>

static GrannyClock __stdcall scene_granny_seconds(void)
{
    GrannyClock clock;
    int64_t now = platform_perf_counter();
    memset(&clock, 0, sizeof clock);
    memcpy(clock.bytes, &now, sizeof now);
    return clock;
}

static float __stdcall scene_granny_elapsed(const GrannyClock *start, const GrannyClock *end)
{
    int64_t a, b;
    memcpy(&a, start->bytes, sizeof a);
    memcpy(&b, end->bytes, sizeof b);
    return (float)((double)(b - a) / (double)platform_perf_frequency());
}

static void patch_imports(HMODULE module, const char *dll)
{
    BYTE *base = (BYTE *)module;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
    IMAGE_DATA_DIRECTORY dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    IMAGE_IMPORT_DESCRIPTOR *d;
    if (!dir.VirtualAddress)
        return;
    for (d = (IMAGE_IMPORT_DESCRIPTOR *)(base + dir.VirtualAddress); d->Name; d++) {
        IMAGE_THUNK_DATA *names, *slots;
        if (lstrcmpiA((const char *)(base + d->Name), dll))
            continue;
        names = (IMAGE_THUNK_DATA *)(base + (d->OriginalFirstThunk ? d->OriginalFirstThunk : d->FirstThunk));
        slots = (IMAGE_THUNK_DATA *)(base + d->FirstThunk);
        for (; names->u1.AddressOfData; names++, slots++) {
            const char *name;
            void *replacement = NULL;
            DWORD old;
            if (IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal))
                continue;
            name = (const char *)((IMAGE_IMPORT_BY_NAME *)(base + names->u1.AddressOfData))->Name;
            if (!strcmp(name, "_GrannyGetSystemSeconds@0"))
                replacement = (void *)scene_granny_seconds;
            else if (!strcmp(name, "_GrannyGetSecondsElapsed@8"))
                replacement = (void *)scene_granny_elapsed;
            if (!replacement || !VirtualProtect(&slots->u1.Function, sizeof slots->u1.Function, PAGE_READWRITE, &old))
                continue;
            slots->u1.Function = (ULONG_PTR)replacement;
            VirtualProtect(&slots->u1.Function, sizeof slots->u1.Function, old, &old);
        }
    }
}

void scene_granny_clock_install(void)
{
    patch_imports(GetModuleHandleA(NULL), "granny2.dll");
}
