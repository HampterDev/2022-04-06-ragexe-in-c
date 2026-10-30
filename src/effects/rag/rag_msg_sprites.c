#include "contents/sound.h"
#include <stdlib.h>
#include "effects/rag/rag_effect.h"

#define FX  "이팩트\\"
#define MON "몬스터\\"
#define MAGIC_CIRCLE FX "Barmund_Magic_Circle\\"

static void sprite(struct EffectPrim *prim, const char *spr, const char *act)
{
    render_object_load_spr_res(&prim->base, spr);
    render_object_load_act_res(&prim->base, act);
}

static void set_action(struct EffectPrim *prim, int action)
{
    render_object_set_action_base(&prim->base, action, 1);
}

static void sprite_action0(struct EffectPrim *prim, const char *spr, const char *act)
{
    sprite(prim, spr, act);
    set_action(prim, 0);
}

static void looping_sprite(struct EffectPrim *prim, float lift, const char *spr, const char *act)
{
    prim->delta2.y = lift;
    prim->list_flags = 1;
    prim->loop_motion = 1;
    sprite_action0(prim, spr, act);
}

static void looping_4(struct EffectPrim *prim, const char *spr, const char *act)
{
    prim->loop_motion = 1;
    prim->motion_period = 4;
    sprite(prim, spr, act);
}

static void sprite_with_wave(struct EffectPrim *prim, const char *spr, const char *act, const char *wave)
{
    prim->motion_period = 4;
    sprite_action0(prim, spr, act);
    sound_play_wave(wave, 0.0f, 0.0f, 0.0f, 250, 40, 1.0f);
}

static void monster_sprite(struct EffectPrim *prim, const char *spr, const char *act, int action_base)
{
    prim->delta2.y = -1.0f;
    prim->list_flags = 5;
    prim->loop_motion = 1;
    prim->motion_period = rag_rand_mod(3) + 8;
    prim->pattern = 0;
    prim->alpha = (float)(rand() % 71 + 70);
    prim->scale = (float)(rand() % 11) * 0.04f + 0.4f;
    prim->r = 0x55;
    prim->g = 0xff;
    prim->b = 0x55;
    sprite(prim, spr, act);
    set_action(prim, rand() % 8 + action_base);
}

static void breeze(struct EffectPrim *prim, int motion_period)
{
    prim->delta2.y = -1.0f;
    prim->list_flags = 5;
    prim->loop_motion = 1;
    prim->motion_period = motion_period;
    prim->pattern = 0;
    prim->alpha = 205.0f;
    prim->scale = 2.5f;
    prim->r = 0x9b;
    prim->g = 0x9b;
    prim->b = 0x9b;
    sprite(prim, MON "breeze.spr", MON "breeze.act");
}

static void magic_circle(struct EffectPrim *prim, const char *spr, const char *act)
{
    prim->loop_motion = 1;
    prim->list_flags = 0xd;
    prim->motion_period = 4;
    sprite(prim, spr, act);
}

static int m_ef02_action(unsigned short deg)
{
    if (deg >= 180 && deg < 270)
        return 2;
    if (deg >= 90 && deg < 180)
        return 3;
    return deg <= 89 ? 0 : 1;
}

static int spread_action(unsigned short deg)
{
    if (deg >= 180 && deg < 270)
        return 4;
    if (deg >= 90 && deg < 180)
        return 6;
    return deg <= 89 ? 0 : 2;
}

void rag_msg_sprite(struct RagEffect *self, char number)
{
    struct EffectPrim *prim;
    int start = number == 30 ? 30 : 0;

    rag_follow_master(self);
    if (self->frame == 99999998)
        self->frame = 0;
    if (self->frame != start)
        return;

    prim = rag_launch_prim(self, 5, 0.0f, 0.0f, 0.0f);
    if (!prim)
        return;
    self->last_prim = prim;
    prim->duration = self->duration;
    prim->delta2.y = 0.0f;
    prim->list_flags |= 0x200;
    prim->pattern |= 0x200;
    prim->motion_period = 2;
    prim->loop_motion = 0;
    prim->alpha_speed = 0.0f;

    switch ((int)number) {
    case 0:
        sprite(prim, FX "캐슬링.spr", FX "캐슬링.act");
        break;
    case 1:
        rag_play_wave_here(self, "effect\\h_blood_lust.wav");
        sprite(prim, FX "블러드러스트.spr",
               FX "블러드러스트.act");
        break;
    case 2:  sprite(prim, FX "item_thunder.spr", FX "item_thunder.act"); break;
    case 3:  sprite(prim, FX "item_cloud.spr", FX "item_cloud.act"); break;
    case 4:  sprite(prim, FX "item_curse.spr", FX "item_curse.act"); break;
    case 5:  sprite(prim, FX "item_zzz.spr", FX "item_zzz.act"); break;
    case 6:  sprite(prim, FX "item_rain.spr", FX "item_rain.act"); break;
    case 7:
        rag_play_wave_here(self, "effect\\ef_firehit.wav");
        prim->motion_period = 3;
        prim->pattern = 0;
        prim->alpha = 220.0f;
        sprite(prim, FX "m_ef01.spr", FX "m_ef01.act");
        break;
    case 8:
        prim->motion_period = 4;
        sprite(prim, FX "m_ef02.spr", FX "m_ef02.act");
        set_action(prim, m_ef02_action(rag_screen_heading(self)));
        break;
    case 9:  prim->motion_period = 4; sprite(prim, FX "m_ef03.spr", FX "m_ef03.act"); break;
    case 10:
        prim->motion_period = 4;
        sprite(prim, FX "m_ef04.spr", FX "m_ef04.act");
        prim->loop_motion = 1;
        break;
    case 11: prim->motion_period = 4; sprite(prim, FX "m_ef05.spr", FX "m_ef05.act"); break;
    case 12: prim->motion_period = 4; sprite(prim, FX "m_ef06.spr", FX "m_ef06.act"); break;
    case 13: prim->motion_period = 4; sprite(prim, FX "m_ef07.spr", FX "m_ef07.act"); break;
    case 14:
        sprite(prim, FX "합격_.spr", FX "합격_.act");
        break;
    case 15:
        prim->motion_period = 4;
        sprite(prim, FX "그림자베기.spr", FX "그림자베기.act");
        set_action(prim, rag_rand_mod(2) == 0 ? 4 : 0);
        break;
    case 16:
        prim->delta2.y = -6.0f;
        prim->list_flags = 1;
        prim->motion_period = 6;
        sprite_action0(prim, FX "다다미.spr", FX "다다미.act");
        break;
    case 17:
        prim->motion_period = 4;
        sprite(prim, FX "안개베기.spr", FX "안개베기.act");
        set_action(prim, rag_rand_mod(2) == 0 ? 2 : 0);
        break;
    case 18:
        prim->delta2.y = -6.0f;
        prim->motion_period = 2;
        sprite_action0(prim, FX "일섬.spr", FX "일섬.act");
        break;
    case 19:
        prim->delta2.y = -1.0f;
        prim->list_flags = 1;
        prim->motion_period = rag_rand_mod(5) + 3;
        prim->loop_motion = 1;
        sprite_action0(prim, FX "화염진.spr", FX "화염진.act");
        break;
    case 20:
        prim->motion_period = 4;
        sprite_action0(prim, FX "데스페라도.spr", FX "데스페라도.act");
        break;
    case 21:
        looping_sprite(prim, -1.0f, FX "라이트닝스피어.spr",
                       FX "라이트닝스피어.act");
        break;
    case 22:
        looping_sprite(prim, -1.0f, FX "블라인드스피어.spr",
                       FX "블라인드스피어.act");
        break;
    case 23:
        looping_sprite(prim, -1.0f, FX "포이즌스피어.spr",
                       FX "포이즌스피어.act");
        break;
    case 24:
        looping_sprite(prim, -1.0f, FX "프리징스피어.spr",
                       FX "프리징스피어.act");
        break;
    case 25:
        looping_sprite(prim, -1.0f, FX "플레어스피어.spr",
                       FX "플레어스피어.act");
        break;
    case 26:
        prim->motion_period = 4;
        sprite_action0(prim, FX "래피드샤워.spr", FX "래피드샤워.act");
        break;
    case 27:
        prim->motion_period = 2;
        sprite_action0(prim, FX "매지컬불릿.spr", FX "매지컬불릿.act");
        break;
    case 28:
        prim->motion_period = 4;
        sprite(prim, FX "스프레드.spr", FX "스프레드.act");
        set_action(prim, spread_action(rag_screen_heading(self)));
        break;
    case 29:
        prim->motion_period = 2;
        sprite_action0(prim, FX "트래킹.spr", FX "트래킹.act");
        break;
    case 30:
        prim->motion_period = 2;
        sprite_action0(prim, FX "트리플액션.spr", FX "트리플액션.act");
        break;
    case 31:
        prim->motion_period = 4;
        sprite_action0(prim, FX "어스퀘이크.spr", FX "어스퀘이크.act");
        break;
    case 32:
        sprite_with_wave(prim, FX "폭죽_러브.spr", FX "폭죽_러브.act",
                         "effect\\itempokjuk.wav");
        break;
    case 33:
        sprite_with_wave(prim, FX "폭죽_화이트데이.spr",
                         FX "폭죽_화이트데이.act", "effect\\itempokjuk.wav");
        break;
    case 34:
        sprite_with_wave(prim, FX "폭죽_발렌타인.spr",
                         FX "폭죽_발렌타인.act", "effect\\itempokjuk.wav");
        break;
    case 35:
        sprite_with_wave(prim, FX "폭죽_생일.spr", FX "폭죽_생일.act",
                         "effect\\itempokjuk.wav");
        break;
    case 36:
        sprite_with_wave(prim, FX "폭죽_크리스마스.spr",
                         FX "폭죽_크리스마스.act", "effect\\itempokjuk.wav");
        break;
    case 37:
        prim->motion_period = 4;
        sprite_action0(prim, FX "mobile_ef04.spr", FX "mobile_ef04.act");
        break;
    case 38:
        sprite_with_wave(prim, FX "mashofa.spr", FX "mashofa.act", "effect\\t_에너지방출.wav");
        break;
    case 39:
        sprite_with_wave(prim, FX "soule.spr", FX "soule.act", "effect\\매직 크래쉬.wav");
        break;
    case 50:
        looping_sprite(prim, -1.0f, FX "기본트랩.spr", FX "기본트랩.act");
        break;
    case 51:
        looping_sprite(prim, -1.0f, FX "담황색트랩.spr", FX "담황색트랩.act");
        break;
    case 52:
        looping_sprite(prim, -1.0f, FX "암청색트랩.spr", FX "암청색트랩.act");
        break;
    case 53:
        looping_sprite(prim, -1.0f, FX "자홍색트랩.spr", FX "자홍색트랩.act");
        break;
    case 54:
        looping_sprite(prim, -1.0f, FX "청록색트랩.spr", FX "청록색트랩.act");
        break;
    case 60:
        prim->motion_period = 4;
        sprite(prim, MON "stapo.spr", MON "stapo.act");
        prim->delta2.y = -8.0f;
        set_action(prim, rand() % 2 == 0 ? 0 : 7);
        break;
    case 61:
        monster_sprite(prim, MON "Ancient_tree.spr", MON "Ancient_tree.act", 0);
        break;
    case 62:
        monster_sprite(prim, MON "ifrit.spr", MON "ifrit.act", 0x20);
        break;
    case 63:
        prim->delta2.y = -12.0f;
        prim->list_flags = 5;
        prim->loop_motion = 1;
        prim->motion_period = 6;
        prim->pattern = 0;
        prim->alpha = 255.0f;
        prim->scale = 0.7f;
        prim->r = 0x9b;
        prim->g = 0xff;
        prim->b = 0x9b;
        sprite(prim, MON "mavka_bullet.spr", MON "mavka_bullet.act");
        set_action(prim, rand() % 8);
        break;
    case 64:
        breeze(prim, 3);
        set_action(prim, 0);
        break;
    case 65:
        breeze(prim, 2);
        set_action(prim, 4);
        break;
    case 70: prim->loop_motion = 1; sprite(prim, FX "거스트.spr", FX "거스트.act"); break;
    case 71: prim->loop_motion = 1; sprite(prim, FX "블래스트.spr", FX "블래스트.act"); break;
    case 72:
        prim->loop_motion = 1;
        sprite(prim, FX "아쿠아플레이.spr", FX "아쿠아플레이.act");
        break;
    case 73: prim->loop_motion = 1; sprite(prim, FX "업히벌.spr", FX "업히벌.act"); break;
    case 74:
        prim->loop_motion = 1;
        sprite(prim, FX "와일드스톰.spr", FX "와일드스톰.act");
        break;
    case 75: prim->loop_motion = 1; sprite(prim, FX "칠리에어.spr", FX "칠리에어.act"); break;
    case 76:
        prim->loop_motion = 1;
        sprite(prim, FX "커즈드소일.spr", FX "커즈드소일.act");
        break;
    case 77: prim->loop_motion = 1; sprite(prim, FX "쿨러.spr", FX "쿨러.act"); break;
    case 78: prim->loop_motion = 1; sprite(prim, FX "트로픽.spr", FX "트로픽.act"); break;
    case 79:
        prim->loop_motion = 1;
        sprite(prim, FX "파이로테크닉.spr", FX "파이로테크닉.act");
        break;
    case 80:
        prim->loop_motion = 1;
        sprite(prim, FX "페트롤로지.spr", FX "페트롤로지.act");
        break;
    case 81: prim->loop_motion = 1; sprite(prim, FX "히터.spr", FX "히터.act"); break;
    case 82: looping_4(prim, FX "silent_brz.spr", FX "silent_brz.act"); break;
    case 83:
        prim->motion_period = 4;
        prim->loop_motion = 0;
        sprite(prim, FX "magma_flow.spr", FX "magma_flow.act");
        break;
    case 84:
        prim->delta2.y = -1.0f;
        prim->list_flags = 1;
        prim->motion_period = 10;
        prim->loop_motion = 1;
        sprite(prim, FX "icemine.spr", FX "icemine.act");
        break;
    case 85:
        prim->delta2.y = -1.0f;
        prim->list_flags = 1;
        prim->motion_period = 10;
        prim->loop_motion = 0;
        sprite(prim, FX "icemine_.spr", FX "icemine_.act");
        set_action(prim, 0);
        break;
    case 86: prim->motion_period = 4; sprite(prim, FX "RK_DRAGONBREATH_WATER.spr", FX "RK_DRAGONBREATH_WATER.act"); break;
    case 87: prim->motion_period = 4; sprite(prim, FX "wideweb.spr", FX "wideweb.act"); break;
    case 88: looping_4(prim, FX "burnt.spr", FX "burnt.act"); break;
    case 90:
        looping_sprite(prim, -1.0f, FX "그라운드드리프트.spr",
                       FX "그라운드드리프트.act");
        break;
    case 91:
        prim->delta2.y = -4.0f;
        looping_4(prim, FX "나뭇잎_완성.spr", FX "나뭇잎_완성.act");
        break;
    case 94:
        prim->delta2.y = -4.0f;
        looping_4(prim, FX "Black_Thunder\\Black_Thunder.spr", FX "Black_Thunder\\Black_Thunder.act");
        break;
    case 95:
        prim->delta2.y = -4.0f;
        looping_4(prim, FX "Black_Thunder\\Black_Thunder_Dark.spr", FX "Black_Thunder\\Black_Thunder_Dark.act");
        break;
    case 96:  magic_circle(prim, MAGIC_CIRCLE "Angel_Magic_Circle.spr", MAGIC_CIRCLE "Angel_Magic_Circle.act"); break;
    case 97:  magic_circle(prim, MAGIC_CIRCLE "Animal_Magic_Circle.spr", MAGIC_CIRCLE "Animal_Magic_Circle.act"); break;
    case 98:  magic_circle(prim, MAGIC_CIRCLE "Devil_Magic_Circle.spr", MAGIC_CIRCLE "Devil_Magic_Circle.act"); break;
    case 99:  magic_circle(prim, MAGIC_CIRCLE "Dragon_Magic_Circle.spr", MAGIC_CIRCLE "Dragon_Magic_Circle.act"); break;
    case 100: magic_circle(prim, MAGIC_CIRCLE "Fish_Magic_Circle.spr", MAGIC_CIRCLE "Fish_Magic_Circle.act"); break;
    case 101: magic_circle(prim, MAGIC_CIRCLE "Human_Magic_Circle.spr", MAGIC_CIRCLE "Human_Magic_Circle.act"); break;
    case 102: magic_circle(prim, MAGIC_CIRCLE "Insect_Magic_Circle.spr", MAGIC_CIRCLE "Insect_Magic_Circle.act"); break;
    case 103: magic_circle(prim, MAGIC_CIRCLE "Nothing_Magic_Circle.spr", MAGIC_CIRCLE "Nothing_Magic_Circle.act"); break;
    case 104: magic_circle(prim, MAGIC_CIRCLE "Plant_Magic_Circle.spr", MAGIC_CIRCLE "Plant_Magic_Circle.act"); break;
    case 105: magic_circle(prim, MAGIC_CIRCLE "Undead_Magic_Circle.spr", MAGIC_CIRCLE "Undead_Magic_Circle.act"); break;
    default:
        break;
    }
    prim_sync_particles(self->last_prim);
}
