float float_sqrt(float x) { abort(); }
void sprite_particle_set_act(struct SpriteParticle *self, const char *name) { abort(); }
void sprite_particle_set_spr(struct SpriteParticle *self, const char *name) { abort(); }
void sprite_particle_render(struct SpriteParticle *self) { abort(); }
struct SpriteParticle *sprite_particle_new(Vec3 pos, int list_flags) { abort(); }
const float g_halfTurnDegrees = 180.0f;
const float g_pi = 3.14159265f;
void effect_mgr_remove(struct EffectMgr *self, struct Effect *effect, unsigned char unlink) { abort(); }
void particle_set_texture(struct Particle *self, struct String name) { abort(); }
void renderer_add_prim(struct Renderer *self, struct RPQuad *prim, int flags) { abort(); }
float get_cos_deg(int deg) { abort(); }
float get_sin_deg(int deg) { abort(); }
unsigned char is_effect_state_star2(unsigned long state) { abort(); }
void *effect_find_actor(int aid) { abort(); }
void effect_play_wave_at(const char *wave, const Vec3 *position, float height) { abort(); }
Vec3 *effect_player_position(Vec3 *out) { abort(); }
float g_cosTable[361] = {0};
struct EffectMgr *g_effectMgr = {0};
struct Renderer *g_renderer = {0};
struct Session g_session1 = {0};
float g_sinTable[361] = {0};
int game_copy_view_matrix(float matrix[12]) { abort(); }
int game_send_message(int message, int a, int b, int c, int d) { abort(); }
void particle_free(struct Particle *p) { abort(); }
struct Particle *particle_new(Vec3 pos, int list_flags) { abort(); }
void *vec_push(struct Vec *v, const void *elem, size_t size) { abort(); }
void renderer_project_vertex(struct Renderer *renderer, const Vec3 *point,
    const float *view, struct TexDrawVertex *vertex) { abort(); }
void string_assign(struct String *s, const char *text, size_t len) { abort(); }
void string_copy(struct String *s, const struct String *src) { abort(); }
void string_assign_cstr(struct String *s, const char *text) { abort(); }
