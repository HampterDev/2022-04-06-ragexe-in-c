#include "contents/pet_evolution.h"
#include "render/rp_pool.h"
#include "iteminfo/item_api.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "core/object_type.h"
#include "contents/connection_api.h"
#include <string.h>
#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/ui_std.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "session/session_talk.h"
#include "ui/controls/window_fit.h"
#include "ui/social/guild_member_sort.h"
#include "ui/wndmgr/make_window_internal.h"

#define PET_EVOLUTION_WND_ID 0x105
#define PET_EVO_MSG_SET 0x8e
#define PET_EVO_PACKET 0x9fb
#define PET_EVO_ITEM_WND_ID 0xc
#define PET_EVO_MSG_SHOW_ITEM 0x18
#define PET_EVO_ACCESSORY_REFUSED 0xa2f
#define MSG_PET_EVO_TITLE 0xa07
#define MSG_PET_EVO_MATERIALS 0xa09
#define MSG_PET_EVO_HINT 0xa0a
#define MSG_PET_EVO_MISSING 0xa0e
#define PET_EVO_REFUSAL_COLOUR 0xff
#define PET_EVO_FRAME_COLOUR 0x9b9b9b
#define PET_EVO_NAME_COLOUR 0xff6464
#define PET_EVO_ILLUST_Y 0x1b
#define PET_EVO_NAME_Y 0xab
#define PET_EVO_MATERIAL_Y 0xe6

struct UIPetEvolutionWnd {
    struct UIFrameWnd frame;
    struct Vec material_names;
    int egg;
    int evolved_egg;
    struct Vec materials;
};
OFFSET_ASSERT(UIPetEvolutionWnd, egg, 0xa0);
OFFSET_ASSERT(UIPetEvolutionWnd, materials, 0xa8);
SIZE_ASSERT(UIPetEvolutionWnd, 0xb4);

void ui_pet_evolution_wnd_destroy(struct UIWindow *window);
void ui_pet_evolution_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_pet_evolution_wnd_on_draw(struct UIWindow *self);
int ui_pet_evolution_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_pet_evolution_wnd_ops = {
    .destroy = ui_pet_evolution_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_pet_evolution_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_pet_evolution_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_pet_evolution_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_frame_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

#pragma pack(push, 1)
struct PetEvolutionPacket {
    WORD id;
    WORD len;
    int evolved_egg;
    WORD pairs[0x7e];
};
#pragma pack(pop)
SIZE_ASSERT(PetEvolutionPacket, 0x104);

static struct BitmapRes *load_bitmap(const char *path)
{
    return object_as_type(res_mgr_get(g_resMgr(), path), OBJECT_BITMAP_RESOURCE);
}

static void add_button(struct UIPetEvolutionWnd *self, const char *const names[3], int double_width, int id)
{
    struct UIWindow *wnd = &self->frame.base;
    struct UIBitmapButton *button = mem_alloc(sizeof(struct UIBitmapButton));
    int x;

    if (button)
        button = ui_bitmap_button_construct(button);
    ui_bitmap_button_set_bitmap(button, names[0], UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(button, names[1], UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(button, names[2], UI_BTN_DOWN, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    x = double_width ? wnd->width - (button->bitmap_width * 2 + 0x14) : wnd->width - button->bitmap_width - 0xa;
    ui_window_move_base(&button->button.base, x, wnd->height - button->bitmap_height - 0xa);
    button->button.base.id = id;
    ui_window_add_child(wnd, &button->button.base);
}

static void draw_pet(struct UIWindow *wnd, int egg, int right)
{
    char path[0x104];
    struct BitmapRes *illust;
    int job = session_get_pet_job_by_egg(&g_session1, egg), x;

    memset(path, 0, sizeof path);
    if (job == -1)
        return;
    session_get_pet_illust_name(&g_session1, job, path);
    illust = load_bitmap(path);
    if (!illust)
        illust = load_bitmap("userinterface\\illust\\pet_noimage.bmp");
    if (!illust)
        return;
    x = right ? wnd->width - illust->width - 0xa : 0xa;
    ui_window_fill_rect(wnd, x + 2, PET_EVO_ILLUST_Y + 2, illust->width, illust->height, PET_EVO_FRAME_COLOUR);
    ui_window_draw_bitmap(wnd, x, PET_EVO_ILLUST_Y, illust, 0);
    ui_window_draw_outline(wnd, x, PET_EVO_ILLUST_Y, illust->width, illust->height, 0);
}

static struct UITextButton *add_name(const char *name, int id, int x, int y,
                                     int fit_less)
{
    struct UITextButton *label = ui_text_button_create();
    ui_text_button_set_text(&label->button.base, name, PET_EVO_NAME_COLOUR, 0, 0xc, 1, 1);
    label->hover_colour = PET_EVO_FRAME_COLOUR;
    ui_text_button_fit_width(label, label->button.base.width - fit_less, label->button.base.height);
    label->button.base.id = id;
    ui_window_move_base(&label->button.base, x, y);
    return label;
}

static void add_pet_name(struct UIPetEvolutionWnd *self, int egg, int x)
{
    const char *name = session_get_pet_string(&g_session1, session_get_pet_job_by_egg(&g_session1, egg));
    struct UITextButton *label;

    if (!name)
        return;
    label = add_name(name, egg, x, PET_EVO_NAME_Y, 0xa);
    ui_window_move_centred_for(&label->button.base, 0x5a, label->button.base.height);
    ui_window_add_child(&self->frame.base, &label->button.base);
}

static void build_labels(struct UIPetEvolutionWnd *self)
{
    struct PetMaterial *material;
    int y = PET_EVO_MATERIAL_Y;

    add_pet_name(self, self->egg, 0xa);
    add_pet_name(self, self->evolved_egg, self->frame.base.width - 0x64);
    self->material_names.last = self->material_names.first;
    for (material = self->materials.first; material != self->materials.last; material++, y += 0x14) {
        struct ItemInfo info;
        const char *name;
        struct UITextButton *label;

        item_info_construct(&info);
        item_info_set_item_id(&info, material->item_id);

        name = item_info_get_name(&info);
        label = add_name(name, material->item_id, 0xa, y, 0);
        ui_window_add_child(&self->frame.base, &label->button.base);
        vec_push(&self->material_names, &label, sizeof label);
        item_info_destruct(&info);
    }
}

static int request_evolution(struct UIPetEvolutionWnd *self)
{
    struct Vec pairs = { NULL, NULL, NULL };
    struct PetMaterial *material;
    struct PetEvolutionPacket packet;
    unsigned short value;
    int count, sent = 0;

    for (material = self->materials.first; material != self->materials.last; material++) {
        struct ItemInfo held;

        session_get_inventory_item_by_id(&g_session1, &held, material->item_id, 1);
        if (held.count < material->count) {
            ui_chat_line(msg_str(MSG_PET_EVO_MISSING), PET_EVO_REFUSAL_COLOUR);
            item_info_destruct(&held);
            goto done;
        }
        value = (unsigned short)held.index;
        vec_push(&pairs, &value, sizeof value);
        value = (unsigned short)material->count;
        vec_push(&pairs, &value, sizeof value);
        item_info_destruct(&held);
    }
    if (g_session1.petAccessory) {
        ui_chat_line(msg_str(PET_EVO_ACCESSORY_REFUSED), PET_EVO_REFUSAL_COLOUR);
        goto done;
    }
    count = (int)((unsigned short *)pairs.last - (unsigned short *)pairs.first);
    memset(packet.pairs, 0, sizeof packet.pairs);
    packet.id = PET_EVO_PACKET;
    packet.len = (WORD)(count * 2 + 8);
    packet.evolved_egg = self->evolved_egg;
    memcpy(packet.pairs, pairs.first, count * 2);
    rag_connection_send_packet(rag_connection_instance(), (short)packet.len, &packet);
    ui_window_mgr_post_quit1(&g_windowMgr, PET_EVOLUTION_WND_ID);
    sent = 1;
done:
    vec_free(&pairs);
    return sent;
}

struct UIPetEvolutionWnd *ui_pet_evolution_wnd_construct(struct UIPetEvolutionWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_pet_evolution_wnd_ops;
    memset(&self->material_names, 0, sizeof self->material_names);
    memset(&self->materials, 0, sizeof self->materials);
    self->egg = -1;
    self->evolved_egg = -1;
    return self;
}

struct UIWindow *ui_pet_evolution_wnd_new(void)
{
    return (struct UIWindow *)ui_pet_evolution_wnd_construct(mem_alloc(sizeof(struct UIPetEvolutionWnd)));
}

void ui_pet_evolution_wnd_destroy(struct UIWindow *window)
{
    struct UIPetEvolutionWnd *self = (struct UIPetEvolutionWnd *)window;
    self->frame.base.ops = &ui_pet_evolution_wnd_ops;
    vec_free(&self->materials);
    vec_free(&self->material_names);
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_pet_evolution_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIPetEvolutionWnd *self = (struct UIPetEvolutionWnd *)window;
    static const char *const ok[3] = { UI_SKIN "btn_big_ok.bmp", UI_SKIN "btn_big_ok_a.bmp",
                                       UI_SKIN "btn_big_ok_b.bmp" };
    static const char *const cancel[3] = { UI_SKIN "btn_big_cancel.bmp", UI_SKIN "btn_big_cancel_a.bmp",
                                           UI_SKIN "btn_big_cancel_b.bmp" };

    ui_window_add_close_button(&self->frame.base, "", 0);
    add_button(self, ok, 1, UI_ID_OK);
    add_button(self, cancel, 0, UI_ID_CANCEL);
}

void ui_pet_evolution_wnd_on_draw(struct UIWindow *window)
{
    struct UIPetEvolutionWnd *self = (struct UIPetEvolutionWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct BitmapRes *arrow;
    struct UIWindow **name;
    struct PetMaterial *material;

    ui_window_clear_canvas(wnd, 0xffffffff);
    ui_window_draw_title_bar(wnd, 1, msg_str(MSG_PET_EVO_TITLE), 0);
    draw_pet(wnd, self->egg, 0);
    arrow = load_bitmap(UI_SKIN "img_next.bmp");
    if (arrow)
        ui_window_draw_bitmap(wnd, (int)((wnd->width - arrow->width) * 0.5),
                            (int)((0x86 - arrow->height) * 0.5 + 27.0), arrow, 1);
    draw_pet(wnd, self->evolved_egg, 1);
    ui_window_text_out(wnd, 0xa, 0xc8, msg_str(MSG_PET_EVO_MATERIALS), 0, 0, 0xc, 0, 0, 0);
    name = self->material_names.first;
    for (material = self->materials.first; material != self->materials.last; material++, name++) {
        char text[0x104];

        memset(text, 0, sizeof text);
        sprintf(text, "%d개", material->count);
        ui_window_text_out(wnd, (*name)->x + 0xa + (*name)->width, (*name)->y + 1, text, 0, 0, 0xc, 0, 1, 0);
    }
    ui_window_text_out(wnd, 0xa, wnd->height - 0x34, msg_str(MSG_PET_EVO_HINT), 0, 0, 0xc, 0, 0, 0);
}

int ui_pet_evolution_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                         int val2, int val3, int val4)
{
    struct UIPetEvolutionWnd *self = (struct UIPetEvolutionWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:
        if (val1 == UI_ID_OK) {
            if (!request_evolution(self))
                return 0;
        } else if (val1 == UI_ID_CANCEL || val1 == UI_ID_CLOSE) {
            ui_window_mgr_post_quit1(&g_windowMgr, PET_EVOLUTION_WND_ID);
        }
        break;
    case UI_MSG_TEXT_BUTTON: {
        struct ItemInfo info;

        item_info_construct(&info);
        item_info_set_item_id(&info, val1);
        info.identified = 1;
        ui_window_send(ui_window_open(PET_EVO_ITEM_WND_ID), NULL, PET_EVO_MSG_SHOW_ITEM, (int)&info, 0, 0, 0);
        item_info_destruct(&info);
        break;
    }
    case PET_EVO_MSG_SET: {
        struct PetFoundList found;

        self->egg = val1;
        self->evolved_egg = val2;
        self->materials.last = self->materials.first;
        pet_evolution_materials(g_petTable, &found, self->egg, self->evolved_egg);
        if (found.found && &self->materials != &found.list)
            vec_assign(&self->materials, found.list.first, found.list.last, sizeof(struct PetMaterial));
        build_labels(self);
        vec_free(&found.list);
        break;
    }
    default:
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    }
    ui_window_invalidate_base(&self->frame.base);
    return 0;
}
