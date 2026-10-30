#include "actor/render_ops.h"
#include "actor/object_ops.h"
#include "contents/sound.h"
#include "effects/durations.h"
#include "game/context.h"
#include <stdlib.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"
#include "actor/actor_common.h"
#include "game/mode_callbacks.h"

#define GAME_MSG_QUAKE 0x7a

static int rag_runs_every_frame(int id)
{
    switch (id) {
    case 25: case 37: case 49: case 51: case 52: case 53: case 147: case 152:
    case 200: case 201: case 202: case 203: case 228: case 254: case 312: case 313:
    case 320: case 325: case 328: case 362: case 397: case 398: case 418: case 436:
    case 437: case 438: case 441: case 454: case 467: case 468: case 469: case 470:
    case 471: case 472: case 473: case 488: case 489: case 490: case 503: case 504:
    case 506: case 523: case 524: case 525: case 526: case 527: case 528: case 529:
    case 530: case 531: case 532: case 533: case 534: case 535: case 536: case 590:
    case 603: case 629: case 678: case 679: case 680: case 755: case 757: case 788:
    case 789: case 790: case 826: case 833: case 837: case 850: case 861: case 865:
    case 867: case 871: case 873: case 875: case 877: case 881: case 911: case 920:
    case 932: case 940: case 1122:
        return 1;
    }
    return 0;
}

static void quake(int par1)
{
    game_send_message(GAME_MSG_QUAKE, par1, 0, 0, 0);
}

static void follow_wave(struct RagEffect *self, const char *wave, float y)
{
    rag_follow_master(self);
    rag_play_wave(self, wave, y);
}

static int str_step(struct RagEffect *self)
{
    return rag_str_step(self, 0.0f, -80.0f, 0.0f) != 0;
}

static int follow_wave_str(struct RagEffect *self, int frame, const char *wave, float y)
{
    if (self->frame == frame)
        follow_wave(self, wave, y);
    return rag_str_only(self) != 0;
}

static void tint_master(struct RagEffect *self, unsigned bit, int r, int g, int b)
{
    self->master->overlay_flags |= bit;
    render_object_set_argb(self->master, -1, r, g, b);
}

static void tint_master_random(struct RagEffect *self)
{
    int b = rag_rand_mod(255);
    int g = rag_rand_mod(255);
    int r = rag_rand_mod(255);

    render_object_set_argb(self->master, -1, r, g, b);
}

static void spin_master(struct RagEffect *self, float step)
{
    self->master->roty += step;
    if (self->master->roty > 360.0f)
        self->master->roty -= 360.0f;
}

static void hit_bursts(struct RagEffect *self, int n, const char *texture, char style)
{
    for (; n > 0; n--)
        rag_hit_burst(self, texture, style);
}

static void storm_and_swirls(struct RagEffect *self, char look)
{
    rag_storm_ring(self, "effect\\storm2.tga", look);
    rag_cloud_swirl(self, 2);
    rag_cloud_swirl(self, 3);
}

unsigned char rag_effect_on_process(struct GameObject *object)
{
    struct RagEffect *self = (struct RagEffect *)object;

    struct RenderObject *fx;
    int steps_due, id, n;
    char message[128];

    if (self->master == NULL)
        return 0;
    steps_due = g_isSkipFrame;
    if (!rag_runs_every_frame(self->effect_id) && steps_due == 0)
        return 1;
    if (steps_due > 0 && self->master->drawn == 1) {
        self->master->overlay_flags = 0;
        self->master->drawn = 0;
    }

    id = self->effect_id;
    if ((unsigned)(id - 300) <= 73) {
        if (!rag_effect_process300(self))
            return 0;
    } else if (id >= 491) {
        if (!rag_process_491(self))
            return 0;
    } else {
        fx = self->master;
        switch (id) {
        case 0:   rag_hit1(self); break;
        case 1:   rag_hit2(self, 0); break;
        case 2:   rag_hit3(self); break;
        case 3:   rag_hit4(self); break;
        case 4:   rag_hit5(self); break;
        case 5:   rag_hit6(self); break;
        case 6:   rag_entry(self); break;
        case 7:   rag_exit(self); break;
        case 8:   rag_warp(self); break;
        case 9:   rag_enhance(self); break;
        case 10:
            if (!rag_coin(self))
                return 0;
            break;
        case 11:  rag_endure(self); break;
        case 12:  rag_cast_yellow_rings_1(self); break;
        case 13:
            if (!rag_glass_wall(self))
                return 0;
            break;
        case 14:  rag_heal_sp(self); break;
        case 15:  rag_soul_strike(self, 0); break;
        case 16:  rag_bash(self); break;
        case 17:  rag_magnum_break(self); break;
        case 18:  rag_steal(self); break;
        case 20: case 125:
            rag_poison_attack(self, 0);
            break;
        case 21:  rag_detoxication(self); break;
        case 22:  rag_sight(self); break;
        case 24:  rag_fire_ball(self); break;
        case 25: case 29:
            if (!str_step(self))
                return 0;
            break;
        case 26:  rag_ice_arrow(self); break;
        case 27:  rag_frost_diver(self, 0); break;
        case 28:  rag_frost_diver2(self); break;
        case 30:
            if (!rag_thunderstorm_wave(self))
                return 0;
            break;
        case 31:  rag_fire_arrow(self); break;
        case 32:  rag_napalm_beat(self); break;
        case 33:  rag_ruwach(self); break;
        case 34:  rag_teleport_ring(self); break;
        case 35:  rag_ready_portal_rings(self); break;
        case 36:  rag_portal(self); break;
        case 37:  rag_inc_agility(self); break;
        case 38:  rag_dec_agility(self); break;
        case 39:  rag_holy_water(self); break;
        case 40:
            if (!rag_signum(self))
                return 0;
            break;
        case 41:
            if (!rag_angelus_wave(self))
                return 0;
            break;
        case 42:  rag_blessing(self); break;
        case 43:  rag_inc_agi_dex(self); break;
        case 44:  rag_chimney_smokes(self); break;
        case 45:  rag_firefly(self); break;
        case 46:  rag_sand_wind(self); break;
        case 47:  rag_torch(self); break;
        case 48:  rag_spray_pond(self); break;
        case 49:
            if (!rag_attr_wave(self))
                return 0;
            break;
        case 50:  rag_fire_ring(self); break;
        case 51:  rag_cold_hit(self); break;
        case 53:  rag_poison_hit(self); break;
        case 54:  rag_cast_blue(self); break;
        case 55:  rag_cast_yellow(self); break;
        case 56:  rag_cast_green_k25(self); break;
        case 57:  rag_cast_yellow_plain(self); break;
        case 58:  rag_cast_white_rings(self); break;
        case 59:  rag_cast_violet_k25(self); break;
        case 60:  rag_lock_on(self); break;
        case 61:  rag_warp_zone(self); break;
        case 62:  rag_sightrasher(self); break;
        case 63:  rag_big_explosion(self); break;
        case 65:
            if (!rag_invenom_wave(self))
                return 0;
            break;
        case 66:
            if (!rag_cure_wave(self))
                return 0;
            break;
        case 67:
            if (!rag_provoke_waves(self))
                return 0;
            break;
        case 68:
            if (!rag_mvp_wave(self))
                return 0;
            break;
        case 69:
            if (!rag_skid_trap_wave(self))
                return 0;
            break;
        case 71:  rag_particle_spray(self); break;
        case 72:  rag_sphere(self); break;
        case 73:  rag_bowling_bash(self); break;
        case 74:  rag_ice_wall(self); break;
        case 75:
            if (!rag_gloria_wave(self))
                return 0;
            break;
        case 76:
            if (!rag_magnificat_wave(self))
                return 0;
            break;
        case 77:
            if (!rag_resurrection_wave(self))
                return 0;
            break;
        case 78:
            if (!rag_recovery_wave(self))
                return 0;
            break;
        case 79:  rag_earth_spike(self, 0); break;
        case 80:  rag_spear_boomerang(self); break;
        case 81:  rag_pierce(self); break;
        case 82:  rag_turn_undead(self); break;
        case 83:
            if (!rag_sanctuary_wave(self))
                return 0;
            break;
        case 84:
            if (!rag_impositio_wave(self))
                return 0;
            break;
        case 85:
            if (!rag_lex_aeterna_wave(self))
                return 0;
            break;
        case 86:
            if (!rag_aspersio(self))
                return 0;
            break;
        case 87:
            if (!rag_lex_divina(self))
                return 0;
            break;
        case 88:
            if (!rag_suffragium_wave(self))
                return 0;
            break;
        case 89:
            if (!rag_storm_gust(self))
                return 0;
            break;
        case 90:
            if (!rag_meteor_storm_waves(self))
                return 0;
            break;
        case 91:
            if (!rag_benedictio_wave(self))
                return 0;
            break;
        case 92:
            if (!rag_meteor_wave_quake(self))
                return 0;
            break;
        case 93:  rag_jupitel(self, 0); break;
        case 94:  rag_jupitel_hit(self, 0); break;
        case 95:
            if (!rag_quagmire_wave(self))
                return 0;
            break;
        case 96:
            if (!rag_fire_pillar_a_wave(self))
                return 0;
            break;
        case 97:
            if (!rag_fire_pillar_b_wave(self))
                return 0;
            break;
        case 98:  rag_adrenaline_rush(self); break;
        case 99:  rag_flasher(self); break;
        case 100: rag_remove_trap(self); break;
        case 101:
            if (!rag_weapon_repair_waves(self))
                return 0;
            break;
        case 102:
            if (!rag_fire_pillar(self))
                return 0;
            break;
        case 103:
            if (!rag_weapon_perfection_wave(self))
                return 0;
            break;
        case 104:
            if (!rag_maximize_power_waves(self))
                return 0;
            break;
        case 106:
            if (!rag_blast_mine_wave(self))
                return 0;
            break;
        case 107:
            if (!rag_claymore_trap_wave(self))
                return 0;
            break;
        case 108:
            if (!rag_freezing_trap_wave(self))
                return 0;
            break;
        case 111:
            if (!rag_spring_trap_wave(self))
                return 0;
            break;
        case 112:
            if (!rag_kyrie_eleison_waves(self))
                return 0;
            break;
        case 113:
            if (!rag_magnus_wave(self))
                return 0;
            break;
        case 114: rag_violet_cross(self); break;
        case 115: rag_blitz_beat(self); break;
        case 116: rag_water_out(self); break;
        case 117: rag_water_ball(self); break;
        case 118: rag_fire_ivy(self); break;
        case 119: rag_detecting(self, 0); break;
        case 120: rag_cloaking(self); break;
        case 121: rag_stone_curse(self); break;
        case 122: rag_sonic_blow_hit(self); break;
        case 123: rag_grimtooth(self); break;
        case 124: case 127:
            if (!rag_poison_react_wave(self))
                return 0;
            break;
        case 128: rag_over_thrust(self); break;
        case 129:
            if (!rag_venom_splasher_wave(self))
                return 0;
            break;
        case 130: case 250:
            rag_two_hand_quicken(self);
            break;
        case 131:
            if (!rag_auto_counter_wave(self))
                return 0;
            break;
        case 132: rag_grimtooth_attack(self); break;
        case 133: case 134:
            if (!rag_str_step_or_stop(self))
                return 0;
            break;
        case 136: case 391: case 406: case 407:
            rag_str_step(self, 0.0f, -80.0f, 0.0f);
            break;
        case 137: rag_green_rings(self); break;
        case 138: rag_fire_pillar_on(self, 0); break;
        case 140: rag_yellow_pillar(self); break;
        case 142: rag_heavens_drive(self); break;
        case 145:
            if (!str_step(self))
                rag_str_rewind(self);
            break;
        case 153:
            if (!rag_concentration(self))
                return 0;
            break;
        case 154:
            if (!rag_refine_success_wave(self))
                return 0;
            break;
        case 155:
            if (!rag_refine_fail_wave(self))
                return 0;
            break;
        case 159: rag_lock_on_ground(self); break;
        case 160: rag_party_ring(self); break;
        case 162: rag_snow(self); break;
        case 163: rag_sakura(self); break;
        case 164: rag_status_icons(self); break;
        case 165: rag_christmas_sprite(self); break;
        case 166: rag_make_blur(self); break;
        case 170:
            if (!rag_cart_revolution(self))
                return 0;
            break;
        case 171: rag_rising_sparks_every5(self); break;
        case 172: rag_quiet_cast_black(self); break;
        case 173: rag_quiet_cast_red(self); break;
        case 174: rag_quiet_cast_blue(self); break;
        case 175: rag_quiet_cast_yellow(self); break;
        case 176: rag_quiet_cast_jadu(self); break;
        case 177: rag_quiet_cast_brown(self); break;
        case 178: rag_quiet_cast_white(self); break;
        case 179: rag_quiet_cast_purple(self); break;
        case 180: case 184:
            rag_dark_hit(self);
            break;
        case 183:
            if (!rag_hit_and_quakes(self))
                return 0;
            break;
        case 185:
            if (!rag_str_step_at_offset(self))
                return 0;
            break;
        case 196: rag_curse_ring(self); break;
        case 200: case 397:
            rag_level99_aura(self, "effect\\ring_blue.tga", 0);
            break;
        case 201: case 398:
            rag_level99_aura2(self, "effect\\pikapika2.bmp", 0);
            break;
        case 202: rag_level99_aura3(self, 0); break;
        case 203: rag_gumgang(self, 1, 0); break;
        case 204: case 205: case 206: case 207: case 210:
            if (!follow_wave_str(self, 0, "_heal_effect.wav", -80.0f))
                return 0;
            break;
        case 208: case 209: case 211:
            if (!follow_wave_str(self, 0, "effect\\흡기.wav"  , -80.0f))
                return 0;
            break;
        case 212: rag_dark_breath(self); break;
        case 215: rag_summon_slave(self); break;
        case 216: rag_blood_drain(self, 3); break;
        case 217: rag_energy_drain(self); break;
        case 218:
            if (!rag_concentration_column_long(self))
                return 0;
            break;
        case 219:
            if (!rag_concentration_column(self))
                return 0;
            break;
        case 220:
            if (!rag_potion_berserk(self))
                return 0;
            break;
        case 221: rag_potion_pillar(self, 1.0f, 50); break;
        case 222: rag_defender(self, "effect\\ring_black.tga"); break;
        case 223: rag_ground_rings(self, "effect\\ring_white.tga", 2); break;
        case 224: rag_wind(self); break;
        case 225: rag_ground_rings(self, "effect\\ring_red.tga", 0); break;
        case 226:
            rag_grand_cross_rings(self, 0, 0);
            rag_grand_cross_beams(self, 0);
            break;
        case 227:
            if (self->frame == 0)
                follow_wave(self, "effect\\EF_Bash.wav", 0.0f);
            if (self->frame == 35)
                rag_play_wave_here(self, "effect\\rog_intimidate.wav");
            for (n = 20; n > 0; n--)
                rag_intimidate(self, 0);
            break;
        case 228: rag_spirit_spheres(self, 0); break;
        case 229:
            for (n = 40; n > 0; n--)
                rag_clouds(self, 0);
            break;
        case 230:
            for (n = 60; n > 0; n--)
                rag_clouds(self, 1);
            break;
        case 231: rag_map_pillar(self, "effect\\ring_blue.tga", 0); break;
        case 232: rag_line_link(self, 0); break;
        case 233:
            for (n = 40; n > 0; n--)
                rag_clouds(self, 2);
            break;
        case 234:
            if (!follow_wave_str(self, 0, "effect\\sage_spell breake.wav", 0.0f))
                return 0;
            break;
        case 236: rag_ground_rings(self, "effect\\ring_blue.tga", 0); break;
        case 237: rag_ground_rings(self, "effect\\magic_green.tga", 0); break;
        case 238: rag_ground_rings(self, "effect\\ring_white.tga", 0); break;
        case 239: rag_bottom_ring(self, "effect\\ring_red.tga", 0); break;
        case 240: rag_bottom_ring(self, "effect\\ring_blue.tga", 0); break;
        case 241: rag_bottom_ring(self, "effect\\magic_green.tga", 0); break;
        case 242: rag_ground_disc(self, "effect\\aaa copy.bmp", 1); break;
        case 244:
            rag_str_only(self);
            rag_magic_rod(self);
            break;
        case 245:
            if (self->frame == 0)
                sound_play_wave("effect\\cru_holy cross.wav", 0.0f, 0.0f, 0.0f, 250, 40, 1.0f);
            if (!rag_str_only(self))
                return 0;
            break;
        case 246:
            if (!rag_shield_charge(self))
                return 0;
            break;
        case 247: rag_map_pillar(self, "effect\\ring_blue.tga", 1); break;
        case 248:
            rag_providence(self, "effect\\ring_yellow.tga", 0);
            if (!str_step(self))
                return 0;
            break;
        case 249: rag_shield_boomerang(self, "effect\\shield_boomerang.bmp"); break;
        case 251:
            if (!rag_str_step(self, 0.0f, -20.0f, 0.0f))
                return 0;
            break;
        case 252: rag_defender(self, "effect\\ring_yellow.tga"); break;
        case 253:
            if (self->frame == 0)
                follow_wave(self, "effect\\흡기.wav"  , -80.0f);
            rag_heal_rings(self, "effect\\ring_blue.tga", 2);
            break;
        case 254: rag_gumgang(self, 4, 0); break;
        case 255: case 256: case 257: case 258:
            if (!follow_wave_str(self, 10, "_enemy_hit_wind1.wav", -150.0f))
                return 0;
            break;
        case 259: rag_map_pillar(self, "effect\\magic_green.tga", 2); break;
        case 260: rag_map_pillar(self, "effect\\ring_red.tga", 3); break;
        case 261: rag_four_rings(self, "effect\\ring_yellow.tga", 0); break;
        case 262: hit_bursts(self, 20, "effect\\alpha_center.tga", 0); break;
        case 263: rag_ground_rings(self, "effect\\ring_yellow.tga", 1); break;
        case 264:
            for (n = 5; n > 0; n--)
                rag_hit_flash(self);
            break;
        case 265: rag_finger_offensive(self); break;
        case 266: hit_bursts(self, 24, "effect\\lens1.tga", 1); break;
        case 267: rag_directed_sprite(self, 0); break;
        case 269: case 270: case 271: case 272:
            if (!follow_wave_str(self, 6, "effect\\복호격.wav"  , 0.0f))
                return 0;
            break;
        case 273:
            if (!follow_wave_str(self, 0, "effect\\mon_연환.wav"  , 0.0f))
                return 0;
            break;
        case 274:
            if (self->frame == 0)
                follow_wave(self, "effect\\rog_steal coin.wav", 0.0f);
            for (n = 30; n > 0; n--)
                rag_coin_burst(self, "effect\\coin_a.bmp");
            break;
        case 275:
            if (self->frame == 20)
                follow_wave(self, "effect\\rog_back stap.wav", 0.0f);
            for (n = 20; n > 0; n--)
                rag_hit_flash(self);
            break;
        case 276:
            for (n = 20; n > 0; n--)
                rag_hit_burst_wide(self, "effect\\lens2.tga", 2);
            break;
        case 277: rag_song_rings(self, "effect\\ring_blue.tga", 0); break;
        case 278: rag_song_notes(self, "effect\\zz.bmp", 1); break;
        case 279: rag_song_notes(self, "effect\\pocket.bmp", 3); break;
        case 280: rag_swirl_ring(self, "effect\\twirl.bmp", 8); break;
        case 281: rag_song_notes(self, "", 2); break;
        case 282: rag_song_notes(self, "effect\\twirl.bmp", 7); break;
        case 283: rag_song_marks(self, "effect\\safeline.bmp", 0); break;
        case 284: rag_song_notes(self, "", 5); break;
        case 285: rag_swirl_ring(self, "effect\\twirl.bmp", 4); break;
        case 286: rag_song_notes(self, "effect\\melody_b.bmp", 0); break;
        case 287: rag_song_rings(self, "effect\\ring_red.tga", 1); break;
        case 288: rag_song_notes(self, "", 8); break;
        case 289: rag_song_notes(self, "effect\\idun_apple.bmp", 6); break;
        case 290: rag_song_rings(self, "effect\\ring_red.tga", 0); break;
        case 291: rag_song_notes(self, "effect\\melody_a.bmp", 0); break;
        case 292: rag_song_rings(self, "effect\\magic_green.tga", 2); break;
        case 293: rag_song_notes(self, "effect\\kiss.bmp", 4); break;
        case 294: rag_song_rings(self, "effect\\safeline.bmp", 4); break;
        case 295: rag_joke_or_scream(self, 1); break;
        case 296: rag_joke_or_scream(self, 0); break;
        case 297: rag_fireworks(self); break;
        case 298:
            rag_throw_item(self, "유저인터페이스\\item\\빈병.bmp"
                            , 0);
            break;
        case 299:
            rag_throw_item(self, "유저인터페이스\\item\\빈포션병.bmp"
                            , 1);
            break;

        case 23: case 52: case 64: case 70: case 109: case 110: case 126: case 135: case 139:
        case 141: case 143: case 144: case 146: case 147: case 148: case 149: case 150: case 151:
        case 152: case 156: case 157: case 158: case 167: case 168: case 169: case 181: case 182:
        case 186: case 187: case 188: case 189: case 190: case 191: case 192: case 193: case 194:
        case 195: case 197: case 199: case 213: case 214: case 235: case 268:
            if (!rag_str_only(self))
                return 0;
            break;

        case 105: case 161: case 198: case 443: case 489: case 490:
            break;

        case 374:
            rag_styled_rings(self, "effect\\alpha_down.tga", 0);
            rag_styled_rings(self, "effect\\alpha_down.tga", 1);
            break;
        case 375: fx->overlay_flags |= 8; break;
        case 376:
            rag_hit_line(self, 2);
            if ((unsigned)(self->frame - 10) <= 10)
                tint_master(self, 8, 255, 120, 50);
            break;
        case 377:
            if (self->frame == 5)
                follow_wave(self, "effect\\mon_맹룡과강.wav"  , 0.0f);
            for (n = 0; n < 8; n++)
                rag_four_turned_rings(self, "effect\\alpha_center.tga", (char)n, 2);
            if ((unsigned)(self->frame - 5) <= 30)
                fx->overlay_flags |= 1;
            break;
        case 378:
            rag_particle_streams(self);
            if ((unsigned)(self->frame - 55) <= 10)
                tint_master(self, 1, 100, 100, 255);
            break;
        case 379:
            if ((unsigned)self->frame <= 200)
                render_object_set_argb(self->master, -1, 205 - self->frame, 205 - self->frame, 255);
            break;
        case 380:
            if (self->frame == 25)
                follow_wave(self, "effect\\매직 크래쉬.wav"  , 0.0f);
            if (self->frame == 30)
                quake(0);
            if ((unsigned)(self->frame - 30) <= 30) {
                fx->overlay_flags |= 1;
                tint_master_random(self);
            }
            break;
        case 381:
            rag_effect_texture(self, "effect\\thunder_center.bmp", 0);
            for (n = 80; n > 0; n--)
                rag_hit_sparks(self, "effect\\white02.bmp");
            break;
        case 382: fx->overlay_flags |= 4; break;
        case 383:
            rag_particle_streams_down(self, 0);
            if ((unsigned)(self->frame - 50) <= 30)
                tint_master(self, 8, (125 - self->frame) * 2, (125 - self->frame) * 2, 250);
            break;
        case 384:
            if (self->frame == 0)
                follow_wave(self, "effect\\소울 체인지.wav"  , 0.0f);
            rag_actor_link(self);
            break;
        case 385:
            if ((unsigned)(self->frame - 40) <= 30)
                tint_master(self, 1, 200, 150, 50);
            break;
        case 386:
            if (self->frame == 0)
                follow_wave(self, "effect\\hunter_detecting.wav", 0.0f);
            for (n = 0; n < 12; n++)
                rag_four_turned_rings(self, "effect\\alpha_center.tga", (char)n, 3);
            break;
        case 387: rag_quake_spin_master(self); break;
        case 388:
            if (self->frame == 30)
                follow_wave(self, "effect\\샤프슈팅.wav"  , 0.0f);
            rag_dashes_to_offset(self);
            break;
        case 389:
            rag_cloud_swirl(self, 0);
            if (self->frame == 0)
                follow_wave(self, "effect\\윈드워크.wav"  , 0.0f);
            if ((unsigned)(self->frame - 5) <= 20)
                tint_master(self, 1, 220, 250, 220);
            break;
        case 390:
            if (self->frame == 0)
                follow_wave(self, "effect\\black_overthrust.wav", 0.0f);
            rag_str_step(self, 0.0f, -80.0f, 0.0f);
            break;
        case 392: rag_kyrie_guard_flicker(self); break;
        case 393:
            if (self->frame == 30)
                follow_wave(self, "effect\\애로우 발칸.wav"  , 0.0f);
            rag_dashes_to_offset_long(self);
            break;
        case 394:
            for (n = 0; n < 45; n += 9)
                rag_four_tilted_rings(self, "effect\\fire_ivy.bmp", n, 1);
            break;
        case 395:
            if (self->frame == 0)
                follow_wave(self, "effect\\소울 체인지.wav"  , 0.0f);
            rag_actor_link_wide(self);
            break;
        case 396:
            fx->overlay_flags |= 1;
            tint_master(self, 8, 255, 89, 182);
            break;
        case 399:
            if (self->frame == 0)
                follow_wave(self, "effect\\헤드 크러쉬.wav"  , 0.0f);
            for (n = 0; n < 6; n++)
                rag_four_turned_rings(self, "effect\\alpha_center.tga", (char)n, 4);
            if ((unsigned)(self->frame - 20) <= 30)
                tint_master(self, 8, 255, 255, 200);
            break;
        case 400:
            if (self->frame == 22)
                follow_wave(self, "effect\\비트 조인트.wav"  , 0.0f);
            for (n = 0; n < 6; n++)
                rag_four_turned_rings(self, "effect\\alpha_center.tga", (char)n, 5);
            if ((unsigned)(self->frame - 20) <= 30)
                tint_master(self, 8, 255, 255, 255);
            break;
        case 401:
            if (self->frame == 20 || self->frame == 30 || self->frame == 40 || self->frame == 50
                || self->frame == 60) {
                follow_wave(self, "effect\\EF_NapalmBeat.wav", 0.0f);
                rag_hit2(self, 0);
            }
            break;
        case 402:
            rag_cloud_swirl(self, 1);
            if ((unsigned)(self->frame - 5) <= 60)
                tint_master(self, 1, 250, 250, 200);
            break;
        case 403:
            if (self->frame == 25)
                follow_wave(self, "effect\\swordman_provoke.wav", 0.0f);
            if ((unsigned)self->frame <= 60)
                tint_master_random(self);
            break;
        case 404: rag_ground_disc(self, "effect\\spiderweb.tga", 2); break;
        case 405: rag_bottom_column(self, "effect\\ring_white.tga", 2); break;
        case 408:
            if (self->frame == 0)
                follow_wave(self, "effect\\EF_Blessing.wav", 0.0f);
            for (n = 0; n < 45; n += 9)
                rag_rising_bits(self, "effect\\fire_ivy.bmp", n);
            break;
        case 409:
            if (self->frame == 0)
                follow_wave(self, "effect\\메테오 어썰트.wav"  , 0.0f);
            if ((unsigned)self->frame <= 10)
                tint_master(self, 1, 255, 0, 255);
            for (n = 0; n < 360; n += 45)
                rag_purple_slashes(self, n);
            break;
        case 410: rag_view_facing_sheet(self, "effect\\alpha_center.tga"); break;
        case 411:
            for (n = 30; n > 0; n--)
                rag_popping_bits(self, "effect\\whitelight.tga", 0);
            for (n = 0; n < 4; n++) {
                rag_spray_at_angle(self, "effect\\peong1.tga", (short)(n * 90), 0);
                rag_spray_at_angle(self, "effect\\peong2.tga", (short)(n * 90), 0);
                rag_spray_at_angle(self, "effect\\peong3.tga", (short)(n * 90), 0);
                rag_spray_at_angle(self, "effect\\peong2.tga", (short)(n * 90), 0);
            }
            break;
        case 412: rag_blue_ivy(self); break;
        case 413:
            if (self->frame == 5)
                follow_wave(self, "effect\\T_내려찍기.wav"  , 0.0f);
            if (!(fx->overlay_flags & 0x20) && (unsigned)(self->frame - 17) <= 33) {
                fx->overlay_flags += 0x20;
                fx->overlay_step = (short)(self->frame - 17);
            }
            break;
        case 414:
            if (!(fx->overlay_flags & 0x40) && self->frame < 18) {
                fx->overlay_flags += 0x40;
                fx->overlay_step = (short)(self->frame * 2);
            }
            break;
        case 415:
            if (self->frame == 17)
                follow_wave(self, "effect\\EF_hit2.wav", 0.0f);
            if (self->frame == 25)
                follow_wave(self, "effect\\EF_hit4.wav", 0.0f);
            if (!(fx->overlay_flags & 0x80) && self->frame >= 30) {
                fx->overlay_flags += 0x80;
                fx->overlay_step = (short)(self->frame - 30);
            }
            break;
        case 416: rag_aimed_texture(self, "effect\\emp shock.tga"); break;
        case 417:
            if (!(fx->overlay_flags & 0x100)) {
                fx->overlay_flags += 0x100;
                if (self->frame < 15)
                    fx->overlay_step = (short)self->frame;
            }
            break;
        case 418: rag_super_rings(self, 2, 2); break;
        case 419: fx->overlay_flags |= 0x200; break;
        case 420:
            if (!(fx->overlay_flags & 0x10)) {
                fx->overlay_flags += 0x10;
                if (self->frame <= 45)
                    fx->swell_step = (unsigned char)self->frame;
            }
            break;
        case 421:
            if (self->frame >= g_effectDurations[421] - 1)
                self->frame = 0;
            if (!(fx->overlay_flags & 0x10)) {
                fx->overlay_flags += 0x10;
                fx->swell_step = 45;
            }
            break;
        case 422:
            if (!(fx->overlay_flags & 0x400)) {
                fx->overlay_flags += 0x400;
                if (self->frame <= 45)
                    fx->shrink_step = (unsigned char)self->frame;
            }
            break;
        case 423:
            if (!(fx->overlay_flags & 0x400)) {
                fx->overlay_flags += 0x400;
                fx->shrink_step = 45;
            }
            break;
        case 424:
            if (!(fx->overlay_flags & 0x800)) {
                fx->overlay_flags += 0x800;
                fx->outline_alpha = (unsigned char)(self->frame < 200 ? self->frame : 200);
            }
            break;
        case 425:
            if (self->frame >= 30) {
                fx->overlay_flags |= 0x1000;
                fx->overlay_step = (short)(self->frame - 30);
            }
            break;
        case 426: case 458:
            if (!(fx->overlay_flags & 0x2000)) {
                fx->overlay_flags += 0x2000;
                fx->overlay_step = (short)self->frame;
            }
            break;
        case 427:
            if (!(fx->overlay_flags & 0x800)) {
                fx->overlay_flags += 0x800;
                if (self->frame < 150)
                    fx->overlay_step = (short)(self->frame + 50);
            }
            break;
        case 428: rag_white_streaks_burst(self, 0); break;
        case 429: rag_white_streaks(self, 3); break;
        case 430: rag_white_streaks_back(self, 3); break;
        case 431: rag_white_streaks_burst(self, 3); break;
        case 432:
            if (self->frame == 25)
                follow_wave(self, "effect\\T_전기.wav"  , 0.0f);
            rag_electric_rings(self, 0, 0);
            rag_electric_rings(self, 0, 180);
            break;
        case 433:
            rag_electric_lines(self, 0);
            rag_electric_lines(self, 0);
            break;
        case 434: rag_white_streaks_burst(self, 1); break;
        case 435:
            if (self->frame == 5)
                follow_wave(self, "effect\\T_회오리차기.wav"  , 0.0f);
            storm_and_swirls(self, 0);
            if (self->frame <= 47)
                spin_master(self, 30.0f);
            break;
        case 436: rag_single_ring(self, "effect\\white02.bmp"); break;
        case 437:
            rag_single_ring(self, "effect\\white02.bmp");
            rag_wind_rings(self, "effect\\ring_blue.tga");
            break;
        case 438:
            if (self->frame == 0)
                follow_wave(self, "effect\\T_마법반사.wav"  , 0.0f);
            rag_cloud_rings(self, "effect\\cloud11.tga");
            break;
        case 439:
            for (n = 4; n > 0; n--) {
                rag_line_bursts(self, 0);
                rag_line_bursts(self, 1);
            }
            break;
        case 440:
            if (self->frame == 0)
                follow_wave(self, "effect\\아숨프티오.wav"  , 0.0f);
            rag_str_step(self, 0.0f, -80.0f, 0.0f);
            break;
        case 441: rag_blue_casting(self); break;
        case 442:
            if (!(fx->overlay_flags & 0x8000)) {
                fx->overlay_flags += 0x8000;
                fx->footprint_effect_frame = (unsigned char)self->frame;
            }
            break;
        case 444:
            if (self->frame == 0)
                follow_wave(self, "effect\\T_효과음" "1.wav"  , 0.0f);
            rag_blue_magic_rings(self, 0);
            rag_blue_magic_rings(self, 45);
            break;
        case 445:
            if (self->frame == 30)
                follow_wave(self, "effect\\T_회피" "2.wav"  , 0.0f);
            if (!(fx->overlay_flags & 0x10000) && self->frame > 35) {
                fx->overlay_flags += 0x10000;
                fx->overlay_step = (short)(self->frame - 35);
            }
            break;
        case 446:
            if (!(fx->overlay_flags & 0x20000)) {
                fx->overlay_flags += 0x20000;
                fx->tint_fade_step = (unsigned char)self->frame;
            }
            if (!(fx->overlay_flags & 0x40) && self->frame < 25) {
                fx->overlay_flags += 0x40;
                fx->overlay_step = (short)(self->frame + 11);
            }
            break;
        case 447: rag_footprint(self, "effect\\foot_l2.tga", 1); break;
        case 448: rag_footprint(self, "effect\\foot_r2.tga", 1); break;
        case 449:
            if (!(fx->overlay_flags & 0x4000)) {
                fx->overlay_flags += 0x4000;
                if (self->frame <= 50)
                    fx->tint_fade_step = (unsigned char)self->frame;
            }
            break;
        case 450:
            rag_grand_cross_rings(self, 0, 1);
            rag_grand_cross_beams(self, 1);
            break;
        case 451: rag_soul_strike(self, 1); break;
        case 452: rag_jupitel_hit(self, 1); break;
        case 453: rag_stop_sprite(self, 0); break;
        case 454: rag_black_casting(self, 0); break;
        case 455: rag_gumgang(self, 2, 1); break;
        case 456: rag_rising_light(self, 0); break;
        case 457: rag_flying_kick(self); break;
        case 459:
            storm_and_swirls(self, 1);
            if (self->frame <= 50)
                spin_master(self, 14.0f);
            break;
        case 460:
            if (self->frame == 5)
                follow_wave(self, "ice_titan_attack.wav", 0.0f);
            storm_and_swirls(self, 6);
            break;
        case 461:
            storm_and_swirls(self, 3);
            break;
        case 462:
            if (self->frame == 15)
                follow_wave(self, "effect\\T_회피.wav"  , 0.0f);
            for (n = 30; n > 0; n--)
                rag_popping_bits(self, "effect\\whitelight.tga", 1);
            break;
        case 463:
            if (self->frame == 15) {
                follow_wave(self, "effect\\T_회피.wav"  , 0.0f);
                self->master->state = 0x25;
                render_object_play_motion_range(self->master, 0x60, 2, 2, 0x60, 2, 1, 25, 0);
            }
            for (n = 30; n > 0; n--)
                rag_popping_bits(self, "effect\\whitelight.tga", 1);
            break;
        case 464:
            if (self->frame == 0)
                follow_wave(self, "effect\\REVERBERATION.wav", 0.0f);
            storm_and_swirls(self, 11);
            break;
        case 465:
            storm_and_swirls(self, 2);
            break;
        case 466:
            if (self->frame == 5)
                follow_wave(self, "effect\\T_돌려차기.wav"  , 0.0f);
            if (!(fx->overlay_flags & 0x40) && (unsigned)(self->frame - 14) <= 35) {
                fx->overlay_flags += 0x40;
                fx->overlay_step = (short)(self->frame - 14);
            }
            break;
        case 467: rag_picture_or_skill_icon(self, "effect\\hanmoon1.tga", 0, 0); break;
        case 468: rag_picture_or_skill_icon(self, "effect\\hanmoon2.tga", 0, 0); break;
        case 469: rag_picture_or_skill_icon(self, "effect\\hanmoon3.tga", 0, 0); break;
        case 470: rag_picture_or_skill_icon(self, "effect\\hanmoon4.tga", 0, 0); break;
        case 471: rag_picture_or_skill_icon(self, "effect\\hanmoon7.tga", 0, 0); break;
        case 472: rag_picture_or_skill_icon(self, "effect\\hanmoon5.tga", 0, 0); break;
        case 473: rag_picture_or_skill_icon(self, "effect\\hanmoon6.tga", 0, 0); break;
        case 474: fx->overlay_flags |= 2; break;
        case 475: case 476: case 477: case 478: case 479:
        case 480: case 481: case 482: case 483: case 484:
            rag_blind(self, (char)(id - 474));
            break;
        case 485: rag_super_rings(self, 2, 1); break;
        case 486: rag_super_rings(self, 2, 0); break;
        case 487: rag_dark_rings(self, "effect\\ring_black.tga"); break;
        case 488: rag_flower_rings_ramp(self); break;

        default:
            sprintf(message, "Invalid Effect Id %d", id);
            error_msg(message, 0);
            return 0;
        }
    }

    if (++self->frame < self->duration)
        return 1;
    return 0;
}

static void water_fall(struct RagEffect *self, unsigned char lengthwise, unsigned char size, unsigned char look)
{
    int n;

    rag_water_fall(self, lengthwise, size, look);
    for (n = size ? 20 : 40; n > 0; n--)
        rag_water_fall_spray(self, lengthwise, size);
}

unsigned char rag_effect_process300(struct RagEffect *self)
{
    int n;

    switch (self->effect_id) {
    case 300:
        if (self->frame == 0)
            follow_wave(self, "apocalips_attack.wav", 0.0f);
        rag_chemical_protection(self, 30);
        rag_chemical_protection(self, 60);
        break;
    case 301: rag_firework_sound(self); break;
    case 302: rag_demonstration(self); break;
    case 303:
        rag_chemical_slash(self, 0);
        rag_chemical_slash(self, 1);
        if (self->frame == 44) {
            quake(0);
            follow_wave(self, "effect\\연주붕격.wav"  , 0.0f);
        }
        if ((unsigned)(self->frame - 20) <= 100)
            tint_master(self, 8, 255, 255, 150);
        break;
    case 304: rag_teleportation(self, "effect\\Magic_Violet.tga", 0); break;
    case 305:
        return (unsigned char)follow_wave_str(self, 0, "effect\\p_success.wav", 0.0f);
    case 306:
        return (unsigned char)follow_wave_str(self, 0, "effect\\p_failed.wav", 0.0f);
    case 307: rag_forest_light(self, "effect\\cloud11.tga", 0); break;
    case 308:
        rag_throw_item(self, "유저인터페이스\\item\\돌.bmp"
                        , 2);
        break;
    case 309: rag_first_aid(self); break;
    case 310:
        for (n = 20; n > 0; n--)
            rag_sprinkle_sand(self);
        break;
    case 311:
        return (unsigned char)follow_wave_str(self, 0, "effect\\고성방가.wav"  ,
                                              0.0f);
    case 312: rag_heal(self, "effect\\alpha_down.tga", 0); break;
    case 313: rag_heal_rings(self, "effect\\ring_white.tga", 0); break;
    case 314: rag_exit_rings(self, "effect\\ring_purple.tga"); break;
    case 315: rag_glass_wall_textured(self, "effect\\alpha_down.tga"); break;
    case 316: rag_ready_portal(self); break;
    case 317: rag_portal_styled(self, 0); break;
    case 318: rag_bottom_column(self, "effect\\ring_red.tga", 0); break;
    case 319: rag_bottom_column(self, "effect\\alpha_down.tga", 1); break;
    case 320: rag_heal(self, "effect\\alpha_down.tga", 1); break;
    case 321: rag_warp_zone_styled(self, "effect\\ring_blue.tga", 0); break;
    case 322: rag_forest_light(self, "effect\\cloud11.tga", 1); break;
    case 323: rag_forest_light(self, "effect\\cloud11.tga", 2); break;
    case 324: rag_forest_light(self, "effect\\cloud11.tga", 10); break;
    case 325: rag_heal_rings(self, "effect\\ring_white.tga", 1); break;
    case 326: rag_footprint(self, "effect\\foot_l_b.tga", 0); break;
    case 327: rag_footprint(self, "effect\\foot_r_b.tga", 0); break;
    case 328: rag_begin_asura(self, 0); break;
    case 329: rag_triple_attack(self); break;
    case 330: rag_hit_line(self, 0); break;
    case 331: rag_time_bubbles(self, "effect\\white02.bmp", 0); break;
    case 332: rag_time_bubbles(self, "effect\\white02.bmp", 1); break;
    case 333: rag_maple(self); break;
    case 334: rag_blind(self, 0); break;
    case 335: rag_poison(self, 0); break;
    case 336: rag_guard(self, "effect\\guardK.tga", 0); break;
    case 337:
        rag_job_level_up50(self, 0);
        rag_job_level_up50(self, 45);
        break;
    case 338:
        rag_angel(self, 0);
        rag_angel(self, 1);
        rag_angel(self, 2);
        rag_angel(self, 3);
        break;
    case 339: rag_magnum_break2(self, "effect\\ring_yellow.tga"); break;
    case 340: rag_ring_at_frame_100(self, "effect\\white02.bmp"); break;
    case 341: rag_portal_styled(self, 3); break;
    case 342: rag_spell_casting_rings(self, "effect\\ring_red.tga"); break;
    case 343: rag_heart_rings(self); break;
    case 344: rag_four_blue_rings(self); break;
    case 345: rag_two_flat_rings(self, "effect\\wing003.bmp"); break;
    case 346:
        for (n = 0; n < 45; n += 9)
            rag_four_tilted_rings(self, "effect\\blue_ivy.bmp", n, 0);
        break;
    case 347:
        for (n = 50; n > 0; n--)
            rag_wedding_paper(self, "effect\\white02.bmp", 0);
        break;
    case 348:
        rag_effect_texture(self, "effect\\thunder_center.bmp", 0);
        for (n = 80; n > 0; n--)
            rag_hit_sparks(self, "effect\\white02.bmp");
        break;
    case 349: case 350: case 351: case 352: case 353: case 354: case 355: case 356:
        n = self->effect_id - 349;
        water_fall(self, (unsigned char)(n & 1), (unsigned char)((n >> 1) & 1), (unsigned char)((n >> 2) & 1));
        break;
    case 358: rag_ghost_or_bat(self, 0); break;
    case 359: rag_ghost_or_bat(self, 1); break;
    case 360: rag_ghost_or_bat(self, 2); break;
    case 361:
        if (self->frame == 20)
            follow_wave(self, "effect\\기공포.wav"  , 0.0f);
        if ((unsigned)self->frame <= 10)
            tint_master(self, 1, 255, 0, 255);
        rag_slash_waves(self, 0);
        break;
    case 362: rag_level99_aura3(self, 1); break;
    case 363: rag_valentine_sprite(self, 0); break;
    case 364: rag_valentine_sprite(self, 1); break;
    case 365:
        if (self->frame == 3)
            follow_wave(self, "effect\\프레셔.wav"  , 0.0f);
        rag_billboard(self, "effect\\cross_old.bmp", 0);
        rag_glow(self, "effect\\explosive_1_128.bmp", -28, 0);
        break;
    case 366:
        if (self->frame == 22)
            follow_wave(self, "effect\\세크리파이스.wav"  , 0.0f);
        for (n = 0; n < 5; n++)
            rag_four_turned_rings(self, "effect\\alpha_center.tga", (char)n, 0);
        if ((unsigned)(self->frame - 20) <= 20)
            tint_master(self, 8, 255, 200, 200);
        break;
    case 367: rag_casting_rings_white_yellow(self); break;
    case 368: tint_master(self, 1, 255, 100, 100); break;
    case 369:
        if (self->frame == 0)
            rag_play_wave_here(self, "effect\\knight_twohandquicken.wav");
        render_object_set_argb(self->master, -1, 255, 255, 160);
        rag_str_step(self, 0.0f, -80.0f, 0.0f);
        break;
    case 370: rag_song_notes(self, "effect\\cross_old.bmp", 9); break;
    case 371: case 372:
        return rag_str_only(self) != 0;
    case 373: rag_chimney_smoke(self); break;
    default:
        break;
    }
    return 1;
}
