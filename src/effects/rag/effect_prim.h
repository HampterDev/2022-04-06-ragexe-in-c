#include "actor/render_ops.h"
#include "actor/object_ops.h"
#include "render/rp_pool.h"
#ifndef EFFECTS_RAG_EFFECT_PRIM_H
#define EFFECTS_RAG_EFFECT_PRIM_H

#include "render/math3d.h"

#include "resources/attr_queries.h"

#include "effects/render_object.h"
#include "effects/particle.h"
#include "effects/rag/rag_effect_types.h"
#include "effects/rag/prim_records.h"

struct RagEffect;
struct Renderer;

struct PrimParticle {
    Vec3 pos;
    Vec3 corners[4];
    float size;
    float scale;
    float heading;
    float matrix[12];
    int r, g, b;
    float alpha;
    unsigned argb;
};
OFFSET_ASSERT(PrimParticle, size, 0x3c);
OFFSET_ASSERT(PrimParticle, scale, 0x40);
OFFSET_ASSERT(PrimParticle, matrix, 0x48);
OFFSET_ASSERT(PrimParticle, argb, 0x88);
SIZE_ASSERT(PrimParticle, 0x8c);

#define PRIM_PARTICLES 50

static __inline unsigned prim_argb(float alpha, int r, int g, int b)
{
    unsigned c = (unsigned)(int)alpha;
    c = (c << 8) + (unsigned char)r;
    c = (c << 8) + (unsigned char)g;
    c = (c << 8) + (unsigned char)b;
    return c;
}

#define PRIM_UNSET 0.0001f

struct PrimChannel {
    float value;
    float speed;
    float accel;
};

#define PRIM_PATTERN_ALPHA_BOUNCE 0x080
#define PRIM_PATTERN_HOLD_MOTION  0x800

struct EffectPrim {
    struct RenderObject base;
    int change_reached;
    int stage;
    struct RagEffect *master;
    struct Texture **textures;
    int texture_frame;
    int arrived;
    int alpha_keys_set_speed;
    int alpha_keys_set_alpha;
    int alpha_keys_set_size;
    int fading_out;
    float matrix[12];
    float matrix2[12];
    unsigned argb;
    int list_flags;
    unsigned char use_list_flags;
    int size2_flip_period;
    int kind;
    int pattern_arg;
    int alpha_key;
    unsigned pattern;
    int frame;
    int duration;
    int motion_period;
    int start_frame;
    int fade_frame;
    int particle_count;
    int spread_degrees;
    int color_hold_count;
    int change_frame;
    int motion_count;
    int bounce_clink;
    int color_hold_frames;
    int endless;
    int r, g, b;
    int r_speed;
    int g_speed;
    int b_speed;
    unsigned char loop_motion;
    struct Vec alpha_key_frames;
    struct Vec alpha_key_values;
    struct Vec alpha_key_ceilings;
    struct Vec alpha_key_floors;
    float arc_degrees;
    int arc_offset;
    Vec3 swing_axis;
    float next_speed;
    float next_accel;
    Vec3 next_velocity;
    Vec3 next_velocity_accel;
    float speed;
    float accel;
    float next_size1_accel;
    float next_size1_speed;
    float next_size2_speed;
    float next_size2_accel;
    float alpha_max;
    float alpha_min;
    int tex_u_left;
    float tex_u_right;
    float tex_v_top;
    float tex_v_bottom;
    float fall_speed;
    float gravity;
    float radius[2];
    float radius_speed[2];
    float radius_accel[2];
    float angle[3];
    float scale;
    float scale_speed;
    float scale_accel;
    float alpha;
    float alpha_speed;
    float fade_scale;
    float angle_speed[3];
    float angle_accel[3];
    struct PrimChannel size[3];
    float size2_max;
    Vec3 velocity;
    Vec3 master_pos;
    Vec3 delta;
    Vec3 delta2;
    Vec3 drift;
    Vec3 velocity_accel;
    Vec3 corners[4];
    unsigned char variant;
    short camera_yaw;
    short camera_pitch;
    union PrimRecords records;
    struct PrimParticle particles[PRIM_PARTICLES];
    struct PrimTriangle triangles[PRIM_TRIANGLES];
    int64_t master_sprite_lift;
};

#define PRIM_VARIANT_STATE_ICON 0x62
#define PRIM_VARIANT_EVERY_FRAME 0x63

#define PRIM_MSG_MASTER_GONE 0x18

struct EffectPrim *effect_prim_construct(struct EffectPrim *self);
void effect_prim_destruct(struct EffectPrim *self);
void effect_prim_init(struct EffectPrim *self, struct RagEffect *master, int kind, Vec3 delta);
void effect_prim_send_msg(struct EffectPrim *self, RenderMsgArg msg);
struct PrimParticle *prim_particle_construct(struct PrimParticle *p);
struct PrimTriangle *prim_triangle_construct(struct PrimTriangle *t);
unsigned char prim_step_motion(struct EffectPrim *self);
unsigned char prim_step_motion_counted(struct EffectPrim *self);
void prim_master_color(struct EffectPrim *self, unsigned char *r, unsigned char *g, unsigned char *b);
void prim_heading_degrees(float x, float z, short *out);
void prim_sync_particles(struct EffectPrim *self);
void prim_update_alpha(struct EffectPrim *self);
void prim_shift_trail(struct EffectPrim *self);
void prim_quad_turn(Vec3 *p0, Vec3 *p1, Vec3 *p2, Vec3 *p3, short yaw, short pitch);

unsigned char prim_is_visible(struct EffectPrim *self);

struct TexDrawVertex *prim_project_vertex(struct TexDrawVertex *out, const float *matrix, Vec3 pos);

void prim_draw_quad(struct EffectPrim *self, int *count, short alpha, Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3,
                               const float *matrix, short blend, char texture, unsigned char in_world,
                               unsigned char r, unsigned char g, unsigned char b, float v_p2p3, float v_p0p1,
                               float u_p0p3, float u_p1p2, unsigned char mode);
#define PRIM_BLEND_ADDITIVE  0
#define PRIM_BLEND_OPAQUE    10
#define PRIM_BLEND_ALPHA     99
#define PRIM_QUAD_PROJECTED  0
#define PRIM_QUAD_IN_FRONT   1
#define PRIM_QUAD_DEPTH_BIAS 2
#define PRIM_QUAD_UPRIGHT    3
void prim_draw_strip(struct EffectPrim *self, int *count, int segment, short alpha, Vec3 p0,
                                Vec3 p1, Vec3 p2, Vec3 p3, const float *matrix, char texture);

unsigned char prim_gravity_process(struct EffectPrim *self);
unsigned char prim_gravity_fixed_process(struct EffectPrim *self);
unsigned char prim_bounce_process(struct EffectPrim *self);
unsigned char prim_circling_process(struct EffectPrim *self);
unsigned char prim_swing_process(struct EffectPrim *self);
unsigned char prim_leaf_process(struct EffectPrim *self);
unsigned char prim_ghost_process(struct EffectPrim *self);
unsigned char prim_angel_process(struct EffectPrim *self);
unsigned char prim_petal_process(struct EffectPrim *self);
void prim_sprite_trail_render(struct EffectPrim *self, const float *view);
void prim_sprite_at_point_process(struct EffectPrim *self);
void prim_sprite_pinned_process(struct EffectPrim *self);

#define PRIM_RING_OFFSET_ON 1234.5f

void prim_rings_render(struct EffectPrim *self, const float *view);
void prim_rings_render_offset(struct EffectPrim *self, const float *view);
unsigned char prim_rings_rise_process(struct EffectPrim *self);
unsigned char prim_rings_rise_sway_process(struct EffectPrim *self);
void prim_rings_spread_process(struct EffectPrim *self);
void prim_rings_widen_process(struct EffectPrim *self);
unsigned char prim_rings_styled_process(struct EffectPrim *self);
unsigned char prim_rings_pulse_process(struct EffectPrim *self);
unsigned char prim_rings_grow_process(struct EffectPrim *self);
void prim_rings_spread_colors_process(struct EffectPrim *self);
void prim_rings_wave_process(struct EffectPrim *self);
void prim_rings_ripple_process(struct EffectPrim *self);
void prim_rings_late_rise_process(struct EffectPrim *self);
void prim_rings_breathe_process(struct EffectPrim *self);
void prim_rings_swell_process(struct EffectPrim *self);
void prim_rings_close_process(struct EffectPrim *self);
void prim_rings_bloom_process(struct EffectPrim *self);
void prim_rings_throb_process(struct EffectPrim *self);
void prim_rings_converge_process(struct EffectPrim *self);
void prim_rings_ripple_from_process(struct EffectPrim *self);

void prim_billboards_render(struct EffectPrim *self, const float *view);
unsigned char prim_billboards_motes_process(struct EffectPrim *self);
void prim_billboards_boomerang_process(struct EffectPrim *self);
void prim_billboards_scatter_process(struct EffectPrim *self);
void prim_billboards_bob_process(struct EffectPrim *self);
void prim_billboards_throw_process(struct EffectPrim *self);
void prim_billboards_circle_up_process(struct EffectPrim *self);
void prim_billboards_fly_process(struct EffectPrim *self);
void prim_billboards_fly_fade_process(struct EffectPrim *self);
unsigned char prim_billboards_sway_process(struct EffectPrim *self);
void prim_billboards_rise_process(struct EffectPrim *self);
void prim_billboards_sink_process(struct EffectPrim *self);
void prim_billboards_glow_process(struct EffectPrim *self);
void prim_billboards_burst_process(struct EffectPrim *self);

void prim_texture_square_process(struct EffectPrim *self);
void prim_texture_frames_process(struct EffectPrim *self);
unsigned char prim_bob_fade_process(struct EffectPrim *self);
void prim_ground_squares_render(struct EffectPrim *self, const float *view);
void prim_texture_quads_render(struct EffectPrim *self, const float *view);
void prim_ground_turn_process(struct EffectPrim *self);
void prim_ground_bob_process(struct EffectPrim *self);
void prim_ground_styled_process(struct EffectPrim *self);
void prim_ground_fade_process(struct EffectPrim *self);
void prim_ground_spread_process(struct EffectPrim *self);
void prim_texture_fade_process(struct EffectPrim *self);
void prim_texture_cycle_process(struct EffectPrim *self);

static __inline Vec3 prim_mat_apply(const float *m, float x, float y, float z)
{
    Vec3 r;

    r.x = m[0] * x + m[3] * y + m[6] * z + m[9];
    r.y = m[1] * x + m[4] * y + m[7] * z + m[10];
    r.z = m[2] * x + m[5] * y + m[8] * z + m[11];
    return r;
}

static __inline void prim_set_vertex(struct TexDrawVertex *out, const float *view, Vec3 pos, unsigned color, float u,
                                     float v)
{
    prim_project_vertex(out, view, pos);
    out->diffuse = color;
    out->u = u;
    out->v = v;
}

static __inline void prim_set_screen_vertex(struct TexDrawVertex *out, float x, float y, unsigned color, float u, float v)
{
    out->x = x;
    out->y = y;
    out->z = 1e-6f;
    out->rhw = 0.999999f;
    out->diffuse = color;
    out->u = u;
    out->v = v;
}

unsigned char prim_screen_sprite_process(struct EffectPrim *self);
void prim_screen_sprite_render(struct EffectPrim *self);
void prim_screen_spike_render(struct EffectPrim *self);
void prim_screen_ring_render(struct EffectPrim *self);
unsigned char prim_screen_orbit_process(struct EffectPrim *self);
void prim_screen_ray_process(struct EffectPrim *self);
void prim_screen_ray_render(struct EffectPrim *self);
void prim_screen_ring_process(struct EffectPrim *self);

void prim_pyramid_process(struct EffectPrim *self);
void prim_ring_process(struct EffectPrim *self);
unsigned char prim_band_process(struct EffectPrim *self);
void prim_pyramid_render(struct EffectPrim *self, const float *view);
void prim_corner_trail_render(struct EffectPrim *self, const float *view);
void prim_ring_render(struct EffectPrim *self, const float *view);
void prim_band_render(struct EffectPrim *self, const float *view);
void prim_corner_trail_process(struct EffectPrim *self);
void prim_pyramid_surge_process(struct EffectPrim *self);

unsigned char prim_flight_process(struct EffectPrim *self);
unsigned char prim_flight_fixed_process(struct EffectPrim *self);
unsigned char prim_rising_band_process(struct EffectPrim *self);
unsigned char prim_light_groups_process(struct EffectPrim *self);
void prim_sphere_render(struct EffectPrim *self, const float *view);
void prim_upright_quad_render(struct EffectPrim *self, const float *view);
void prim_crossed_quads_render(struct EffectPrim *self, const float *view);
void prim_light_groups_render(struct EffectPrim *self, const float *view);
void prim_sphere_process(struct EffectPrim *self);
void prim_streak_render(struct EffectPrim *self, const float *view);
void prim_screen_card_render(struct EffectPrim *self, const float *view);

unsigned char prim_aura_particles_process(struct EffectPrim *self);
void prim_aura_particles_render(struct EffectPrim *self, const float *view);
unsigned char prim_glow_orb_process(struct EffectPrim *self);
void prim_glow_orb_render(struct EffectPrim *self, const float *view);
void prim_ring_wall_render(struct EffectPrim *self, const float *view);
unsigned char prim_spark_groups_process(struct EffectPrim *self);
void prim_spark_groups_render(struct EffectPrim *self, const float *view, float scale, unsigned char r,
                                         unsigned char g, unsigned char b);
void prim_ring_wall_process(struct EffectPrim *self);

void prim_box_render(struct EffectPrim *self, const float *view);
void prim_tube_render(struct EffectPrim *self, const float *view);
void prim_orbit_square_render(struct EffectPrim *self, const float *view);
void prim_upright_bar_render(struct EffectPrim *self, const float *view);
void prim_wall_render(struct EffectPrim *self, const float *view);
void prim_sparks_process(struct EffectPrim *self);
void prim_sparks_render(struct EffectPrim *self, const float *view);
void prim_needle_render(struct EffectPrim *self, const float *view);
void prim_stripes_process(struct EffectPrim *self);
void prim_stripes_render(struct EffectPrim *self, const float *view);
void prim_box_process(struct EffectPrim *self);
void prim_tube_process(struct EffectPrim *self);
void prim_orbit_square_process(struct EffectPrim *self);
void prim_upright_bar_process(struct EffectPrim *self);
void prim_wall_process(struct EffectPrim *self);
void prim_needle_process(struct EffectPrim *self);

void prim_slide_stripe_render(struct EffectPrim *self, const float *view);
void prim_slash_render(struct EffectPrim *self, const float *view);
unsigned char prim_strip_ring_process(struct EffectPrim *self);
void prim_strip_ring_render(struct EffectPrim *self, const float *view);
void prim_open_box_render(struct EffectPrim *self, const float *view);
void prim_prism_render(struct EffectPrim *self, const float *view);
unsigned char prim_halo_process(struct EffectPrim *self);
void prim_halo_render(struct EffectPrim *self, const float *view);
void prim_pipe_render(struct EffectPrim *self, const float *view);
void prim_whip_render(struct EffectPrim *self, const float *view);
void prim_slide_stripe_process(struct EffectPrim *self);
void prim_slash_process(struct EffectPrim *self);
void prim_open_box_process(struct EffectPrim *self);
void prim_prism_process(struct EffectPrim *self);
void prim_pipe_process(struct EffectPrim *self);
void prim_whip_process(struct EffectPrim *self);

void prim_framed_panel_render(struct EffectPrim *self, const float *view);
void prim_scroll_grid_process(struct EffectPrim *self);
void prim_scroll_grid_render(struct EffectPrim *self, const float *view);
void prim_leaning_ring_render(struct EffectPrim *self, const float *view);
void prim_column_ring_render(struct EffectPrim *self, const float *view);
void prim_blade_fan_render(struct EffectPrim *self, const float *view);
void prim_arch_render(struct EffectPrim *self, const float *view);
void prim_tilted_band_render(struct EffectPrim *self, const float *view);
void prim_tilted_square_render(struct EffectPrim *self, const float *view);
void prim_scroll_column_render(struct EffectPrim *self, const float *view);
void prim_framed_panel_process(struct EffectPrim *self);
void prim_leaning_ring_process(struct EffectPrim *self);
void prim_column_ring_process(struct EffectPrim *self);
void prim_blade_fan_process(struct EffectPrim *self);
void prim_arch_process(struct EffectPrim *self);
void prim_tilted_band_process(struct EffectPrim *self);
void prim_tilted_square_process(struct EffectPrim *self);
void prim_scroll_column_process(struct EffectPrim *self);
void prim_widening_band_process(struct EffectPrim *self);
void prim_camera_panel_render(struct EffectPrim *self, const float *view);

void prim_flakes_process(struct EffectPrim *self);
void prim_spike_render(struct EffectPrim *self, const float *view);
void prim_turned_square_render(struct EffectPrim *self, const float *view);
void prim_fan_wedge_render(struct EffectPrim *self, const float *view);
void prim_spike_process(struct EffectPrim *self);
void prim_turned_square_process(struct EffectPrim *self);
void prim_fan_wedge_process(struct EffectPrim *self);
void prim_flying_square_process(struct EffectPrim *self);

void prim_light_box_process(struct EffectPrim *self);
void prim_light_box_render(struct EffectPrim *self, const float *view);
void prim_rainbow_quad(struct EffectPrim *self, int *count, int seg, short alpha, Vec3 p0,
                                  Vec3 p1, Vec3 p2, Vec3 p3, const float *matrix, unsigned char blend,
                                  unsigned char r, unsigned char g, unsigned char b);
void prim_rainbow_render(struct EffectPrim *self, const float *view);
void prim_glow_squares_render(struct EffectPrim *self, const float *view);
unsigned char prim_drift_squares_process(struct EffectPrim *self);
void prim_drift_squares_render(struct EffectPrim *self, const float *view);
void prim_upright_square_render(struct EffectPrim *self, const float *view);
void prim_orbit_glows_render(struct EffectPrim *self, const float *view);
void prim_arc_tube_render(struct EffectPrim *self, const float *view);
void prim_jagged_tube_render(struct EffectPrim *self, const float *view);
void prim_rainbow_process(struct EffectPrim *self);
void prim_glow_squares_process(struct EffectPrim *self);
void prim_upright_square_process(struct EffectPrim *self);
void prim_arc_tube_process(struct EffectPrim *self);
void prim_jagged_tube_process(struct EffectPrim *self);

void prim_swing_tube_process(struct EffectPrim *self);
void prim_swing_tube_render(struct EffectPrim *self, const float *view);
void prim_square_stack_render(struct EffectPrim *self, const float *view);
void prim_banded_sphere_render(struct EffectPrim *self, const float *view);
void prim_energy_ring_render(struct EffectPrim *self, const float *view);
void prim_tilted_ring_render(struct EffectPrim *self, const float *view);
void prim_launch_square_render(struct EffectPrim *self, const float *view);
unsigned char prim_spark_trio_process(struct EffectPrim *self);
void prim_spark_burst_process(struct EffectPrim *self);
void prim_spark_trio_render(struct EffectPrim *self, const float *view);
void prim_ground_target_render(struct EffectPrim *self, const float *view);
void prim_spin_box_render(struct EffectPrim *self, const float *view);
void prim_square_stack_process(struct EffectPrim *self);
void prim_banded_sphere_process(struct EffectPrim *self);
void prim_energy_ring_process(struct EffectPrim *self);
void prim_tilted_ring_process(struct EffectPrim *self);
void prim_launch_square_process(struct EffectPrim *self);
void prim_ground_target_process(struct EffectPrim *self);
void prim_spin_box_process(struct EffectPrim *self);

void prim_flat_square_render(struct EffectPrim *self, const float *view);
void prim_homing_square_process(struct EffectPrim *self);
void prim_veering_square_process(struct EffectPrim *self);
void prim_spawn_effects_render(struct EffectPrim *self);
void prim_arc_wall_render(struct EffectPrim *self, const float *view);
void prim_leaning_square_render(struct EffectPrim *self, const float *view);
void prim_flat_square_process(struct EffectPrim *self);
void prim_spawn_effects_process(struct EffectPrim *self);
void prim_arc_wall_process(struct EffectPrim *self);
void prim_leaning_square_process(struct EffectPrim *self);

unsigned char prim_wander_glows_process(struct EffectPrim *self);
void prim_wander_glows_render(struct EffectPrim *self, const float *view);
unsigned char prim_nested_squares_process(struct EffectPrim *self);
void prim_nested_squares_render(struct EffectPrim *self, const float *view);
void prim_standing_square_render(struct EffectPrim *self, const float *view);
void prim_growing_row_render(struct EffectPrim *self, const float *view);
void prim_advancing_row_render(struct EffectPrim *self, const float *view);
void prim_standing_square_process(struct EffectPrim *self);
void prim_growing_row_process(struct EffectPrim *self);
void prim_advancing_row_process(struct EffectPrim *self);
void prim_wide_rings_process(struct EffectPrim *self);
void prim_wide_rings_render(struct EffectPrim *self, const float *view);

#include <stdlib.h>
#include <stdint.h>
static __inline int prim_rand_below(int n)
{
    int r;

    if (n == 0)
        return 0;
    r = rand() % n;
    return r < 0 ? 0 : r;
}

void prim_swing_arc_process(struct EffectPrim *self);
void prim_swing_arc_render(struct EffectPrim *self, const float *view);
void prim_spiral_column_process(struct EffectPrim *self);
void prim_spiral_column_render(struct EffectPrim *self, const float *view);
void prim_spiral_column_draw(struct EffectPrim *self, const float *view, int *count, float scale,
                                        short index, short blend, unsigned char r, unsigned char g,
                                        unsigned char b);
void prim_crown_process(struct EffectPrim *self);
void prim_crown_render(struct EffectPrim *self, const float *view);
void prim_stretch_ray_process(struct EffectPrim *self);
void prim_stretch_ray_render(struct EffectPrim *self, const float *view);
void prim_animated_panel_process(struct EffectPrim *self);
void prim_animated_panel_render(struct EffectPrim *self, const float *view);
void prim_face_camera(struct EffectPrim *self);

unsigned char prim_drift_orb_process(struct EffectPrim *self);
void prim_drift_orb_render(struct EffectPrim *self, const float *view);
unsigned char prim_spin_square_process(struct EffectPrim *self);
void prim_spin_square_render(struct EffectPrim *self, const float *view);
void prim_wave_ring_process(struct EffectPrim *self);
void prim_wave_ring_render(struct EffectPrim *self, const float *view);
unsigned char prim_orb_blades_process(struct EffectPrim *self);
void prim_orb_blades_render(struct EffectPrim *self, const float *view);
unsigned char prim_walking_ring_process(struct EffectPrim *self);
void prim_walking_ring_render(struct EffectPrim *self, const float *view);
unsigned char prim_rising_bars_process(struct EffectPrim *self);
void prim_spin_square_corners(struct PrimSpinSquare *sq);
void prim_drift_angles(float (*pairs)[2], int count);

unsigned char prim_crossfade_square_process(struct EffectPrim *self);
void prim_crossfade_square_render(struct EffectPrim *self, const float *view);
void prim_styled_ring_process(struct EffectPrim *self);
void prim_styled_ring_render(struct EffectPrim *self, const float *view);
void prim_rising_motes_process(struct EffectPrim *self);
void prim_rising_motes_render(struct EffectPrim *self, const float *view);
unsigned char prim_orbit_launcher_process(struct EffectPrim *self);
void prim_orbit_launcher_render(struct EffectPrim *self);
unsigned char prim_sprite_ghosts_process(struct EffectPrim *self);
void prim_sprite_ghosts_render(struct EffectPrim *self, const float *view);
unsigned char prim_sprite_halo_process(struct EffectPrim *self);
void prim_sprite_halo_render(struct EffectPrim *self, const float *view);
void prim_wedge_pair_process(struct EffectPrim *self);
void prim_wedge_pair_render(struct EffectPrim *self, const float *view);
unsigned char prim_twin_wisps_process(struct EffectPrim *self);
void prim_twin_wisps_render(struct EffectPrim *self, const float *view);
void prim_camera_square(struct EffectPrim *self, float size, Vec3 q[4]);

void prim_jitter_quad_process(struct EffectPrim *self);
void prim_jitter_quad_render(struct EffectPrim *self, const float *view);
unsigned char prim_drop_square_process(struct EffectPrim *self);
void prim_drop_square_render(struct EffectPrim *self, const float *view);
void prim_lean_ring_process(struct EffectPrim *self);
void prim_lean_ring_render(struct EffectPrim *self, const float *view);
void prim_whirl_disc_process(struct EffectPrim *self);
void prim_whirl_disc_render(struct EffectPrim *self, const float *view);
void prim_pulse_fan_process(struct EffectPrim *self);
void prim_pulse_fan_render(struct EffectPrim *self, const float *view);
void prim_blue_wedge_process(struct EffectPrim *self);
void prim_blue_wedge_render(struct EffectPrim *self, const float *view);
void prim_blue_blade_process(struct EffectPrim *self);
void prim_blue_blade_render(struct EffectPrim *self, const float *view);
void prim_dash_line_process(struct EffectPrim *self);
void prim_dash_line_render(struct EffectPrim *self, const float *view);
void prim_bent_dashes_process(struct EffectPrim *self);
void prim_bent_dashes_render(struct EffectPrim *self, const float *view);

void prim_ribbon_process(struct EffectPrim *self);
void prim_ribbon_render(struct EffectPrim *self, const float *view);
unsigned char prim_flying_sprites_process(struct EffectPrim *self);
void prim_flying_sprites_render(struct EffectPrim *self, const float *view);
unsigned char prim_ground_sprite_process(struct EffectPrim *self);
void prim_ground_sprite_render(struct EffectPrim *self, const float *view);
unsigned char prim_wisps_process(struct EffectPrim *self);
void prim_facing_square_render(struct EffectPrim *self, const float *view);
void prim_swell_rings_process(struct EffectPrim *self);
void prim_swell_rings_render(struct EffectPrim *self, const float *view);
void prim_facing_square_process(struct EffectPrim *self);

#endif
