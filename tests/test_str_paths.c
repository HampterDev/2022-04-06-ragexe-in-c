#include "render/rp_pool.h"
#include "effects/rag/rag_effect.h"
#include "resources/ezeffect.h"
#include "effects/particle.h"
#include "effects/str_path.h"
#include "fileio/resource_api.h"
#include "session/state.h"
#include "world_state_memory.h"
#include <stdio.h>

struct Session g_session1;
static int manager;
static struct EzEffectRes resource;
static unsigned lookups, releases, probes, locks;
static int resource_missing;
static int min_exists;
static char last_lookup[512], last_probe[512];

struct ResMgr *g_resMgr(void) { return (struct ResMgr *)&manager; }
void *res_mgr_get(struct ResMgr *self, const char *path)
{
    assert((void *)self == &manager);
    assert(strlen(path) < sizeof last_lookup);
    strcpy(last_lookup, path);
    ++lookups;
    return resource_missing ? NULL : &resource;
}
unsigned char res_mgr_is_available(struct ResMgr *self, const char *path)
{
    assert((void *)self == &manager);
    assert(strlen(path) < sizeof last_probe);
    strcpy(last_probe, path);
    ++probes;
    return (unsigned char)min_exists;
}
void res_lock(struct Res *self)
{ assert(self == &resource.res); ++locks; }
void res_unlock(struct Res *self)
{ assert((void *)self == &resource); ++releases; }
struct Particle *particle_construct(struct Particle *self, Vec3 pos, int flags)
{ memset(self, 0, sizeof *self); self->pos = pos; self->list_flags = flags; return self; }
void particle_reset(struct Particle *self) { (void)self; }
struct Renderer *g_renderer;
int game_copy_view_matrix(float view[12]) { (void)view; abort(); }
void renderer_project_vertex(struct Renderer *r, const Vec3 *p, const float *view, struct TexDrawVertex *out)
{ (void)r; (void)p; (void)view; (void)out; abort(); }
struct UIWindow *ui_window_find(int id) { (void)id; abort(); }
struct TexDrawVertex *prim_project_vertex(struct TexDrawVertex *out, const float *view, Vec3 pos)
{ (void)out; (void)view; (void)pos; abort(); }

static void check_min(const char *path, const char *candidate, int exists)
{
    struct String out;
    unsigned before = probes;
    min_exists = exists;
    ez2str_min_path(&out, path);
    assert(!strcmp(string_cstr(&out), candidate && exists ? candidate : path));
    assert(probes == before + (candidate != NULL));
    if (candidate) assert(!strcmp(last_probe, candidate));
    string_free(&out);
    assert(live_blocks == 0);
}

int main(void)
{
    struct Ez2StrParticle particle = {0};
    char borrowed[] = "effects\\mutable_input.str";
    unsigned before;
    check_min("effects\\fire.str", "effects\\min_fire.str", 1);
    check_min("effects\\fire.str", "effects\\min_fire.str", 0);
    check_min("a\\b\\name_01.str", "a\\b\\min_name_01.str", 1);
    check_min("a\\b.c\\name.str", "a\\b.c\\min_name.str", 1);
    check_min("fire.str", NULL, 1);
    check_min("\\fire.str", NULL, 1);
    check_min("effects\\bad-name.str", NULL, 1);
    check_min("effects\\fire.STR", NULL, 1);
    check_min("effects\\.str", NULL, 1);
    check_min("a\nb\\fire.str", NULL, 1);
    check_min("a\rb\\fire.str", NULL, 1);
    check_min("", NULL, 1);
    string_init(&particle.path);
    string_init(&particle.texture_set);
    g_session1.map_rule_flags[0] = 0;
    before = probes;
    ez2_str_particle_open(&particle, borrowed);
    borrowed[0] = 'X';
    assert(!strcmp(string_cstr(&particle.path), "effects\\mutable_input.str"));
    assert(!strcmp(string_cstr(&particle.texture_set), "effects\\"));
    assert(probes == before && lookups == 1 && releases == 0 && locks == 1);
    g_session1.map_rule_flags[0] = 1;
    min_exists = 1;
    ez2_str_particle_open(&particle, "effects\\fire.str");
    assert(!strcmp(last_lookup, "effects\\min_fire.str"));
    assert(!strcmp(string_cstr(&particle.texture_set), "effects\\"));
    min_exists = 0;
    ez2_str_particle_open(&particle, "effects\\fire.str");
    assert(!strcmp(last_lookup, "effects\\fire.str"));
    ez2_str_particle_load(&particle, string_cstr(&particle.path));
    assert(!strcmp(string_cstr(&particle.path), "effects\\fire.str"));
    assert(lookups == 4 && releases == 3 && locks == 4);
    assert(particle.anim == &resource.anim);
    before = locks;
    ez2_str_particle_rewind(&particle);
    ez2_str_particle_rewind(&particle);
    assert(locks == before && releases == 3 && particle.frame == 0);
    ez2_str_particle_reset(&particle);
    assert(releases == 4 && !particle.res && !particle.anim);
    ez2_str_particle_reset(&particle);
    assert(releases == 4);
    ez2_str_particle_load(&particle, "again.str");
    assert(locks == 5);
    resource_missing = 1;
    ez2_str_particle_load(&particle, "missing.str");
    assert(releases == 5 && locks == 5 && !particle.res && !particle.anim);
    resource_missing = 0;
    ez2_str_particle_load(&particle, "final.str");
    ez2_str_particle_destruct(&particle);
    assert(releases == 6 && locks == 6 && !particle.res && !particle.anim);
    assert(live_blocks == 0);
    memset(&particle, 0xa5, sizeof particle);
    {
        Vec3 pos = {0};
        ez2_str_particle_construct(&particle, pos, 5);
    }
    assert(!particle.res && releases == 6);
    ez2_str_particle_load(&particle, "constructed.str");
    ez2_str_particle_destruct(&particle);
    assert(locks == 7 && releases == 7 && live_blocks == 0);
    {
        struct RagEffect *effect = calloc(1, sizeof *effect);
        assert(effect);
        rag_str_load(effect, "legacy.str");
        assert(effect->str_res == &resource && effect->str_anim == &resource.anim);
        assert(locks == 8 && releases == 7);
        rag_str_rewind(effect); rag_str_rewind(effect);
        assert(locks == 8 && releases == 7);
        rag_str_load(effect, "legacy-again.str");
        assert(locks == 9 && releases == 8);
        resource_missing = 1;
        rag_str_load(effect, "missing.str");
        assert(locks == 9 && releases == 9 && !effect->str_res && !effect->str_anim);
        resource_missing = 0;
        rag_str_load(effect, "legacy-final.str");
        rag_str_unload(effect); rag_str_unload(effect);
        assert(locks == 10 && releases == 10 && !effect->str_res && !effect->str_anim);
        free(effect);
    }
    puts("STR ownership: load, rewind, pooled reset, failed reload and both player releases passed");
    puts("STR paths: borrowed filenames, owned copies, minimal variants, lookup order and cleanup passed");
    return 0;
}
