#include "../src/effects/skills/heal_effect.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

float g_degreeTables[6 * 361];

void *mem_alloc(size_t bytes)
{
    void *p = malloc(bytes);
    assert(p);
    return p;
}
void mem_free(void *p) { free(p); }
void *mem_try_alloc(size_t bytes) { (void)bytes; abort(); }
__declspec(noreturn) void fatal_error(const char *message) { (void)message; abort(); }
const char *game_current_map_name(void) { return "prontera.rsw"; }
float game_camera_pitch(void) { return -45.0f; }
float game_camera_heading(void) { return 0.0f; }
void effect_add_sound_cue(struct Effect *self, int kind, const char *wave, int frame, float height)
{ (void)self; (void)kind; (void)wave; (void)frame; (void)height; abort(); }
unsigned char effect_is_within_duration(struct Effect *self) { (void)self; abort(); }
void particle_quad_unit_corners(struct TexDrawVertex *quad) { (void)quad; }
void particle_render(struct Particle *self, unsigned char facing_fixed, float v_top, float v_bottom,
                     float u_left, float u_right, unsigned char with_depth)
{ (void)self; (void)facing_fixed; (void)v_top; (void)v_bottom; (void)u_left; (void)u_right; (void)with_depth; abort(); }
void particle_render_with(struct Particle *self, const struct ParticleRenderArgs *args)
{ (void)self; (void)args; abort(); }

void string_assign_cstr(struct String *s, const char *text)
{
    size_t length = strlen(text);
    free(s->data);
    s->data = mem_alloc(length + 1);
    memcpy(s->data, text, length + 1);
    s->size = s->capacity = length;
}

#define MAX_PARTICLES 128
struct Record {
    struct Particle *p;
    char first_texture[64];
    int colours, r, g, b;
};
static struct Record records[MAX_PARTICLES];
static int nrecords, live;

static struct Record *record_of(const struct Particle *p)
{
    int i;
    for (i = 0; i < nrecords; i++)
        if (records[i].p == p)
            return &records[i];
    abort();
}

struct Particle *particle_new(Vec3 pos, int list_flags)
{
    struct Particle *p = calloc(1, sizeof *p);
    assert(p && list_flags == HEAL_LIST && nrecords < MAX_PARTICLES);
    p->pos = pos;
    memset(&records[nrecords], 0, sizeof records[nrecords]);
    records[nrecords++].p = p;
    live++;
    return p;
}
void particle_set_texture(struct Particle *self, struct String name)
{
    struct Record *record = record_of(self);
    if (!record->first_texture[0]) {
        assert(strlen(name.data) < sizeof record->first_texture);
        strcpy(record->first_texture, name.data);
    }
    free(name.data);
}
void particle_set_color(struct Particle *self, short r, short g, short b)
{
    struct Record *record = record_of(self);
    record->colours++;
    record->r = r; record->g = g; record->b = b;
}
void particle_free(struct Particle *p)
{
    record_of(p);
    free(p);
    live--;
}

struct Expected {
    int type;
    const char *column_texture;
    int column_r, column_g, column_b;
    int sparkle_r, sparkle_g, sparkle_b;
};

static void check(const struct Vec *particles, int count, const char *texture, int r, int g, int b)
{
    struct Particle **it;
    int n = 0;
    for (it = particles->first; it != (struct Particle **)particles->last; it++, n++) {
        struct Record *record = record_of(*it);
        assert(!strcmp(record->first_texture, texture));
        assert(record->r == r && record->g == g && record->b == b);
    }
    assert(n == count);
}

static void run(const struct Expected *expected)
{
    static struct HealEffect effect;
    int frame;

    memset(&effect, 0, sizeof effect);
    nrecords = 0;
    effect.base.type = expected->type;
    effect.base.pos.x = 10.0f;
    effect.base.pos.z = 20.0f;
    for (frame = 0; frame < HEAL_DURATION; frame++)
        heal_effect_process(&effect.base);
    if (vec_count(&effect.columns, sizeof(struct Particle *)) == 0) {
        printf("Heal type 0x%x drew nothing\n", expected->type);
        exit(1);
    }
    check(&effect.columns, 2 * STRIP_QUADS, expected->column_texture, expected->column_r,
          expected->column_g, expected->column_b);
    check(&effect.cones, 0, "", 0, 0, 0);
    check(&effect.sparkles, 9 * SPARKLES_PER_SPAWN, "effect\\pok1.tga", expected->sparkle_r,
          expected->sparkle_g, expected->sparkle_b);
    heal_effect_release(&effect.base);
    vec_free(&effect.columns);
    vec_free(&effect.cones);
    vec_free(&effect.sparkles);
    assert(!live);
}

int main(void)
{
    static const struct Expected cases[] = {
        { 0x138, "effect\\alpha_down.tga", 140, 210, 140, 220, 250, 220 },
        { 0x459, "effect\\ring_white.tga", 115, 25, 115, 220, 250, 220 },
        { 0x477, "effect\\alpha_down.tga", 0, 255, 255, 0, 255, 255 },
        { 0x478, "effect\\alpha_down.tga", 255, 255, 0, 255, 255, 0 },
    };
    size_t i;

    for (i = 0; i < sizeof cases / sizeof cases[0]; i++)
        run(&cases[i]);
    puts("Heal colours: 0x459 purple, 0x477 cyan and 0x478 yellow spawn their columns and sparkles, as 0x138 does in green.");
    return 0;
}
