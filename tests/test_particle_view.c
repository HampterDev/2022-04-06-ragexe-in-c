#include "effects/particle.h"
#include "game/context.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static float camera[12];
static struct Ez2StrParticle particle;
static int camera_present, copies, draws;

int game_copy_view_matrix(float view[12])
{
    ++copies;
    if (!camera_present)
        return 0;
    memcpy(view, camera, sizeof camera);
    return 1;
}

void ez2_str_particle_render(struct Ez2StrParticle *self, const float *view)
{
    assert(self == &particle && view != camera);
    if (camera_present)
        assert(memcmp(view, camera, sizeof camera) == 0);
    else
        assert(ez2str_particle_is_ui(self) && view == NULL);
    ++draws;
}

int main(void)
{
    int i;
    ez2_str_particle_render_current(NULL);
    assert(copies == 0 && draws == 0);
    ez2_str_particle_render_current(&particle);
    assert(copies == 1 && draws == 0);
    for (i = 0; i < 12; ++i)
        camera[i] = (float)i * 0.25f - 10.0f;
    camera_present = 1;
    ez2_str_particle_render_current(&particle);
    assert(copies == 2 && draws == 1);
    camera[9] = 17.0f;
    ez2_str_particle_render_current(&particle);
    assert(copies == 3 && draws == 2);
    camera_present = 0;
    ez2_str_particle_render_current(&particle);
    assert(copies == 4 && draws == 2);
    particle.base.list_flags = PARTICLE_LIST_UI_SCREEN;
    ez2_str_particle_render_current(&particle);
    assert(copies == 5 && draws == 3);
    particle.base.list_flags = PARTICLE_LIST_UI_WINDOW;
    ez2_str_particle_render_current(&particle);
    assert(copies == 6 && draws == 4);
    puts("particle view: absent camera, copied matrices, updates and teardown passed");
    return 0;
}
