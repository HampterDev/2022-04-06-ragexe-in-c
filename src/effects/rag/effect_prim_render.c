#include "actor/object_ops.h"
#include "effects/rag/rag_effect.h"

void effect_prim_render(struct GameObject *object, const float *view)
{
    struct EffectPrim *self = (struct EffectPrim *)object;

    if (!self->base.visible || !prim_is_visible(self))
        return;

    switch (self->kind) {
    case 0: case 1:
        prim_screen_sprite_render(self);
        break;
    case 2:
        prim_screen_spike_render(self);
        break;
    case 3: case 19:
        prim_screen_ray_render(self);
        break;
    case 4:
        prim_screen_ring_render(self);
        break;

    case 5: case 6: case 7: case 8: case 21: case 40: case 46: case 51: case 84: case 85: case 124:
        prim_sprite_trail_render(self, view);
        break;
    case 9: case 117:
        prim_pyramid_render(self, view);
        break;
    case 10: case 11:
        prim_corner_trail_render(self, view);
        break;
    case 12:
        prim_ring_render(self, view);
        break;
    case 13:
        prim_band_render(self, view);
        break;
    case 14:
        prim_sphere_render(self, view);
        break;
    case 15:
        prim_upright_quad_render(self, view);
        break;
    case 16:
        prim_crossed_quads_render(self, view);
        break;
    case 20:
        prim_streak_render(self, view);
        break;
    case 22:
        prim_screen_card_render(self, view);
        break;
    case 23: case 24: case 25: case 30: case 31: case 41: case 50: case 57: case 60: case 63: case 65:
    case 107: case 108: case 113: case 118: case 139:
        prim_rings_render(self, view);
        break;
    case 26: case 28: case 39: case 67: case 88: case 161:
        prim_ground_squares_render(self, view);
        break;
    case 27:
        prim_light_groups_render(self, view);
        break;
    case 29: case 80:
        prim_aura_particles_render(self, view);
        break;
    case 32:
        prim_glow_orb_render(self, view);
        break;
    case 33: case 42: case 45: case 47: case 49: case 53: case 58: case 59: case 87: case 97: case 116:
    case 123: case 127:
        prim_billboards_render(self, view);
        break;
    case 34:
        prim_ring_wall_render(self, view);
        break;
    case 35:
        prim_spark_groups_render(self, view, 1.0f, 100, 100, 255);
        break;
    case 36: case 110: case 169:
        prim_rings_render_offset(self, view);
        break;
    case 37:
        prim_box_render(self, view);
        break;
    case 38:
        prim_tube_render(self, view);
        break;
    case 43:
        prim_orbit_square_render(self, view);
        break;
    case 44:
        prim_upright_bar_render(self, view);
        break;
    case 48:
        prim_wall_render(self, view);
        break;
    case 52:
        prim_sparks_render(self, view);
        break;
    case 54:
        prim_needle_render(self, view);
        break;
    case 55:
        prim_stripes_render(self, view);
        break;
    case 56:
        prim_slide_stripe_render(self, view);
        break;
    case 61:
        prim_slash_render(self, view);
        break;
    case 62:
        prim_strip_ring_render(self, view);
        break;
    case 64:
        prim_open_box_render(self, view);
        break;
    case 66:
        prim_prism_render(self, view);
        break;
    case 68: case 147:
        prim_halo_render(self, view);
        break;
    case 69:
        prim_pipe_render(self, view);
        break;
    case 70:
        prim_whip_render(self, view);
        break;
    case 71:
        prim_framed_panel_render(self, view);
        break;
    case 72:
        prim_scroll_grid_render(self, view);
        break;
    case 73:
        prim_leaning_ring_render(self, view);
        break;
    case 74:
        prim_column_ring_render(self, view);
        break;
    case 75:
        prim_blade_fan_render(self, view);
        break;
    case 76:
        prim_arch_render(self, view);
        break;
    case 77: case 91:
        prim_tilted_band_render(self, view);
        break;
    case 78:
        prim_tilted_square_render(self, view);
        break;
    case 79:
        prim_scroll_column_render(self, view);
        break;
    case 81:
        prim_spike_render(self, view);
        break;
    case 82: case 95: case 128: case 140:
        prim_texture_quads_render(self, view);
        break;
    case 86: case 92:
        prim_turned_square_render(self, view);
        break;
    case 89:
        prim_fan_wedge_render(self, view);
        break;
    case 90:
        prim_light_box_render(self, view);
        break;
    case 93:
        prim_rainbow_render(self, view);
        break;
    case 94:
        prim_glow_squares_render(self, view);
        break;
    case 96:
        prim_drift_squares_render(self, view);
        break;
    case 98:
        prim_upright_square_render(self, view);
        break;
    case 99:
        prim_orbit_glows_render(self, view);
        break;
    case 100:
        prim_arc_tube_render(self, view);
        break;
    case 101:
        prim_jagged_tube_render(self, view);
        break;
    case 102:
        prim_swing_tube_render(self, view);
        break;
    case 103:
        prim_square_stack_render(self, view);
        break;
    case 104:
        prim_banded_sphere_render(self, view);
        break;
    case 105:
        prim_energy_ring_render(self, view);
        break;
    case 106:
        prim_tilted_ring_render(self, view);
        break;
    case 109:
        prim_launch_square_render(self, view);
        break;
    case 111: case 115:
        prim_spark_trio_render(self, view);
        break;
    case 112:
        prim_ground_target_render(self, view);
        break;
    case 114:
        prim_spin_box_render(self, view);
        break;
    case 119: case 122: case 125:
        prim_flat_square_render(self, view);
        break;
    case 120:
        prim_spawn_effects_render(self);
        break;
    case 121:
        prim_arc_wall_render(self, view);
        break;
    case 126:
        prim_leaning_square_render(self, view);
        break;
    case 129:
        prim_wide_rings_render(self, view);
        break;
    case 130:
        prim_wander_glows_render(self, view);
        break;
    case 131:
        prim_nested_squares_render(self, view);
        break;
    case 132:
        prim_standing_square_render(self, view);
        break;
    case 133:
        prim_growing_row_render(self, view);
        break;
    case 134:
        prim_advancing_row_render(self, view);
        break;
    case 135:
        prim_swing_arc_render(self, view);
        break;
    case 136:
        prim_spiral_column_render(self, view);
        break;
    case 137:
        prim_crown_render(self, view);
        break;
    case 138:
        prim_drift_orb_render(self, view);
        break;
    case 141:
        prim_stretch_ray_render(self, view);
        break;
    case 142:
        prim_animated_panel_render(self, view);
        break;
    case 143:
        prim_spin_square_render(self, view);
        break;
    case 144:
        prim_wave_ring_render(self, view);
        break;
    case 145:
        prim_orb_blades_render(self, view);
        break;
    case 146:
        prim_walking_ring_render(self, view);
        break;
    case 148:
        prim_crossfade_square_render(self, view);
        break;
    case 149:
        prim_styled_ring_render(self, view);
        break;
    case 150:
        prim_rising_motes_render(self, view);
        break;
    case 151:
        prim_orbit_launcher_render(self);
        break;
    case 152:
        prim_sprite_ghosts_render(self, view);
        break;
    case 153:
        prim_sprite_halo_render(self, view);
        break;
    case 154:
        prim_wedge_pair_render(self, view);
        break;
    case 155: case 170:
        prim_twin_wisps_render(self, view);
        break;
    case 156:
        prim_jitter_quad_render(self, view);
        break;
    case 157:
        prim_drop_square_render(self, view);
        break;
    case 158:
        prim_lean_ring_render(self, view);
        break;
    case 159:
        prim_whirl_disc_render(self, view);
        break;
    case 160:
        prim_pulse_fan_render(self, view);
        break;
    case 162:
        prim_blue_wedge_render(self, view);
        break;
    case 163:
        prim_blue_blade_render(self, view);
        break;
    case 164:
        prim_dash_line_render(self, view);
        break;
    case 165:
        prim_bent_dashes_render(self, view);
        break;
    case 166:
        prim_ribbon_render(self, view);
        break;
    case 167:
        prim_flying_sprites_render(self, view);
        break;
    case 168:
        prim_ground_sprite_render(self, view);
        break;
    case 171:
        prim_facing_square_render(self, view);
        break;
    case 172:
        prim_camera_panel_render(self, view);
        break;
    case 173:
        prim_swell_rings_render(self, view);
        break;
    default:
        break;
    }
}
