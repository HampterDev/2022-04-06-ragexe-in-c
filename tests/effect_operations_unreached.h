void effect_mgr_remove(struct EffectMgr *self, struct Effect *effect, unsigned char unlink) { abort(); }
void particle_render_with(struct Particle *self, const struct ParticleRenderArgs *args) { abort(); }
void particle_set_color(struct Particle *self, short r, short g, short b) { abort(); }
unsigned char is_effect_state_cloaking(unsigned long state) { abort(); }
float deg_to_rad(int deg) { abort(); }
void effect_play_wave_at(const char *wave, const Vec3 *position, float height) { abort(); }
Vec3 *effect_player_position(Vec3 *out) { abort(); }
__declspec(noreturn) void fatal_error(const char *message) { abort(); }
struct EffectMgr *g_effectMgr = {0};
struct Mat34 g_identityMatrix = {0};
struct Session g_session1 = {0};
float game_camera_pitch(void) { abort(); }
intptr_t game_send_message(int message, intptr_t a, intptr_t b, intptr_t c, intptr_t d) { abort(); }
void mat_multiply(float *out, const float *a, const float *b) { abort(); }
void mat_rotate_x(float *m, float rad) { abort(); }
void mat_rotate_y(float *m, float rad) { abort(); }
void mat_rotate_z(float *m, float rad) { abort(); }
void mat_set_rotate_x(float *m, float rad) { abort(); }
void mat_set_scale(float *m, float x, float y, float z) { abort(); }
void mat_translate(float *m, const struct Vec3 *v) { abort(); }
void particle_free(struct Particle *p) { abort(); }
struct Particle *particle_new(Vec3 pos, int list_flags) { abort(); }
struct Particle *particle_slots_acquire(struct ParticleSlots *s) { abort(); }
void particle_slots_add(struct ParticleSlots *s, struct Particle *p) { abort(); }
void particle_slots_clear(struct ParticleSlots *s) { abort(); }
void particle_slots_construct(struct ParticleSlots *s) { abort(); }
void particle_slots_destroy(struct ParticleSlots *s) { abort(); }
void particle_slots_use_all(struct ParticleSlots *s) { abort(); }
