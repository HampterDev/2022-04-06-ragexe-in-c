#include "contents/client_lua_calls.h"
#include "contents/sound.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "session/csession.h"
#include "session/session_options.h"

#include <string.h>
#include "core/app.h"
#include "ui/render/graphic_setting.h"
#include "game/mouse_camera.h"
#include "render/texture_cache.h"
#include "game/camera.h"

void session_load_options(struct Session *self)
{
    struct String text, *skin;
    OSVERSIONINFOA version;
    int effectVolume;
    float farthest;

    client_lua_call(g_session1.luaState, "SetCmdOnOffList", "");
    file_make_file_name("OptionInfo.lua", 1);
    g_trilinearFilter = session_get_option_int(self, "Trilinear", 0);
    effectVolume = session_get_option_int(self, "Effect_Volume", 100);
    sound_set_bgm_volume(session_get_option_int(self, "Bgm_Volume", 100));
    sound_set_bgm_fade_target(sound_bgm_volume());
    sound_set_effect_volume(effectVolume);
    sound_set_effect_volume_3d(effectVolume);

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
    if (-55.0f > g_fixedViewPitch)
        g_fixedViewPitch = -55.0f;
    else if (g_fixedViewPitch > -35.0f)
        g_fixedViewPitch = -35.0f;
    if (g_viewDistanceMin > g_fixedViewDistance)
        g_fixedViewDistance = g_viewDistanceMin;
    if (g_fixedViewDistance > g_fixedViewDistanceMax)
        g_fixedViewDistance = g_fixedViewDistanceMax;

    self->zoomFarLimit = session_get_option_int(self, msg_str(0xdb9), 0);
    if (self->zoomFarLimit) {
        farthest = 480.0f;
        g_maxViewDistance = 480.0f;
    } else {
        farthest = 400.0f;
        g_maxViewDistance = 400.0f;
    }
    if (-65.0f > g_freeViewPitch)
        g_freeViewPitch = -65.0f;
    else if (g_freeViewPitch > -25.0f)
        g_freeViewPitch = -25.0f;
    if (g_viewDistanceMin > g_freeViewDistance)
        g_freeViewDistance = g_viewDistanceMin;
    if (g_freeViewDistance > farthest)
        g_freeViewDistance = farthest;

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
    g_fullscreenMode = read_display_options(self, "ISFULLSCREENMODE", 0);
    g_displayWidth = session_get_option_int(self, "WIDTH", 0x400);
    g_displayHeight = session_get_option_int(self, "HEIGHT", 0x300);
    g_savedWidth = session_get_option_int(self, "OLD_WIDTH", 0);
    g_savedHeight = session_get_option_int(self, "OLD_HEIGHT", 0);
    if ((unsigned)g_displayHeight < 0x298) {
        g_displayWidth = 0x400;
        g_displayHeight = 0x300;
    }
    g_screenWidth = g_displayWidth;
    g_screenHeight = g_displayHeight;
    g_bitsPerPixel = session_get_option_int(self, "BITPERPIXEL", 16);
    memset(&version, 0, sizeof version);
    version.dwOSVersionInfoSize = sizeof version;
    GetVersionExA(&version);
    if (version.dwMajorVersion >= 6 && version.dwMinorVersion >= 2)
        g_bitsPerPixel = 32;
    g_displayDeviceIndex = session_get_option_int(self, "DEVICECNT", 0);
    g_displayModeIndex = session_get_option_int(self, "MODECNT", 0);
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
