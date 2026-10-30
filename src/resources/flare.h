#include "render/rp_pool.h"
#ifndef RESOURCES_FLARE_H
#define RESOURCES_FLARE_H

#include "platform/error_message.h"
#include "std/string.h"
#include "resources/resource_ops.h"
#include "fileio/resource_api.h"
#include "fileio/res_mgr.h"
#include "resources/resource_factories.h"

struct FlareElem {
    int type;
    float pos;
    float size;
    float r, g, b;
    float world[3];
    float scaled;
};
SIZE_ASSERT(FlareElem, 0x28);

struct Flare {
    struct FlareElem elems[12];
    DWORD colors[12];
    char texName[0x104];
    float sun[3];
};
OFFSET_ASSERT(Flare, colors, 0x1e0);
OFFSET_ASSERT(Flare, sun, 0x314);
SIZE_ASSERT(Flare, 0x320);

struct SkyBox {
    struct TexDrawVertex corners[8];
    struct TexDrawVertex sun[2];
    float angle;
    struct Flare flare;
};
OFFSET_ASSERT(SkyBox, angle, 0x140);
SIZE_ASSERT(SkyBox, 0x464);

struct C3dActor;
struct SkyModel {
    struct C3dModelRes *model;
    struct C3dActor *actor;
    struct Flare flare;
};

void ui_edit_ctrl_wheel_on_wheel(struct SkyModel *self);

#endif
