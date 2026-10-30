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

static unsigned char wave_then_step(struct RagEffect *self, int frame, const char *wave)
{
    if (self->frame == frame)
        rag_play_wave_here(self, wave);
    return step(self);
}

static void mode_message(int msg, int par1)
{
    game_send_message(msg, par1, 0, 0, 0);
}

unsigned char rag_angelus_wave(struct RagEffect *self)
{
    return wave_then_step(self, 0, "effect\\EF_Angelus.wav");
}

unsigned char rag_auto_counter_wave(struct RagEffect *self)
{
    return wave_then_step(self, 0, "effect\\knight_autocounter.wav");
}

unsigned char rag_benedictio_wave(struct RagEffect *self)
{
    return wave_then_step(self, 0, "effect\\priest_benedictio.wav");
}

unsigned char rag_blast_mine_wave(struct RagEffect *self)
{
    return wave_then_step(self, 20, "effect\\hunter_blastmine.wav");
}

unsigned char rag_claymore_trap_wave(struct RagEffect *self)
{
    return wave_then_step(self, 0, "effect\\hunter_claymoretrap.wav");
}

unsigned char rag_cure_wave(struct RagEffect *self)
{
    return wave_then_step(self, 0, "effect\\Acolyte_cure.wav");
}

unsigned char rag_fire_pillar_a_wave(struct RagEffect *self)
{
    return wave_then_step(self, 75, "effect\\wizard_fire_pillar_a.wav");
}

unsigned char rag_fire_pillar_b_wave(struct RagEffect *self)
{
    return wave_then_step(self, 0, "effect\\wizard_fire_pillar_b.wav");
}

unsigned char rag_freezing_trap_wave(struct RagEffect *self)
{
    return wave_then_step(self, 0, "effect\\hunter_freezingtrap.wav");
}

unsigned char rag_gloria_wave(struct RagEffect *self)
{
    if (self->frame == 0)
        rag_play_wave(self, "effect\\priest_gloria.wav", 1.0f);
    return step(self);
}

unsigned char rag_impositio_wave(struct RagEffect *self)
{
    return wave_then_step(self, 60, "effect\\priest_impositio.wav");
}

unsigned char rag_invenom_wave(struct RagEffect *self)
{
    return wave_then_step(self, 0, "effect\\thief_invenom.wav");
}

unsigned char rag_kyrie_eleison_waves(struct RagEffect *self)
{
    if (self->frame == 0)
        rag_play_wave_here(self, "effect\\priest_kyrie_eleison_b.wav");
    if (self->frame == 15)
        rag_play_wave_here(self, "effect\\priest_kyrie_eleison_a.wav");
    return step(self);
}

unsigned char rag_lex_aeterna_wave(struct RagEffect *self)
{
    return wave_then_step(self, 15, "effect\\priest_lexaeterna.wav");
}

unsigned char rag_magnificat_wave(struct RagEffect *self)
{
    return wave_then_step(self, 0, "effect\\priest_magnificat.wav");
}

unsigned char rag_magnus_wave(struct RagEffect *self)
{
    if (self->frame == 0)
        rag_play_wave(self, "effect\\priest_magnus.wav", 1.0f);
    return step(self);
}

unsigned char rag_maximize_power_waves(struct RagEffect *self)
{
    if (self->frame == 0)
        rag_play_wave_here(self, "effect\\black_maximize_power_circle.wav");
    if (self->frame == 40)
        rag_play_wave_here(self, "effect\\black_maximize_power_sword.wav");
    if (self->frame == 47)
        rag_play_wave_here(self, "effect\\black_maximize_power_sword.wav");
    if (self->frame == 65)
        rag_play_wave_here(self, "effect\\black_maximize_power_sword_bic.wav");
    return step(self);
}

unsigned char rag_meteor_wave_quake(struct RagEffect *self)
{
    if (self->frame == 0)
        rag_play_wave_here(self, "effect\\wizard_meteor.wav");
    if (self->frame == 40)
        mode_message(GAME_MSG_QUAKE, 0);
    return step(self);
}

unsigned char rag_mvp_wave(struct RagEffect *self)
{
    return wave_then_step(self, 0, "effect\\st_mvp.wav");
}

unsigned char rag_weapon_perfection_wave(struct RagEffect *self)
{
    return wave_then_step(self, 0, "effect\\black_weapon_perfection.wav");
}

unsigned char rag_poison_react_wave(struct RagEffect *self)
{
    return wave_then_step(self, 0, "effect\\assasin_poisonreact.wav");
}

unsigned char rag_provoke_waves(struct RagEffect *self)
{
    if (self->frame == 5)
        rag_play_wave_here(self, "effect\\swordman_provoke.wav");
    if (self->frame == 30)
        rag_play_wave_here(self, "effect\\swordman_provoke.wav");
    return step(self);
}

unsigned char rag_quagmire_wave(struct RagEffect *self)
{
    if (self->frame == 0 && rag_rand_mod(12) == 0)
        rag_play_wave(self, "effect\\wizard_quagmire.wav", -100.0f);
    return step(self);
}

unsigned char rag_recovery_wave(struct RagEffect *self)
{
    return wave_then_step(self, 0, "effect\\priest_recovery.wav");
}

unsigned char rag_refine_fail_wave(struct RagEffect *self)
{
    return wave_then_step(self, 5, "effect\\bs_refinefailed.wav");
}

unsigned char rag_refine_success_wave(struct RagEffect *self)
{
    return wave_then_step(self, 15, "effect\\bs_refinesuccess.wav");
}

unsigned char rag_weapon_repair_waves(struct RagEffect *self)
{
    if (self->frame == 20)
        rag_play_wave_here(self, "effect\\black_weapon_repair_a.wav");
    if (self->frame == 55)
        rag_play_wave_here(self, "effect\\black_weapon_repair_a.wav");
    return step(self);
}

unsigned char rag_resurrection_wave(struct RagEffect *self)
{
    return wave_then_step(self, 0, "effect\\priest_resurrection.wav");
}

unsigned char rag_sanctuary_wave(struct RagEffect *self)
{
    return wave_then_step(self, 0, "effect\\priest_sanctuary.wav");
}

unsigned char rag_skid_trap_wave(struct RagEffect *self)
{
    return wave_then_step(self, 0, "effect\\hunter_skidtrap.wav");
}

unsigned char rag_venom_splasher_wave(struct RagEffect *self)
{
    return wave_then_step(self, 10, "effect\\assasin_venomsplasher.wav");
}

unsigned char rag_spring_trap_wave(struct RagEffect *self)
{
    return wave_then_step(self, 0, "effect\\hunter_springtrap.wav");
}

unsigned char rag_suffragium_wave(struct RagEffect *self)
{
    return wave_then_step(self, 0, "effect\\priest_suffragium.wav");
}

unsigned char rag_thunderstorm_wave(struct RagEffect *self)
{
    return wave_then_step(self, 0, "effect\\magician_thunderstorm.wav");
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
    case 10:
        rag_play_wave_here(self, "effect\\hunter_blastmine.wav");
        break;
    case 20: case 50: case 80: case 100: case 130: case 140: case 170: case 200:
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

unsigned char rag_str_step_or_stop(struct RagEffect *self)
{
    if (!rag_str_step(self, 0.0f, -80.0f, 0.0f)) {
        self->frame = 9999;
        return 0;
    }
    return 1;
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
