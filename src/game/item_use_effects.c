#include "session/state.h"
#include "game/skill_use.h"
#include "actor/job.h"
#include "core/msg_str.h"
#include "session/session_items.h"

enum {
    EF_POTION_RED = 0xcc,
    EF_POTION_ORANGE = 0xcd,
    EF_POTION_YELLOW = 0xce,
    EF_POTION_WHITE = 0xcf,
    EF_POTION_BLUE = 0xd0,
    EF_POTION_GREEN = 0xd1,
    EF_FOOD_STR = 0x251,
    EF_FOOD_INT = 0x252,
    EF_FOOD_VIT = 0x253,
    EF_FOOD_AGI = 0x254,
    EF_FOOD_DEX = 0x255,
    EF_FOOD_LUK = 0x256,
};

#define ITEM_JOB_MESSAGE 0x3113
#define CHAT_LINE 1
#define CHAT_LINE_CYAN 0xffff00

static const char WAVE_HEAL[] = "_heal_effect.wav";
static const char WAVE_ITEM_POKJUK[] = "effect\\itempokjuk.wav";

static void show_item_effect(struct GameActor *actor, int effect_id)
{
    actor_remove_all_effects(actor, effect_id);
    if (effect_mgr_is_registered(g_effectMgr, effect_id))
        spawn_on(actor->aid, effect_id);
    else
        actor_launch_effect(actor, effect_id);
}

static int stat_dish_effect(int item_id)
{
    if ((item_id >= 0x2f09 && item_id <= 0x2f0d) || (item_id >= 0x2f27 && item_id <= 0x2f2b) || item_id == 0x2faa)
        return EF_FOOD_STR;
    if ((item_id >= 0x2f0e && item_id <= 0x2f12) || (item_id >= 0x2f2c && item_id <= 0x2f30) || item_id == 0x2fac)
        return EF_FOOD_INT;
    if ((item_id >= 0x2f13 && item_id <= 0x2f17) || (item_id >= 0x2f31 && item_id <= 0x2f35) || item_id == 0x2faf)
        return EF_FOOD_VIT;
    if ((item_id >= 0x2f18 && item_id <= 0x2f1c) || (item_id >= 0x2f36 && item_id <= 0x2f3a) || item_id == 0x2fab)
        return EF_FOOD_AGI;
    if ((item_id >= 0x2f1d && item_id <= 0x2f21) || (item_id >= 0x2f3b && item_id <= 0x2f3f) || item_id == 0x2fad)
        return EF_FOOD_DEX;
    if ((item_id >= 0x2f22 && item_id <= 0x2f26) || (item_id >= 0x2f40 && item_id <= 0x2f44) || item_id == 0x2fae)
        return EF_FOOD_LUK;
    return 0;
}

static int item_effect(struct GameActor *actor, int item_id, int *second, int *final)
{
    *second = -1;
    *final = 0;
    switch (item_id) {
    case 501: case 507: case 545: case 569: case 578: case 598:
    case 11567: case 11568: case 11570: case 0x3909:
        return EF_POTION_RED;
    case 502: case 579: case 599: case 11569: case 0x311e:
        return EF_POTION_ORANGE;
    case 503: case 508: case 546: case 11566: case 11574: case 0x2cec:
        return EF_POTION_YELLOW;
    case 504: case 509: case 547: case 555: case 556: case 557:
    case 11501: case 11503: case 11548: case 11557: case 11558: case 11565: case 11573:
        return EF_POTION_WHITE;
    case 505: case 510: case 11502: case 11504: case 11549: case 11572:
        return EF_POTION_BLUE;
    case 506: case 511: case 11571:
        return EF_POTION_GREEN;
    case 512: case 513: case 515: case 516: case 531: case 532: case 534: case 535:
    case 548: case 549: case 550: case 574: case 582: case 682: case 12342: case 12344:
        return 0xd2;
    case 514: case 533: case 568: case 607: case 680: case 683:
    case 12333: case 12343: case 12345: case 12353:
        return 0xd3;
    case 517: case 518: case 519: case 520: case 521: case 526: case 528: case 529: case 530:
    case 536: case 537: case 538: case 539: case 540: case 541: case 542: case 543: case 544:
    case 551: case 552: case 553: case 562: case 563: case 564: case 565: case 566: case 567:
    case 570: case 571: case 572: case 575: case 576: case 577: case 580: case 581:
    case 583: case 584: case 585: case 586: case 663:
    case 11547: case 11575: case 11576: case 11577: case 11578: case 11579: case 11580:
    case 11581: case 11582:
        return 7;
    case 522:
        play_skill_wave(WAVE_HEAL);
        return -1;
    case 523:
        play_skill_wave("effect\\EF_Blessing.wav");
        return -1;
    case 554: case 12122: case 12123: case 12124: case 12195: case 12196: case 12197: case 12198:
    case 12238: case 12239: case 0x5817: case 0x5818: case 0x5819:
        play_skill_wave(WAVE_HEAL);
        return 0x1eb;
    case 558: case 561: case 11584: case 11587: case 12237:
        return 0x16c;
    case 559: case 560: case 11585: case 11586: case 11701:
        return 0x16b;
    case 573: case 596: case 597: case 11583: case 12234:
        return 0x1fd;
    case 645: case 0x38ad: case 0x580e:
        return 0xda;
    case 656: case 0x38ae: case 0x5810:
        return 0xdb;
    case 657: case 0x38af: case 0x580f:
        return 0xdc;
    case 662: case 12262:
        play_skill_wave("effect\\mapae.wav");
        return 0x1fb;
    case 684:
        actor_launch_effect(actor, 0x105);
        return 0xdc;
    case 685:
        return 0x1ec;
    case 12016: case 0x5811:
        play_skill_wave("effect\\fast.wav");
        return 0x207;
    case 12017: case 0x5812:
        return 0x26;
    case 12018:
        play_skill_wave(WAVE_ITEM_POKJUK);
        return 0x1fc;
    case 12023:
        play_skill_wave("effect\\ro2effect.wav");
        return 0x221;
    case 12027: case 0x588d:
        play_skill_wave("wraith_die.wav");
        return -1;
    case 12028: return 0x240;
    case 12029: return 0x241;
    case 12030: return 0x242;
    case 12031: return 0x243;
    case 12032: return 0x244;
    case 12033: return 0x245;
    case 12034:
        play_skill_wave("ork_warrior_breath.wav");
        return -1;
    case 12118: play_skill_wave("evil_cloud_hermit_attack.wav"); return 0x25b;
    case 12119: play_skill_wave("evil_cloud_hermit_attack.wav"); return 0x33;
    case 12120: play_skill_wave("evil_cloud_hermit_attack.wav"); return 0x93;
    case 12121: play_skill_wave("evil_cloud_hermit_attack.wav"); return 0x34;
    case 12131:
        return 0x264;
    case 12218:
        actor_launch_effect(actor, 0x177);
        actor_launch_effect(actor, 0x1b8);
        return -1;
    case 12250: return EF_FOOD_STR;
    case 12251: return EF_FOOD_AGI;
    case 12252: return EF_FOOD_INT;
    case 12253: return EF_FOOD_DEX;
    case 12254: return EF_FOOD_LUK;
    case 12255: return EF_FOOD_VIT;
    case 0x3016: case 0x3017:
        return 0x2bb;
    case 12315: return 0x152;
    case 12316: return 0x2c1;
    case 12317:
        play_skill_wave("effect\\storm.wav");
        return 0x2c2;
    case 12318: return 0x2c3;
    case 12326:
        play_skill_wave("effect\\pokjuk_jap.wav");
        return 0x2c5;
    case 0x38d3: return 0x2ab;
    case 0x38d4: return 0x2ac;
    case 0x38d5: return 0x2ad;
    case 0x38d6: return 0x2ae;
    case 0x18733: case 0x18734: case 0x18735:
        *second = 0x63e;
        return 0x63d;
    case 0x3198:
        play_skill_wave(WAVE_ITEM_POKJUK);
        *final = 1;
        return 0x3db;
    case 0x3199:
        play_skill_wave(WAVE_ITEM_POKJUK);
        *final = 1;
        return 0x3dc;
    case 0x31f4:
        play_skill_wave(WAVE_ITEM_POKJUK);
        *final = 1;
        return 0x3ee;
    case 0x3280:
        *final = 1;
        return 0x56;
    case 0x38d2:
        *final = 1;
        return 0x2aa;
    case 0x18736:
        *second = 0x63c;
        *final = 1;
        return 0x63b;
    }
    return -1;
}

static struct GameActor *item_user(struct GameMode *mode, int item_id, unsigned aid)
{
    struct GameActor *player;
    char line[256];

    if (aid != (unsigned)g_session1.ownAid)
        return world_get_game_actor_by_aid(mode->world, aid);
    player = (struct GameActor *)mode->world->player;
    if (player != 0 && item_id == ITEM_JOB_MESSAGE) {
        sprintf(line, msg_str((unsigned)(game_actor_get_job(player) - 4054) <= 0x3a ? 0x704 : 0x703));
        ui_window_mgr_send_msg_(&g_windowMgr, CHAT_LINE, (int)line, CHAT_LINE_CYAN, 7, 0);
    }
    return player;
}

void game_mode_show_item_use_effect(struct GameMode *self, int item_id, unsigned aid)
{
    struct ItemInfo item;
    struct GameActor *actor;
    int effect, second, final;

    session_get_inventory_item_by_id(&g_session1, &item, item_id, 0);
    actor = item_user(self, item_id, aid);
    if (actor != 0) {
        effect = item_effect(actor, item_id, &second, &final);
        if (!final && stat_dish_effect(item_id) != 0)
            effect = stat_dish_effect(item_id);
        if (effect > 0)
            show_item_effect(actor, effect);
        if (second > 0)
            show_item_effect(actor, second);
    }
    item_info_destruct(&item);
}
