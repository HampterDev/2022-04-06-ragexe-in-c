#include "actor/render_ops.h"
#include "effects/durations.h"
#include "session/state.h"
#include <limits.h>
#include <string.h>
#include "effects/rag/rag_process_moves.h"
#include "effects/effect_mgr.h"
#include "render/render_api.h"
#include "actor/game_actor.h"

unsigned char rag_process_800(struct RagEffect *self)
{
    int i, n;

    switch (self->effect_id) {
    case 800:
        for (i = 0; i < 3; i++)
            rag_ring_burst(self, "effect\\ring_red.tga", 3);
        break;
    case 801:
        rag_frost_diver(self, 3); break;
    case 802:
        if (self->frame == 6)
            rag_wave_plain("effect\\wl_whiteimprison.wav");
        rag_styled_rings(self, "effect\\alpha_down.tga", 20);
        rag_styled_rings(self, "effect\\alpha_down.tga", 21);
        break;
    case 803:
        for (i = 0; i < 10; i++)
            rag_picture_or_skill_icon(self, "", (short)(i * 30 + 1), 0);
        break;
    case 804: case 970: case 971: case 972: case 973:
        if (self->frame == 0 || self->frame == 40 || self->frame == 80)
            rag_quake();
        rag_tetra_vortex(self, 4);
        for (i = 0; i < 7; i++)
            rag_ring_scatter_137(self, "effect\\ring_red.tga", 10);
        break;
    case 805:
        rag_random_tint_rings(self); break;
    case 806:
        rag_rocket_punches(self); break;
    case 807:
        if (self->frame == 0) {
            rag_follow_master(self);
            rag_play_wave_here(self, "effect\\EF_Bash.wav");
        }
        if (self->frame == 35)
            rag_play_wave_here(self, "effect\\rog_intimidate.wav");
        for (i = 0; i < 20; i++)
            rag_intimidate(self, 3);
        break;
    case 808:
        for (i = 0; i < 5; i++)
            rag_picture_or_skill_icon(self, "", (short)(i * 30 + 1), 2);
        break;
    case 809:
        rag_cloaking_fade(self); break;
    case 810:
        if (self->frame == 25)
            rag_play_wave_here(self, "effect\\triangle.wav");
        for (i = 0; i < 10; i++)
            rag_intimidate(self, 4);
        break;
    case 811:
        rag_freezing_columns(self, "effect\\freezing_stick.bmp", 0); break;
    case 812:
        rag_detecting_blink(self); break;
    case 813:
        rag_rising_sparks(self);
        rag_str_step_up80(self);
        break;
    case 814:
        rag_rising_sparks(self);
        rag_str_step_up80(self);
        break;
    case 815:
        rag_rising_sparks(self);
        rag_str_step_up80(self);
        break;
    case 816:
        rag_rising_sparks(self);
        rag_str_step_up80(self);
        break;
    case 817:
        rag_rising_sparks(self);
        rag_str_step_up80(self);
        break;
    case 818:
        rag_rising_sparks(self);
        rag_str_step_up80(self);
        break;
    case 819:
        rag_line_link(self, 2); break;
    case 820:
        rag_trinket_toss(self); break;
    case 821:
        rag_water_ring(self); break;
    case 822:
        rag_ground_disc(self, "effect\\twirl.bmp", 4);
        rag_white_rings_scaled(self, 0);
        break;
    case 823:
        if (!(self->master->overlay_flags & 0x100000))
            self->master->overlay_flags += 0x100000;
        break;
    case 824:
        rag_hallucination_double(self); break;
    case 825:
        if (self->frame == 1)
            rag_wave_plain("effect\\dimension.wav");
        rag_forest_light(self, "effect\\cloud11.tga", 13);
        rag_ground_disc(self, "effect\\구체" "15.bmp", 4);
        rag_white_rings_scaled(self, 0);
        break;
    case 826:
        rag_black_casting(self, 2); break;
    case 827:
        rag_torch_ring_143(self, 0, 0);
        rag_white_rings_scaled(self, 1);
        break;
    case 828:
        if (g_session1.map_rule_flags[0] == 1) {
            rag_ground_disc(self, "effect\\mineffect\\new_maelstrom\\new_maelstrom\\mae_flare_00.bmp", 10);
            rag_ground_disc(self, "effect\\mineffect\\new_maelstrom\\new_maelstrom\\mae_ring_00.tga", 11);
            rag_ground_disc(self, "effect\\mineffect\\new_maelstrom\\new_maelstrom\\mae_ring_01.tga", 12);
        } else {
            rag_ground_disc(self, "effect\\new_maelstrom\\new_maelstrom\\mae_flare_00.bmp", 10);
            rag_ground_disc(self, "effect\\new_maelstrom\\new_maelstrom\\mae_ring_00.tga", 11);
            rag_ground_disc(self, "effect\\new_maelstrom\\new_maelstrom\\mae_ring_01.tga", 12);
        }
        break;
    case 829:
        rag_torch_ring_143(self, 1, 0);
        rag_torch_ring_143(self, 1, 120);
        rag_torch_ring_143(self, 1, 240);
        rag_white_rings_scaled(self, 2);
        break;
    case 830:
        if (self->frame == 1)
            rag_wave_plain("effect\\EF_BeginSpell.wav");
        rag_bubble_rings_144(self, 45, 1);
        rag_bubble_rings_144(self, 25, 0);
        break;
    case 831:
        rag_cast_bubbles_149(self, 0);
        rag_two_rings_161(self, "effect\\cir0002.bmp", 3);
        break;
    case 832:
        rag_flicker_tint(self, 4, 52, 0, 9, 5, 5, 255);
        rag_bubble_spiral_146(self, 0);
        break;
    case 833:
        rag_thunder_spheres(self); break;
    case 834:
        rag_job_level_up50(self, 0);
        rag_job_level_up50(self, 45);
        break;
    case 835:
        rag_line_burst_quake(self, 0); break;
    case 836: case 904:
        for (i = 0; i < 4; i++) {
            rag_throw_lines(self, 10, "effect\\throw.bmp");
            rag_throw_lines(self, 11, "effect\\throw.bmp");
        }
        break;
    case 838:
        rag_torch_ring_143(self, 2, 0); break;
    case 839:
        rag_two_frame_ring_148(self, "effect\\W_Comp_00.tga", "effect\\W_Comp_24.tga"); break;
    case 840:
        rag_cast_bubbles_149(self, 3); break;
    case 841:
        rag_cast_bubbles_149(self, 4); break;
    case 842:
        rag_cast_bubbles_149(self, 5); break;
    case 843:
        rag_cast_bubbles_149(self, 6); break;
    case 844:
        rag_cast_bubbles_149(self, 7); break;
    case 845:
        rag_cast_bubbles_149(self, 8); break;
    case 846:
        for (i = 0; i < 40; i++)
            rag_ice_shards_150(self);
        break;
    case 847:
        rag_flicker_tint(self, 4, 90, 0, 1, 255, 89, 182);
        rag_tinted_sparks(self, 1);
        break;
    case 848:
        rag_flicker_tint(self, 4, 90, 0, 1, 182, 89, 255);
        rag_tinted_sparks(self, 2);
        break;
    case 849:
        rag_whitelight_151(self, "effect\\whitelight.tga", 0); break;
    case 850:
        rag_light_spheres(self, "effect\\whitelight.tga", 2); break;
    case 851:
        rag_mavka_152(self, 0); break;
    case 852:
        rag_mavka_152(self, 2); break;
    case 853:
        rag_summoned_monsters(self, 0); break;
    case 854:
        rag_line_to_offset_154(self, 0, "effect\\line3.tga");
        rag_melody_notes_155(self, "effect\\melody_a.bmp", 0);
        rag_melody_notes_155(self, "effect\\melody_b.bmp", 0);
        break;
    case 856:
        rag_melody_column_156(self, "effect\\melody_a.bmp", 0); break;
    case 857:
        rag_falling_shards_157(self, "effect\\번개" "1.bmp"); break;
    case 858:
        rag_line_to_offset_154(self, 1, "effect\\line3.tga");
        rag_melody_notes_155(self, "effect\\melody_a.bmp", 1);
        rag_melody_notes_155(self, "effect\\melody_b.bmp", 1);
        break;
    case 859:
        rag_ground_ring_147(self, "effect\\priest_spell.bmp", 1); break;
    case 860:
        rag_melody_column_156(self, "effect\\melody_a.bmp", 1); break;
    case 861:
        rag_cloud_bursts(self, "effect\\freeze_a.bmp", 2); break;
    case 862:
        rag_ground_ring_147(self, "effect\\priest_spell.bmp", 2); break;
    case 863:
        rag_line_to_offset_154(self, 2, "effect\\line3.tga"); break;
    case 864:
        rag_flicker_tint(self, 4, 90, 0, 1, 182, 89, 255);
        rag_tinted_sparks(self, 2);
        rag_rising_sparks(self);
        break;
    case 865:
        rag_cloud_bursts(self, "effect\\freeze_a.bmp", 3); break;
    case 866:
        rag_summoned_monsters(self, 2); break;
    case 867:
        rag_cloud_bursts(self, "effect\\freeze_a.bmp", 4); break;
    case 868:
        rag_summoned_monsters(self, 3); break;
    case 869:
        rag_toss_wave(self);
        for (i = 0; i < 2; i++) {
            rag_tumbling_items(self, "effect\\melody_a.bmp", 6.0f, 1);
            rag_tumbling_items(self, "effect\\melody_b.bmp", 6.0f, 1);
        }
        break;
    case 870:
        rag_summoned_monsters(self, 4); break;
    case 871:
        rag_cloud_bursts(self, "effect\\freeze_a.bmp", 5); break;
    case 872:
        rag_summoned_monsters(self, 5); break;
    case 873:
        rag_cloud_bursts(self, "effect\\freeze_a.bmp", 6); break;
    case 874:
        rag_summoned_monsters(self, 6); break;
    case 875:
        rag_cloud_bursts(self, "effect\\freeze_a.bmp", 7); break;
    case 876:
        rag_summoned_monsters(self, 7); break;
    case 877:
        rag_cloud_bursts(self, "effect\\freeze_a.bmp", 8); break;
    case 878:
        rag_summoned_monsters(self, 8); break;
    case 879:
        rag_ground_ring_147(self, "effect\\kiss2.bmp", 2); break;
    case 880:
        rag_bubble_rings_158(self, 45, 1);
        rag_bubble_rings_158(self, 25, 1);
        break;
    case 881:
        rag_level99_aura2(self, "effect\\emp shock.tga", 2);
        rag_level99_aura2(self, "effect\\cir0002.bmp", 3);
        break;
    case 882:
        rag_wave_at(self, 10, "effect\\LG_EXEEDBREAK.wav");
        for (i = 0; i < 40; i++)
            rag_sphere_motes_159(self, "effect\\alpha_center.tga", 0);
        break;
    case 883:
        rag_epitree_aura_160(self, "effect\\ef_epitree.tga", 30);
        rag_white_rings(self);
        rag_spreading_rings(self, "effect\\alpha_down.tga", 1);
        break;
    case 884:
        rag_three_rings_60(self, "effect\\white02.bmp"); break;
    case 885:
        rag_wave_at(self, 5, "effect\\mon_맹룡과강.wav");
        for (i = 0; i < 8; i++)
            rag_four_turned_rings(self, "effect\\alpha_center.tga", (char)i, 6);
        break;
    case 886:
        rag_tinted_rings(self, "effect\\ring_blue.tga", 3, 0, 0, 255); break;
    case 887:
        rag_hit_line(self, 8); break;
    case 888:
        rag_elec_rays_101(self, 0, 0);
        rag_elec_rays_101(self, 0, 180);
        break;
    case 889:
        rag_wave_at(self, 5, "effect\\mon_맹룡과강.wav");
        n = g_session1.map_rule_flags[0] == 1 ? 6 : 20;
        for (i = 0; i < n; i++)
            rag_hit_burst(self, "effect\\alpha_center.tga", 2);
        break;
    case 890:
        rag_master_whirl(self); break;
    case 891:
        rag_blue_fireballs(self); break;
    case 892:
        rag_cloud_dash_69(self); break;
    case 893:
        for (i = 0; i < 4; i++) {
            rag_lines_to_offset_56(self, 0);
            rag_lines_to_offset_56(self, 1);
        }
        break;
    case 894:
        if (self->frame == 3)
            rag_follow_master(self);
        rag_glow(self, "effect\\explosive_1_128.bmp", 0, 4);
        break;
    case 895:
        rag_shockwave_162(self, "effect\\alpha_center.tga"); break;
    case 896:
        rag_shockwave_163(self, "effect\\shockwave_c.bmp"); break;
    case 897:
        rag_shockwave_164(self, "effect\\shockwave_c.bmp"); break;
    case 898:
        rag_shockwave_165(self, "effect\\shockwave_c.bmp"); break;
    case 899:
        for (i = 0; i < 6; i++)
            rag_ring_burst(self, "effect\\ring_blue.tga", 12);
        break;
    case 900:
        rag_double_shockwave_166(self, "effect\\shockwave_c.bmp"); break;
    case 901:
        rag_spr_ring_167(self, 9); break;
    case 902:
        rag_effect_texture(self, "pe_fur1_01.bmp", 1); break;
    case 903:
        rag_cast_bubbles_149(self, 10); break;
    case 905:
        if (self->frame == 3)
            rag_follow_master(self);
        rag_glow(self, "effect\\explosive_1_128.bmp", 0, 10);
        break;
    case 906:
        rag_wave_at(self, 3, "effect\\프레셔.wav");
        rag_wave_at(self, 25, "effect\\LG_SHIELDPRESS.wav");
        rag_billboard(self, "effect\\shield.bmp", 0);
        rag_glow(self, "effect\\explosive_1_128.bmp", -28, 0);
        break;
    case 907:
        rag_wave_at(self, 26, "effect\\LG_PINPOINTATTACK.wav");
        if (self->frame % 4 <= 1)
            self->master->overlay_flags |= 2;
        for (i = 0; i < 4; i++) {
            rag_tumbling_items(self, "effect\\shield.bmp", 5.0f, 7);
            rag_tumbling_items(self, "effect\\sword.bmp", 9.0f, 7);
        }
        break;
    case 908:
        if (self->frame == 0)
            rag_follow_master(self);
        for (i = 0; i < 40; i++)
            rag_sphere_motes_159(self, "effect\\alpha_center.tga", 1);
        break;
    case 909:
        rag_wave_at(self, 10, "effect\\LG_BANDING.wav");
        for (i = 0; i < 40; i++)
            rag_sphere_motes_159(self, "effect\\alpha_center.tga", 2);
        break;
    case 910:
        rag_wave_at(self, 10, "effect\\LG_INSPIRATION.wav");
        for (i = 0; i < 40; i++)
            rag_sphere_motes_159(self, "effect\\alpha_center.tga", 3);
        break;
    case 911:
        rag_begin_spell_rings(self); break;
    case 912:
        rag_msg_sprite(self, 0x3d); break;
    case 913:
        rag_pale_fireballs(self); break;
    case 914:
        rag_throw_item(self, UI_ITEM "폭탄버섯포자.bmp", 1);
        break;
    case 915:
        rag_summoned_monsters(self, 9); break;
    case 916:
        rag_summoned_monsters(self, 10); break;
    case 917:
        rag_summoned_monsters(self, 11); break;
    case 918:
        rag_summoned_monsters(self, 12); break;
    case 919: case 1610:
        rag_msg_sprite(self, 0x3f); break;
    case 920:
        if (!rag_str_raised(self))
            return 0;
        break;
    case 921:
        rag_msg_sprite(self, 0x40);
        rag_msg_sprite(self, 0x41);
        break;
    case 922:
        rag_summoned_monsters(self, 13); break;
    case 923:
        rag_spr_ring_167(self, 10); break;
    case 925:
        rag_hallu_doubles_2(self); break;
    case 926:
        rag_str_or_rewind(self); break;
    case 927:
        rag_summoned_monsters(self, 14); break;
    case 928:
        rag_flicker_tint(self, 6, 40, 8, 1, 0, 0, 255);
        for (i = 0; i < 3; i++)
            rag_snow_134(self, 0);
        break;
    case 929:
        rag_summoned_monsters(self, 15); break;
    case 930:
        rag_wave_at(self, 3, "effect\\프레셔.wav");
        rag_wave_at(self, 22, "effect\\S바레티르스피어.wav");
        rag_billboard(self, UI_ITEM "스피어.bmp", 4);
        rag_billboard(self, UI_ITEM "파이크.bmp", 4);
        rag_billboard(self, UI_ITEM "궁그닐.bmp", 4);
        rag_glow(self, "effect\\explosive_1_128.bmp", -28, 0);
        break;
    case 931:
        rag_whitelight_151(self, "effect\\whitelight.tga", 2);
        rag_whitelight_151(self, "effect\\whitelight.tga", 1);
        break;
    case 932:
        rag_light_spheres(self, "effect\\whitelight.tga", 3); break;
    case 934:
        rag_ground_rings(self, "effect\\ring_white.tga", 4);
        if (self->frame >= 10 && self->frame <= 34 && self->frame % 4 < 2) {
            self->master->overlay_flags |= 1;
            self->master->overlay_flags |= 8;
        }
        break;
    case 935:
        rag_green_rocket_punches(self); break;
    case 936:
        rag_tinted_rings(self, "effect\\ring_red.tga", 4, 255, 0, 0); break;
    case 937:
        rag_tinted_rings(self, "effect\\ring_yellow.tga", 4, 255, 255, 0); break;
    case 938:
        rag_tinted_rings(self, "effect\\ring_brown.tga", 4, 80, 40, 0); break;
    case 939:
        rag_blue_rings_169(self, "effect\\ring_blue.tga", 0); break;
    case 940:
        rag_effect_texture(self, "effect\\얼음결정체" "1.tga", 22);
        break;
    case 941:
        rag_spr_ring_167(self, 20); break;
    case 942:
        rag_water_ball_rings(self); break;
    case 943:
        rag_flicker_tint(self, 4, 52, 0, 9, 55, 255, 55);
        rag_bubble_spiral_146(self, 1);
        break;
    case 944:
        if (self->frame == 1)
            rag_wave_plain("effect\\매직 크래쉬.wav");
        rag_falling_shards_157(self, "effect\\stone_part.bmp");
        break;
    case 945:
        for (i = 0; i < 65; i++)
            rag_clouds(self, 9);
        break;
    case 946:
        rag_soul_particles(self, 2); break;
    case 947:
        rag_msg_sprite(self, 0x46); break;
    case 948:
        rag_msg_sprite(self, 0x47); break;
    case 949:
        rag_msg_sprite(self, 0x48); break;
    case 950:
        rag_msg_sprite(self, 0x49); break;
    case 951:
        rag_msg_sprite(self, 0x4a); break;
    case 952:
        rag_msg_sprite(self, 0x4b); break;
    case 953:
        rag_msg_sprite(self, 0x4c); break;
    case 954:
        rag_msg_sprite(self, 0x4d); break;
    case 955:
        rag_msg_sprite(self, 0x4e); break;
    case 956:
        rag_msg_sprite(self, 0x4f); break;
    case 957:
        rag_msg_sprite(self, 0x50); break;
    case 958:
        rag_msg_sprite(self, 0x51); break;
    case 961:
        rag_msg_sprite(self, 0x52); break;
    case 962:
        rag_msg_sprite(self, 0x53); break;
    case 963:
        render_object_set_argb(self->master, -1, 0x69, 0x69, 0x69); break;
    case 969:
        rag_melody_notes_170(self, "effect\\melody_a.bmp", 1);
        rag_melody_notes_170(self, "effect\\melody_b.bmp", 1);
        rag_melody_notes_170(self, "effect\\melody_a.bmp", 1);
        rag_melody_notes_170(self, "effect\\melody_b.bmp", 1);
        break;
    case 1003:
        if (!(self->master->overlay_flags & 0x4000)) {
            self->master->overlay_flags += 0x4000;
            self->master->tint_fade_step = 20;
        }
        break;
    case 1004:
        rag_after_image(self); break;
    case 1015:
        rag_wave_at(self, 0, "effect\\p_success.wav");
        if (!rag_str_only(self))
            return 0;
        break;
    case 1016: case 1017: case 1018:
        rag_wave_at(self, 0, "effect\\p_failed.wav");
        if (!rag_str_only(self))
            return 0;
        break;
    case 1026:
        rag_msg_sprite(self, 0x54); break;
    case 1028:
        rag_msg_sprite(self, 0x55); break;
    case 1030:
        rag_cloud_dash_2(self); break;
    case 1032:
        rag_guard(self, "effect\\guardK.tga", 3); break;
    case 1037:
        render_object_set_argb(self->master, -1, 251, 200, 4); break;
    case 1038:
        for (i = 0; i < 50; i++)
            rag_wedding_paper(self, "effect\\white02.bmp", 1);
        break;
    case 1040:
        if (!rag_str_step(self, 0.0f, 40.0f, 0.0f))
            return 0;
        break;
    case 1041:
        rag_msg_sprite(self, 0x56); break;
    case 1042:
        if (self->frame == 0)
            rag_play_wave_here(self, "effect\\all_full_throttle.wav");
        if (!rag_str_step_up80(self))
            return 0;
        break;
    case 1043:
        if (self->frame == 0)
            rag_play_wave_here(self, "effect\\sr_flashcombo.wav");
        if (!rag_str_step_up80(self))
            return 0;
        break;
    case 1044:
        if (self->frame == 0)
            rag_play_wave_here(self, "effect\\rk_luxanima.wav");
        if (!rag_str_step_up80(self))
            return 0;
        break;
    case 1045:
        for (i = 0; i < 30; i++)
            rag_clouds(self, 5);
        break;
    case 1046:
        if (self->frame == 0)
            rag_play_wave_here(self, "effect\\so_elemental_shield.wav");
        if (!rag_str_step_up80(self))
            return 0;
        break;
    case 1047:
        if (self->frame == 0)
            rag_play_wave_here(self, "effect\\ab_offertorium.wav");
        if (!rag_str_step_up80(self))
            return 0;
        break;
    case 1048:
        if (!rag_str_step(self, 0.0f, 0.0f, 0.0f))
            return 0;
        break;
    case 1049:
        if (self->frame == 0)
            rag_play_wave_here(self, "effect\\gn_illusiondoping.wav");
        if (!rag_str_step_up80(self))
            return 0;
        break;
    case 1050:
        if (!rag_str_step(self, 0.0f, -25.0f, 0.0f))
            return 0;
        break;
    case 1051:
        rag_kings_grace(self); break;
    case 1052:
        rag_blood_drain(self, 1); break;
    case 1053:
        rag_msg_sprite(self, 0x57); break;
    case 1054:
        rag_msg_sprite(self, 0x58); break;
    case 1057:
        if (!rag_str_step(self, 0.0f, -5.0f - (float)game_actor_get_height_offset((struct GameActor *)self->master), 0.0f))
            return 0;
        break;
    case 1065:
        self->master->overlay_flags |= 1;
        render_object_set_argb(self->master, -1, 255, 255, 255);
        break;
    case 1072:
        rag_wave_at(self, 0, "effect\\T_회피" "2.wav");
        if (!(self->master->overlay_flags & 0x10000) && self->frame > 10) {
            self->master->overlay_flags += 0x10000;
            self->master->overlay_step = (short)(self->frame - 10);
        }
        break;
    case 1081:
        rag_hallu_walk(self); break;
    case 1082:
        rag_wave_at(self, 20, "effect\\기공포.wav");
        if ((unsigned)self->frame <= 10) {
            self->master->overlay_flags |= 1;
            render_object_set_argb(self->master, -1, 0, 255, 0);
        }
        rag_slash_waves(self, 2);
        break;
    case 1088:
        rag_bottom_column(self, "effect\\ring_yellow.tga", 4); break;
    case 1115:
        for (i = 0; i < 40; i++) {
            rag_clouds(self, 11);
            rag_clouds(self, 12);
            rag_clouds(self, 13);
            rag_clouds(self, 14);
        }
        break;
    case 1120:
        rag_msg_sprite(self, 0x5a); break;
    case 1122:
        if (self->frame >= g_effectDurations[1122] - 1)
            self->frame = 0;
        rag_clouds_textured(self, "effect\\emp shock.tga", 0);
        break;
    case 1131:
        if (self->frame >= g_effectDurations[1131] - 1)
            self->frame = 0;
        self->master->overlay_flags |= 1;
        render_object_set_argb(self->master, -1, 255, 255, 255);
        break;
    case 1132:
        rag_wave_at(self, 0, "effect\\mon_금강불괴.wav");
        break;
    case 1135:
        rag_spirit_spheres(self, 4); break;
    case 1145:
        for (i = 0; i < 65; i++)
            rag_clouds(self, 15);
        break;
    case 1148:
        render_object_set_argb(self->master, -1, 220, 150, 0); break;
    case 1149:
        rag_guard(self, "effect\\guardK.tga", 4); break;
    case 1150:
        for (i = 0; i < 30; i++)
            rag_popping_bits(self, "effect\\whitelight.tga", 1);
        for (n = 0; n < 4; n++) {
            rag_spray_at_angle(self, "effect\\peong1.tga", (short)(n * 90), 1);
            rag_spray_at_angle(self, "effect\\peong2.tga", (short)(n * 90), 1);
            rag_spray_at_angle(self, "effect\\peong3.tga", (short)(n * 90), 1);
            rag_spray_at_angle(self, "effect\\peong2.tga", (short)(n * 90), 1);
        }
        break;
    case 1185:
        rag_warp_zone_styled(self, "effect\\ring_red.tga", 1); break;
    case 1195:
        rag_wave_at(self, 0, "effect\\EF_BeginSpell.wav");
        if (self->master->revealed != 0 && self->master->state == 0)
            rag_master_spin(self);
        break;
    case 1204:
        rag_columns(self, 99); break;
    case 1205:
        rag_blue_rings_121(self); break;
    case 1206:
        rag_flying_kick_wave(self);
        rag_columns_at_offset(self, "effect\\270바람.tga", 4);
        rag_columns_at_offset(self, "effect\\270바람.tga", 5);
        break;
    case 1207:
        rag_particle_streams_down(self, 1);
        rag_streams_tint(self);
        break;
    case 1208:
        rag_particle_streams_down(self, 2);
        rag_streams_tint(self);
        break;
    case 1209:
        if (self->frame < 61) {
            if (self->master->overlay_flags & 8)
                self->master->overlay_flags += 8;
            if (!(self->frame & 1)) {
                if (self->master->overlay_flags & 1)
                    self->master->overlay_flags += 1;
                render_object_set_argb(self->master, -1, 155, 155, 255);
            } else {
                render_object_set_argb(self->master, -1, 50, 50, 50);
            }
        }
        break;
    case 1210:
        rag_line_link(self, 0); break;
    case 1224:
        rag_wave_at(self, 5, "effect\\mon_침투경.wav");
        if (!(self->master->overlay_flags & 0x40) && self->frame >= 14
            && rag_effect_get_duration(self) > self->frame) {
            self->master->overlay_flags += 0x40;
            self->master->overlay_step = (short)(self->frame - 14);
        }
        break;
    case 1240:
        rag_digital_space_168(self, 1240); break;
    case 1286:
        if (self->frame % 30 == 0)
            effect_mgr_spawn(g_effectMgr, 0, g_session1.map_rule_flags[0] == 1 ? 0x5fd : 0x505, self->master->pos, 0, -1, 0, 0);
        rag_music_rings_173(self, 3, 3.0f);
        rag_music_rings_173(self, 3, 3.0f);
        rag_music_rings_173(self, 3, 8.0f);
        rag_music_rings_173(self, 3, 8.0f);
        break;
    case 1365:
        rag_wave_at(self, 5, "effect\\T_회오리차기.wav");
        if (self->frame <= 47) {
            self->master->roty += 30.0f;
            if (self->master->roty > 360.0f)
                self->master->roty -= 360.0f;
        }
        break;
    case 1399:
        rag_heavens_drive(self); break;
    case 1409:
        for (i = 0; i < 4; i++) {
            rag_yellow_lines_56(self, 0);
            rag_yellow_lines_56(self, 1);
        }
        break;
    case 1418:
        rag_follow_master(self);
        if (self->frame == 0) {
            rag_play_wave_here(self, "effect\\EF_BeginSpell.wav");
            rag_casting_circle(self, 45, "effect\\new_comet\\new_comet_cast_ring\\ring_rainbow.tga", 20);
        }
        break;
    case 1420:
        if (!(self->master->overlay_flags & 0x4000)) {
            self->master->overlay_flags += 0x4000;
            self->master->tint_fade_step = self->frame <= 150 ? (unsigned char)(self->frame / 8) : 15;
        }
        break;
    case 1448:
        rag_jack_frost(self); break;
    case 1449:
        rag_spell_rings(self, "effect\\new_jackfrost\\ring_ice.tga", 2); break;
    case 1525:
        rag_effect_texture(self, "effect\\Buff_Prison.bmp", 31); break;
    case 1531:
        rag_msg_sprite(self, 0x5b); break;
    case 1545:
        rag_tint_mark(self);
        rag_tint_early(self, 0, 0, 255);
        break;
    case 1556:
        rag_spell_circles_low(self, "effect\\ring_blue.tga", 20, 21); break;
    case 1560:
        rag_spell_circles(self, "effect\\ring_white.tga", 22); break;
    case 1565:
        rag_spell_circles(self, "effect\\ring_yellow.tga", 23); break;
    case 1568:
        for (i = 0; i < 2; i++) {
            if (g_session1.map_rule_flags[0] == 1) {
                rag_throw_lines(self, 10, "effect\\mineffect\\new_crossimpact\\new_crossimpact_line\\line_pitch.tga");
                rag_throw_lines(self, 11, "effect\\mineffect\\new_crossimpact\\new_crossimpact_line\\line_pitch.tga");
                rag_throw_lines(self, 10, "effect\\mineffect\\new_crossimpact\\new_crossimpact_line\\line_red.tga");
                rag_throw_lines(self, 11, "effect\\mineffect\\new_crossimpact\\new_crossimpact_line\\line_red.tga");
            } else {
                rag_throw_lines(self, 10, "effect\\new_crossimpact\\new_crossimpact_line\\line_pitch.tga");
                rag_throw_lines(self, 11, "effect\\new_crossimpact\\new_crossimpact_line\\line_pitch.tga");
                rag_throw_lines(self, 10, "effect\\new_crossimpact\\new_crossimpact_line\\line_red.tga");
                rag_throw_lines(self, 11, "effect\\new_crossimpact\\new_crossimpact_line\\line_red.tga");
            }
        }
        break;
    case 1569:
        rag_melody_column_156(self, g_session1.map_rule_flags[0] == 1 ? "effect\\mineffect\\new_reverberation\\melody_a.bmp"
                                                         : "effect\\new_reverberation\\melody_a.bmp", 2);
        rag_white_rings_63(self, 28);
        break;
    case 1575:
        for (i = 0; i < 4; i++) {
            if (g_session1.map_rule_flags[0] == 1) {
                rag_throw_lines(self, 10, "effect\\mineffect\\new_gateofhell\\line_hell.tga");
                rag_throw_lines(self, 11, "effect\\mineffect\\new_gateofhell\\line_hell.tga");
            } else {
                rag_throw_lines(self, 10, "effect\\new_gateofhell\\line_hell.tga");
                rag_throw_lines(self, 11, "effect\\new_gateofhell\\line_hell.tga");
            }
        }
        break;
    case 1584:
        if (g_session1.map_rule_flags[0] == 1) {
            rag_line_to_offset_154(self, 2, "effect\\mineffect\\new_metalicsound\\new_metalicsound_line\\met_damege_line_1.tga");
            rag_melody_notes_155(self, "effect\\mineffect\\new_metalicsound\\new_metalicsound_icon\\met_particle_1.tga", 2);
            rag_melody_notes_155(self, "effect\\mineffect\\new_metalicsound\\new_metalicsound_icon\\met_particle_2.tga", 2);
            rag_melody_notes_155(self, "effect\\mineffect\\new_metalicsound\\new_metalicsound_icon\\met_particle_3.tga", 2);
        } else {
            rag_line_to_offset_154(self, 2, "effect\\new_metalicsound\\new_metalicsound_line\\met_damege_line_1.tga");
            rag_melody_notes_155(self, "effect\\new_metalicsound\\new_metalicsound_icon\\met_particle_1.tga", 2);
            rag_melody_notes_155(self, "effect\\new_metalicsound\\new_metalicsound_icon\\met_particle_2.tga", 2);
            rag_melody_notes_155(self, "effect\\new_metalicsound\\new_metalicsound_icon\\met_particle_3.tga", 2);
        }
        break;
    case 1594:
        if (g_session1.map_rule_flags[0] == 1) {
            rag_melody_notes_170(self, "effect\\mineffect\\new_reverberation\\rev_note_01.tga", 3);
            rag_melody_notes_170(self, "effect\\mineffect\\new_reverberation\\rev_note_02.tga", 3);
        } else {
            rag_melody_notes_170(self, "effect\\new_reverberation\\rev_note_01.tga", 3);
            rag_melody_notes_170(self, "effect\\new_reverberation\\rev_note_02.tga", 3);
        }
        break;
    case 1600:
        rag_guard(self, "effect\\guardK.tga", 5); break;
    case 1601:
        rag_effect_texture(self, MAGIC_CIRCLE "1-3.bmp", 32); break;
    case 1602: case 1603: case 1604: case 1605: case 1606:
        if (self->frame == 0)
            rag_plant_actor(self);
        break;
    case 1655:
        rag_white_rings_63(self, 29); break;
    case 1656:
        rag_cast_bubbles_149(self, 9); break;
    case 1664:
        rag_spell_rings(self, "effect\\ring_yellow.tga", 33); break;
    case 1677:
        rag_spell_circles_low(self, "effect\\ring_white.tga", 31, 31); break;
    case 1678:
        rag_sonic_waves_146(self, "effect\\new_sonicwave\\new_sonicwave\\son_wave_01.tga", 1);
        rag_sonic_waves_146(self, "effect\\new_sonicwave\\new_sonicwave\\son_wave_00.tga", 0);
        break;
    case 1709:
        rag_spell_rings(self, g_session1.map_rule_flags[0] == 1 ? "effect\\mineffect\\new_earthdrive\\ring_brown.tga"
                                            : "effect\\new_earthdrive\\ring_brown.tga", 33);
        break;
    case 1872:
        rag_spell_circles(self, "effect\\ring_white.tga", 42); break;
    case 1876:
        rag_spell_circles(self, "effect\\ring_white.tga", 43); break;
    case 1902:
        rag_spell_circles(self, "effect\\new_abr_mother\\abr_mother.tga", 0); break;
    case 1910:
        if (self->master == NULL)
            break;
        rag_spell_circles(self, "effect\\new_abr_infinity\\ring_infinty.tga", 0);
        break;
    case 1918:
        rag_spell_circles(self, "effect\\new_d_cannon\\ABR_DUAL_CANNON.tga", 0); break;
    case 1923:
        rag_spell_circles_low(self, "effect\\ring_red.tga", 44, 44); break;
    case 1932:
        rag_follow_master(self);
        if (self->frame == 0) {
            rag_play_wave(self, "effect\\EF_BeginSpell.wav", -100.0f);
            rag_casting_circle(self, 90, "effect\\new_pramen\\ring_blue.tga", 5);
        }
        break;
    case 1938:
        rag_spell_circles(self, "effect\\new_battle_w\\ABR_BATTLE_WARIOR.tga", 0); break;
    case 1959:
        rag_spell_circles(self, "effect\\pneumaticusprocella\\pneumaticusprocella_cast\\ring_yellow.tga", 0); break;
    case 1963:
        rag_spell_circles(self, "effect\\ring_white.tga", 45); break;
    case 1986:
        rag_spell_circles(self, "effect\\ring_white.tga", 46); break;
    case 1998:
        rag_spell_circles(self, "effect\\ring_white.tga", 47); break;
    case 2240:
        rag_spell_circles(self, "effect\\ring_white.tga", 48); break;
    case 2249:
        rag_spell_circles(self, "effect\\ring_white.tga", 49); break;
    case 2255:
        rag_spell_circles(self, "effect\\ring_white.tga", 50); break;
    case 2288:
        return rag_status_icon(self, 1, "effect\\status_effect\\C_rate_Up.bmp");
    case 2289:
        return rag_status_icon(self, 0, "effect\\status_effect\\C_rate_Down.bmp");
    case 2290:
        return rag_status_icon(self, 1, "effect\\status_effect\\H_plus_Up.bmp");
    case 2291:
        return rag_status_icon(self, 0, "effect\\status_effect\\H_plus_Down.bmp");
    case 2292:
        return rag_status_icon(self, 1, "effect\\status_effect\\Mres_Up.bmp");
    case 2293:
        return rag_status_icon(self, 0, "effect\\status_effect\\Mres_Down.bmp");
    case 2294:
        return rag_status_icon(self, 1, "effect\\status_effect\\P_atk_Up.bmp");
    case 2295:
        return rag_status_icon(self, 0, "effect\\status_effect\\P_atk_Down.bmp");
    case 2296:
        return rag_status_icon(self, 1, "effect\\status_effect\\Res_Up.bmp");
    case 2297:
        return rag_status_icon(self, 0, "effect\\status_effect\\Res_Down.bmp");
    case 2298:
        return rag_status_icon(self, 1, "effect\\status_effect\\S_matk_Up.bmp");
    case 2299:
        return rag_status_icon(self, 0, "effect\\status_effect\\S_matk_Down.bmp");
    case 2300:
        return rag_status_icon(self, 1, "effect\\status_effect\\P_atk_S_matk_Up.bmp");
    case 2338:
        rag_follow_master(self);
        if (self->frame == 0)
            rag_fighter_rings_31(self, "effect\\the_one_fighter_rises\\the_one_fighter_rises\\the_ring_00.tga");
        break;
    case 2346:
        rag_msg_sprite(self, 0x5e); break;
    case 2347:
        rag_msg_sprite(self, 0x5f); break;
    case 2356:
        rag_msg_sprite(self, 0x60); break;
    case 2357:
        rag_msg_sprite(self, 0x61); break;
    case 2358:
        rag_msg_sprite(self, 0x62); break;
    case 2359:
        rag_msg_sprite(self, 0x63); break;
    case 2360:
        rag_msg_sprite(self, 0x64); break;
    case 2361:
        rag_msg_sprite(self, 0x65); break;
    case 2362:
        rag_msg_sprite(self, 0x66); break;
    case 2363:
        rag_msg_sprite(self, 0x67); break;
    case 2364:
        rag_msg_sprite(self, 0x68); break;
    case 2365:
        rag_msg_sprite(self, 0x69); break;
    case 2367:
        rag_spell_circles(self, "effect\\ring_yellow.tga", 0); break;
    case 2368:
        rag_cast_bubbles_149(self, 11); break;
    case 2369:
        if (self->frame == 0)
            rag_wave_plain("effect\\EF_BeginSpell.wav");
        rag_four_rings_139(self, 25, "effect\\ring_white.tga", 0);
        rag_four_rings_139(self, 45, "effect\\ring_white.tga", 0);
        break;
    case 2373:
        rag_tinted_rings(self, "effect\\ring_black.tga", 4, 255, 0, 0); break;
    default:
        break;
    }
    return 1;
}
