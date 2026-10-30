#include "effects/particle.h"
#include "game/context.h"

void ez2_str_particle_render_current(struct Ez2StrParticle *self)
{
    float view[12];
    if (!self)
        return;
    if (game_copy_view_matrix(view))
        ez2_str_particle_render(self, view);
    else if (ez2str_particle_is_ui(self))
        ez2_str_particle_render(self, NULL);
}
