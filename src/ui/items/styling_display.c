#include "ui/character/character_draw.h"
#include "ui/controls/buttons.h"
#include "render/rp_pool.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "core/memory.h"
#include "ui/items/styling_shop.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_jobs.h"
#include "session/session_talk.h"
#include "platform/clock.h"

struct UIDisplayCharacter;
struct UIWindow;
void ui_display_character_on_create(struct UIWindow *self, int width, int height);
void ui_display_character_on_draw(struct UIWindow *self);
intptr_t ui_display_character_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_display_character_ops = {
    .destroy = ui_display_character_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_display_character_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_display_character_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_display_character_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

#define STYLE_SHADOW_OFF STYLE_SKIN "shadow_chr_off.bmp"
#define STYLE_FAVOURITE_OFF STYLE_SKIN "btn_favorite_off.bmp"
#define STYLE_FAVOURITE_ON STYLE_SKIN "btn_favorite_on.bmp"

struct String *style_kind_name(struct String *out, int kind)
{
    static const char *const names[7] = { "HeadPalette", "HeadStyle", "BodyPalette", "TopAccessory",
                                          "MidAccessory", "BottomAccessory", "BodyStyle" };
    struct String name;

    string_init(&name);
    if (kind >= STYLE_HEAD_PALETTE && kind <= STYLE_BODY_STYLE)
        string_assign_cstr(&name, names[kind - 1]);
    if (kind != STYLE_HEAD_PALETTE && (kind <= STYLE_BODY_PALETTE || kind > STYLE_BOTTOM_ACCESSORY)
        && style_is_doram()) {
        string_init(out);
        string_assign_cstr(out, "Doram");
        string_append(out, string_cstr(&name), name.size);
        string_free(&name);
    } else {
        *out = name;
    }
    return out;
}

int style_kind_value(const struct UIDisplayCharacter *display, int kind)
{
    switch (kind) {
    case STYLE_HEAD_PALETTE: return display->head_palette;
    case STYLE_HEAD_STYLE: return display->head_style;
    case STYLE_BODY_PALETTE: return display->body_palette;
    case STYLE_TOP_ACCESSORY: return display->goods_top;
    case STYLE_MID_ACCESSORY: return display->goods_mid;
    case STYLE_BOTTOM_ACCESSORY: return display->goods_bottom;
    default: return display->body_style;
    }
}

BYTE style_is_doram(void)
{
    return session_is_doram_job(&g_session1, session_get_drawn_job(&g_session1));
}

struct UIBitmapButton *style_add_button(struct UIWindow *parent, struct UIBitmapButton *btn, const char *up,
                                        const char *over, const char *down, int x, int y, int id)
{
    ui_bitmap_button_set_bitmap(btn, up, UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(btn, over, UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(btn, down, UI_BTN_DOWN, 0);
    ui_window_create(&btn->button.base, btn->bitmap_width, btn->bitmap_height);
    ui_window_move_base(&btn->button.base, x, y);
    btn->button.base.id = id;
    ui_window_add_child(parent, &btn->button.base);
    return btn;
}

static struct UIBitmapButton *style_new_button(void)
{
    struct UIBitmapButton *btn = mem_try_alloc(sizeof *btn);

    if (btn)
        btn = ui_bitmap_button_construct(btn);
    return btn;
}

void style_lua_size(const char *name, int *count)
{
    client_lua_call(g_session1.luaState, "StylingShop_GetSizeInTable", "s>d", name, count);
}
void style_lua_seq(const char *name, int goods, int *seq)
{
    client_lua_call(g_session1.luaState, "StylingShop_GetSeqnumber_by_goods", "sd>d", name, goods, seq);
}
static void style_lua_goods(const char *name, int index, int *goods)
{
    client_lua_call(g_session1.luaState, "StylingShop_GetGoods", "sd>d", name, index, goods);
}

struct StyleTab *style_my_style_tab(struct UIStylingShopWnd *shop)
{
    struct TreeNode *node;

    for (node = tree_first(&shop->tabs); node != shop->tabs.head; node = tree_next(node)) {
        struct StyleTab *entry = TREE_VALUE(node, struct StyleTab);

        if (entry->button->id == STYLE_TAB_MY_STYLE)
            return entry;
    }
    return NULL;
}

struct UIDisplayCharacter *style_display_construct(struct UIDisplayCharacter *self, BYTE no_buttons)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_display_character_ops;
    self->slot = 0;
    self->direction = 0;
    self->robe = 0;
    self->head_style = 0;
    self->body_style = 0;
    self->head_palette = 0;
    self->body_palette = 0;
    self->accessory_top = 0;
    self->accessory_mid = 0;
    self->accessory_bottom = 0;
    self->no_buttons = no_buttons;
    self->goods_top = 0;
    self->goods_mid = 0;
    self->goods_bottom = 0;
    self->draw_x = 0;
    self->draw_y = 0;
    self->shadow_bitmap = STYLE_SHADOW_OFF;
    return self;
}

struct UIDisplayCharacter *ui_display_character_construct(struct UIDisplayCharacter *self)
{
    return style_display_construct(self, 0);
}

void ui_display_character_destroy(struct UIWindow *window)
{
    struct UIDisplayCharacter *self = (struct UIDisplayCharacter *)window;
    self->base.ops = &ui_display_character_ops;
    ui_window_destruct(&self->base);
    mem_free(self);
}

void ui_display_character_copy_look(struct UIDisplayCharacter *self, const struct UIDisplayCharacter *other)
{
    self->slot = other->slot;
    self->direction = other->direction;
    self->robe = other->robe;
    self->head_style = other->head_style;
    self->body_style = other->body_style;
    self->head_palette = other->head_palette;
    self->body_palette = other->body_palette;
    self->accessory_top = other->accessory_top;
    self->accessory_mid = other->accessory_mid;
    self->accessory_bottom = other->accessory_bottom;
    self->goods_top = other->goods_top;
    self->goods_mid = other->goods_mid;
    self->goods_bottom = other->goods_bottom;
    ui_window_invalidate_base(&self->base);
}

void display_set_favourite(struct UIDisplayCharacter *display, BYTE saved)
{
    struct UIStylingShopWnd *shop = (struct UIStylingShopWnd *)ui_get_window(WND_STYLING_SHOP);
    struct UIBitmapButton *fav;
    struct StyleTab *entry;
    int i;

    if (!shop)
        return;
    entry = style_my_style_tab(shop);
    if (!entry) {
        ui_bitmap_button_set_tool_tip(display->favourite, "");
    } else {
        struct UIMyStyleWnd *my_style = (struct UIMyStyleWnd *)entry->tab;

        for (i = 0; i < MY_STYLE_SLOTS; i++) {
            if (!my_style->slots[i]) {
                ui_bitmap_button_set_tool_tip(display->favourite, "");
                break;
            }
        }
    }
    fav = display->favourite;
    fav->button.cmd_value = 0;
    ui_bitmap_button_set_bitmap(display->favourite, STYLE_FAVOURITE_OFF, UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(display->favourite, STYLE_FAVOURITE_OFF, UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(display->favourite, STYLE_FAVOURITE_OFF, UI_BTN_DOWN, 0);
    if (saved == 1) {
        display->favourite->button.cmd_value = 1;
        ui_bitmap_button_set_bitmap(display->favourite, STYLE_FAVOURITE_ON, UI_BTN_UP, 0);
        ui_bitmap_button_set_bitmap(display->favourite, STYLE_FAVOURITE_ON, UI_BTN_HOVER, 0);
        ui_bitmap_button_set_bitmap(display->favourite, STYLE_FAVOURITE_ON, UI_BTN_DOWN, 0);
    }
    ui_window_invalidate_base(&display->favourite->button.base);
}

static void display_add_buttons(struct UIDisplayCharacter *self)
{
    struct UIWindow *wnd = &self->base;

    style_add_button(wnd, style_new_button(), STYLE_SKIN "arrow_chr_l_upper_out.bmp",
                     STYLE_SKIN "arrow_chr_l_upper_over.bmp", STYLE_SKIN "arrow_chr_l_upper_over.bmp", 0xf, 0x1e,
                     STYLE_ID_HAIR_PREV);
    style_add_button(wnd, style_new_button(), STYLE_SKIN "arrow_chr_r_upper_out.bmp",
                     STYLE_SKIN "arrow_chr_r_upper_over.bmp", STYLE_SKIN "arrow_chr_r_upper_over.bmp", 0x5f, 0x1e,
                     STYLE_ID_HAIR_NEXT);
    style_add_button(wnd, style_new_button(), STYLE_SKIN "arrow_chr_l_upper_out.bmp",
                     STYLE_SKIN "arrow_chr_l_upper_over.bmp", STYLE_SKIN "arrow_chr_l_upper_over.bmp", 0xf, 0x3b,
                     STYLE_ID_PALETTE_PREV);
    style_add_button(wnd, style_new_button(), STYLE_SKIN "arrow_chr_r_upper_out.bmp",
                     STYLE_SKIN "arrow_chr_r_upper_over.bmp", STYLE_SKIN "arrow_chr_r_upper_over.bmp", 0x5f, 0x3b,
                     STYLE_ID_PALETTE_NEXT);
    self->favourite = style_new_button();
    style_add_button(wnd, self->favourite, STYLE_FAVOURITE_OFF, STYLE_SKIN "btn_favorite_off_over.bmp",
                     STYLE_FAVOURITE_ON, 7, 0x53, STYLE_ID_FAVOURITE);
    style_add_button(wnd, style_new_button(), STYLE_SKIN "btn_refresh_out.bmp", STYLE_SKIN "btn_refresh_over.bmp",
                     STYLE_SKIN "btn_refresh_press.bmp", 7, 0x65, STYLE_ID_RESET);
    style_add_button(wnd, style_new_button(), STYLE_SKIN "btn_rotatearrow_l_out.bmp",
                     STYLE_SKIN "btn_rotatearrow_l_over.bmp", STYLE_SKIN "btn_rotatearrow_l_press.bmp", 0x27, 0x6c,
                     STYLE_ID_TURN_LEFT);
    style_add_button(wnd, style_new_button(), STYLE_SKIN "btn_rotatearrow_r_out.bmp",
                     STYLE_SKIN "btn_rotatearrow_r_over.bmp", STYLE_SKIN "btn_rotatearrow_r_press.bmp", 0x3c, 0x6c,
                     STYLE_ID_TURN_RIGHT);
}

static void display_wear_my_look(struct UIDisplayCharacter *self)
{
    self->robe = g_session1.robe;
    self->head_style = g_session1.hairStyle;
    self->body_style = g_session1.body;
    self->head_palette = g_session1.hairColor;
    self->body_palette = g_session1.clothesColor;
    self->accessory_top = g_session1.headTop;
    self->accessory_mid = g_session1.headMid;
    self->accessory_bottom = g_session1.headBottom;
}

void ui_display_character_on_create(struct UIWindow *window, int width, int height)
{
    struct UIDisplayCharacter *self = (struct UIDisplayCharacter *)window;
    ui_window_resize_base(&self->base, width, height);
    self->start_time = platform_ticks_ms();
    if (!self->no_buttons)
        display_add_buttons(self);
    display_wear_my_look(self);
    self->draw_x = 0x20;
    self->draw_y = 0x4c;
}

void ui_display_character_on_draw(struct UIWindow *window)
{
    struct UIDisplayCharacter *self = (struct UIDisplayCharacter *)window;
    struct UIWindow *wnd = &self->base;
    struct BitmapRes *shadow;

    ui_window_clear_canvas(wnd, 0xffffffff);
    shadow = ui_skin_bitmap(self->shadow_bitmap);
    if (shadow)
        ui_window_draw_bitmap(wnd, self->draw_x, self->draw_y, shadow, 0);
    struct CharacterDrawOptions options = {
        .x = wnd->width / 2 + 1,
        .y = wnd->height - 0x21,
        .sex = session_get_sex(&g_session1),
        .job = g_session1.job,
        .body = (unsigned short)self->body_style,
        .head = (unsigned short)self->head_style,
        .accessory = (unsigned short)self->accessory_bottom,
        .accessory2 = (unsigned short)self->accessory_top,
        .accessory3 = (unsigned short)self->accessory_mid,
        .robe = self->robe,
        .riding = 0,
        .scaled = 0,
        .body_palette = self->body_palette,
        .head_palette = self->head_palette,
        .action = self->direction,
        .motion = 0,
        .start_time = self->start_time,
    };
    ui_draw_character(wnd, &options, 0);
}

static BYTE style_three_colours(const char *name)
{
    if (session_is_fourth_job(&g_session1, session_get_drawn_job(&g_session1)))
        return 0;
    return strcmp(name, "BodyPalette") == 0 || strcmp(name, "DoramBodyPalette") == 0;
}

static void style_next(const char *name, int *value)
{
    int count = 0, seq, next;

    style_lua_size(name, &count);
    if (count == 0)
        return;
    if (style_three_colours(name))
        count = 3;
    seq = 0;
    style_lua_seq(name, *value, &seq);
    next = seq + 1;
    if (count < next)
        next = 1;
    style_lua_seq(name, next, value);
}

static void style_prev(const char *name, int *value)
{
    int count = 0, seq, prev;

    style_lua_size(name, &count);
    if (count == 0)
        return;
    if (style_three_colours(name))
        count = 3;
    seq = 0;
    style_lua_seq(name, *value, &seq);
    prev = seq - 1;
    if (prev < 1)
        prev = count;
    if (prev <= 0)
        prev = 1;
    style_lua_goods(name, prev, value);
}

static void display_step(int kind, int *value, int forward)
{
    struct String name;

    style_kind_name(&name, kind);
    if (forward)
        style_next(string_cstr(&name), value);
    else
        style_prev(string_cstr(&name), value);
    string_free(&name);
}

intptr_t ui_display_character_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                          intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIDisplayCharacter *self = (struct UIDisplayCharacter *)window;
    struct UIWindow *shop = ui_get_window(WND_STYLING_SHOP);

    if (msg != UI_MSG_COMMAND)
        return ui_window_send_msg_base(&self->base, sender, msg, val1, val2, val3, val4);
    switch (val1) {
    case STYLE_ID_RESET:
        display_wear_my_look(self);
        break;
    case STYLE_ID_TURN_RIGHT:
        if (self->direction <= 0)
            self->direction = 7;
        else
            self->direction--;
        break;
    case STYLE_ID_TURN_LEFT:
        if (self->direction >= 7)
            self->direction = 0;
        else
            self->direction++;
        break;
    case STYLE_ID_PALETTE_PREV:
        display_step(STYLE_BODY_PALETTE, &self->body_palette, 0);
        break;
    case STYLE_ID_PALETTE_NEXT:
        display_step(STYLE_BODY_PALETTE, &self->body_palette, 1);
        break;
    case STYLE_ID_HAIR_PREV:
        display_step(STYLE_HEAD_STYLE, &self->head_style, 0);
        break;
    case STYLE_ID_HAIR_NEXT:
        display_step(STYLE_HEAD_STYLE, &self->head_style, 1);
        break;
    case STYLE_ID_PALETTE:
        ui_window_send_msg(shop, NULL, UI_MSG_COMMAND, STYLE_ID_PALETTE, 0, 0, 0);
        break;
    case STYLE_ID_FAVOURITE:
        if (self->favourite->button.cmd_value == 0) {
            if (styling_shop_save_look((struct UIStylingShopWnd *)shop) == 1)
                ui_message_box(msg_str(0xb03));
            else
                ui_bitmap_button_set_tool_tip(self->favourite, msg_str(0xb01));
        }
        break;
    }
    ui_window_invalidate_base(&self->base);
    ui_window_invalidate_children(shop);
    return 0;
}
