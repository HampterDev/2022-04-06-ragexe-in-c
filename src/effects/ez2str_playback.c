#include "render/rp_pool.h"
#include "resources/ezeffect.h"
#include <limits.h>
#include "fileio/resource_api.h"
#include "ui/window_access.h"
#include "effects/particle.h"
#include "effects/effect_math.h"
#include "game/context.h"
#include "render/render_api.h"
#include "ui/window_types.h"
#include <string.h>

void ez2_str_particle_rewind(struct Ez2StrParticle *self)
{
    self->anim = self->res ? &self->res->anim : NULL;
    self->layers_done = 0;
    self->frame = 0;
    self->layers_to_play = str_animation_rewind(self->anim, self->key_index,
                                               self->state, self->drawn);
}

static unsigned char particle_finished(struct Ez2StrParticle *self)
{
    self->base.life_seconds = -1.0f;
    return 0;
}

unsigned char ez2_str_particle_process(struct Ez2StrParticle *self, Vec3 offset)
{
    struct TexDrawVertex projected;
    Vec3 pos;
    float view[12];

    if (!self->res || !self->anim)
        return 0;
    if (self->start_delay > 0) {
        self->start_delay -= 16;
        return 1;
    }
    if (self->stop_frame != 0)
        self->stopping = 1;
    if (self->stopping == 1 && self->frame >= self->stop_frame)
        return particle_finished(self);

    if (self->layers_done >= self->layers_to_play) {
        if (!self->loop_forever && self->frame >= self->min_frames && self->frame >= self->stop_frame) {
            if (self->loops <= 0)
                return particle_finished(self);
            self->loops--;
        }
        ez2_str_particle_rewind(self);
    }

    pos = self->base.pos;
    if (game_copy_view_matrix(view)) {
        memset(&projected, 0, sizeof(projected));
        projected.specular = 0xff000000;
        renderer_project_vertex(g_renderer, &pos, view, &projected);
        self->screen_pos.x = projected.x + offset.x;
        self->screen_pos.y = projected.y + offset.y;
        self->screen_pos.z = projected.z + offset.z;
    } else if (ez2str_particle_is_ui(self)) {
        self->screen_pos.z = offset.z;
    } else {
        return particle_finished(self);
    }

    if (ez2str_particle_is_ui(self)) {
        float centre_y;
        self->screen_pos.x = (float)g_renderer->centreX + self->ui_offset.x;
        centre_y = (float)g_renderer->centreY;
        self->screen_pos.y = centre_y - centre_y * 0.2f + self->ui_offset.y;
        if (self->anchor_to_window == 1) {
            const struct UIWindow *wnd = ui_window_find(self->window_id);
            if (wnd) {
                self->visible = 1;
                self->screen_pos.x = (float)wnd->x + self->ui_offset.x;
                self->screen_pos.y = (float)wnd->y + self->ui_offset.y;
            } else {
                self->visible = 0;
            }
        }
    }

    self->layers_done += str_animation_step(self->anim, self->frame,
            self->key_index, self->state, self->drawn);

    if (self->frame < INT_MAX)
        self->frame++;
    if (self->stopping == 1)
        self->stop_frame = self->stop_frame >= INT_MIN + 16 ? self->stop_frame - 16 : INT_MIN;
    return 1;
}
