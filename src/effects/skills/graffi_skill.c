#include "actor/actor_ops.h"
#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "actor/appearance.h"
#include "render/texture_cache.h"
#include "render/texture.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "world/ground_api.h"
#include "core/object_type_tables.h"
#include "effects/skills/skill.h"
#include "effects/skills/skill_view.h"
#include <string.h>
#include "actor/game_actor.h"

#define MSG_SET_UNIT_AID 0x16
#define MSG_SET_END_TICK 0x73
#define MSG_SET_TEXT 0x75

#define GLYPH 32
#define NARROW 16
#define LINE 36
#define WIDTH 0x160
#define NO_GLYPH 0x63

struct GraffiSkill {
    struct Skill base;
    int cell_x, cell_y;
    int end_tick;
    void *surface;
    void *font;
    char text[0x100];
    int cursor_x, cursor_y;
    float angle;
};

struct RenderObject;

const struct GameActorOps graffi_skill_ops = {
    .base = {
        .base = {
            .destroy = graffi_skill_delete,
            .on_process = graffi_skill_on_process,
            .send_msg = graffi_skill_send_msg,
            .render = graffi_skill_render,
            .get_direction = game_actor_dir_of,
        },
        .render_body = skill_render_body,
        .set_action = game_actor_set_action,
        .update_motion = render_object_update_motion_base,
        .get_effect_state = game_actor_get_effect_state,
    },
    .process_state = game_actor_process_state,
    .set_state = game_actor_set_state,
    .set_unit_cell = game_actor_set_unit_cell,
    .clear_unit_cell = game_actor_clear_unit_cell,
    .set_job_model = actor_default_pair,
    .set_look = actor_default_look,
    .set_look_with_state = actor_default_look_state,
    .set_spr_job = actor_default_int,
    .set_head = actor_default_int,
    .set_weapon = actor_default_weapon,
    .set_weapon_look = actor_default_weapon,
    .set_accessory = actor_default_int,
    .set_accessory2 = actor_default_int,
    .set_accessory3 = actor_default_int,
    .set_shield = actor_default_int,
    .set_appearance_value = actor_default_int,
    .set_imf_name = actor_default_noop,
    .set_head_pal_name = actor_default_noop,
    .set_body_pal_name = actor_default_noop,
    .set_body_palette = actor_default_int,
    .set_head_palette = actor_default_int,
    .get_weapon = actor_default_no_weapon,
    .walk_motion = game_actor_walk_motion,
    .get_attack_motion = game_actor_get_attack_motion,
    .make_corpse = game_actor_make_corpse,
    .set_attack_speed = game_actor_set_attack_speed,
    .set_honor = actor_default_int,
    .set_honor_virtue = actor_default_int_flags,
    .get_char_id = actor_default_zero,
    .set_robe = actor_default_int_bool,
    .set_body_style = actor_default_int_bool,
    .set_mounted = game_actor_set_mounted,
    .get_sprite_variant = actor_default_zero,
    .set_sprite_variant = actor_default_int,
    .get_skip_arrow = actor_default_false,
    .set_skip_arrow = actor_default_bool,
    .refresh_accessory3 = actor_default_noop,
    .get_head_extent = game_actor_get_head_extent,
};

static struct GraffiSkill *graffi_skill_construct(struct GraffiSkill *self)
{
    skill_construct(&self->base, 0);
    self->base.base.ops = &graffi_skill_ops.base.base;
    return self;
}

struct GraffiSkill *graffi_skill_new(void)
{
    struct GraffiSkill *self = mem_alloc(sizeof *self);

    return self ? graffi_skill_construct(self) : NULL;
}

void graffi_skill_release_surface(struct GraffiSkill *self)
{
    if (self->surface)
        texture_destroy(self->surface);
    self->surface = NULL;
}

void graffi_skill_destruct(struct GraffiSkill *self)
{
    self->base.base.ops = &graffi_skill_ops.base.base;
    graffi_skill_release_surface(self);
    skill_destruct(&self->base);
}

void graffi_skill_delete(struct GameObject *object)
{
    struct GraffiSkill *self = (struct GraffiSkill *)object;

    graffi_skill_destruct(self);
    mem_free(self);
}

unsigned char graffi_skill_on_process(struct GameObject *object)
{
    return 1;
}

static void new_line(struct GraffiSkill *self, int *x)
{
    self->cursor_y += LINE;
    *x = 0;
    self->cursor_x = 0;
}

void graffi_skill_on_init(struct GraffiSkill *self)
{
    int len, i, x, width;

    self->cursor_x = 0;
    self->cursor_y = 0;
    self->font = texture_cache_get(&g_textureCache, "effect\\alpabet.bmp", 0, 0);
    self->surface = texture_create(0x180, 0x180, 0);
    len = (int)strlen(self->text);
    self->cursor_y += (0x90 - len) / 24 * LINE;
    width = len;
    if (len > 0) {
        x = self->cursor_x;
        for (i = 0; i < len; i++) {
            unsigned char c = (unsigned char)self->text[i];
            unsigned char glyph;

            if ((unsigned char)(c - 'a') <= 25) {
                glyph = (unsigned char)(c + 0xb9);
                width = GLYPH;
            } else if ((unsigned char)(c - 'A') <= 25) {
                glyph = (unsigned char)(c - 'A');
                width = GLYPH;
            } else if ((unsigned char)(c - '0') <= 9) {
                glyph = (unsigned char)(c + 6);
                width = GLYPH;
            } else if (c == '?') {
                glyph = 0x35;
                width = NARROW;
            } else if (c == '!') {
                glyph = 0x36;
                width = NARROW;
            } else if (c == ' ') {
                if ((unsigned char)self->text[i + 1] != ' ') {
                    const char *p = &self->text[i + 1];
                    unsigned char n = (unsigned char)*p;
                    while (n) {
                        n = (unsigned char)*++p;
                        x += GLYPH;
                        if (n == ' ')
                            break;
                    }
                }
                glyph = NO_GLYPH;
                if (x >= WIDTH)
                    new_line(self, &x);
                else
                    x = self->cursor_x;
            } else {
                glyph = NO_GLYPH;
                width = NARROW;
            }
            if (x + width >= WIDTH)
                new_line(self, &x);
            if (glyph != NO_GLYPH) {
                texture_blit_rect(self->surface, self->font, (glyph & 7) << 5, (glyph >> 3) << 5, GLYPH, GLYPH,
                           x, self->cursor_y, width, GLYPH);
                x = self->cursor_x;
            }
            x += width;
            self->cursor_x = x;
        }
    }
    {
        const struct RenderObject *player = (const struct RenderObject *)game_current_player();
        self->angle = player ? player->roty : 0.0f;
    }
}

void graffi_skill_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct GraffiSkill *self = (struct GraffiSkill *)object;

    switch (msg) {
    case MSG_SET_UNIT_AID:
        self->base.unit_aid = (int)par1;
        return;
    case MSG_SET_END_TICK:
        self->end_tick = (int)par1;
        return;
    case MSG_SET_TEXT:
        strcpy(self->text, (const char *)MSG_PTR(par1));
        self->cell_x = (int)par2;
        self->cell_y = (int)par3;
        graffi_skill_on_init(self);
        return;
    }
    game_actor_send_msg((struct GameObject *)&self->base.base, source, msg, par1, par2, par3, 0);
}

void graffi_skill_render(struct GameObject *object, const float *view)
{
    struct GraffiSkill *self = (struct GraffiSkill *)object;

    struct C3dGround15 *ground = game_current_ground();
    float s = float_sin(self->angle / 180.0f * 3.141592f);
    float c = float_cos(self->angle / 180.0f * 3.141592f);
    int row, col;

    if (!ground)
        return;
    for (row = 0; row < 18; row++) {
        float r0 = (float)row - 9.0f, r1 = (float)(row + 1) - 9.0f;
        float r0s = r0 * s, r0c = r0 * c, r1s = r1 * s, r1c = r1 * c;

        for (col = 0; col < 18; col++) {
            float a = (float)col - 9.0f, b = (float)(col + 1) - 9.0f;
            float ac = a * c, as = a * s, bs = b * s, bc = b * c;
            float uv[8];

            uv[0] = 1.0f - (r0s + ac + 9.0f) / 18.0f;
            uv[1] = (r0c - as + 9.0f) / 18.0f;
            uv[2] = 1.0f - (bc + r0s + 9.0f) / 18.0f;
            uv[3] = (r0c - bs + 9.0f) / 18.0f;
            uv[4] = 1.0f - (r1s + ac + 9.0f) / 18.0f;
            uv[5] = (r1c - as + 9.0f) / 18.0f;
            uv[6] = 1.0f - (r1s + bc + 9.0f) / 18.0f;
            uv[7] = (r1c - bs + 9.0f) / 18.0f;
            c3d_ground15_draw_attr_quad(ground, view, self->cell_x - 9 + col, row - 9 + self->cell_y, 0xffffffffu,
                       self->surface, uv);
        }
    }
}
