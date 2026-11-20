#ifndef UI_RENDER_GRAPHIC_SETTING_H
#define UI_RENDER_GRAPHIC_SETTING_H

#include "platform/win_types.h"
#include "base.h"
#include "ui/controls/controls.h"
#include "render/render_options.h"
#include "render/display_options.h"

struct UIGraphicSettingWnd {
    struct UIFrameWnd frame;
    /* custom: the resolution picker (render/display_options.h); the sizes its list was opened with,
       and as text; the size it shows, the window's until one is picked (picked), which Apply
       sets; and the window's mode and size as the dialog last showed them */
    struct DisplayPicker picker;
    struct DisplayPicker listed;
    struct Vec mode_names;
    int pick_width, pick_height;
    int picked;
    int seen_fullscreen, seen_width, seen_height;
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
