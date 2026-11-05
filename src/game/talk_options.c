#include "contents/frame_timer.h"
#include "contents/sound.h"
#include "session/state.h"
#include "session/session_talk.h"
#include "ui/window_access.h"
#include <windows.h>
#include "game/talk_type.h"
#include "game/client.h"
#include "core/msg_str.h"
#include "ui/render/graphic_setting.h"

int g_isFixedCamera;

static int talk_type_of(const char *text)
{
    struct String args[3] = {0};
    int type = TALK_UNKNOWN, i;

    session_get_talk_type(&g_session1, text, &type, args);
    for (i = 3; i-- > 0;)
        string_free(&args[i]);
    return type;
}

void talk_print(const char *text, unsigned color)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_CHAT_PRINT, (int)text, color, 0, 0);
}

void talk_print_msg(int msg_id, unsigned color)
{
    talk_print(msg_str(msg_id), color);
}

void talk_refresh_option_window(void)
{
    struct UIWindow *w = ui_window_find(0x131);

    if (w)
        ui_window_send(w, NULL, 0x17, 0, 0, 0, 0);
}

enum {
    TOGGLE_FLIP = 0,
    TOGGLE_NOT = 1,
    TOGGLE_QUIET = 2,
    TOGGLE_REFRESH = 4,
};

struct OptionToggle {
    unsigned char type;
    unsigned char how;
    short on_msg, off_msg;
    int *value;
};

static const struct OptionToggle g_toggles[] = {
    { 0x35, TOGGLE_FLIP, 0x13d, 0x13e, &g_session1.makeMissEffect },
    { 0x67, TOGGLE_FLIP | TOGGLE_REFRESH, 0x2bd, 0x2be, &g_session1.noShift },
    { 0x6b, TOGGLE_FLIP | TOGGLE_REFRESH, 0x2cd, 0x2ce, &g_session1.noCtrl },
    { 0x8b, TOGGLE_FLIP, 0x352, 0x353, &g_session1.windowSnap },
    { 0x6f, TOGGLE_FLIP, 0x2da, 0x2db, &g_session1.showSkillFail },
    { 0xa3, TOGGLE_FLIP, 0x414, 0x415, &g_session1.showLoginNotices },
    { 0xa5, TOGGLE_FLIP, 0x454, 0x455, &g_session1.shopOneClick },
    { 0x76, TOGGLE_FLIP, 0x2fd, 0x2fe, &g_session1.quickSpellRightClick },
    { 0x77, TOGGLE_FLIP, 0x300, 0x301, &g_session1.quickSpellWheel },
    { 0xb8, TOGGLE_FLIP, 0x555, 0x556, &g_session1.battleChat },
    { 0x70, TOGGLE_FLIP, 0x2de, 0x2dd, &g_session1.hideChat },
    { 0x92, TOGGLE_FLIP, 0x38d, 0x38c, &g_session1.hideChatAndGuild },
    { 0x86, TOGGLE_FLIP, 0x325, 0x326, &g_session1.refuseTrades },
    { 0xa2, TOGGLE_NOT, 0x3ff, 0x400, &g_session1.hoUserAI },
    { 0xac, TOGGLE_NOT, 0x4f9, 0x4fa, &g_session1.merc_command[4] },
    { 0x36, TOGGLE_FLIP, 0x13f, 0x140, &g_isFixedCamera },
    { 0x2e, TOGGLE_FLIP | TOGGLE_REFRESH, 0x10f, 0x110, &g_session1.monsterSnapOnNoSkill },
    { 0x2f, TOGGLE_FLIP | TOGGLE_REFRESH, 0x114, 0x115, &g_session1.itemSnap },
    { 0x2d, TOGGLE_FLIP | TOGGLE_REFRESH, 0x111, 0x112, &g_session1.monsterSnapOnSkill },
    { 0x0a, TOGGLE_FLIP, 0x1d, 0x1e, &g_frameskip },
    { 0xd3, TOGGLE_NOT, 0x884, 0x885, &g_session1.showMonsterHp },
    { 0xf1, TOGGLE_NOT, 0xdc7, 0xdc8, &g_session1.emblemFrame },
    { 0xdb, TOGGLE_NOT | TOGGLE_QUIET, 0xaf5, 0xaf6, &g_session1.screenShake },
    { 0xee, TOGGLE_NOT | TOGGLE_QUIET, 0xd7b, 0xd7c, &g_session1.showSpiritMarbles },
};

void talk_option_toggle(int type)
{
    const struct OptionToggle *t;

    for (t = g_toggles; t < g_toggles + sizeof g_toggles / sizeof g_toggles[0]; t++) {
        if (t->type != type)
            continue;
        *t->value = (t->how & TOGGLE_NOT) ? *t->value == 0 : 1 - *t->value;
        if (!(t->how & TOGGLE_QUIET))
            ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
        talk_print_msg(*t->value ? t->on_msg : t->off_msg, TALK_COLOR_NOTICE);
        if (t->how & TOGGLE_REFRESH)
            talk_refresh_option_window();
        return;
    }
}

struct TalkOption talk_option_find(const char *text)
{
    int type = TALK_UNKNOWN;
    int *value = NULL;
    unsigned char *byte = NULL;
    struct TalkOption option;

    if (text && *text) {
        type = talk_type_of(text);
        switch (type) {
        case 0x86: value = &g_session1.refuseTrades; break;
        case 0x67: value = &g_session1.noShift; break;
        case 0x6b: value = &g_session1.noCtrl; break;
        case 0x6f: value = &g_session1.showSkillFail; break;
        case 0x70: value = &g_session1.hideChat; break;
        case 0x92: value = &g_session1.hideChatAndGuild; break;
        case 0x6a: byte = &g_session1.showName; break;
        case 0x3c: value = &g_session1.fogOn; break;
        case 0x68: value = &g_session1.auraFull; break;
        case 0x69: value = &g_session1.auraOff; break;
        case 0x8b: value = &g_session1.windowSnap; break;
        case 0x35: value = &g_session1.makeMissEffect; break;
        case 0x76: value = &g_session1.quickSpellRightClick; break;
        case 0x77: value = &g_session1.quickSpellWheel; break;
        case 0x08: value = &g_session1.effectOn; break;
        case 0x07: value = &g_sound_settings.bgm_on; break;
        case 0x0b: value = &g_sound_settings.effects_on; break;
        case 0xc7: value = &g_session1.showStateInfo; break;
        case 0xa3: value = &g_session1.showLoginNotices; break;
        case 0xa5: value = &g_session1.shopOneClick; break;
        case 0x2f: value = &g_session1.itemSnap; break;
        case 0xa2: value = &g_session1.hoUserAI; break;
        case 0xac: value = &g_session1.merc_command[4]; break;
        case 0x2d: value = &g_session1.monsterSnapOnSkill; break;
        case 0x2e: value = &g_session1.monsterSnapOnNoSkill; break;
        case 0x36: value = &g_isFixedCamera; break;
        case 0x75: value = &g_lightmap; break;
        case 0xc3: byte = &g_session1.showBattleFieldIcon; break;
        case 0xd3: value = &g_session1.showMonsterHp; break;
        case 0xd8: value = &g_session1.showCashShopIcon; break;
        case 0xd9: value = &g_session1.showGoldPcIcon; break;
        case 0xda: value = &g_session1.showRouletteIcon; break;
        case 0xdb: value = &g_session1.screenShake; break;
        case 0xdd: value = &g_session1.minimapButtons; break;
        case TALK_SHOW_NAME: value = &g_session1.map_rule_flags[0]; break;
        case 0xee: value = &g_session1.showSpiritMarbles; break;
        case 0xef: value = &g_session1.zoomFarLimit; break;
        }
    }
    option.type = type;
    option.integer = value;
    option.byte = byte;
    return option;
}

int talk_option_set(const char *text, int value)
{
    struct TalkOption option = talk_option_find(text);

    if (option.type == TALK_UNKNOWN || !text || !*text)
        return 0;
    if (!talk_option_settings_add(&g_session1.talkOptionSettings, text_view_cstr(text), option, value))
        return 0;
    /* The client stores every option as an int, the one-byte name option too, so the int's second byte
     * lands on /loading's flag right after it: the name option restored at start-up leaves the loading
     * screens taking turns rather than coming at random. */
    if (option.byte == &g_session1.showName)
        g_session1.loadingScreenRandom = (BYTE)((unsigned)value >> 8);
    if (option.type == TALK_SHOW_NAME)
        g_session1.map_rule_flags[1] = value;
    return 1;
}
