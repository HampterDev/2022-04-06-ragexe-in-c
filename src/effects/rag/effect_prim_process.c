#include "actor/object_ops.h"
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"
#include "game/mode_callbacks.h"

#define PRIM_KIND_LAST 173

static int prim_kind_follows(int kind)
{
    switch (kind) {
    case 23: case 28: case 29: case 30: case 32: case 35: case 60: case 68: case 99:
    case 104: case 105: case 106: case 108: case 109: case 111: case 130: case 131:
    case 132: case 138: case 139: case 144: case 145: case 146: case 147: case 149:
    case 151: case 158: case 161: case 171: case 173:
        return 1;
    }
    return 0;
}

static int prim_held_this_frame(struct EffectPrim *self)
{
    int passes;

    switch (self->kind) {
    case 0:
        passes = (signed char)self->variant >= PRIM_VARIANT_STATE_ICON;
        break;
    case 16:
        passes = self->variant == PRIM_VARIANT_EVERY_FRAME;
        break;
    default:
        passes = 0;
        break;
    }
    if (passes)
        return 0;
    return g_isSkipFrame == 0;
}

unsigned char effect_prim_on_process(struct GameObject *object)
{
    struct EffectPrim *self = (struct EffectPrim *)object;

    char text[128];

    if (self->master == NULL)
        return 0;
    if ((unsigned)self->kind <= PRIM_KIND_LAST && prim_kind_follows(self->kind))
        self->base.pos = self->master->base.pos;
    if (prim_held_this_frame(self))
        return 1;

    switch ((unsigned)self->kind) {
    case 0: case 2:
        if (!prim_screen_sprite_process(self))
            return 0;
        break;
    case 1:
        if (!prim_screen_orbit_process(self))
            return 0;
        break;
    case 3: case 19:
        prim_screen_ray_process(self);
        break;
    case 4:
        prim_screen_ring_process(self);
        break;
    case 5:
        if (!prim_gravity_process(self))
            return 0;
        break;
    case 6:
        if (!prim_bounce_process(self))
            return 0;
        break;
    case 7:
        if (!prim_circling_process(self))
            return 0;
        break;
    case 8:   prim_swing_process(self); break;
    case 9:   prim_pyramid_process(self); break;
    case 10: case 11:
        prim_corner_trail_process(self);
        break;
    case 12:  prim_ring_process(self); break;
    case 13:  prim_band_process(self); break;
    case 14:  prim_sphere_process(self); break;
    case 15: case 16:
        prim_flight_process(self);
        break;
    case 17:
        break;
    case 18:  prim_rising_band_process(self); break;
    case 20:
        self->base.pos.y += 5.0f;
        break;
    case 21:
        if (!prim_gravity_fixed_process(self))
            return 0;
        break;
    case 22:
        if (!prim_flight_fixed_process(self))
            return 0;
        break;
    case 23:  prim_rings_rise_process(self); break;
    case 24:  prim_rings_wave_process(self); break;
    case 25:  prim_rings_rise_sway_process(self); break;
    case 26:  prim_ground_turn_process(self); break;
    case 27:  prim_light_groups_process(self); break;
    case 28:  prim_ground_bob_process(self); break;
    case 29:  prim_aura_particles_process(self); break;
    case 30:  prim_rings_spread_process(self); break;
    case 31:  prim_rings_widen_process(self); break;
    case 32:
        if (!prim_glow_orb_process(self))
            return 0;
        break;
    case 33:  prim_billboards_motes_process(self); break;
    case 34:  prim_ring_wall_process(self); break;
    case 35: case 99:
        prim_spark_groups_process(self);
        break;
    case 36:  prim_rings_ripple_process(self); break;
    case 37:  prim_box_process(self); break;
    case 38:  prim_tube_process(self); break;
    case 39:  prim_ground_styled_process(self); break;
    case 40:  prim_leaf_process(self); break;
    case 41:  prim_rings_late_rise_process(self); break;
    case 42:  prim_billboards_boomerang_process(self); break;
    case 43:  prim_orbit_square_process(self); break;
    case 44:  prim_upright_bar_process(self); break;
    case 45:  prim_billboards_scatter_process(self); break;
    case 46:  prim_sprite_at_point_process(self); break;
    case 47:  prim_billboards_bob_process(self); break;
    case 48:  prim_wall_process(self); break;
    case 49:  prim_billboards_sink_process(self); break;
    case 50:  prim_rings_breathe_process(self); break;
    case 51:  prim_sprite_pinned_process(self); break;
    case 52:  prim_sparks_process(self); break;
    case 53:  prim_billboards_throw_process(self); break;
    case 54:  prim_needle_process(self); break;
    case 55:  prim_stripes_process(self); break;
    case 56:  prim_slide_stripe_process(self); break;
    case 57:  prim_rings_swell_process(self); break;
    case 58:  prim_billboards_glow_process(self); break;
    case 59:  prim_billboards_burst_process(self); break;
    case 60: case 110:
        prim_rings_styled_process(self);
        break;
    case 61:  prim_slash_process(self); break;
    case 62:  prim_strip_ring_process(self); break;
    case 63:  prim_rings_pulse_process(self); break;
    case 64:  prim_open_box_process(self); break;
    case 65:  prim_rings_close_process(self); break;
    case 66:  prim_prism_process(self); break;
    case 67:  prim_ground_fade_process(self); break;
    case 68:  prim_halo_process(self); break;
    case 69:  prim_pipe_process(self); break;
    case 70:  prim_whip_process(self); break;
    case 71:  prim_framed_panel_process(self); break;
    case 72: case 172:
        prim_scroll_grid_process(self);
        break;
    case 73:  prim_leaning_ring_process(self); break;
    case 74:  prim_column_ring_process(self); break;
    case 75:  prim_blade_fan_process(self); break;
    case 76:  prim_arch_process(self); break;
    case 77:  prim_tilted_band_process(self); break;
    case 78:  prim_tilted_square_process(self); break;
    case 79:  prim_scroll_column_process(self); break;
    case 80:  prim_flakes_process(self); break;
    case 81:  prim_spike_process(self); break;
    case 82:  prim_texture_square_process(self); break;
    case 84:  prim_ghost_process(self); break;
    case 85:  prim_angel_process(self); break;
    case 86:  prim_turned_square_process(self); break;
    case 87:  prim_billboards_circle_up_process(self); break;
    case 88:  prim_ground_spread_process(self); break;
    case 89:  prim_fan_wedge_process(self); break;
    case 90:  prim_light_box_process(self); break;
    case 91:  prim_widening_band_process(self); break;
    case 92:  prim_flying_square_process(self); break;
    case 93:  prim_rainbow_process(self); break;
    case 94:  prim_glow_squares_process(self); break;
    case 95:  prim_texture_fade_process(self); break;
    case 96:  prim_drift_squares_process(self); break;
    case 97:  prim_billboards_fly_process(self); break;
    case 98:  prim_upright_square_process(self); break;
    case 100: prim_arc_tube_process(self); break;
    case 101: prim_jagged_tube_process(self); break;
    case 102: prim_swing_tube_process(self); break;
    case 103: prim_square_stack_process(self); break;
    case 104: prim_banded_sphere_process(self); break;
    case 105: prim_energy_ring_process(self); break;
    case 106: prim_tilted_ring_process(self); break;
    case 107: prim_rings_bloom_process(self); break;
    case 108: prim_rings_throb_process(self); break;
    case 109: prim_launch_square_process(self); break;
    case 111: prim_spark_trio_process(self); break;
    case 112: prim_ground_target_process(self); break;
    case 113: prim_rings_converge_process(self); break;
    case 114: prim_spin_box_process(self); break;
    case 115: prim_spark_burst_process(self); break;
    case 116: prim_billboards_fly_fade_process(self); break;
    case 117: prim_pyramid_surge_process(self); break;
    case 118: prim_rings_grow_process(self); break;
    case 119: prim_flat_square_process(self); break;
    case 120: prim_spawn_effects_process(self); break;
    case 121: prim_arc_wall_process(self); break;
    case 122: prim_homing_square_process(self); break;
    case 123: prim_billboards_sway_process(self); break;
    case 124: prim_petal_process(self); break;
    case 125: prim_veering_square_process(self); break;
    case 126: prim_leaning_square_process(self); break;
    case 127: prim_billboards_rise_process(self); break;
    case 128: prim_texture_cycle_process(self); break;
    case 129: prim_wide_rings_process(self); break;
    case 130: prim_wander_glows_process(self); break;
    case 131: prim_nested_squares_process(self); break;
    case 132: prim_standing_square_process(self); break;
    case 133: prim_growing_row_process(self); break;
    case 134: prim_advancing_row_process(self); break;
    case 135: prim_swing_arc_process(self); break;
    case 136: prim_spiral_column_process(self); break;
    case 137: prim_crown_process(self); break;
    case 138:
        if (!prim_drift_orb_process(self))
            return 0;
        break;
    case 139: prim_rings_spread_colors_process(self); break;
    case 140: prim_texture_frames_process(self); break;
    case 141: prim_stretch_ray_process(self); break;
    case 142: prim_animated_panel_process(self); break;
    case 143: prim_spin_square_process(self); break;
    case 144: prim_wave_ring_process(self); break;
    case 145:
        if (!prim_orb_blades_process(self))
            return 0;
        break;
    case 146: prim_walking_ring_process(self); break;
    case 147: prim_rising_bars_process(self); break;
    case 148: prim_crossfade_square_process(self); break;
    case 149: prim_styled_ring_process(self); break;
    case 150: prim_rising_motes_process(self); break;
    case 151: prim_orbit_launcher_process(self); break;
    case 152: prim_sprite_ghosts_process(self); break;
    case 153: prim_sprite_halo_process(self); break;
    case 154: prim_wedge_pair_process(self); break;
    case 155: prim_twin_wisps_process(self); break;
    case 156: prim_jitter_quad_process(self); break;
    case 157: prim_drop_square_process(self); break;
    case 158: prim_lean_ring_process(self); break;
    case 159: prim_whirl_disc_process(self); break;
    case 160: prim_pulse_fan_process(self); break;
    case 161: prim_bob_fade_process(self); break;
    case 162: prim_blue_wedge_process(self); break;
    case 163: prim_blue_blade_process(self); break;
    case 164: prim_dash_line_process(self); break;
    case 165: prim_bent_dashes_process(self); break;
    case 166: prim_ribbon_process(self); break;
    case 167:
        if (!prim_flying_sprites_process(self))
            return 0;
        break;
    case 168: prim_ground_sprite_process(self); break;
    case 169: prim_rings_ripple_from_process(self); break;
    case 170: prim_wisps_process(self); break;
    case 171: prim_facing_square_process(self); break;
    case 173: prim_swell_rings_process(self); break;
    default:
        sprintf(text, "Illegal Effect PrimitiveId %d", self->kind);
        error_msg(text, 0);
        break;
    }

    if (++self->frame > self->duration)
        return 0;
    return 1;
}
