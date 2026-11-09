#ifndef WORLD_VIEW_TYPES_H
#define WORLD_VIEW_TYPES_H

#include <stdint.h>
#include "world/frustum.h"

struct World;
struct SkyModel;

struct View {
    float quakeSideways, quakeDistance, quakeLatitude;
    int quaking;
    uint32_t quakeStart, quakeDuration;
    float at[3];
    float latitude, longitude, distance;
    float destAt[3];
    float destLatitude, destLongitude, destDistance;
    float from[3], up[3];
    float viewMatrix[12], invViewMatrix[12];
    struct ViewFrustum frustum;
    struct World *world;
    struct SkyModel *sky;
    /* custom: the performance counter at the camera's last easing step (view_process_camera), 0
       before the first */
    int64_t easedAt;
};

#endif
