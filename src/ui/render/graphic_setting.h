#ifndef UI_RENDER_GRAPHIC_SETTING_H
#define UI_RENDER_GRAPHIC_SETTING_H

#include <windows.h>
#include "base.h"
#include "ui/controls/controls.h"

extern int g_textureMode;
extern int g_lightmap;
extern int g_displayHeight;
extern int g_displayWidth;
extern int g_sprHalfHeight;
extern int g_sprHalfWidth;
extern int g_spriteMode;
extern int g_trilinearFilter;

struct UIGraphicSettingWnd {
    struct UIFrameWnd frame;
    struct Vec device_names;
    struct Vec mode_names;
    struct Vec modes;
    int device;
    int mode;
    struct String device_name;
    GUID device_guid;
    GUID adapter_guid;
    struct WindowInfo *info;
    int tab;
    struct UITabControl *tabs;
    struct UIBitmapButton *btn_reset;
    struct UIBitmapButton *btn_apply;
    struct UIBitmapButton *btn_close;
    struct UIBasicButton *btn_commands;
    struct UINoneVisibleButton *skin_box;
    struct UICheckBox *box_snap;
    struct UICheckBox *box_skillsnap;
    struct UICheckBox *box_itemsnap;
    struct UICheckBox *box_fog;
    struct UICheckBox *box_aura;
    struct UICheckBox *box_aura2;
    struct UICheckBox *box_effect;
    struct UICheckBox *box_lightmap;
    struct UICheckBox *box_noctrl;
    struct UICheckBox *box_noshift;
    struct UIScrollBar *bgm_volume;
    struct UIScrollBar *effect_volume;
    struct UICheckBox *box_bgm;
    struct UICheckBox *box_sound;
    struct UICheckBox *box_zoom;
    struct UICheckBox *box_emblem_frame;
    struct UICheckBox *box_extend_damage;
    struct UICheckBox *box_map_name;
    struct UINoneVisibleButton *device_box;
    struct UINoneVisibleButton *mode_box;
    int sprite_mode;
    struct UIScrollBar *sprite_bar;
    int texture_detail;
    struct UIScrollBar *texture_bar;
    struct UICheckBox *box_fullscreen;
    struct UICheckBox *box_mouse;
    struct UICheckBox *box_trilinear;
    struct UICheckBox *box_rodex_all;
};

#endif
