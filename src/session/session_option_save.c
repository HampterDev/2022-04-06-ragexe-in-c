#include "contents/client_lua_calls.h"
#include "contents/sound.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "session/csession.h"
#include "session/session_options.h"
#include "core/app.h"
#include "ui/render/graphic_setting.h"
#include "game/camera.h"
#include "platform/window.h"

#define OPT_INT(name, value)    session_write_option_int(self, &out, (name), (value))

void session_save_options(struct Session *self)
{
    struct String path;
    struct File in, out;
    RECT window;
    char *text;
    int size;

    string_init(&path);
    string_assign_cstr(&path, "SaveData\\OptionInfo.lua");
    client_lua_call(self->luaState, "SaveToFileCmdOnOffValueEx", "d", 0);
    file_construct(&in);
    if (file_open(&in, string_cstr(&path), 0, 0, 1)) {
        size = in.size;
        text = (char *)mem_alloc((size_t)size + 1);
        file_read(&in, text, size);
        text[size] = 0;
        file_close(&in);
        file_construct(&out);
        if (file_open(&out, string_cstr(&path), 0x1001, 1, 1)) {
            file_write(&out, text, size);
            mem_free(text);
            OPT_INT("Trilinear", g_trilinearFilter);
            OPT_INT("Bgm_Volume", sound_bgm_fade_target());
            OPT_INT("Effect_Volume", sound_effect_volume());
            OPT_INT("AutoOpen1to1Window", self->showWhisperWnd);
            OPT_INT("AutoOpen1to1Window_Friend", self->showWhisperWndFriend);
            OPT_INT("PlaySound_Open1to1Window", self->playWhisperOpenSound);
            session_write_option_float(self, &out, "Outdoor_ViewLatitude", g_freeViewPitch);
            session_write_option_float(self, &out, "Outdoor_ViewDistance", g_freeViewDistance);
            session_write_option_float(self, &out, "Indoor_ViewLatitude", g_fixedViewPitch);
            session_write_option_float(self, &out, "Indoor_ViewDistance", g_fixedViewDistance);
            session_write_option_string(self, &out, "SkinName", skin_mgr_get_current_name(&g_skinMgr));
            OPT_INT("ChannelCopID", self->channelCopId);
            platform_window_rect(g_hMainWnd, &window);
            OPT_INT("Window_XPos", window.left);
            OPT_INT("Window_YPos", window.top);
            OPT_INT("Simplicity_SkillList", self->simpleSkillWnd);
            OPT_INT("Show_SkillDescript", self->showSkillDescript);
            OPT_INT("ChangeChatMode", g_windowMgr.chat_bar_toggleable);
            OPT_INT("LockMouse", self->lockMouse);
            OPT_INT("MouseExclusive", g_session1.mouseExclusiveOption);
            write_display_options(self, &out, "ISFULLSCREENMODE", g_fullscreenMode);
            OPT_INT("WIDTH", g_displayWidth);
            OPT_INT("HEIGHT", g_displayHeight);
            OPT_INT("OLD_WIDTH", g_displayWidth);
            OPT_INT("OLD_HEIGHT", g_displayHeight);
            OPT_INT("BITPERPIXEL", g_bitsPerPixel);
            OPT_INT("DEVICECNT", g_displayDeviceIndex);
            OPT_INT("MODECNT", g_displayModeIndex);
            OPT_INT("SPRITEMODE", g_spriteMode);
            OPT_INT("TEXTUREMODE", g_textureMode);
            OPT_INT("bLockItemDropFromItemWnd", self->itemDropLock);
            OPT_INT("bWarningPriceCheck", self->noPriceCheckWarning == 0);
            OPT_INT("bSoundEffectInItemSellWnd", self->noSellWndSound == 0);
            OPT_INT("bGuildMemberListSort", self->guildMemberSort != 0);
            OPT_INT("bBlockPrivateTab", self->blockPrivateTab != 0);
            OPT_INT("bShowMenuIcon", self->showMenuIcon != 0);
            OPT_INT("bItemDefaultSort", self->itemSort[0] != 0);
            OPT_INT("bItemAscendingOrder", self->itemSort[1] != 0);
            OPT_INT("bItemDescendingOrder", self->itemSort[2] != 0);
            OPT_INT(msg_str(0xdb9), self->zoomFarLimit);
            OPT_INT(msg_str(0xdc6), self->emblemFrame);
            OPT_INT("bExtendDamage", self->extendDamage);
            file_close(&out);
        }
        file_destruct(&out);
    }
    file_destruct(&in);
    string_free(&path);
}
