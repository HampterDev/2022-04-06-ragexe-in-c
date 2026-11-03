#include "actor/render_ops.h"
#include "actor/object_ops.h"
#include "render/texture_cache.h"
#ifndef EFFECTS_RAG_RAG_EFFECT_H
#define EFFECTS_RAG_RAG_EFFECT_H

#include "render/math3d.h"

#include "core/memory.h"
#include <stdint.h>
#include "core/fatal.h"
#include "world/mode_manager.h"

#include "game/context.h"
#include "effects/rag/effect_prim.h"

#include "effects/rag/rag_effect_types.h"

#define RAG_MSG_SET_OFFSETS  0x0e
#define RAG_MSG_MASTER_GONE  0x18
#define RAG_MSG_PRIM_GONE    0x19
#define RAG_MSG_SET_DURATION 0x50
#define RAG_MSG_SET_COLOR    0x7f

static __inline void rag_play_wave(struct RagEffect *self, const char *wave, float y)
{
    effect_play_wave_at(wave, &self->base.pos, y);
}

static __inline void rag_play_wave_here(struct RagEffect *self, const char *wave)
{
    rag_play_wave(self, wave, 0.0f);
}

struct RagEffect *rag_effect_construct(struct RagEffect *self);
void rag_effect_destruct(struct RagEffect *self);
void rag_effect_send_msg(struct RagEffect *self, struct RenderObject *sender, RenderMsgArg msg,
                                    RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3);
int rag_effect_get_duration(struct RagEffect *self);
void rag_effect_set_duration(struct RagEffect *self, int frame);
struct EffectPrim *rag_launch_prim(struct RagEffect *self, int kind, float dx, float dy, float dz);

void rag_effect_init(struct RagEffect *self, struct RenderObject *master, int effect_id, Vec3 offset);
void rag_str_rewind(struct RagEffect *self);
void rag_str_load(struct RagEffect *self, const char *name);
void rag_str_unload(struct RagEffect *self);
unsigned char rag_str_step(struct RagEffect *self, float dx, float dy, float dz);
struct RagStrBinding;
unsigned char rag_str_play(struct RagEffect *self, const struct RagStrBinding *binding);

#include <stdlib.h>

static __inline int rag_rand_mod(int n)
{
    int r = rand() % n;
    return r < 0 ? 0 : r;
}

static __inline void rag_follow_master(struct RagEffect *self)
{
    self->base.pos = self->master->pos;
}

static __inline struct Texture **rag_prim_new_textures(const struct EffectPrim *prim)
{
    size_t count = (size_t)prim->motion_count;
    size_t size = sizeof *prim->textures;
    return mem_alloc(count > SIZE_MAX / size ? SIZE_MAX : count * size);
}

static __inline unsigned short rag_screen_heading(const struct RagEffect *self)
{
    short heading;
    int deg;

    prim_heading_degrees(self->base.pos.x - self->offset.x, self->base.pos.z - self->offset.z, &heading);
    deg = (short)((int)game_camera_heading() + heading) % 360;
    if ((short)deg < 0)
        deg += 360;
    return (unsigned short)deg;
}

void rag_msg_sprite(struct RagEffect *self, char number);

unsigned char rag_quagmire_wave(struct RagEffect *self);
unsigned char rag_attr_wave(struct RagEffect *self);
unsigned char rag_meteor_storm_waves(struct RagEffect *self);
unsigned char rag_hit_and_quakes(struct RagEffect *self);
unsigned char rag_concentration_column(struct RagEffect *self);
unsigned char rag_concentration_column_long(struct RagEffect *self);
unsigned char rag_str_step_at_offset(struct RagEffect *self);
void rag_cast_blue(struct RagEffect *self);
void rag_cast_yellow(struct RagEffect *self);
void rag_cast_green_k25(struct RagEffect *self);
void rag_cast_yellow_plain(struct RagEffect *self);
void rag_cast_white_rings(struct RagEffect *self);
void rag_cast_violet_k25(struct RagEffect *self);
void rag_cast_yellow_rings_1(struct RagEffect *self);
void rag_quiet_cast_black(struct RagEffect *self);
void rag_quiet_cast_red(struct RagEffect *self);
void rag_quiet_cast_blue(struct RagEffect *self);
void rag_quiet_cast_yellow(struct RagEffect *self);
void rag_quiet_cast_jadu(struct RagEffect *self);
void rag_quiet_cast_brown(struct RagEffect *self);
void rag_quiet_cast_white(struct RagEffect *self);
void rag_quiet_cast_purple(struct RagEffect *self);

void rag_thunderstorm_sprite(struct RagEffect *self);
unsigned char rag_kings_grace(struct RagEffect *self);
void rag_holy_water(struct RagEffect *self);
void rag_christmas_sprite(struct RagEffect *self);
void rag_big_explosion(struct RagEffect *self);
void rag_particle_spray(struct RagEffect *self);
void rag_curse_ring(struct RagEffect *self);
void rag_dark_breath(struct RagEffect *self);
void rag_chimney_smoke(struct RagEffect *self);
void rag_chimney_smokes(struct RagEffect *self);
void rag_fire_ivy(struct RagEffect *self);
void rag_water_out(struct RagEffect *self);
void rag_fire_ring(struct RagEffect *self);
void rag_party_ring(struct RagEffect *self);
void rag_poison_hit(struct RagEffect *self);
void rag_ready_portal_rings(struct RagEffect *self);
void rag_yellow_pillar(struct RagEffect *self);
void rag_stone_curse(struct RagEffect *self);
void rag_teleport_ring(struct RagEffect *self);
void rag_rising_sparks_every5(struct RagEffect *self);
void rag_lock_on_ground(struct RagEffect *self);
void rag_torch(struct RagEffect *self);

void rag_hit1(struct RagEffect *self);
void rag_hit2(struct RagEffect *self, int tinted);
void rag_hit3(struct RagEffect *self);
void rag_hit4(struct RagEffect *self);
void rag_hit5(struct RagEffect *self);
void rag_hit6(struct RagEffect *self);
void rag_entry(struct RagEffect *self);
void rag_exit(struct RagEffect *self);
void rag_warp(struct RagEffect *self);
void rag_enhance(struct RagEffect *self);
unsigned char rag_coin(struct RagEffect *self);
void rag_endure(struct RagEffect *self);
void rag_casting_circle(struct RagEffect *self, int alpha, const char *texture, char style);
unsigned char rag_glass_wall(struct RagEffect *self);
void rag_heal_sp(struct RagEffect *self);
void rag_soul_strike(struct RagEffect *self, char ghost);
void rag_bash(struct RagEffect *self);
void rag_magnum_break(struct RagEffect *self);
void rag_steal(struct RagEffect *self);

void rag_poison_attack(struct RagEffect *self, char variant);
void rag_detoxication(struct RagEffect *self);
void rag_sight(struct RagEffect *self);
void rag_fire_ball(struct RagEffect *self);
void rag_ice_arrow(struct RagEffect *self);
void rag_frost_diver(struct RagEffect *self, char variant);
void rag_frost_diver2(struct RagEffect *self);
void rag_fire_arrow(struct RagEffect *self);
void rag_napalm_beat(struct RagEffect *self);
void rag_ruwach(struct RagEffect *self);
void rag_portal(struct RagEffect *self);
void rag_inc_agility(struct RagEffect *self);
void rag_dec_agility(struct RagEffect *self);

unsigned char rag_signum(struct RagEffect *self);
void rag_blessing(struct RagEffect *self);
void rag_inc_agi_dex(struct RagEffect *self);
void rag_firefly(struct RagEffect *self);
void rag_sand_wind(struct RagEffect *self);
void rag_spray_pond(struct RagEffect *self);
void rag_cold_hit(struct RagEffect *self);
void rag_casting_rings(struct RagEffect *self, const char *texture, char style);
void rag_lock_on(struct RagEffect *self);
void rag_warp_zone(struct RagEffect *self);
void rag_sightrasher(struct RagEffect *self);
void rag_sphere(struct RagEffect *self);
void rag_bowling_bash(struct RagEffect *self);
void rag_ice_wall(struct RagEffect *self);
void rag_earth_spike(struct RagEffect *self, char variant);

void rag_spear_boomerang(struct RagEffect *self);
void rag_pierce(struct RagEffect *self);
void rag_turn_undead(struct RagEffect *self);
unsigned char rag_aspersio(struct RagEffect *self);
unsigned char rag_storm_gust(struct RagEffect *self);
void rag_jupitel(struct RagEffect *self, char variant);
void rag_jupitel_hit(struct RagEffect *self, char variant);
void rag_adrenaline_rush(struct RagEffect *self);
void rag_flasher(struct RagEffect *self);
void rag_remove_trap(struct RagEffect *self);
void rag_violet_cross(struct RagEffect *self);
void rag_blitz_beat(struct RagEffect *self);
void rag_water_ball(struct RagEffect *self);
void rag_detecting(struct RagEffect *self, char variant);

void rag_cloaking(struct RagEffect *self);
void rag_sonic_blow_hit(struct RagEffect *self);
void rag_grimtooth(struct RagEffect *self);
void rag_over_thrust(struct RagEffect *self);
void rag_grimtooth_attack(struct RagEffect *self);
void rag_green_rings(struct RagEffect *self);
void rag_fire_pillar_on(struct RagEffect *self, unsigned char white);
void rag_heavens_drive(struct RagEffect *self);
unsigned char rag_concentration(struct RagEffect *self);
void rag_snow(struct RagEffect *self);
void rag_sakura(struct RagEffect *self);
void rag_status_icons(struct RagEffect *self);
void rag_make_blur(struct RagEffect *self);
unsigned char rag_cart_revolution(struct RagEffect *self);
void rag_dark_hit(struct RagEffect *self);
void rag_level99_aura(struct RagEffect *self, const char *texture, char big);
void rag_level99_aura2(struct RagEffect *self, const char *texture, char size);
void rag_level99_aura3(struct RagEffect *self, char style);
void rag_gumgang(struct RagEffect *self, char count, char red);
void rag_summon_slave(struct RagEffect *self);
void rag_blood_drain(struct RagEffect *self, int count);
void rag_energy_drain(struct RagEffect *self);
void rag_potion_pillar(struct RagEffect *self, float rise, int frames);

unsigned char rag_potion_berserk(struct RagEffect *self);
void rag_bottom_ring(struct RagEffect *self, const char *texture, char scale);
void rag_ground_disc(struct RagEffect *self, const char *texture, char style);
void rag_defender(struct RagEffect *self, const char *texture);
void rag_grand_cross_rings(struct RagEffect *self, unsigned short alpha, char black);
void rag_grand_cross_beams(struct RagEffect *self, char black);
void rag_ground_rings(struct RagEffect *self, const char *texture, char style);
void rag_providence(struct RagEffect *self, const char *texture, char no_rings);
void rag_map_pillar(struct RagEffect *self, const char *texture, char style);
void rag_heal_rings(struct RagEffect *self, const char *texture, char style);
void rag_wind(struct RagEffect *self);
void rag_intimidate(struct RagEffect *self, char style);
void rag_line_link(struct RagEffect *self, char style);
void rag_shield_boomerang(struct RagEffect *self, const char *texture);
void rag_spirit_spheres(struct RagEffect *self, char style);
void rag_clouds(struct RagEffect *self, char style);
void rag_heal_bubbles(struct RagEffect *self, unsigned char with_pok3, float size, float rise);

void rag_swirl_ring(struct RagEffect *self, const char *texture, char style);
void rag_song_notes(struct RagEffect *self, const char *texture, char style);
void rag_song_marks(struct RagEffect *self, const char *texture, char style);
void rag_song_rings(struct RagEffect *self, const char *texture, char style);
void rag_four_rings(struct RagEffect *self, const char *texture, char style);
void rag_hit_burst(struct RagEffect *self, const char *texture, char style);
void rag_hit_burst_wide(struct RagEffect *self, const char *texture, char style);
void rag_hit_flash(struct RagEffect *self);
void rag_finger_offensive(struct RagEffect *self);
void rag_directed_sprite(struct RagEffect *self, char style);
void rag_coin_burst(struct RagEffect *self, const char *texture);
void rag_fireworks(struct RagEffect *self);
void rag_joke_or_scream(struct RagEffect *self, char which);
void rag_throw_item(struct RagEffect *self, const char *texture, char style);

void rag_chemical_protection(struct RagEffect *self, int turn);
void rag_firework_sound(struct RagEffect *self);
void rag_demonstration(struct RagEffect *self);
void rag_chemical_slash(struct RagEffect *self, short side);
void rag_teleportation(struct RagEffect *self, const char *texture, short n);
void rag_forest_light(struct RagEffect *self, const char *texture, char style);
void rag_first_aid(struct RagEffect *self);
void rag_sprinkle_sand(struct RagEffect *self);
void rag_heal(struct RagEffect *self, const char *texture, char style);
void rag_exit_rings(struct RagEffect *self, const char *texture);
unsigned char rag_glass_wall_textured(struct RagEffect *self, const char *texture);
void rag_ready_portal(struct RagEffect *self);
void rag_portal_styled(struct RagEffect *self, char style);
void rag_bottom_column(struct RagEffect *self, const char *texture, char style);

void rag_warp_zone_styled(struct RagEffect *self, const char *texture, char style);
void rag_footprint(struct RagEffect *self, const char *texture, char style);
void rag_begin_asura(struct RagEffect *self, char style);
void rag_triple_attack(struct RagEffect *self);
void rag_hit_line(struct RagEffect *self, char style);
void rag_time_bubbles(struct RagEffect *self, const char *texture, char style);
void rag_maple(struct RagEffect *self);
void rag_blind(struct RagEffect *self, char level);
void rag_poison(struct RagEffect *self, char style);
void rag_guard(struct RagEffect *self, const char *texture, char style);
unsigned char rag_job_level_up50(struct RagEffect *self, int turn);
void rag_angel(struct RagEffect *self, char style);
void rag_magnum_break2(struct RagEffect *self, const char *texture);

#define RAG_SIZE_PARAM(e) ((e)->msg_params[0])

void rag_ring_at_frame_100(struct RagEffect *self, const char *texture);
void rag_spell_casting_rings(struct RagEffect *self, const char *texture);
void rag_casting_rings_white_yellow(struct RagEffect *self);
void rag_two_rings(struct RagEffect *self, const char *texture, float offset_x, float offset_z);
void rag_four_blue_rings(struct RagEffect *self);
void rag_two_flat_rings(struct RagEffect *self, const char *texture);
void rag_wedding_paper(struct RagEffect *self, const char *texture, unsigned char silent);
void rag_water_fall(struct RagEffect *self, unsigned char lengthwise, unsigned char size,
                               unsigned char look);
void rag_water_fall_spray(struct RagEffect *self, unsigned char sideways, unsigned char narrow);
void rag_ghost_or_bat(struct RagEffect *self, char mode);
void rag_valentine_sprite(struct RagEffect *self, char look);
void rag_billboard(struct RagEffect *self, const char *texture, char style);
void rag_four_turned_rings(struct RagEffect *self, const char *texture, char turn, char style);
void rag_effect_texture(struct RagEffect *self, const char *texture, char style);
void rag_four_tilted_rings(struct RagEffect *self, const char *texture, int angle, unsigned char wide);
void rag_glow(struct RagEffect *self, const char *texture, int age, char style);
void rag_hit_sparks(struct RagEffect *self, const char *texture);
void rag_slash_waves(struct RagEffect *self, char sonic);
void rag_heart_rings(struct RagEffect *self);

void rag_styled_rings(struct RagEffect *self, const char *texture, char style);
void rag_particle_streams(struct RagEffect *self);
void rag_particle_streams_down(struct RagEffect *self, char colour);
void rag_actor_link(struct RagEffect *self);
void rag_actor_link_wide(struct RagEffect *self);
void rag_dashes_to_offset(struct RagEffect *self);
void rag_dashes_to_offset_long(struct RagEffect *self);
void rag_cloud_swirl(struct RagEffect *self, char look);
void rag_rising_bits(struct RagEffect *self, const char *texture, int angle);
void rag_purple_slashes(struct RagEffect *self, int angle);
void rag_view_facing_sheet(struct RagEffect *self, const char *texture);
void rag_spray_at_angle(struct RagEffect *self, const char *texture, short angle, int mode);
void rag_popping_bits(struct RagEffect *self, const char *texture, int mode);
void rag_blue_ivy(struct RagEffect *self);
void rag_aimed_texture(struct RagEffect *self, const char *texture);
void rag_super_rings(struct RagEffect *self, char count, char tint);
void rag_quake_spin_master(struct RagEffect *self);
void rag_kyrie_guard_flicker(struct RagEffect *self);

void rag_white_streaks_burst(struct RagEffect *self, int look);
void rag_white_streaks(struct RagEffect *self, char look);
void rag_white_streaks_back(struct RagEffect *self, char look);
void rag_electric_rings(struct RagEffect *self, char look, int angle);
void rag_electric_lines(struct RagEffect *self, char look);
void rag_storm_ring(struct RagEffect *self, const char *texture, char look);
void rag_single_ring(struct RagEffect *self, const char *texture);
void rag_wind_rings(struct RagEffect *self, const char *texture);
void rag_cloud_rings(struct RagEffect *self, const char *texture);
void rag_line_bursts(struct RagEffect *self, short style);
void rag_blue_casting(struct RagEffect *self);
unsigned char rag_blue_magic_rings(struct RagEffect *self, int angle);
void rag_stop_sprite(struct RagEffect *self, char look);
void rag_black_casting(struct RagEffect *self, char silent);
void rag_rising_light(struct RagEffect *self, char look);
void rag_picture_or_skill_icon(struct RagEffect *self, const char *texture, short start, short look);
void rag_dark_rings(struct RagEffect *self, const char *texture);
void rag_flower_rings(struct RagEffect *self, const char *texture, char scale, char look);
void rag_flying_kick(struct RagEffect *self);
void rag_flower_rings_ramp(struct RagEffect *self);

void rag_poison_particles(struct RagEffect *self);
void rag_tumbling_items(struct RagEffect *self, const char *texture, float size, char look);
void rag_guard_ring(struct RagEffect *self, const char *texture);
void rag_soul_light(struct RagEffect *self, const char *texture);
void rag_soul_link_letters(struct RagEffect *self);
void rag_light_spheres(struct RagEffect *self, const char *texture, char look);
void rag_magic_target(struct RagEffect *self);
void rag_explosion_rings(struct RagEffect *self, const char *texture);
void rag_floor_disc(struct RagEffect *self, const char *texture);
void rag_light_orbits(struct RagEffect *self, const char *texture);
void rag_shield_boomerang_volley(struct RagEffect *self, const char *texture);
void rag_stone_and_ice(struct RagEffect *self);
void rag_fire_wall_rings(struct RagEffect *self, const char *texture);
void rag_pok_bursts(struct RagEffect *self, char look);
void rag_rising_columns(struct RagEffect *self, const char *texture);
void rag_columns(struct RagEffect *self, char look);
void rag_three_rings(struct RagEffect *self, const char *texture);
void rag_columns_at_offset(struct RagEffect *self, const char *texture, char look);
void rag_pok_burst_at_offset(struct RagEffect *self, char look);
void rag_water_pillar(struct RagEffect *self, char lengthwise, char look, char textures);
void rag_line_burst_quake(struct RagEffect *self, unsigned char with_wave);

void rag_fire_change_rings(struct RagEffect *self, char look);
void rag_falling_items(struct RagEffect *self, char look);
unsigned char rag_blue_magic_rings_now(struct RagEffect *self, int angle);
void rag_master_copies(struct RagEffect *self);
void rag_sakura_petals(struct RagEffect *self);
void rag_columns_wide(struct RagEffect *self, const char *texture, char look);
void rag_columns_shifted(struct RagEffect *self, const char *texture);
void rag_aura_rings(struct RagEffect *self, const char *texture);
void rag_aura_pair(struct RagEffect *self, const char *texture);
void rag_aura_disc(struct RagEffect *self, const char *texture, char look);
void rag_aura_particles(struct RagEffect *self, const char *texture, char height, char look);
void rag_thunderstorm_particle(struct RagEffect *self);
void rag_colored_quad(struct RagEffect *self, char style, unsigned char r, unsigned char g,
                                 unsigned char b);
void rag_texture_animation(struct RagEffect *self, char look);
void rag_sight_trail(struct RagEffect *self);

#define PRIM_YAW(p)    ((p)->angle[0])
#define PRIM_PITCH(p)  ((p)->angle[1])

static __inline int rag_rand(int n)
{
    int r = rand() % n;
    return r < 0 ? 0 : r;
}

#define RAG_NEW_TEXTURES(p) ((p)->textures = rag_prim_new_textures(p))
#define RAG_TEXTURE(name) texture_cache_get(&g_textureCache, (name), 0, 0)

#define MASTER_TINTED(m) ((m)->saved_tint[0])
#define MASTER_TINT(m, i) ((m)->tint_rgb[(i)])
#define HALF_TINT(self, m, i, v) ((int)((float)MASTER_TINT(m, i) + (v)) >> 1)

static __inline Vec3 rag_mat_apply(const float *m, float x, float y, float z)
{
    Vec3 v;

    v.x = m[3] * y + m[0] * x + m[6] * z + m[9];
    v.y = m[4] * y + m[1] * x + m[7] * z + m[10];
    v.z = m[5] * y + m[2] * x + m[8] * z + m[11];
    return v;
}

static __inline void prim_sub_ring(struct PrimRing *ring, int age, float height, short turn, float radius,
                                   short tilt, short mode, unsigned char column_flags)
{
    int i;

    ring->active = 1;
    ring->age = age;
    ring->span = 360;
    ring->height = height;
    ring->turn = turn;
    ring->alpha = 0;
    ring->radius = radius;
    ring->tilt = tilt;
    ring->mode = mode;
    ring->offset_flag = 0.0f;
    for (i = 0; i < PRIM_RING_COLUMNS; i++) {
        ring->column[i] = 0.0f;
        ring->column_flag[i] = column_flags;
    }
}

void rag_stone_chips(struct RagEffect *self);
void rag_fireball_volleys(struct RagEffect *self);
void rag_hallucination_doubles(struct RagEffect *self);
unsigned char rag_tinted_puffs(struct RagEffect *self);
void rag_cast_circle_131(struct RagEffect *self, const char *tex);
void rag_three_rings_129(struct RagEffect *self, const char *tex, unsigned char style);
void rag_clouds_textured(struct RagEffect *self, const char *tex, signed char style);
void rag_electric_arcs(struct RagEffect *self, signed char style, int angle);
void rag_torch_flame(struct RagEffect *self, unsigned char flag);
unsigned char rag_spreading_rings(struct RagEffect *self, const char *tex, unsigned char wide);
void rag_blur_trail(struct RagEffect *self);
void rag_white_rings(struct RagEffect *self);
void rag_soul_particles(struct RagEffect *self, unsigned char style);
void rag_ground_ring_147(struct RagEffect *self, const char *tex, short style);
void rag_scan_arcs(struct RagEffect *self, short style);
void rag_ring_burst(struct RagEffect *self, const char *tex, signed char style);
void rag_snowfall(struct RagEffect *self, unsigned char style);
void rag_white_flash_136(struct RagEffect *self, const char *tex);
void rag_weapon_toss(struct RagEffect *self);
static __inline void rag_toss_wave(struct RagEffect *self)
{
    if (self->frame == 1 || self->frame == 11 || self->frame == 21 || self->frame == 31) {
        rag_follow_master(self);
        rag_play_wave_here(self, "effect\\복호격.wav");
    }
}

void rag_hallucination_double(struct RagEffect *self);
void rag_bubble_rings_144(struct RagEffect *self, int alpha_base,
                                     signed char style);
void rag_four_rings_139(struct RagEffect *self, int alpha_base, const char *tex, signed char style);
void rag_cast_bubbles_149(struct RagEffect *self,
                                     signed char style);
void rag_torch_ring_143(struct RagEffect *self, signed char style, short angle);
void rag_two_frame_ring_148(struct RagEffect *self, const char *tex0, const char *tex1);
void rag_throw_lines(struct RagEffect *self, short side, const char *tex);
void rag_thunder_spheres(struct RagEffect *self);
void rag_cloaking_fade(struct RagEffect *self);
void rag_rising_sparks(struct RagEffect *self);
void rag_tetra_vortex(struct RagEffect *self, signed char style);
void rag_ring_scatter_137(struct RagEffect *self, const char *tex, signed char style);
void rag_rocket_punches(struct RagEffect *self);
void rag_bubble_spiral_146(struct RagEffect *self, unsigned char style);
void rag_two_rings_161(struct RagEffect *self, const char *tex, signed char style);
void rag_white_rings_scaled(struct RagEffect *self, signed char style);
void rag_freezing_columns(struct RagEffect *self, const char *tex, unsigned char style);
void rag_water_ring(struct RagEffect *self);
void rag_project_master(struct RagEffect *self);
struct EffectPrim *rag_spark_launch(struct RagEffect *self, float rise, float accel);
void rag_spark_finish(struct EffectPrim *p);
void rag_random_tint_rings(struct RagEffect *self);
void rag_detecting_blink(struct RagEffect *self);
void rag_trinket_toss(struct RagEffect *self);

void rag_melody_column_156(struct RagEffect *self, const char *tex, unsigned char style);
void rag_line_to_offset_154(struct RagEffect *self, unsigned char style, const char *tex);
void rag_cloud_bursts(struct RagEffect *self, const char *tex, signed char colour);
void rag_tinted_sparks(struct RagEffect *self, signed char style);
void rag_falling_shards_157(struct RagEffect *self, const char *tex);
void rag_melody_notes_155(struct RagEffect *self, const char *tex, unsigned char style);
void rag_whitelight_151(struct RagEffect *self, const char *tex, unsigned char style);
void rag_mavka_152(struct RagEffect *self, unsigned char style);
void rag_summoned_monsters(struct RagEffect *self, signed char style);
void rag_ice_shards_150(struct RagEffect *self);

void rag_elec_rays_101(struct RagEffect *self, signed char style, int angle_base);
void rag_bubble_rings_158(struct RagEffect *self, int alpha_base,
                                     signed char style);
void rag_lines_to_offset_56(struct RagEffect *self, short sel);
void rag_shockwave_162(struct RagEffect *self, const char *tex);
void rag_shockwave_163(struct RagEffect *self, const char *tex);
void rag_shockwave_164(struct RagEffect *self, const char *tex);
void rag_shockwave_165(struct RagEffect *self, const char *tex);
void rag_epitree_aura_160(struct RagEffect *self, const char *tex, signed char style);
void rag_spr_ring_167(struct RagEffect *self, signed char style);
void rag_blue_fireballs(struct RagEffect *self);
void rag_pale_fireballs(struct RagEffect *self);
void rag_three_rings_60(struct RagEffect *self, const char *tex);
void rag_begin_spell_rings(struct RagEffect *self);
void rag_sphere_motes_159(struct RagEffect *self, const char *tex, signed char style);
void rag_double_shockwave_166(struct RagEffect *self, const char *tex);
void rag_cloud_dash_69(struct RagEffect *self);
void rag_master_whirl(struct RagEffect *self);

void rag_hallu_doubles_2(struct RagEffect *self);
void rag_vec3_mat_apply(Vec3 *out, const Vec3 *v, const float *m);
void rag_water_ball_rings(struct RagEffect *self);
void rag_blue_rings_169(struct RagEffect *self, const char *tex, signed char scale);
void rag_snow_134(struct RagEffect *self, unsigned char style);
void rag_blue_rings_121(struct RagEffect *self);
void rag_melody_notes_170(struct RagEffect *self, const char *tex, unsigned char style);
void rag_music_rings_173(struct RagEffect *self, signed char style,
                                    float rim);
void rag_yellow_lines_56(struct RagEffect *self, short sel);
void rag_green_rocket_punches(struct RagEffect *self);
void rag_white_rings_63(struct RagEffect *self, signed char style);
void rag_sonic_waves_146(struct RagEffect *self, const char *tex, signed char style);
void rag_digital_space_168(struct RagEffect *self, int effect_id);
void rag_cloud_dash_2(struct RagEffect *self);
void rag_hallu_walk(struct RagEffect *self);
void rag_fighter_rings_31(struct RagEffect *self, const char *tex);
void rag_after_image(struct RagEffect *self);
void rag_jack_frost(struct RagEffect *self);

unsigned char rag_effect_process300(struct RagEffect *self);

unsigned char rag_process_491(struct RagEffect *self);

#endif
