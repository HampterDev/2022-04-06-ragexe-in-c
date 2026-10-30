#include "render/texture_globals.h"
#ifndef RESOURCES_EZEFFECT_H
#define RESOURCES_EZEFFECT_H

#include "platform/error_message.h"
#include "std/string.h"
#include "resources/resource_ops.h"
#include "fileio/resource_api.h"
#include "fileio/res_mgr.h"
#include "resources/resource_factories.h"

#include "resources/str_data.h"

#define STR_VERSION 0x94

struct EzEffectRes {
    struct Res res;
    struct StrAnim anim;
    struct File file;
};

BYTE e_zeffect_res_load(struct EzEffectRes *self, const char *name);
void effect_resource_clear(struct EzEffectRes *self);
void effect_resource_finish(struct EzEffectRes *self);

#include "render/render_api.h"

#endif
