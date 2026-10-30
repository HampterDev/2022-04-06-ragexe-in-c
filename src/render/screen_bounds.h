#ifndef RENDER_SCREEN_BOUNDS_H
#define RENDER_SCREEN_BOUNDS_H

struct ScreenBounds {
    float left, top, right, bottom;
    float z;
};

void bounds_set(struct ScreenBounds *bounds, float left, float top,
                float right, float bottom, float z);

#endif
