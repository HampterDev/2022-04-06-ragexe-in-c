#include "contents/client_lua_calls.h"
#include "session/csession.h"
#include "session/session_talk.h"

#include <string.h>

static int pet_level(int value)
{
    if ((unsigned)(value - 91) <= 9)
        return 4;
    if ((unsigned)(value - 76) <= 14)
        return 3;
    if ((unsigned)(value - 26) <= 49)
        return 2;
    return (unsigned)(value - 11) <= 14;
}

int session_pet_hunger_level(int fullness)
{
    return pet_level(fullness);
}

int session_pet_intimacy_level(int intimacy)
{
    int value = 0;

    if (intimacy)
        value = intimacy / 10;
    return pet_level(value);
}

int session_get_pet_emotion(struct Session *self, int hungerLevel, int intimacyLevel,
                                     int action)
{
    int emotion = self->petEmotion[hungerLevel][intimacyLevel][action];

    if (emotion == -1)
        return -1;
    return emotion * 10 + 2;
}

int session_make_pet_talk_id(int job, int action, int hunger)
{
    g_session1.sexCopy = g_session1.sex;
    return g_session1.sexCopy + 10 * (hunger + 10 * (action + 10 * job));
}

int session_get_sex(struct Session *self)
{
    if (self->sexCopy == self->sex)
        return self->sexCopy;
    self->sexCopy = self->sex;
    return self->sex;
}

void session_set_sex(struct Session *self, int sex)
{
    self->sexCopy = sex & 1;
    self->sex = sex & 1;
}

const char *session_get_pet_act_text(int action)
{
    switch (action) {
    case 0:  return "feeding";
    case 1:  return "hunting";
    case 2:  return "danger";
    case 3:  return "dead";
    case 4:  return "stand";
    case 5:  return "perfor_s";
    case 6:  return "levelup";
    case 7:  return "perfor_1";
    case 8:  return "perfor_2";
    case 9:  return "perfor_3";
    case 10: return "connect";
    default: return "normal";
    }
}

void session_get_pet_intimacy_text(int intimacy, char *out)
{
    if (intimacy >= 0 && intimacy <= 100)
        strcpy(out, "서먹서먹함");
    else if (intimacy > 100 && intimacy <= 250)
        strcpy(out, "어색함");
    else if (intimacy > 250 && intimacy <= 750)
        strcpy(out, "보통");
    else if (intimacy > 750 && intimacy <= 900)
        strcpy(out, "친함");
    else if (intimacy > 900 && intimacy <= 1000)
        strcpy(out, "절친함");
    else
        strcpy(out, "저는 모릅니다아아아.");
}

void session_get_pet_hunger_text(int fullness, char *out)
{
    if (fullness >= 0 && fullness <= 10)
        strcpy(out, "배고픔");
    else if (fullness > 10 && fullness <= 25)
        strcpy(out, "출출함");
    else if (fullness > 25 && fullness <= 75)
        strcpy(out, "보통");
    else if (fullness > 75 && fullness <= 90)
        strcpy(out, "배부름");
    else if (fullness > 90 && fullness <= 100)
        strcpy(out, "아주 배부름");
    else
        strcpy(out, "몰라요.");
}

int session_get_pet_food(struct Session *self, int petJob)
{
    int food = 0;

    if (!client_lua_call(self->luaState, "GetPetFood", "d>d", petJob, &food))
        return -1;
    return food;
}

int session_get_pet_job_by_egg(struct Session *self, int eggItemId)
{
    int job = 0;

    if (!client_lua_call(self->luaState, "GetPetJTID_by_PetEggITID", "d>d", eggItemId, &job))
        return -1;
    return job;
}

const char *session_get_pet_string(struct Session *self, int petJob)
{
    const char *text = NULL;

    if (!client_lua_call(self->luaState, "GetPetString", "d>s", petJob, &text))
        return NULL;
    return text;
}

void session_get_pet_illust_name(struct Session *self, int petJob, char *out, size_t outSize)
{
    static const char fallback[] = "UserInterface\\illust\\PET_NOIMAGE.bmp";
    const char *name = NULL;
    size_t length;

    if (outSize == 0)
        return;
    if (!client_lua_call(self->luaState, "GetPetIllustName_V3", "db>s", petJob, 1, &name)
        || name == NULL)
        name = fallback;
    length = strlen(name);
    if (length >= outSize) {
        name = fallback;
        length = sizeof fallback - 1;
    }
    if (length >= outSize) {
        out[0] = '\0';
        return;
    }
    memcpy(out, name, length + 1);
}

const char *session_get_pet_accessory_act(struct Session *self, int accessory)
{
    const char *name = NULL;

    if (client_lua_call(self->luaState, "GetPetAccActName", "d>s", accessory, &name)
        && res_mgr_is_available(g_resMgr(), name))
        return name;
    return "몬스터\\chocho_방독면.act";
}

const char *session_get_pet_name(int petJob)
{
    switch (petJob) {
    case 0x3ed: return "FARMILIAR";
    case 0x3f2: return "wilow";
    case 0x3f3: return "chonchon";
    case 0x3f6: return "spore";
    case 0x3fb: return "pecopeco";
    case 0x3ff: return "ork_warrior";
    case 0x402: return "munak";
    case 0x405: return "isis";
    case 0x407: return "poporing";
    case 0x40b: return "hunter_fly";
    case 0x410: return "GOLEM";
    case 0x411: return "mummy";
    case 0x412: return "steel_chonchon";
    case 0x419: return "picky";
    case 0x41c: return "rocker";
    case 0x420: return "smokie";
    case 0x421: return "yoyo";
    case 0x422: return "metaller";
    case 0x427: return "lunatic";
    case 0x435: return "poison_spore";
    case 0x442: return "mastering";
    case 0x448: return "angeling";
    case 0x44d: return "baphomet_j";
    case 0x452: return "DESERT_WOLF";
    case 0x453: return "desert_wolf_babe";
    case 0x455: return "deviruchi";
    case 0x456: return "dokebi";
    case 0x459: return "drops";
    case 0x462: return "goblin_1";
    case 0x463: return "goblin_2";
    case 0x465: return "goblin_4";
    case 0x477: return "MARIONETTE";
    case 0x47c: return "MEDUSA";
    case 0x47e: return "moonlight";
    case 0x483: return "petit";
    case 0x487: return "phreeoni";
    case 0x48e: return "savage";
    case 0x48f: return "savage_babe";
    case 0x492: return "sohee";
    case 0x49b: return "WHISPER";
    case 0x49c: return "nine_tail";
    case 0x4a4: return "bon_gun";
    case 0x4ae: return "DARK_PRIEST";
    case 0x4b0: return "zherlthsh";
    case 0x4b8: return "wander_man";
    case 0x4bd: return "high_orc";
    case 0x4be: return "choco";
    case 0x4da: return "MARIN";
    case 0x4dd: return "gobline_xmas";
    case 0x4fb: return "alice";
    case 0x511: return "ancient_mummy";
    case 0x513: return "GOBLIN_LEADER";
    case 0x515: return "am_mut";
    case 0x51b: return "cat_o_nine_tail";
    case 0x559: return "grand_peco";
    case 0x55a: return "SUCCUBUS";
    case 0x55e: return "INCUBUS";
    case 0x563: return "NIGHTMARE_TERROR";
    case 0x566: return "diabolic";
    case 0x569: return "deleter_";
    case 0x579: return "SHINOBI";
    case 0x57c: return "MIYABI_NINGYO";
    case 0x588: return "WICKED_NYMPH";
    case 0x5d7: return "STONE_SHOOTER";
    case 0x5e0: return "DULLAHAN";
    case 0x5e1: return "LOLI_RURI";
    case 0x5e8: return "hyegun";
    case 0x5e9: return "CIVIL_SERVANT";
    case 0x5ef: return "CHUNG_E";
    case 0x632: return "LEAF_CAT";
    case 0x656: return "teddy_bear";
    case 0x65e: return "BACSOJIN_";
    case 0x65f: return "CHUNG_E";
    case 0x660: return "gremlin";
    case 0x6c7: return "ALICEL";
    case 0x6c8: return "ALIOT";
    case 0x6c9: return "ALIZA";
    case 0x6ed: return "hodremlin";
    case 0x6f6: return "roween";
    case 0x717: return "event_ricecake";
    case 0x72d: return "IMP";
    case 0x757: return "ECLIPSE_P";
    case 0x766: return "pouring";
    case 0x7ab: return "P_CHUNG_E";
    case 0x898: return "J_TAINI";
    case 0x8a2: return "XMAS_LUNATIC";
    case 0x909: return "TIKBALANG";
    case 0x920: return "DOMOVOI";
    case 0x95e: return "LITTLE_PORING";
    case 0xbb3: return "xm_teddy_bear";
    case 0xbcf: return "FIRE_GOLEM";
    case 0xcb9: return "male_poring";
    case 0xcba: return "female_poring";
    case 0xcbd: return "BLUE_UNICORN";
    case 0xda7: return "dr_eggring";
    case 0xda8: return "dr_lunatic";
    case 0xe34: return "LITTLE_ISIS";
    case 0xe55: return "diabolic2";
    case 0xe56: return "deleter_2";
    case 0xe93: return "scatleton";
    case 0xece: return "sweets_drops";
    case 0xf83: return "skelion";
    case 0x4f72: return "nightmare_terror_mj";
    case 0x4f95: return "nightmare_terror_mj";
    case 0x4fc4: return "wander_man_mj";
    case 0x4fc7: return "bacsojin2";
    case 0x4fc8: return "moonlight";
    case 0x4fc9: return "phreeoni";
    case 0x502d: return "ILL_BAPHOMET_J";
    case 0x505b: return "ORK_HERO2";
    case 0x508b: return "gloomundernight2";
    case 0x50d8: return "child_admin_b";
    case 0x50d9: return "child_admin_a";
    case 0x5261: return "WANDERING_DUCK";
    case 0x532a: return "pet_kiel";
    case 0x532b: return "capoo_tw";
    case 0x53fe: return "BAPHOMET2";
    case 0x547e: return "WILD_ROSE3";
    case 0x547f: return "BLUEMOON_LOLI_RURI2";
    case 0x5480: return "FARMILIAR2";
    case 0x59d8: return "DARK_ILLUSION2";
    default:     return "poring";
    }
}
