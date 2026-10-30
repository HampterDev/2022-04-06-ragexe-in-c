#include "actor/render_ops.h"
#include "actor/object_ops.h"
#include "render/projection.h"
#include "render/math3d.h"
#include "render/rp_pool.h"
#include "resources/ezeffect.h"
#include "fileio/resource_api.h"
#include "effects/durations.h"
#include "session/state.h"
#include <stdlib.h>
#include <string.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"
#include "session/session_state.h"
#include "effects/rag/rag_str_anim.h"

const double k_strAnglePerDegree = 2.8444444444444;

#define FULL_OR_MIN(full, min) (g_session1.map_rule_flags[0] == 0 ? (full) : (min))

static int roll(int n)
{
    int r = rand() % n;
    return (r < 0 ? 0 : r) + 1;
}

void rag_effect_init(struct RagEffect *self, struct RenderObject *master, int effect_id,
                                 Vec3 offset)
{
    char file_name[64];
    const char *name = NULL;
    int variant = 0;

    self->effect_id = effect_id;
    self->master = master;
    self->base.pos = master->pos;
    self->master_roty = master->roty;
    rag_effect_set_duration(self, g_effectDurations[self->effect_id]);
    self->offset = offset;
    sprintf(file_name, "");

    switch (self->effect_id) {
    case 10:   name = FULL_OR_MIN("Maemor.str", "memor_min.str"); break;
    case 13:   name = "SafetyWall.str"; break;
    case 23:   name = "StoneCurse.str"; break;
    case 25:   variant = roll(2); name = "Firewall%d.str"; break;
    case 29:   name = "Lightning.str"; break;
    case 30:   name = "ThunderStorm.str"; break;
    case 40:   name = "Cross.str"; break;
    case 41:   name = FULL_OR_MIN("Angelus.str", "jong_mini.str"); break;
    case 49:   variant = roll(3); name = "FireHit%d.str"; break;
    case 52:   variant = roll(3); name = "WindHit%d.str"; break;
    case 64:   name = "ArrowShot.str"; break;
    case 65:   name = "Invenom.str"; break;
    case 66:   name = FULL_OR_MIN("Cure.str", "cure_min.str"); break;
    case 67:   name = "Provoke.str"; break;
    case 68:   name = "Mvp.str"; break;
    case 69:   name = "SkidTrap.str"; break;
    case 70:   name = "Brandish.str"; break;
    case 75:   name = FULL_OR_MIN("Gloria.str", "gloria_min.str"); break;
    case 76:   name = FULL_OR_MIN("Magnificat.str", "magnificat_min.str"); break;
    case 77:   name = FULL_OR_MIN("Resurrection.str", "resurrection_min.str"); break;
    case 78:   name = "Recovery.str"; break;
    case 83:   name = "Sanctuary.str"; break;
    case 84:   name = "Impositio.str"; break;
    case 85:   name = FULL_OR_MIN("LexAeterna.str", "lexaeterna_min.str"); break;
    case 86:   name = "Aspersio.str"; break;
    case 87:   name = "LexDivina.str"; break;
    case 88:   name = FULL_OR_MIN("Suffragium.str", "suffragium_min.str"); break;
    case 89:   name = "StormGust.str"; break;
    case 90:   name = "Lord.str"; break;
    case 91:   name = "Benedictio.str"; break;
    case 92:   variant = roll(3); name = "Meteor%d.str"; break;
    case 95:   name = "Quagmire.str"; break;
    case 96:   name = "FirePillar.str"; break;
    case 97:   name = "FirePillarBomb.str"; break;
    case 101:  name = "RepairWeapon.str"; break;
    case 102:  name = "CrashEarth.str"; break;
    case 103:  name = FULL_OR_MIN("WeaponPerfection.str", "WeaponPerfection_min.str"); break;
    case 104:  name = FULL_OR_MIN("MaximizePower.str", "maximize_min.str"); break;
    case 106:  name = "BlastMine.str"; break;
    case 107:  name = "Claymore.str"; break;
    case 108:  name = "Freezing.str"; break;
    case 109:  variant = roll(4); name = "Bubble%d.str"; break;
    case 110:  name = "GasPush.str"; break;
    case 111:  name = "Spring.str"; break;
    case 112:  name = FULL_OR_MIN("Kyrie.str", "kyrie_min.str"); break;
    case 113:  name = "Magnus.str"; break;
    case 124:  name = "VenomDust.str"; break;
    case 126:  name = "PoisonReact_1st.str"; break;
    case 127:  name = "PoisonReact.str"; break;
    case 129:  name = "VenomSplasher.str"; break;
    case 130:  name = "TwoHand.str"; break;
    case 131:  name = "AutoCounter.str"; break;
    case 133:  name = "Freeze.str"; break;
    case 134:  name = "Freezed.str"; break;
    case 135:  name = "IceCrash.str"; break;
    case 136:  name = "slowp.str"; break;
    case 139:  name = "SandMan.str"; break;
    case 141:  variant = roll(3); name = "Pneuma%d.str"; break;
    case 143:  name = "SonicBlow.str"; break;
    case 144:  name = "Brandish2.str"; break;
    case 145:  name = "ShockWave.str"; break;
    case 146:  name = "ShockWaveHit.str"; break;
    case 147:  name = "EarthHit.str"; break;
    case 148:  name = "Pierce.str"; break;
    case 149:  name = "Bowling.str"; break;
    case 150:  name = "SpearStab.str"; break;
    case 151:  name = "SpearBoomerang.str"; break;
    case 152:  name = "HolyHit.str"; break;
    case 153:  name = "Concentration.str"; break;
    case 154:  name = "bs_RefineSuccess.str"; break;
    case 155:  name = "bs_RefineFailed.str"; break;
    case 156:  name = "JobChange.str"; break;
    case 157:  name = "LevelUP.str"; break;
    case 158:  name = "JobLvUP.str"; break;
    case 167:  name = "TamingSuccess.str"; break;
    case 168:  name = "TamingFailed.str"; break;
    case 169:  name = "EnergyCoat.str"; break;
    case 170:  name = "CartRevolution.str"; break;
    case 181:  name = "MentalBreak.str"; break;
    case 182:  name = "magical.str"; break;
    case 183:  name = "sui_explosion.str"; break;
    case 185:  name = "suicide.str"; break;
    case 186:  name = "yunta_1.str"; break;
    case 187:  name = "yunta_2.str"; break;
    case 188:  name = "yunta_3.str"; break;
    case 189:  name = "yunta_4.str"; break;
    case 190:  name = "yunta_5.str"; break;
    case 191:  name = "homing.str"; break;
    case 192:  name = "poison.str"; break;
    case 193:  name = "silence.str"; break;
    case 194:  name = "stun.str"; break;
    case 195:  name = "StoneCurse.str"; break;
    case 197:  name = "sleep.str"; break;
    case 199:  variant = roll(3); name = "Pong%d.str"; break;
    case 204:  name = "빨간포션.str"; break;
    case 205:  name = "주홍포션.str"; break;
    case 206:  name = "노란포션.str"; break;
    case 207:  name = "하얀포션.str"; break;
    case 208:  name = "파란포션.str"; break;
    case 209:  name = "초록포션.str"; break;
    case 210:  name = "fruit.str"; break;
    case 211:  name = "fruit_.str"; break;
    case 213:  name = "Deffender.str"; break;
    case 214:  name = "Keeping.str"; break;
    case 218:  name = "집중.str"; break;
    case 219:  name = "각성.str"; break;
    case 220:  name = "버서크.str"; break;
    case 234:  name = "spell.str"; break;
    case 235:  name = "디스펠.str"; break;
    case 244:  name = "매직로드.str"; break;
    case 245:  name = "holy_cross.str"; break;
    case 246:  name = "shield_charge.str"; break;
    case 248:  name = "providence.str"; break;
    case 250:  name = "TwoHand.str"; break;
    case 251:  name = "devotion.str"; break;
    case 255:  name = "enc_fire.str"; break;
    case 256:  name = "enc_ice.str"; break;
    case 257:  name = "enc_wind.str"; break;
    case 258:  name = "enc_earth.str"; break;
    case 268:  name = "steal_coin.str"; break;
    case 269:  name = "strip_weapon.str"; break;
    case 270:  name = "strip_shield.str"; break;
    case 271:  name = "strip_armor.str"; break;
    case 272:  name = "strip_helm.str"; break;
    case 273:  name = "연환.str"; break;
    case 305:  name = "p_success.str"; break;
    case 306:  name = "p_failed.str"; break;
    case 311:  name = "loud.str"; break;
    case 315:  name = "SafetyWall.str"; break;
    case 337:  name = "JobLvUP.str"; break;
    case 369:  name = "TwoHand.str"; break;
    case 371:  name = "angel.str"; break;
    case 372:  name = "devil.str"; break;
    case 390:  name = "melt.str"; break;
    case 391:  name = "cart.str"; break;
    case 392:  name = "sword.str"; break;
    case 406:  name = "소울번.str"; break;
    case 407:  name = "사람효과.str"; break;
    case 440:  name = "asum.str"; break;
    case 491:  name = "찹쌀떡.str"; break;
    case 492:  name = "ramadan.str"; break;
    case 507:  name = "mapae.str"; break;
    case 508:  name = "itempokjuk.str"; break;
    case 565:  name = "moonlight_1.str"; break;
    case 566:  name = "moonlight_2.str"; break;
    case 567:  name = "moonlight_3.str"; break;
    case 568:  name = "h_levelup.str"; break;
    case 569:  name = "defense.str"; break;
    case 593:  name = "food_str.str"; break;
    case 594:  name = "food_int.str"; break;
    case 595:  name = "food_vit.str"; break;
    case 596:  name = "food_agi.str"; break;
    case 597:  name = "food_dex.str"; break;
    case 598:  name = "food_luk.str"; break;
    case 603:  variant = roll(3); name = "FireHit%d.str"; break;
    case 608:  name = "cook_suc.str"; break;
    case 609:  name = "cook_fail.str"; break;
    case 612:  name = "itempokjuk.str"; break;
    case 635:  name = "fire dragon.str"; break;
    case 636:  name = "icy.str"; break;
    case 646:  name = "트랙킹.str"; break;
    case 649:  name = "불스아이.str"; break;
    case 668:  name = "dfear.str"; break;
    case 669:  name = "wideb.str"; break;
    case 670:  name = "dfear.str"; break;
    case 677:  name = "cwound.str"; break;
    case 699:  name = "flower_leaf.str"; break;
    case 704:  name = "mobile_ef02.str"; break;
    case 705:  name = "mobile_ef01.str"; break;
    case 706:  name = "mobile_ef03.str"; break;
    case 708:  name = "storm_min.str"; break;
    case 709:  name = "pokjuk_jap.str"; break;
    case 717:  name = FULL_OR_MIN("Angelus.str", "jong_mini.str"); break;
    case 721:  name = "ado.str"; break;
    case 722:  name = "이그니션브레이크.str"; break;
    case 727:  name = "crimson_r.str"; break;
    case 728:  name = "hell_in.str"; break;
    case 731:  name = "dragon_h.str"; break;
    case 734:  name = "chainlight.str"; break;
    case 745:  name = "aimed.str"; break;
    case 746:  name = "arrowstorm.str"; break;
    case 747:  name = "laulamus.str"; break;
    case 748:  name = "lauagnus.str"; break;
    case 749:  name = "mil_shield.str"; break;
    case 750:  name = "Concentration.str"; break;
    case 756:  name = "버서크.str"; break;
    case 795:  name = "powerswing.str"; break;
    case 813:  name = "enervation.str"; break;
    case 814:  name = "groomy.str"; break;
    case 815:  name = "ignorance.str"; break;
    case 816:  name = "laziness.str"; break;
    case 817:  name = "unlucky.str"; break;
    case 818:  name = "weakness.str"; break;
    case 920:  name = "Firewall_per.str"; break;
    case 926:  name = "hunter_shockwave_blue.str"; break;
    case 959:  name = "poison_mist.str"; break;
    case 960:  name = "eraser_cutter.str"; break;
    case 964:  name = "lava_slide.str"; break;
    case 965:  name = "sonic_claw.str"; break;
    case 966:  name = "tinder.str"; break;
    case 967:  name = "mid_frenzy.str"; break;
    case 975:  name = "vash00.str"; break;
    case 987:  name = "rwc2011.str"; break;
    case 988:  name = "rwc2011_2.str"; break;
    case 1006: name = "100.str"; break;
    case 1015: name = "rune_success.str"; break;
    case 1016: name = "rune_fail.str"; break;
    case 1017: name = "changematerial_su.str"; break;
    case 1018: name = "changematerial_fa.str"; break;
    case 1019: name = "Guardian.str"; break;
    case 1020: variant = roll(4); name = "bubble%d_1.str"; break;
    case 1021: name = "dust.str"; self->str_max_loops = 10; break;
    case 1029: name = "dancingblade.str"; break;
    case 1031: name = "INVINCIBLEOFF2.str"; break;
    case 1033: name = "devil.str"; break;
    case 1040: name = "gc_darkcrow.str"; self->str_max_loops = 9999; break;
    case 1042: name = "all_full_throttle.str"; break;
    case 1043: name = "sr_flashcombo.str"; break;
    case 1044: name = "rk_luxanima.str"; break;
    case 1046: name = "so_elemental_shield.str"; break;
    case 1047: name = "AB_OFFERTORIUM.str"; break;
    case 1048: name = "WL_TELEKINESIS_INTENSE.str"; self->str_max_loops = 9999; break;
    case 1049: name = "GN_ILLUSIONDOPING.str"; break;
    case 1050: name = "NC_MAGMA_ERUPTION.str"; self->str_max_loops = 9999; break;
    case 1055: name = "chill.str"; self->str_max_loops = 9999; break;
    case 1057: name = "AB_OFFERTORIUM_ring.str"; self->str_max_loops = 9999; break;
    default:   break;
    }
    if (name) {
        if (variant)
            sprintf(file_name, name, variant);
        else
            sprintf(file_name, name);
    }

    if (strlen(file_name) == 0)
        return;
    rag_str_load(self, file_name);
}

static void render_layer(struct RagEffect *self, struct StrLayer *layer, const struct StrKeyState *s, const float *view)
{
    struct TexDrawPrim *batch;
    struct TexDrawVertex *q;
    Vec3 pos = self->master->pos;
    float sx, sy, rhw, scale, rad, sin_a, cos_a, u_scale, v_scale;
    struct Texture *texture;
    unsigned color;
    int tex_frame, i;

    renderer_project_point(g_renderer, &pos, view, &sx, &sy, &rhw);
    scale = renderer_screen_scale(g_renderer, rhw);
    batch = renderer_new_prim(g_renderer);
    q = batch->base.verts;

    if (s->corner_x[1] > s->corner_x[0]) {
        q[0].x = s->corner_x[3]; q[1].x = s->corner_x[0]; q[2].x = s->corner_x[2]; q[3].x = s->corner_x[1];
        q[1].u = q[0].u = s->u;
        q[3].u = q[2].u = s->us + s->u;
    } else {
        q[0].x = s->corner_x[2]; q[1].x = s->corner_x[1]; q[2].x = s->corner_x[3]; q[3].x = s->corner_x[0];
        q[1].u = q[0].u = s->us + s->u;
        q[3].u = q[2].u = s->u;
    }
    if (s->corner_y[2] > s->corner_y[1]) {
        q[0].y = s->corner_y[3]; q[1].y = s->corner_y[0]; q[2].y = s->corner_y[2]; q[3].y = s->corner_y[1];
        q[3].v = q[1].v = s->v;
        q[2].v = q[0].v = s->vs + s->v;
    } else {
        q[0].y = s->corner_y[0]; q[1].y = s->corner_y[3]; q[2].y = s->corner_y[1]; q[3].y = s->corner_y[2];
        q[2].v = q[0].v = s->v;
        q[3].v = q[1].v = s->vs + s->v;
    }

    tex_frame = (int)s->aniframe;
    if (tex_frame <= 0)
        tex_frame = 0;
    texture = str_layer_get_texture(layer, tex_frame);
    u_scale = (float)(unsigned)texture->reqWidth / (float)(unsigned)texture->base.width;
    v_scale = (float)(unsigned)texture->reqHeight / (float)(unsigned)texture->base.height;
    for (i = 0; i < 4; i++) {
        q[i].u = q[i].u * u_scale;
        q[i].v = v_scale * q[i].v;
    }

    rad = (float)(s->angle / k_strAnglePerDegree) / g_halfTurnDegrees * g_pi;
    sin_a = float_sin(rad);
    rad = (float)(s->angle / k_strAnglePerDegree) / g_halfTurnDegrees * g_pi;
    cos_a = float_cos(rad);
    for (i = 0; i < 4; i++) {
        float x = q[i].x, y = q[i].y, turned_y, d;
        turned_y = y * cos_a + x * sin_a;
        q[i].x = ((x * cos_a - y * sin_a) + (s->x - STR_SCREEN_CX)) * scale + self->screen_pos.x;
        d = ((s->y - STR_SCREEN_CY) + turned_y) * scale;
        if (session_is_screen_flipped(&g_session1))
            q[i].y = (self->screen_pos.y - d) + (sy - self->screen_pos.y + (sy - self->screen_pos.y));
        else
            q[i].y = d + self->screen_pos.y;
    }

    color = (unsigned)(int)s->color[3];
    color = (color << 8) + (unsigned char)(int)s->color[0];
    color = (color << 8) + (unsigned char)(int)s->color[1];
    color = (color << 8) + (unsigned char)(int)s->color[2];
    for (i = 0; i < 4; i++) {
        q[i].diffuse = color;
        q[i].specular = 0xffffffff;
        q[i].z = 3.102121991105378e-05f;
        q[i].rhw = rhw;
    }

    batch->base.texture = texture;
    batch->base.srcBlend = s->src_blend;
    batch->base.destBlend = s->dest_blend;
    batch->base.stageMode = s->mtpreset;
    renderer_add_prim(g_renderer, &batch->base, 5);
}

void rag_effect_render(struct GameObject *object, const float *view)
{
    struct RagEffect *self = (struct RagEffect *)object;

    int i, depth = 0;

    if (g_session1.effectOn == 0 && self->always_draw == 0)
        return;
    if (!self->str_anim || !self->master)
        return;
    for (i = 1; i < self->str_anim->layer_count; i++) {
        if (self->str_drawn[i]) {
            render_layer(self, &self->str_anim->layers[i], &self->str_state[i], view);
            depth += 2;
        }
    }
}
