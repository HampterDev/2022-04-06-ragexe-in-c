#include "fileio/resource_api.h"
#include "contents/sound.h"
#include "session/state.h"
#include <stdlib.h>
#include <string.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"
#include "session/session_names.h"

#define GAME_MSG_QUAKE 0x7a

static unsigned char step(struct RagEffect *self)
{
    return rag_str_step(self, 0.0f, -80.0f, 0.0f) != 0;
}

static void mode_message(int msg, int par1)
{
    game_send_message(msg, par1, 0, 0, 0);
}

unsigned char rag_quagmire_wave(struct RagEffect *self)
{
    if (self->frame == 0 && rag_rand_mod(12) == 0)
        rag_play_wave(self, "effect\\wizard_quagmire.wav", -100.0f);
    return step(self);
}

unsigned char rag_attr_wave(struct RagEffect *self)
{
    if (self->frame == 0)
        rag_play_wave_here(self, session_get_attr_wave_name(3));
    return step(self);
}

unsigned char rag_meteor_storm_waves(struct RagEffect *self)
{
    int frame;

    switch (self->frame) {
    case 0:
        rag_play_wave_here(self, "effect\\wizard_meteo.wav");
        /* Falls through, as in the original: frame 0 also plays the blast mine wave. */
    case 10:
        rag_play_wave_here(self, "effect\\hunter_blastmine.wav");
        break;
    case 20: case 50: case 80: case 100: case 130: case 140: case 180: case 200:
        rag_play_wave_here(self, "effect\\wizard_meteo.wav");
        break;
    default:
        break;
    }
    frame = self->frame;
    if (frame >= 50 && frame % 50 == 0 && frame < 300)
        mode_message(GAME_MSG_QUAKE, 0);
    return step(self);
}

unsigned char rag_hit_and_quakes(struct RagEffect *self)
{
    if (self->frame == 0)
        rag_hit2(self, 1);
    else if (self->frame == 3 || self->frame == 7 || self->frame == 9)
        mode_message(GAME_MSG_QUAKE, 1);
    return step(self);
}

unsigned char rag_concentration_column(struct RagEffect *self)
{
    rag_follow_master(self);
    if (self->frame == 30) {
        sound_play_wave("effect\\ac_concentration.wav", 0.0f, 0.0f, 0.0f, 250, 40, 1.0f);
        rag_potion_pillar(self, 1.9f, 30);
    }
    return step(self);
}

unsigned char rag_concentration_column_long(struct RagEffect *self)
{
    rag_follow_master(self);
    if (self->frame == 0) {
        sound_play_wave("effect\\ac_concentration.wav", 0.0f, 0.0f, 0.0f, 250, 40, 1.0f);
        rag_potion_pillar(self, 2.0f, 50);
    }
    return step(self);
}

unsigned char rag_str_step_at_offset(struct RagEffect *self)
{
    unsigned char going;

    self->offset2 = self->master->pos;
    self->master->pos = self->offset;
    going = rag_str_step(self, 0.0f, -80.0f, 0.0f);
    self->master->pos = self->offset2;
    return going ? 1 : 0;
}

static void spell_cast(struct RagEffect *self, const char *texture, char style)
{
    rag_follow_master(self);
    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\EF_BeginSpell.wav");
    rag_casting_rings(self, texture, style);
}

static void spell_rings(struct RagEffect *self, const char *texture, char style)
{
    rag_follow_master(self);
    if (self->frame != 0)
        return;
    rag_play_wave_here(self, "effect\\EF_BeginSpell.wav");
    rag_casting_circle(self, 45, texture, style);
    rag_casting_circle(self, 25, texture, style);
}

void rag_cast_blue(struct RagEffect *self)
{
    spell_cast(self, "effect\\ring_blue.tga", 2);
}

void rag_cast_yellow(struct RagEffect *self)
{
    spell_cast(self, "effect\\ring_yellow.tga", 1);
}

void rag_cast_green_k25(struct RagEffect *self)
{
    spell_cast(self, "effect\\Magic_Green.tga", 0x21);
}

void rag_cast_yellow_plain(struct RagEffect *self)
{
    spell_cast(self, "effect\\ring_yellow.tga", 0);
}

void rag_cast_white_rings(struct RagEffect *self)
{
    spell_rings(self, "effect\\ring_white.tga", 0);
}

void rag_cast_violet_k25(struct RagEffect *self)
{
    spell_cast(self, "effect\\Magic_Violet.tga", 0x21);
}

void rag_cast_yellow_rings_1(struct RagEffect *self)
{
    spell_rings(self, "effect\\ring_yellow.tga", 1);
}

static void quiet_cast(struct RagEffect *self, const char *texture)
{
    if (self->frame == 0)
        rag_casting_rings(self, texture, 3);
}

void rag_quiet_cast_blue(struct RagEffect *self)
{
    quiet_cast(self, "effect\\ring_blue.tga");
}

void rag_quiet_cast_black(struct RagEffect *self)
{
    quiet_cast(self, "effect\\ring_black.tga");
}

void rag_quiet_cast_brown(struct RagEffect *self)
{
    quiet_cast(self, "effect\\ring_brown.tga");
}

void rag_quiet_cast_red(struct RagEffect *self)
{
    quiet_cast(self, "effect\\ring_red.tga");
}

void rag_quiet_cast_jadu(struct RagEffect *self)
{
    quiet_cast(self, "effect\\ring_jadu.tga");
}

void rag_quiet_cast_white(struct RagEffect *self)
{
    quiet_cast(self, "effect\\ring_white.tga");
}

void rag_quiet_cast_purple(struct RagEffect *self)
{
    quiet_cast(self, "effect\\ring_purple.tga");
}

void rag_quiet_cast_yellow(struct RagEffect *self)
{
    quiet_cast(self, "effect\\ring_yellow.tga");
}
