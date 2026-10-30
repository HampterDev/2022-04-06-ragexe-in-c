#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static struct { unsigned char *base; size_t size; } allocations[64];
static unsigned live_allocations, allocation_calls;
static int fail_next;

static void *guarded_malloc(size_t size)
{
    unsigned i;
    unsigned char *p;
    allocation_calls++;
    if (fail_next) { fail_next = 0; return NULL; }
    p = malloc(size + 32);
    assert(p);
    memset(p, 0xa5, size + 32);
    for (i = 0; i < 64 && allocations[i].base; ++i) {}
    assert(i < 64);
    allocations[i].base = p;
    allocations[i].size = size;
    live_allocations++;
    return p + 16;
}

static void guarded_free(void *memory)
{
    unsigned i, j;
    if (!memory) return;
    for (i = 0; i < 64 && (!allocations[i].base || allocations[i].base + 16 != memory); ++i) {}
    assert(i < 64);
    for (j = 0; j < 16; ++j) {
        assert(allocations[i].base[j] == 0xa5);
        assert(allocations[i].base[16 + allocations[i].size + j] == 0xa5);
    }
    free(allocations[i].base);
    allocations[i].base = NULL;
    live_allocations--;
}

static void *guarded_realloc(void *memory, size_t size)
{
    unsigned i;
    void *result = guarded_malloc(size);
    if (!result || !memory) return result;
    for (i = 0; i < 64 && (!allocations[i].base || allocations[i].base + 16 != memory); ++i) {}
    assert(i < 64);
    memcpy(result, memory, allocations[i].size < size ? allocations[i].size : size);
    guarded_free(memory);
    return result;
}

#define malloc guarded_malloc
#define realloc guarded_realloc
#define free guarded_free
#include "../src/render/display_devices.c"
#undef malloc
#undef realloc
#undef free

static const GUID adapter_id = {1, 2, 3, {4, 5, 6, 7, 8, 9, 10, 11}};
static const GUID device_id = {12, 13, 14, {15, 16, 17, 18, 19, 20, 21, 22}};

static void modes_and_failures(void)
{
    struct DisplayModeList list = {0};
    struct DisplayMode *original;
    size_t capacity;
    unsigned i;
    display_modes_sort(&list);
    display_modes_destroy(&list);
    fail_next = 1;
    assert(!display_modes_append(&list, 800, 600, 32));
    assert(!list.items && !list.count && !list.capacity);
    for (i = 0; i < 8; ++i)
        assert(display_modes_append(&list, 800 + i, 600, 32));
    original = list.items; capacity = list.capacity;
    fail_next = 1;
    assert(!display_modes_append(&list, 900, 600, 32));
    assert(list.items == original && list.count == 8 && list.capacity == capacity);
    for (i = 0; i < 8; ++i) assert(list.items[i].width == 800 + i);
    assert(display_modes_append(&list, 1024, 768, 16));
    assert(list.count == 9 && list.items[8].bpp == 16);
    display_modes_destroy(&list);
    display_modes_destroy(&list);
    assert(!live_allocations);
    list.count = list.capacity = SIZE_MAX / sizeof(struct DisplayMode);
    assert(!display_modes_append(&list, 1, 1, 1));
    assert(!list.items);
}

static void selection_and_ownership(void)
{
    struct DisplayModeList list = {0};
    const struct DisplayDevice *device;
    struct DisplayMode expected[] = {{800, 600, 16}, {800, 600, 32}, {1920, 1080, 32}};
    unsigned i, before;
    GUID changing = device_id;
    char long_name[80];
    memset(long_name, 'X', sizeof long_name - 1); long_name[sizeof long_name - 1] = 0;
    display_devices_reset();
    assert(display_devices_result() == (int)0x81000002u && !display_devices_at(0));
    assert(display_modes_append(&list, 1920, 1080, 32));
    assert(display_modes_append(&list, 800, 600, 24));
    assert(display_modes_append(&list, 800, 600, 8));
    assert(display_modes_append(&list, 800, 600, 32));
    assert(display_modes_append(&list, 800, 600, 16));
    display_modes_sort(&list);
    assert(list.items[0].bpp == 8 && list.items[1].bpp == 16 && list.items[2].bpp == 24);
    assert(!display_devices_add("adapter", &adapter_id, "software", &device_id, 0, 7, &list));
    assert(!display_devices_count() && display_devices_result() == (int)0x81000003u);
    assert(!display_devices_add("adapter", NULL, "no depth", &device_id, 1, 0, &list));
    fail_next = 1;
    assert(!display_devices_add("adapter", NULL, "no memory", &device_id, 1, 7, &list));
    assert(!display_devices_count() && live_allocations == 1);
    assert(display_devices_add("adapter", NULL, "primary software", &changing, 0, DISPLAY_DEPTH_24, &list));
    device = display_devices_at(0);
    assert(!device->hardware && device->mode_count == 1 && device->modes[0].bpp == 24);
    assert(!strcmp(device->name, "primary software") && !memcmp(&device->device_guid, &device_id, sizeof device_id));
    changing.Data1 = 999;
    assert(device->device_guid.Data1 == device_id.Data1);
    assert(!device->adapter_guid.Data1);
    assert(display_devices_add(long_name, &adapter_id, "device", &device_id, 1,
                               DISPLAY_DEPTH_16 | DISPLAY_DEPTH_32, &list));
    device = display_devices_at(1);
    assert(device->mode_count == 3 && !memcmp(device->modes, expected, sizeof expected));
    assert(strlen(device->name) == 38 && !memcmp(&device->adapter_guid, &adapter_id, sizeof adapter_id));
    memset(list.items, 0, list.count * sizeof *list.items);
    display_modes_destroy(&list);
    assert(!memcmp(device->modes, expected, sizeof expected));
    assert(display_devices_result() == 0 && !display_devices_at(2) && !display_devices_at(SIZE_MAX));
    display_devices_reset();
    display_devices_reset();
    assert(!live_allocations && !display_devices_count());

    assert(display_modes_append(&list, 1024, 768, 16));
    for (i = 0; i < 40; ++i) {
        changing.Data1 = i;
        before = allocation_calls;
        assert(display_devices_add("adapter", NULL, "device", &changing, 1, DISPLAY_DEPTH_16, &list) == (i < 20));
        if (i >= 20) assert(before == allocation_calls);
    }
    assert(display_devices_count() == 20 && display_devices_at(19)->device_guid.Data1 == 19);
    assert(!display_devices_at(20) && live_allocations == 21);
    display_devices_reset();
    assert(live_allocations == 1 && display_devices_result() == (int)0x81000002u);
    display_modes_destroy(&list);
    assert(!live_allocations);
}

int main(void)
{
    modes_and_failures();
    selection_and_ownership();
    puts("display devices: depth filtering, retained counts, capacity, allocation failure and owned reset passed");
    return 0;
}
