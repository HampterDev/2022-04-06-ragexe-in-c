#include "render/rp_pool.h"
#include "resources/ezeffect.h"
#include "effects/particle.h"
#include "render/render_api.h"
#include "ui/window_types.h"
#include "ui/window_access.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static struct Renderer renderer;
struct Renderer *g_renderer = &renderer;
static struct Ez2StrParticle particle;
static struct EzEffectRes resource;
static struct StrKey keys[2];
static struct UIWindow window;
static int camera_present, window_present, projections;

int game_copy_view_matrix(float view[12])
{
    int i;
    if (!camera_present)
        return 0;
    for (i = 0; i < 12; ++i)
        view[i] = (float)i;
    return 1;
}

void renderer_project_vertex(struct Renderer *r, const Vec3 *pos,
                             const float *view, struct TexDrawVertex *out)
{
    assert(r == &renderer && camera_present && view[11] == 11.0f);
    assert(pos->x == 2.0f && pos->y == 3.0f && pos->z == 4.0f);
    ++projections;
    out->x = 20.0f;
    out->y = 30.0f;
    out->z = 40.0f;
}

struct UIWindow *ui_window_find(int id)
{
    assert(id == 42);
    return window_present ? &window : NULL;
}

void res_lock(struct Res *res)
{
    (void)res;
    assert(!"these frames must not rewind");
}

static void prepare(int list)
{
    memset(&particle, 0, sizeof particle);
    particle.res = &resource;
    particle.anim = &resource.anim;
    particle.layers_to_play = 1;
    particle.visible = 1;
    particle.base.list_flags = list;
    particle.base.pos.x = 2.0f;
    particle.base.pos.y = 3.0f;
    particle.base.pos.z = 4.0f;
    particle.ui_offset.x = 8.0f;
    particle.ui_offset.y = 6.0f;
    particle.window_id = 42;
}

int main(void)
{
    Vec3 offset = { 1.0f, -80.0f, 2.0f };
    resource.anim.layer_count = 1;
    resource.anim.layers[0].key_count = 2;
    resource.anim.layers[0].keys = keys;
    keys[0].s.x = 12.0f;
    keys[1].frame = 100;
    renderer.centreX = 320;
    renderer.centreY = 240;
    window.x = 15;
    window.y = 25;

    prepare(5);
    assert(!ez2_str_particle_process(&particle, offset));
    assert(particle.base.life_seconds == -1.0f && particle.frame == 0);
    assert(projections == 0);

    camera_present = 1;
    prepare(5);
    assert(ez2_str_particle_process(&particle, offset));
    assert(particle.screen_pos.x == 21.0f && particle.screen_pos.y == -50.0f);
    assert(particle.screen_pos.z == 42.0f && particle.frame == 1);
    assert(particle.drawn[0] && particle.state[0].x == 12.0f);
    assert(projections == 1);
    camera_present = 0;
    assert(!ez2_str_particle_process(&particle, offset));
    assert(particle.base.life_seconds == -1.0f && particle.frame == 1);

    prepare(0x205);
    assert(ez2_str_particle_process(&particle, offset));
    assert(particle.screen_pos.x == 328.0f && particle.screen_pos.y == 198.0f);
    assert(particle.screen_pos.z == offset.z && particle.frame == 1);
    assert(particle.drawn[0] && projections == 1);

    prepare(0x225);
    particle.anchor_to_window = 1;
    window_present = 1;
    assert(ez2_str_particle_process(&particle, offset));
    assert(particle.visible && particle.screen_pos.x == 23.0f && particle.screen_pos.y == 31.0f);
    window_present = 0;
    assert(ez2_str_particle_process(&particle, offset));
    assert(!particle.visible && particle.frame == 2);
    window_present = 1;
    window.x = -40;
    camera_present = 1;
    assert(ez2_str_particle_process(&particle, offset));
    assert(particle.visible && particle.screen_pos.x == -32.0f);
    assert(particle.screen_pos.z == 42.0f && particle.frame == 3 && projections == 2);
    puts("particle process: world teardown, camera-free UI animation and window anchoring passed");
    return 0;
}
