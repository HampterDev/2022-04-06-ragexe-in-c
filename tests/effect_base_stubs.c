#include "effects/effect.h"
#include <stdlib.h>

struct Effect *effect_construct(struct Effect *self, int type) { abort(); }
void effect_destruct(struct Effect *self) { abort(); }
unsigned char effect_is_owner_alive(struct Effect *self) { abort(); }
void effect_update_range(struct Effect *self) { abort(); }
void effect_set_submit(struct Effect *self, ParticleSubmitFn fn) { abort(); }
void effect_restart(struct Effect *self) { abort(); }
void effect_set_color(struct Effect *self, int r, int g, int b) { abort(); }
void effect_set_texture(struct Effect *self, struct String name) { abort(); }
void effect_after_init(struct Effect *self) { abort(); }
void effect_skip(struct Effect *self, float fraction) { abort(); }
void effect_set_stop_frame(struct Effect *self, float frame) { abort(); }
void effect_set_start_delay(struct Effect *self, int ms) { abort(); }
void effect_set_spin(struct Effect *self, float degrees) { abort(); }
void effect_set_str_name(struct Effect *self, const char *name) { abort(); }
void effect_set_list_flags(struct Effect *self, int list_flags) { abort(); }
void effect_set_zoom(struct Effect *self, float zoom) { abort(); }
void effect_set_alpha_scale(struct Effect *self, float scale) { abort(); }
void effect_fade_out(struct Effect *self) { abort(); }
