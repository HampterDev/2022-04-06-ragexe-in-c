#ifndef DISPLAY_DEVICES_H
#define DISPLAY_DEVICES_H

#include <stddef.h>
#include "platform/win_types.h"

struct DisplayMode {
    unsigned width, height, bpp;
};

struct DisplayModeList {
    struct DisplayMode *items;
    size_t count, capacity;
};

struct DisplayDevice {
    char name[39];
    GUID device_guid, adapter_guid;
    int hardware;
    struct DisplayMode *modes;
    size_t mode_count;
};

enum DisplayDepth {
    DISPLAY_DEPTH_16 = 1,
    DISPLAY_DEPTH_24 = 2,
    DISPLAY_DEPTH_32 = 4
};

int display_modes_append(struct DisplayModeList *list, unsigned width, unsigned height, unsigned bpp);
void display_modes_sort(struct DisplayModeList *list);
void display_modes_destroy(struct DisplayModeList *list);

int display_devices_add(const char *adapter_name, const GUID *adapter_guid,
                        const char *device_name, const GUID *device_guid,
                        int hardware, unsigned depths, const struct DisplayModeList *modes);
size_t display_devices_count(void);
const struct DisplayDevice *display_devices_at(size_t index);
int display_devices_result(void);
void display_devices_reset(void);

int render_enum_adapters(void);

#endif
