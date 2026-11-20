#include "contents/client_lua_calls.h"
#include "contents/sound.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "session/csession.h"
#include "session/session_options.h"

#include <string.h>
#include "core/window_state.h"
#include "ui/render/graphic_setting.h"
#include "render/texture_cache.h"
#include "game/camera.h"
#include "render/display_options.h"

static int option_int(void *session, const char *name, int fallback)
{
    return session_get_option_int(session, name, fallback);
}

void session_load_options(struct Session *self)
{
    struct String text, *skin;
    struct DisplayOptions display;
    int effectVolume;

    client_lua_call(g_session1.luaState, "SetCmdOnOffList", "");
    file_make_file_name("OptionInfo.lua", 1);
    g_trilinearFilter = session_get_option_int(self, "Trilinear", 0);
    effectVolume = session_get_option_int(self, "Effect_Volume", 100);
    sound_set_bgm_volume(session_get_option_int(self, "Bgm_Volume", 100));
    sound_set_bgm_fade_target(sound_bgm_volume());
    sound_set_effect_volume(effectVolume);

    self->showWhisperWnd = session_get_option_int(self, "AutoOpen1to1Window", 1);
    self->showWhisperWndFriend = session_get_option_int(self, "AutoOpen1to1Window_Friend", 1);
    self->playWhisperOpenSound = session_get_option_int(self, "PlaySound_Open1to1Window", 1);
    self->simpleSkillWnd = session_get_option_int(self, "Simplicity_SkillList", 0);
    self->showSkillDescript = session_get_option_int(self, "Show_SkillDescript", 0);
    g_windowMgr.chat_bar_toggleable = session_get_option_int(self, "ChangeChatMode", 1) != 0;
    self->lockMouse = session_get_option_int(self, "LockMouse", 0) != 0;
    self->channelCopId = session_get_option_int(self, "ChannelCopID", 0);

    g_freeViewPitch = session_get_option_float(self, "Outdoor_ViewLatitude", -45.0f);
    g_freeViewDistance = session_get_option_float(self, "Outdoor_ViewDistance", 300.0f);
    g_fixedViewPitch = session_get_option_float(self, "Indoor_ViewLatitude", -45.0f);
    g_fixedViewDistance = session_get_option_float(self, "Indoor_ViewDistance", 300.0f);
    if (VIEW_PITCH_STEEPEST > g_fixedViewPitch)
        g_fixedViewPitch = VIEW_PITCH_STEEPEST;
    else if (g_fixedViewPitch > VIEW_PITCH_FLATTEST)
        g_fixedViewPitch = VIEW_PITCH_FLATTEST;
    if (g_viewDistanceMin > g_fixedViewDistance)
        g_fixedViewDistance = g_viewDistanceMin;
    if (g_fixedViewDistance > VIEW_DISTANCE_LIMIT)
        g_fixedViewDistance = VIEW_DISTANCE_LIMIT;

    self->zoomFarLimit = session_get_option_int(self, msg_str(0xdb9), 0);
    if (VIEW_PITCH_STEEPEST > g_freeViewPitch)
        g_freeViewPitch = VIEW_PITCH_STEEPEST;
    else if (g_freeViewPitch > VIEW_PITCH_FLATTEST)
        g_freeViewPitch = VIEW_PITCH_FLATTEST;
    if (g_viewDistanceMin > g_freeViewDistance)
        g_freeViewDistance = g_viewDistanceMin;
    if (g_freeViewDistance > VIEW_DISTANCE_LIMIT)
        g_freeViewDistance = VIEW_DISTANCE_LIMIT;

    skin = session_get_option_string(self, &text, "SkinName", "<Basic Skin>");
    if (skin != &g_skinMgr.name) {
        string_free(&g_skinMgr.name);
        string_move(&g_skinMgr.name, skin);
    }
    string_free(&text);
    self->monsterSnapInEffect = 0;
    if (self->monsterSnapOnNoSkill != 0)
        self->monsterSnapInEffect = 1;

    g_session1.mouseExclusiveOption = session_get_option_int(self, "MouseExclusive", 1) != 0;
    /* custom: windowed or fullscreen, and the windowed size (render/display_options.h) */
    display_options_read(&display, option_int, self);
    g_fullscreenMode = display.fullscreen;
    g_windowMaximized = display.maximized;
    g_displayWidth = display.width;
    g_displayHeight = display.height;
    g_windowX = display.x;
    g_windowY = display.y;
    g_savedWidth = display.placedWidth;
    g_savedHeight = display.placedHeight;
    g_spriteMode = session_get_option_int(self, "SPRITEMODE", 2);
    switch (g_spriteMode) {
    case 0:
        g_sprHalfWidth = 1;
    case 1:
        g_sprHalfHeight = 1;
        break;
    default:
        g_sprHalfWidth = 0;
        g_sprHalfHeight = 0;
        break;
    }
    g_textureMode = session_get_option_int(self, "TEXTUREMODE", 2);
    switch (g_textureMode) {
    case 0:
        g_textureDivisor = 4;
        break;
    case 1:
        g_textureDivisor = 2;
        break;
    default:
        g_textureDivisor = 1;
        break;
    }

    self->itemDropLock = session_get_option_int(self, "bLockItemDropFromItemWnd", 0) != 0;
    self->noPriceCheckWarning = session_get_option_int(self, "bWarningPriceCheck", 0) == 0;
    self->noSellWndSound = session_get_option_int(self, "bSoundEffectInItemSellWnd", 0) == 0;
    self->guildMemberSort = session_get_option_int(self, "bGuildMemberListSort", 1) != 0;
    self->blockPrivateTab = session_get_option_int(self, "bBlockPrivateTab", 0) != 0;
    self->itemSort[0] = session_get_option_int(self, "bItemDefaultSort", 0) != 0;
    self->itemSort[1] = session_get_option_int(self, "bItemAscendingOrder", 0) != 0;
    self->itemSort[2] = session_get_option_int(self, "bItemDescendingOrder", 0) != 0;
    self->showMenuIcon = session_get_option_int(self, "bShowMenuIcon", 1) != 0;
    self->emblemFrame = session_get_option_int(self, msg_str(0xdc6), 0);
    self->extendDamage = session_get_option_int(self, "bExtendDamage", 1);
}
