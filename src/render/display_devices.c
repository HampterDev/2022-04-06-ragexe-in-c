#include "render/display_devices.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DISPLAY_DEVICES 20

static struct DisplayDevice devices[MAX_DISPLAY_DEVICES];
static size_t device_count;
static size_t reported_devices;

int display_modes_append(struct DisplayModeList *list, unsigned width, unsigned height, unsigned bpp)
{
    if (list->count == list->capacity) {
        size_t capacity;
        struct DisplayMode *items;
        if (list->capacity > SIZE_MAX / 2)
            return 0;
        capacity = list->capacity ? list->capacity * 2 : 8;
        if (capacity > SIZE_MAX / sizeof *items)
            return 0;
        items = realloc(list->items, capacity * sizeof *items);
        if (!items)
            return 0;
        list->items = items;
        list->capacity = capacity;
    }
    list->items[list->count].width = width;
    list->items[list->count].height = height;
    list->items[list->count].bpp = bpp;
    list->count++;
    return 1;
}

static int compare_modes(const void *left, const void *right)
{
    const struct DisplayMode *a = left, *b = right;
    if (a->width != b->width)
        return a->width < b->width ? -1 : 1;
    if (a->height != b->height)
        return a->height < b->height ? -1 : 1;
    if (a->bpp != b->bpp)
        return a->bpp < b->bpp ? -1 : 1;
    return 0;
}

void display_modes_sort(struct DisplayModeList *list)
{
    if (list->count > 1)
        qsort(list->items, list->count, sizeof *list->items, compare_modes);
}

void display_modes_destroy(struct DisplayModeList *list)
{
    free(list->items);
    memset(list, 0, sizeof *list);
}

static int supports_depth(unsigned depths, unsigned bpp)
{
    return (bpp == 16 && (depths & DISPLAY_DEPTH_16))
        || (bpp == 24 && (depths & DISPLAY_DEPTH_24))
        || (bpp == 32 && (depths & DISPLAY_DEPTH_32));
}

int display_devices_add(const char *adapter_name, const GUID *adapter_guid,
                        const char *device_name, const GUID *device_guid,
                        int hardware, unsigned depths, const struct DisplayModeList *modes)
{
    struct DisplayDevice *device;
    struct DisplayMode *selected;
    size_t count = 0, i, name_length;
    const char *name;

    reported_devices++;
    if (device_count == MAX_DISPLAY_DEVICES || (adapter_guid && !hardware))
        return 0;
    for (i = 0; i < modes->count; ++i)
        if (supports_depth(depths, modes->items[i].bpp))
            count++;
    if (!count || count > SIZE_MAX / sizeof *selected)
        return 0;
    selected = malloc(count * sizeof *selected);
    if (!selected)
        return 0;
    count = 0;
    for (i = 0; i < modes->count; ++i)
        if (supports_depth(depths, modes->items[i].bpp))
            selected[count++] = modes->items[i];

    device = &devices[device_count++];
    memset(device, 0, sizeof *device);
    name = adapter_guid ? adapter_name : device_name;
    name_length = strlen(name);
    if (name_length >= sizeof device->name)
        name_length = sizeof device->name - 1;
    memcpy(device->name, name, name_length);
    device->device_guid = *device_guid;
    if (adapter_guid)
        device->adapter_guid = *adapter_guid;
    device->hardware = hardware != 0;
    device->modes = selected;
    device->mode_count = count;
    return 1;
}

size_t display_devices_count(void)
{
    return device_count;
}

const struct DisplayDevice *display_devices_at(size_t index)
{
    return index < device_count ? &devices[index] : NULL;
}

int display_devices_result(void)
{
    if (!reported_devices)
        return (int)0x81000002u;
    return device_count ? 0 : (int)0x81000003u;
}

void display_devices_reset(void)
{
    size_t i;
    for (i = 0; i < device_count; ++i)
        free(devices[i].modes);
    memset(devices, 0, sizeof devices);
    device_count = 0;
    reported_devices = 0;
}
