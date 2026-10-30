#include "world/camera.h"
#include <string.h>
#include "resources/flare.h"

struct View *view_construct(struct View *self)
{
    memset(self, 0, sizeof *self);
    view_frustum_construct(&self->frustum);
    return self;
}

void view_destruct(struct View *self)
{
    void *obj;

    obj = self->sky;
    if (obj) {
        ui_edit_ctrl_wheel_on_wheel(obj);
        mem_free(obj);
        self->sky = NULL;
    }
    view_frustum_clear(&self->frustum);
}

void view_delete(struct View *self)
{
    view_destruct(self);
    mem_free(self);
}

void view_reset(struct View *self)
{
    self->longitude = 0;
    self->destLongitude = 0;
    self->distance = 300.0f;
    self->destDistance = 300.0f;
    self->latitude = -45.0f;
    self->destLatitude = -45.0f;
    self->at[0] = 0;
    self->at[1] = 0;
    self->at[2] = 0;
    self->destAt[0] = 0;
    self->destAt[1] = 0;
    self->destAt[2] = 0;
    self->up[0] = 0;
    self->up[1] = 1.0f;
    self->up[2] = 0;
}
