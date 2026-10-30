#include "effects/durations.h"
#include <limits.h>
#include <string.h>
#include "effects/rag/rag_process_moves.h"
#include "effects/effect_mgr.h"
#include "render/render_api.h"

unsigned char rag_process_491(struct RagEffect *self)
{
    int i;

    switch (self->effect_id) {
    case 491: case 492: case 507: case 508: case 646: case 649: case 670: case 699: case 704:
    case 705: case 706: case 709: case 721: case 731: case 987: case 988: case 1006:
        rag_str_step_up80(self); break;
    case 493:
        rag_poison_particles(self);
        rag_flicker_tint(self, INT_MIN, 80, 0, 1, 255, 0, 255);
        break;
    case 494:
        rag_shield_boomerang(self, "effect\\토마.bmp");
        break;
    case 495:
        rag_toss_wave(self);
        for (i = 0; i < 4; i++) {
            rag_tumbling_items(self, "effect\\shield.bmp", 5.0f, 0);
            rag_tumbling_items(self, "effect\\sword.bmp", 9.0f, 0);
        }
        break;
    case 496:
        rag_guard_ring(self, "effect\\a01.bmp");
        rag_flicker_tint(self, 10, 30, 0, 1, 250, 250, 250);
        break;
    case 497:
        rag_billboard(self, UI_ITEM "레드슬림포션.bmp", 1);
        rag_glow(self, "effect\\bbbb.bmp", -16, 1);
        break;
    case 498:
        rag_billboard(self, UI_ITEM "옐로우슬림포션.bmp", 2);
        rag_glow(self, "effect\\bbbb.bmp", -16, 2);
        break;
    case 499:
        rag_billboard(self, UI_ITEM "화이트슬림포션.bmp", 3);
        rag_glow(self, "effect\\bbbb.bmp", -16, 0);
        break;
    case 500:
        rag_flicker_tint(self, INT_MIN, 30, 8, 1, 0, 0, 255); break;
    case 501:
        rag_wave_at(self, 0, "effect\\EF_BeginSpell.wav");
        if (self->master->state == 0)
            rag_master_spin(self);
        break;
    case 502:
        rag_wave_at(self, 0, "effect\\EF_BeginSpell.wav");
        rag_flicker_tint(self, 0, 140, 8, 1, 250, 250, 100);
        break;
    case 503:
        rag_follow_master(self);
        if (self->frame == 0)
            rag_play_wave(self, "effect\\T_캐스팅.wav", 0.0f);
        if (self->frame == 145)
            rag_play_wave(self, "effect\\T_영혼.wav", -50.0f);
        rag_soul_light(self, "effect\\whitelight.tga");
        rag_soul_link_letters(self);
        break;
    case 504:
        rag_spirit_spheres(self, 1); break;
    case 505:
        if (self->frame == 0)
            rag_play_wave_here(self, "effect\\priest_suffragium.wav");
        rag_flicker_tint(self, 10, 35, 0, 1, 250, 250, 100);
        break;
    case 506:
        rag_light_spheres(self, "effect\\whitelight.tga", 0); break;
    case 509:
        rag_valentine_sprite(self, 2); break;
    case 510:
        rag_begin_asura(self, 1); break;
    case 512:
        rag_line_burst_quake(self, 1); break;
    case 513:
        rag_magic_target(self); break;
    case 514:
        rag_explosion_rings(self, "effect\\ring_blue.tga"); break;
    case 515:
        for (i = 0; i < 80; i++)
            rag_clouds(self, 3);
        break;
    case 516:
        for (i = 0; i < 80; i++)
            rag_clouds(self, 4);
        break;
    case 517:
        rag_floor_disc(self, "effect\\white02.bmp"); break;
    case 518:
        rag_wave_at(self, 30, "effect\\wizard_fire_pillar_b.wav");
        for (i = 0; i < 4; i++)
            rag_light_orbits(self, "effect\\whitelight.tga");
        break;
    case 519:
        rag_valentine_sprite(self, 3); break;
    case 520:
        rag_shield_boomerang_volley(self, "effect\\shield_boomerang.bmp"); break;
    case 521:
        rag_flicker_tint(self, 10, 50, 8, 1, 255, 0, 0); break;
    case 522:
        rag_stone_and_ice(self); break;
    case 523:
        rag_effect_texture(self, "effect\\Tarot01.tga", 1); break;
    case 524:
        rag_effect_texture(self, "effect\\Tarot02.tga", 1); break;
    case 525:
        rag_effect_texture(self, "effect\\Tarot03.tga", 1); break;
    case 526:
        rag_effect_texture(self, "effect\\Tarot04.tga", 1); break;
    case 527:
        rag_effect_texture(self, "effect\\Tarot05.tga", 1); break;
    case 528:
        rag_effect_texture(self, "effect\\Tarot06.tga", 1); break;
    case 529:
        rag_effect_texture(self, "effect\\Tarot07.tga", 1); break;
    case 530:
        rag_effect_texture(self, "effect\\Tarot08.tga", 1); break;
    case 531:
        rag_effect_texture(self, "effect\\Tarot09.tga", 1); break;
    case 532:
        rag_effect_texture(self, "effect\\Tarot10.tga", 1); break;
    case 533:
        rag_effect_texture(self, "effect\\Tarot11.tga", 1); break;
    case 534:
        rag_effect_texture(self, "effect\\Tarot12.tga", 1); break;
    case 535:
        rag_effect_texture(self, "effect\\Tarot13.tga", 1); break;
    case 536:
        rag_effect_texture(self, "effect\\Tarot14.tga", 1); break;
    case 537:
        if (self->frame == 5)
            rag_quake();
        rag_fire_wall_rings(self, "effect\\ring_black.tga");
        break;
    case 538:
        rag_flicker_tint(self, INT_MIN, 30, 8, 1, 0, 255, 0); break;
    case 539:
        rag_throw_item(self, UI_ITEM "화염병.bmp", 10);
        rag_throw_item(self, UI_ITEM "염산병.bmp", 11);
        break;
    case 540:
        if (self->frame <= 15 && !(self->master->overlay_flags & 0x10)) {
            self->master->overlay_flags += 0x10;
            self->master->swell_step = (unsigned char)(45 - self->frame * 3);
        }
        break;
    case 541:
        rag_throw_item(self, UI_ITEM "버서크포션.bmp", 1);
        break;
    case 542:
        if (!(self->master->overlay_flags & 0x4000) && self->frame <= 150) {
            self->master->overlay_flags += 0x4000;
            self->master->tint_fade_step = (unsigned char)(self->frame / 3);
        }
        break;
    case 543:
        if (self->frame < 200 && !(self->master->overlay_flags & 0x4000)) {
            self->master->overlay_flags += 0x4000;
            if (self->frame <= 25)
                self->master->tint_fade_step = (unsigned char)self->frame;
            if (self->frame >= 151 && self->frame <= 175)
                self->master->tint_fade_step = (unsigned char)(175 - self->frame);
        }
        rag_pok_bursts(self, 1);
        break;
    case 544: case 1398:
        if (!(self->master->overlay_flags & 0x40000)) {
            self->master->overlay_flags += 0x40000;
            self->master->tint_fade_step = self->frame <= 200 ? (unsigned char)(int)((float)self->frame * 0.125f) : 25;
        }
        break;
    case 545:
        rag_valentine_sprite(self, 4); break;
    case 546:
        rag_pok_bursts(self, 2); break;
    case 547:
        rag_wave_at(self, 10, "effect\\T_바람마법.wav");
        rag_rising_columns(self, "effect\\270바람.tga");
        break;
    case 548:
        if (self->frame <= 16) {
            self->master->overlay_flags |= 0x40000;
            self->master->tint_fade_step = (unsigned char)(self->frame * 3);
        }
        break;
    case 549:
        if (self->frame <= 16) {
            self->master->overlay_flags |= 0x4000;
            self->master->tint_fade_step = (unsigned char)(self->frame * 3);
        }
        break;
    case 550:
        if (self->frame >= 31 && self->frame <= 50) {
            self->master->overlay_flags |= 0x2000;
            self->master->overlay_step = (short)self->frame;
        }
        break;
    case 551:
        rag_columns(self, 0); break;
    case 552:
        rag_three_rings(self, "effect\\ring_blue.tga"); break;
    case 553:
        rag_flying_kick_wave(self);
        rag_columns_at_offset(self, "effect\\270바람.tga", 0);
        rag_columns_at_offset(self, "effect\\270바람.tga", 1);
        break;
    case 554:
        rag_effect_texture(self, "effect\\freeze_a.bmp", 2); break;
    case 555:
        rag_wave_at(self, 0, "effect\\T_에너지방출.wav");
        rag_columns(self, 1);
        break;
    case 556:
        rag_pok_burst_at_offset(self, 1); break;
    case 557:
        rag_water_pillar(self, 0, 0, 0); break;
    case 558:
        rag_water_pillar(self, 1, 0, 0); break;
    case 559:
        rag_water_pillar(self, 0, 1, 0); break;
    case 560:
        rag_water_pillar(self, 1, 1, 0); break;
    case 561:
        rag_fire_change_rings(self, 0); break;
    case 562:
        rag_fire_change_rings(self, 1); break;
    case 563:
        if (self->frame < 100 && self->frame % 5 == 0)
            rag_quake();
        break;
    case 564:
        rag_spell_casting_rings(self, "effect\\ring_red.tga"); break;
    case 565:
        rag_wave_at(self, 0, "effect\\h_moonlight_1.wav");
        if (!rag_str_only(self))
            return 0;
        break;
    case 566:
        rag_wave_at(self, 0, "effect\\h_moonlight_2.wav");
        if (!rag_str_only(self))
            return 0;
        break;
    case 567:
        rag_wave_at(self, 0, "effect\\h_moonlight_3.wav");
        if (!rag_str_only(self))
            return 0;
        break;
    case 568:
        rag_wave_at(self, 0, "levelup.wav");
        if (!rag_str_only(self))
            return 0;
        break;
    case 569: case 603: case 717: case 959: case 960: case 964: case 965: case 966: case 967:
    case 975: case 1019: case 1020: case 1021: case 1029: case 1031: case 1033: case 1055:
        if (!rag_str_only(self))
            return 0;
        break;
    case 570:
        rag_msg_sprite(self, 0); break;
    case 571:
        rag_msg_sprite(self, 1); break;
    case 572:
        rag_wave_at(self, 10, "effect\\T_따듯한마법.wav");
        rag_pok_bursts(self, 3);
        break;
    case 573:
        for (i = 0; i < 80; i++)
            rag_falling_items(self, 0);
        break;
    case 574:
        for (i = 0; i < 80; i++)
            rag_falling_items(self, 1);
        break;
    case 575:
        for (i = 0; i < 80; i++)
            rag_falling_items(self, 2);
        break;
    case 576:
        rag_msg_sprite(self, 2); break;
    case 577:
        rag_msg_sprite(self, 3); break;
    case 578:
        rag_msg_sprite(self, 4); break;
    case 579:
        rag_msg_sprite(self, 5); break;
    case 580:
        rag_msg_sprite(self, 6); break;
    case 581:
        rag_forest_light(self, "effect\\cloud11.tga", 11); break;
    case 582:
        rag_angel(self, 11);
        rag_angel(self, 10);
        break;
    case 583:
        rag_msg_sprite(self, 7); break;
    case 584:
        rag_wave_at(self, 5, "effect\\풀버스터.wav");
        rag_msg_sprite(self, 8);
        break;
    case 585:
        rag_msg_sprite(self, 9); break;
    case 586:
        rag_msg_sprite(self, 0xa); break;
    case 587:
        rag_msg_sprite(self, 0xb); break;
    case 588:
        rag_msg_sprite(self, 0xc); break;
    case 589:
        rag_msg_sprite(self, 0xd); break;
    case 590:
        if (self->frame == 15) {
            rag_blue_magic_rings_now(self, 0);
            rag_blue_magic_rings_now(self, 45);
        }
        break;
    case 592:
        for (i = 0; i < 80; i++)
            rag_clouds(self, 5);
        break;
    case 593: case 594: case 595: case 596: case 597: case 598:
        rag_wave_at(self, 0, "_heal_effect.wav");
        if (!rag_str_only(self))
            return 0;
        break;
    case 599:
        if (self->frame == 5)
            rag_play_wave_here(self, "effect\\EF_BeginSpell.wav");
        rag_flicker_tint(self, 5, 25, 0, 1, 250, 250, 100);
        break;
    case 600:
        rag_throw_item(self, UI_ITEM "베넘나이프.bmp", 3);
        break;
    case 601:
        rag_sight_trail(self); break;
    case 602:
        rag_wave_at(self, 20, "effect\\ef_hit6.wav");
        if (self->frame > 20) {
            self->master->overlay_flags |= 0x2000;
            self->master->overlay_step = (short)self->frame;
            if (self->frame & 1)
                render_object_set_argb(self->master, -1, 255, 255, 255);
            else
                render_object_set_argb(self->master, -1, 250, 50, 50);
        }
        break;
    case 604:
        rag_stop_sprite(self, 2); break;
    case 605:
        if (self->frame == 0)
            render_object_send(self->master, NULL, 0x6d, 0x25c, 0, 0, 0);
        break;
    case 606:
        rag_directed_sprite(self, 2); break;
    case 607:
        rag_directed_sprite(self, 1); break;
    case 608:
        if (self->frame == 45) {
            rag_follow_master(self);
            rag_play_wave_here(self, "_heal_effect.wav");
        }
        if (!rag_str_only(self))
            return 0;
        break;
    case 609:
        rag_wave_at(self, 50, "caramel_die.wav");
        if (!rag_str_only(self))
            return 0;
        break;
    case 610:
        rag_wave_at(self, 0, "_heal_effect.wav");
        rag_effect_texture(self, "effect\\success.bmp", 3);
        break;
    case 611:
        rag_wave_at(self, 0, "caramel_die.wav");
        rag_effect_texture(self, "effect\\failed.bmp", 3);
        break;
    case 612:
        rag_msg_sprite(self, 0xe);
        rag_str_step_up80(self);
        break;
    case 613:
        rag_throw_item(self, UI_ITEM "수리검.bmp", 3);
        break;
    case 614:
        rag_throw_item(self, UI_ITEM "쿠나이_독.bmp", 3);
        break;
    case 615:
        rag_throw_item(self, UI_ITEM "풍마_열화.bmp", 3);
        break;
    case 616:
        rag_throw_item(self, "effect\\coin_a.bmp", 3); break;
    case 617:
        rag_master_copies(self); break;
    case 618:
        rag_sakura_petals(self);
        for (i = 0; i < 2; i++)
            rag_four_turned_rings(self, "effect\\alpha_center.tga", (char)i, 2);
        break;
    case 619:
        rag_earth_spike(self, 2); break;
    case 620:
        rag_bottom_ring(self, "effect\\ring_blue.tga", 14); break;
    case 621:
        rag_wave_at(self, 0, "effect\\풍인.wav");
        for (i = 0; i < self->hit_count; i++)
            rag_columns_wide(self, "effect\\270바람.tga", (char)(i % 2));
        break;
    case 622:
        rag_thunderstorm_sprite(self); break;
    case 623:
        rag_wave_at(self, 28, "effect\\피어싱샷.wav");
        for (i = 0; i < 4; i++) {
            rag_line_bursts(self, 20);
            rag_line_bursts(self, 21);
        }
        break;
    case 624:
        rag_wave_at(self, 1, "effect\\t_바람방출.wav");
        rag_columns_shifted(self, "effect\\270바람.tga");
        break;
    case 625:
        if (self->frame % 4 == 0 && !(self->master->overlay_flags & 0x4000)) {
            self->master->overlay_flags += 0x4000;
            self->master->tint_fade_step = 20;
        }
        break;
    case 626:
        if (self->frame % 4 == 0 && !(self->master->overlay_flags & 0x40000)) {
            self->master->overlay_flags += 0x40000;
            self->master->tint_fade_step = 20;
        }
        break;
    case 627:
        rag_wave_at(self, 21, "effect\\디스암.wav");
        rag_tumbling_items(self, UI_ITEM "골드룩스.bmp", 5.0f, 0);
        rag_tumbling_items(self, UI_ITEM "더개리슨.bmp", 5.0f, 0);
        rag_tumbling_items(self, UI_ITEM "더블샷건.bmp", 5.0f, 0);
        rag_tumbling_items(self, UI_ITEM "더스크.bmp", 5.0f, 0);
        break;
    case 628:
        rag_wave_at(self, 0, "effect\\더스트샷.wav");
        for (i = 0; i < 6; i++)
            rag_four_turned_rings(self, "effect\\alpha_center.tga", (char)i, 5);
        break;
    case 629:
        rag_spirit_spheres(self, 3); break;
    case 630:
        rag_wave_at(self, 0, "effect\\그림자베기.wav");
        rag_msg_sprite(self, 0x0f);
        break;
    case 631:
        rag_msg_sprite(self, 0x10); break;
    case 632:
        rag_wave_at(self, 0, "effect\\안개베기.wav");
        rag_msg_sprite(self, 0x11);
        break;
    case 633:
        rag_wave_at(self, 0, "effect\\일섬.wav");
        rag_msg_sprite(self, 0x12);
        break;
    case 634:
        rag_msg_sprite(self, 0x13); break;
    case 635:
        rag_wave_at(self, 0, "effect\\폭염룡.wav");
        rag_str_step_up80(self);
        break;
    case 636:
        rag_wave_at(self, 0, "effect\\빙정락.wav");
        rag_str_step_up80(self);
        break;
    case 637:
        rag_wave_at(self, 10, "effect\\데스페라도.wav");
        rag_msg_sprite(self, 0x14);
        break;
    case 638:
        rag_msg_sprite(self, 0x15); break;
    case 639:
        rag_msg_sprite(self, 0x16); break;
    case 640:
        rag_msg_sprite(self, 0x17); break;
    case 641:
        rag_msg_sprite(self, 0x18); break;
    case 642:
        rag_msg_sprite(self, 0x19); break;
    case 643:
        rag_wave_at(self, 20, "effect\\래피드샤워.wav");
        rag_msg_sprite(self, 0x1a);
        break;
    case 644:
        rag_wave_at(self, 0, "effect\\매지컬블릿.wav");
        rag_msg_sprite(self, 0x1b);
        break;
    case 645:
        rag_msg_sprite(self, 0x1c); break;
    case 647:
        rag_msg_sprite(self, 0x1d); break;
    case 648:
        if (self->frame == 25) {
            rag_follow_master(self);
            rag_wave_plain("effect\\트리플액션.wav");
        }
        rag_msg_sprite(self, 0x1e);
        break;
    case 650:
        rag_aura_disc(self, "effect\\mjin2.tga", 0);
        rag_aura_disc(self, "effect\\mjin.tga", 1);
        rag_aura_pair(self, "effect\\pikapika2.bmp");
        rag_aura_rings(self, "effect\\ring_blue.tga");
        break;
    case 651:
        rag_aura_disc(self, "effect\\mjin2.tga", 2);
        rag_aura_disc(self, "effect\\mjin.tga", 3);
        for (i = 2; i < 21; i += 2)
            rag_aura_particles(self, "effect\\mpa3.tga", (char)i, 0);
        break;
    case 652:
        if (self->frame == 0) {
            rag_follow_master(self);
            rag_play_wave_here(self, "effect\\EF_hit2.wav");
            rag_master_flash(self, 0xffff0000);
        }
        break;
    case 653:
        if (self->frame == 0)
            rag_master_flash(self, 0xffff0000);
        break;
    case 654:
        if (self->frame == 0)
            rag_master_flash(self, 0xffff00ff);
        break;
    case 655:
        if (!(self->master->overlay_flags & 0x80000)) {
            self->master->overlay_flags += 0x80000;
            self->master->outline_alpha = self->frame < 200 ? (unsigned char)self->frame : 200;
        }
        break;
    case 656:
        if (self->frame < 40 && !(self->master->overlay_flags & 0x80000)) {
            self->master->overlay_flags += 0x80000;
            self->master->outline_alpha = (unsigned char)(200 - self->frame * 5);
        }
        break;
    case 665:
        rag_thunderstorm_particle(self); break;
    case 666:
        rag_wave_at(self, 0, "effect\\earth_quake.wav");
        rag_msg_sprite(self, 0x1f);
        if (self->frame == 0 || self->frame == 35 || self->frame == 70)
            rag_quake();
        break;
    case 667:
        if (!(self->master->overlay_flags & 2)) {
            self->master->overlay_flags += 2;
            self->master->outline_alpha = self->frame < 25 ? (unsigned char)(self->frame << 3) : 200;
        }
        break;
    case 668:
        if (self->frame == 1) {
            rag_follow_master(self);
            rag_play_wave_here(self, "effect\\dragonfear.wav");
            rag_quake();
        }
        rag_str_step_up80(self);
        break;
    case 669:
        rag_wave_at(self, 1, "effect\\wideb.wav");
        rag_colored_quad(self, 1, 255, 0, 0);
        rag_str_step_up80(self);
        break;
    case 671:
        rag_ground_disc(self, "effect\\hanmoon1.tga", 3); break;
    case 672:
        rag_ground_disc(self, "effect\\hanmoon2.tga", 3); break;
    case 673:
        rag_colored_quad(self, 2, 0, 0, 255); break;
    case 674:
        rag_song_notes(self, "effect\\curse.bmp", 6); break;
    case 675:
        rag_guard(self, "effect\\guardK.tga", 2); break;
    case 676:
        rag_effect_texture(self, "effect\\blast_mine##clock.bmp", 4);
        rag_wave_at(self, 10, "effect\\EF_DecAgility.wav");
        break;
    case 677:
        rag_wave_at(self, 1, "effect\\wideb.wav");
        rag_str_step_up80(self);
        break;
    case 678:
        rag_level99_aura3(self, 2); break;
    case 679:
        rag_level99_aura(self, "effect\\ring_white.tga", 1); break;
    case 680:
        rag_level99_aura2(self, "effect\\pikapika2.bmp", 1); break;
    case 681:
        rag_effect_texture(self, SPHERE "2.bmp", 12);
        rag_effect_texture(self, SPHERE "3.bmp", 13);
        rag_effect_texture(self, SPHERE "4.bmp", 11);
        rag_effect_texture(self, SPHERE "5.bmp", 10);
        break;
    case 682:
        rag_msg_sprite(self, 0x20); break;
    case 683:
        rag_msg_sprite(self, 0x21); break;
    case 684:
        rag_msg_sprite(self, 0x22); break;
    case 685:
        rag_msg_sprite(self, 0x23); break;
    case 686:
        rag_msg_sprite(self, 0x24); break;
    case 687:
        rag_aura_disc(self, MAGIC_CIRCLE "1-1.bmp", 4);
        rag_aura_disc(self, MAGIC_CIRCLE "1-2.bmp", 14);
        rag_aura_disc(self, MAGIC_CIRCLE "1-3.bmp", 4);
        break;
    case 688:
        rag_aura_disc(self, MAGIC_CIRCLE "2-1.bmp", 4);
        rag_aura_disc(self, MAGIC_CIRCLE "2-2.bmp", 14);
        rag_aura_disc(self, MAGIC_CIRCLE "2-3.bmp", 4);
        break;
    case 689:
        rag_texture_animation(self, 0); break;
    case 690:
        rag_texture_animation(self, 1); break;
    case 691:
        rag_texture_animation(self, 2); break;
    case 692:
        rag_level99_aura3(self, 3); break;
    case 693:
        rag_effect_texture(self, "effect\\glow01.bmp", 14); break;
    case 694:
        rag_effect_texture(self, "effect\\glow02.bmp", 14); break;
    case 695:
        rag_aura_disc(self, "effect\\glow04.bmp", 5);
        for (i = 2; i < 21; i += 2)
            rag_aura_particles(self, "effect\\mpa3.tga", (char)i, 2);
        break;
    case 696:
        rag_texture_animation(self, 3); break;
    case 697:
        for (i = 0; i < 80; i++)
            rag_clouds(self, 7);
        break;
    case 698:
        for (i = 0; i < 80; i++)
            rag_clouds(self, 8);
        break;
    case 700:
        rag_effect_texture(self, SPHERE "12.bmp", 12);
        rag_effect_texture(self, SPHERE "3.bmp", 13);
        rag_effect_texture(self, SPHERE "14.bmp", 11);
        rag_effect_texture(self, SPHERE "15.bmp", 10);
        break;
    case 701:
        rag_effect_texture(self, "effect\\glow11.bmp", 15); break;
    case 702:
        rag_effect_texture(self, "effect\\glow12b .bmp", 14); break;
    case 703:
        rag_three_rings_129(self, "effect\\alpha_upper.tga", 0);
        rag_level99_aura3(self, 4);
        break;
    case 707:
        rag_msg_sprite(self, 0x25); break;
    case 708:
        if (self->frame == 0)
            rag_play_wave_here(self, "effect\\wizard_stormgust.wav");
        rag_str_step_up80(self);
        break;
    case 710:
        rag_effect_texture(self, "effect\\b.bmp", 18);
        rag_effect_texture(self, "effect\\glow02.bmp", 16);
        rag_effect_texture(self, "effect\\glow02.bmp", 17);
        rag_level99_aura3(self, 5);
        break;
    case 711:
        rag_water_pillar(self, 0, 10, 10);
        for (i = 0; i < 2; i++)
            rag_level99_aura3(self, 6);
        break;
    case 712:
        rag_effect_texture(self, "effect\\b.bmp", 18);
        rag_effect_texture(self, "effect\\glow03.bmp", 16);
        rag_effect_texture(self, "effect\\glow03.bmp", 17);
        rag_level99_aura3(self, 7);
        break;
    case 713:
        rag_soul_particles(self, 0); break;
    case 714:
        rag_soul_particles(self, 1); break;
    case 715: case 716:
        rag_styled_rings(self, "effect\\ring_blue.tga", 10);
        rag_styled_rings(self, "effect\\ring_blue.tga", 11);
        break;
    case 718:
        rag_fire_pillar_on(self, 1);
        for (i = 0; i < 3; i++)
            rag_ring_burst(self, "effect\\ring_white.tga", 11);
        break;
    case 719:
        rag_forest_light(self, "effect\\cloud11.tga", 12); break;
    case 720:
        rag_wave_at(self, 20, "effect\\기공포.wav");
        if ((unsigned)self->frame <= 10) {
            self->master->overlay_flags |= 1;
            render_object_set_argb(self->master, -1, 205, 205, 0);
        }
        rag_slash_waves(self, 1);
        break;
    case 722:
        rag_detecting(self, 7);
        rag_str_step_up80(self);
        break;
    case 723:
        rag_follow_master(self);
        if (self->frame == 0)
            rag_play_wave_here(self, "hundred.wav");
        rag_directed_sprite(self, 12);
        break;
    case 724:
        rag_wave_at(self, 1, "effect\\wl_siennaexecrate.wav");
        rag_detecting(self, 8);
        break;
    case 725:
        rag_detecting(self, 2); break;
    case 726:
        rag_wave_at(self, 10, "effect\\t_에너지방출.wav");
        rag_cast_circle_131(self, "effect\\freeze_a.bmp");
        rag_snowfall(self, 1);
        break;
    case 727:
        rag_follow_master(self);
        if (self->frame == 1)
            rag_play_wave_here(self, "crimson_r.wav");
        rag_str_step_up80(self);
        break;
    case 728:
        rag_follow_master(self);
        if (self->frame == 1)
            rag_play_wave_here(self, "hell_in.wav");
        rag_str_step_up80(self);
        break;
    case 729:
        rag_msg_sprite(self, 0x26); break;
    case 730:
        rag_msg_sprite(self, 0x27); break;
    case 732:
        rag_stone_chips(self); break;
    case 733:
        rag_wave_at(self, 20, "effect\\기공포.wav");
        if ((unsigned)self->frame <= 10) {
            self->master->overlay_flags |= 1;
            render_object_set_argb(self->master, -1, 155, 155, 255);
        }
        rag_slash_waves(self, 1);
        break;
    case 734:
        rag_follow_master(self);
        if (self->frame == 1)
            rag_play_wave_here(self, "chainlight.wav");
        rag_str_step_up80(self);
        break;
    case 735:
        rag_spirit_spheres(self, 10); break;
    case 736:
        rag_spirit_spheres(self, 11); break;
    case 737:
        rag_spirit_spheres(self, 12); break;
    case 738:
        rag_spirit_spheres(self, 13); break;
    case 739:
        rag_msg_sprite(self, 0x35); break;
    case 740:
        rag_msg_sprite(self, 0x34); break;
    case 741:
        rag_msg_sprite(self, 0x33); break;
    case 742:
        rag_msg_sprite(self, 0x36); break;
    case 743:
        rag_msg_sprite(self, 0x32); break;
    case 744:
        rag_cloaking(self); break;
    case 745:
        rag_follow_master(self);
        if (self->frame == 1)
            rag_play_wave_here(self, "aimed.wav");
        rag_str_step_up80(self);
        break;
    case 746:
        rag_follow_master(self);
        if (self->frame == 1)
            rag_play_wave_here(self, "arrowstorm.wav");
        rag_str_step_up80(self);
        break;
    case 747: case 748:
        rag_follow_master(self);
        if (self->frame == 1)
            rag_play_wave_here(self, "lauagnus.wav");
        rag_str_step_up80(self);
        break;
    case 749:
        rag_follow_master(self);
        rag_str_step_up80(self);
        break;
    case 750:
        if (self->frame >= 10 && self->frame <= 40) {
            if (!(self->frame & 1))
                self->master->overlay_flags |= 1;
            render_object_set_argb(self->master, -1, 155, 155, 255);
        }
        if (!rag_concentration(self))
            return 0;
        break;
    case 751:
        rag_fireball_volleys(self); break;
    case 752:
        rag_blur_trail(self); break;
    case 753:
        if (self->frame == 1)
            rag_play_wave_here(self, "effect\\샤프슈팅.wav");
        rag_time_bubbles(self, "effect\\white02.bmp", 1);
        rag_storm_ring(self, "effect\\storm2.tga", 8);
        break;
    case 754:
        rag_white_rings(self);
        if (!rag_spreading_rings(self, "effect\\alpha_down.tga", 0))
            return 0;
        break;
    case 755:
        rag_effect_texture(self, "effect\\마법진" "1-3.bmp", 19);
        break;
    case 756:
        if (!rag_tinted_puffs(self))
            return 0;
        break;
    case 757:
        rag_clouds_textured(self, "effect\\emp shock.tga", 0); break;
    case 758:
        rag_ground_ring_147(self, SPIN_COUNTER "1.tga", 0); break;
    case 759:
        rag_ground_ring_147(self, SPIN_COUNTER "2.tga", 0); break;
    case 760:
        rag_ground_ring_147(self, SPIN_COUNTER "3.tga", 0); break;
    case 761:
        rag_ground_ring_147(self, SPIN_COUNTER "4.tga", 0); break;
    case 762:
        rag_ground_ring_147(self, SPIN_COUNTER "5.tga", 0); break;
    case 763:
        rag_ground_ring_147(self, SPIN_COUNTER "6.tga", 0); break;
    case 764:
        rag_ground_ring_147(self, SPIN_COUNTER "7.tga", 0); break;
    case 765:
        rag_ground_ring_147(self, SPIN_COUNTER "8.tga", 0); break;
    case 766:
        rag_ground_ring_147(self, SPIN_COUNTER "9.tga", 0); break;
    case 767:
        rag_ground_ring_147(self, SPIN_COUNTER "10.tga", 0); break;
    case 768:
        rag_flicker_tint(self, INT_MIN, 30, 8, 1, 255, 0, 255); break;
    case 769:
        rag_flying_kick_wave(self); break;
    case 770:
        rag_weapon_toss(self); break;
    case 771:
        if (self->frame == 1) {
            rag_wave_plain("effect\\assasin_enchantpoison.wav");
            rag_wave_plain("effect\\EF_PoisonAttack.wav");
        }
        break;
    case 772: case 924:
        rag_poison_attack(self, 1); break;
    case 773:
        rag_ground_rings(self, "effect\\ring_purple.tga", 1); break;
    case 774:
        rag_shield_boomerang(self, UI_ITEM "배틀액스.bmp");
        break;
    case 775:
        rag_wave_at(self, 0, "effect\\EF_BeginSpell.wav");
        if (self->master->state == 0 || self->master->state == 2)
            rag_master_spin(self);
        break;
    case 776:
        if (self->frame == 45)
            rag_wave_plain("effect\\데스페라도.wav");
        break;
    case 777:
        rag_flicker_tint(self, INT_MIN, 60, 8, 1, 155, 155, 255);
        rag_rising_light(self, 2);
        break;
    case 778:
        rag_detecting(self, 3); break;
    case 779:
        rag_flicker_tint(self, INT_MIN, 60, 8, 1, 155, 155, 255);
        rag_rising_light(self, 3);
        break;
    case 780:
        rag_detecting(self, 4); break;
    case 781:
        rag_wave_at(self, 5, "effect\\MAGNETICFIELD.wav");
        rag_electric_arcs(self, 0, 0);
        rag_electric_arcs(self, 0, 180);
        break;
    case 782:
        rag_guard_ring(self, "effect\\성체강복.tga");
        rag_flicker_tint(self, 10, 30, 0, 1, 250, 250, 0);
        break;
    case 783:
        rag_swirl_ring(self, "effect\\slash01.tga", 8); break;
    case 784:
        rag_swirl_ring(self, "effect\\썬더스톰파티클.tga", 8);
        break;
    case 785:
        rag_time_bubbles(self, UI_ITEM "스패너.bmp", 2);
        break;
    case 786:
        rag_flicker_tint(self, 10, 90, 0, 1, 250, 250, 250);
        rag_detecting(self, 5);
        rag_effect_texture(self, "effect\\target3.bmp", 21);
        break;
    case 787:
        rag_torch_flame(self, 0); break;
    case 788:
        rag_effect_texture(self, "effect\\베놈_상태.bmp", 22);
        break;
    case 789:
        rag_effect_texture(self, "effect\\빙결_상태.bmp", 22);
        break;
    case 790:
        rag_effect_texture(self, "effect\\발화_상태.bmp", 22);
        break;
    case 791:
        if (self->frame == 0)
            rag_wave_plain("ice_titan_attack.wav");
        rag_flicker_tint(self, 6, 40, 8, 1, 0, 0, 255);
        for (i = 0; i < 3; i++)
            rag_snow_134(self, 0);
        break;
    case 792:
        rag_hallucination_doubles(self); break;
    case 793:
        rag_time_bubbles(self, "effect\\white02.bmp", 3); break;
    case 794:
        if (self->frame == 0)
            rag_wave_plain("effect\\INFRAREDSCAN.wav");
        rag_scan_arcs(self, 0);
        rag_scan_arcs(self, 1);
        break;
    case 795:
        if (!rag_fire_pillar(self))
            return 0;
        break;
    case 796:
        rag_storm_ring(self, "effect\\storm2.tga", 10);
        rag_white_flash_136(self, "effect\\white01.bmp");
        break;
    case 797:
        if (self->frame == 0)
            rag_wave_plain("effect\\wl_siennaexecrate.wav");
        rag_msg_sprite(self, 0x3c);
        break;
    case 798:
        for (i = 0; i < 16; i++)
            rag_intimidate(self, 2);
        break;
    case 799:
        if (self->frame == 6)
            rag_wave_plain("effect\\wl_stasis.wav");
        rag_flicker_tint(self, 6, 40, 0, 1, 0, 0, 255);
        rag_glow(self, "effect\\thunder_ball_0001.bmp", -10, 3);
        rag_glow(self, "effect\\thunder_ball_0002.bmp", -14, 3);
        rag_glow(self, "effect\\thunder_ball_0003.bmp", -18, 3);
        rag_glow(self, "effect\\thunder_ball_0004.bmp", -22, 3);
        rag_glow(self, "effect\\thunder_ball_0005.bmp", -26, 3);
        rag_glow(self, "effect\\thunder_ball_0006.bmp", -30, 3);
        break;
    default:
        return rag_process_800(self);
    }
    return 1;
}
