#include "world/model_types.h"
#include "resources/model_res.h"
#include "render/projection.h"
#include "render/rp_pool.h"
#include "render/texture_cache.h"
#include "actor/weather.h"
#include <math.h>
#include <string.h>
#include "render/render_api.h"
#include "render/math3d.h"
#include "resources/flare.h"

void ui_edit_ctrl_wheel_on_wheel(struct SkyModel *self)
{
    if (self->actor) {
        self->actor->ops->destroy(self->actor);
        self->actor = NULL;
    }
}
