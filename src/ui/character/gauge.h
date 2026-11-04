#ifndef UI_CHARACTER_GAUGE_H
#define UI_CHARACTER_GAUGE_H

#include "platform/win_types.h"

#include "ui/character/gauge_value.h"
#include "ui/window_types.h"
#include <stdint.h>

int ui_monster_gauge_hp(const struct UIWindow *window);
int ui_monster_gauge_max_hp(const struct UIWindow *window);
int ui_monster_gauge_two_bars(const struct UIWindow *window);

enum PlayerGaugeBar { PLAYER_GAUGE_HP, PLAYER_GAUGE_SP, PLAYER_GAUGE_FOOD };
enum ValueGaugeStyle { VALUE_GAUGE_PLAYER, VALUE_GAUGE_HOMUN };

struct UIWindow *ui_player_gauge_create(void);
void ui_player_gauge_set(struct UIWindow *window, enum PlayerGaugeBar bar,
    struct GaugeValue current, struct GaugeValue maximum);
void ui_player_gauge_set_layout(struct UIWindow *window, int layout);

struct UIWindow *ui_value_gauge_create(enum ValueGaugeStyle style);
void ui_value_gauge_set(struct UIWindow *window,
    struct GaugeValue current, struct GaugeValue maximum);

struct UIBasicInfoWnd;
struct UIBasicInfoWnd *ui_basic_info_wnd_construct(struct UIBasicInfoWnd *self);
struct UIWindow *ui_basic_info_wnd_new(void);
void ui_basic_info_wnd_set_height(struct UIBasicInfoWnd *self, int height);
void ui_basic_info_wnd_set_base_exp(struct UIBasicInfoWnd *self, int64_t current, int64_t maximum);
void ui_basic_info_wnd_set_job_exp(struct UIBasicInfoWnd *self, int64_t current, int64_t maximum);

struct UIHomunInfoWnd *ui_homun_info_wnd_construct(struct UIHomunInfoWnd *self);
struct UIWindow *ui_homun_info_wnd_new(void);
struct UIMerInfoWnd *ui_mer_info_wnd_construct(struct UIMerInfoWnd *self);
struct UIWindow *ui_mer_info_wnd_new(void);

struct UIGage {
    struct UIWindow base;
    COLORREF clear_colour;
    COLORREF back_colour;
    COLORREF bar_colour;
    COLORREF second_colour;
    COLORREF low_colour;
    COLORREF food_colour, food_low_colour;
};

struct UIPcGage {
    struct UIGage gage;
    int two_bars;
    int hp, max_hp;
    int sp, max_sp;
};

void ui_gage_set_back_colour(struct UIGage *self, COLORREF colour);
struct UIMonsterGage *ui_monster_gage_new(void);
struct UIPcGage *ui_pc_gage_new(void);
void ui_pc_gage_set_hp(struct UIPcGage *self, int hp, int max_hp);
void ui_pc_gage_set_sp(struct UIPcGage *self, int sp, int max_sp);
void ui_pc_gage_set_two_bars(struct UIPcGage *self, int two_bars);
struct UIRechargeGage *ui_recharge_gage_new(void);
void ui_recharge_gage_set_amount(struct UIRechargeGage *self, int value, int maximum);

#endif
