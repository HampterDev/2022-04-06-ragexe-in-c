#include "effects/effect_allocator.h"
#include "effects/effect.h"

struct ChangeSizeEffect;
struct Effect;
static void change_size_effect_destroy(struct Effect *effect);

static const struct EffectOps change_size_effect_ops = {
    .set_submit = effect_set_submit,
    .init = effect_init,
    .update_range = effect_update_range,
    .process = effect_process,
    .render = effect_render,
    .release = effect_release,
    .is_alive = effect_is_owner_alive,
    .restart = effect_restart,
    .set_color = effect_set_color,
    .set_texture = effect_set_texture,
    .after_init = effect_after_init,
    .skip = effect_skip,
    .set_stop_frame = effect_set_stop_frame,
    .set_start_delay = effect_set_start_delay,
    .set_spin = effect_set_spin,
    .set_str_name = effect_set_str_name,
    .set_list_flags = effect_set_list_flags,
    .set_zoom = effect_set_zoom,
    .set_alpha_scale = effect_set_alpha_scale,
    .destroy = change_size_effect_destroy,
    .fade_out = effect_fade_out,
};

struct ChangeSizeEffect {
    struct Effect base;
};

struct ChangeSizeEffect *change_size_effect_construct(struct ChangeSizeEffect *self)
{
    effect_construct(&self->base, 0x946);
    self->base.ops = &change_size_effect_ops;
    return self;
}

struct Effect *change_size_effect_new(void)
{
    struct ChangeSizeEffect *self = mem_try_alloc(sizeof *self);

    return self ? &change_size_effect_construct(self)->base : NULL;
}

/* Effect types created as ChangeSizeEffect that effects/effect_registry.c registers after the
   EZ2STR file types (see g_effectTypes there). A type missing here is not created. */
static const struct EffectTypeRow change_size_effect_type_rows[] = {
    EFFECT_TYPE(0x946, change_size_effect_new),
};
const struct EffectTypeTable change_size_effect_types = EFFECT_TYPE_TABLE(change_size_effect_type_rows);

static void change_size_effect_destroy(struct Effect *effect)
{
    struct ChangeSizeEffect *self = (struct ChangeSizeEffect *)effect;
    effect_destruct(&self->base);
    mem_free(self);
}
