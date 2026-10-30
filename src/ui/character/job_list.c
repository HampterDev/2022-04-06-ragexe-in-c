#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "contents/connection_api.h"
#include "ui/character/character.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "std/tree.h"

struct UIJobListWnd;
struct UIWindow;
void ui_job_list_wnd_destroy(struct UIWindow *window);
void ui_job_list_wnd_store_info(struct UIWindow *self);
void ui_job_list_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_job_list_wnd_on_draw(struct UIWindow *self);
int ui_job_list_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_job_list_wnd_ops = {
    .destroy = ui_job_list_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_job_list_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_job_list_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_job_list_wnd_on_draw,
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
    .send_msg = ui_job_list_wnd_send_msg,
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

struct WantedJob {
    int job;
    char checked;
};
static struct Tree job_map;

#define WND_ID_JOB_LIST 0xaa
#define JOB_LIST_ID_OK 0xb0
#define JOB_LIST_SLOTS 6
#define PACKET_PARTY_JOBS 0x808
#define MSG_JOB_LIST_NONE_CHECKED 0x6e7
#define MSG_JOB_NAME_FIRST 0x6af
#define MSG_JOB_NAME_FIRST_5 0x658
#define UI_MSG_RESIZE 0xe
#define UI_MSG_REBUILD 0x17

struct UIJobListWnd {
    struct UIFrameWnd frame;
    struct UIWindow *title;
    struct UIWindow *boxes[JOB_LIST_SLOTS];
    struct UIWindow *ok;
    int checked_count;
    struct WindowInfo *info;
};
OFFSET_ASSERT(UIJobListWnd, ok, 0xb0);

struct UIJobListWnd *ui_job_list_wnd_construct(struct UIJobListWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_job_list_wnd_ops;
    self->title = NULL;
    self->ok = NULL;
    self->checked_count = 0;
    return self;
}

struct UIWindow *ui_job_list_wnd_new(void)
{
    return (struct UIWindow *)ui_job_list_wnd_construct(mem_alloc(sizeof(struct UIJobListWnd)));
}

void ui_job_list_wnd_destroy(struct UIWindow *window)
{
    struct UIJobListWnd *self = (struct UIJobListWnd *)window;
    self->frame.base.ops = &ui_job_list_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_job_list_wnd_store_info(struct UIWindow *window)
{
    struct UIJobListWnd *self = (struct UIJobListWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (!self->info)
        return;
    self->info->width = wnd->width;
    self->info->height = wnd->height;
    self->info->x = wnd->x;
    self->info->y = wnd->y;
}

void ui_job_list_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIJobListWnd *self = (struct UIJobListWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    char title_text[9] = { 0 };
    struct UIWindow *title;
    struct UIBitmapButton *ok;
    int i;

    sprintf(title_text, "Job List");
    title = mem_alloc(sizeof(struct UIStaticText));
    if (title)
        title = ui_static_text_construct(title);
    self->title = title;
    ui_window_create(title, wnd->width - 0x20, 0xf);
    ui_static_text_set_text_ellipsis(self->title, title_text);
    ui_window_move(self->title, 0x10, 2);
    ((struct UIStaticText *)self->title)->white_shadow = 1;
    ui_window_add_child(wnd, self->title);
    ui_window_add_close_button(wnd, "", 0);

    ok = ui_bitmap_button(UI_SKIN "btn_ok.bmp", UI_SKIN "btn_ok_a.bmp", UI_SKIN "btn_ok_b.bmp");
    self->ok = &ok->button.base;
    ui_window_move_base(self->ok, wnd->width - 0x5c, wnd->height - 0x18);
    self->ok->id = JOB_LIST_ID_OK;
    ui_window_add_child(wnd, self->ok);
    for (i = 0; i < JOB_LIST_SLOTS; i++)
        self->boxes[i] = NULL;
}

void ui_job_list_wnd_on_draw(struct UIWindow *window)
{
    struct UIJobListWnd *self = (struct UIJobListWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int red, green, blue;

    ui_window_draw_title_bar(wnd, 1, "", 0);
    ui_skin_color_chip(&g_skinMgr, 2, 2, &red, &green, &blue);
    ui_window_fill_rect(wnd, 0, 0x11, wnd->width, wnd->height - 0x26, RGB((BYTE)red, (BYTE)green, (BYTE)blue));
    ui_window_draw_button_bar(wnd, 2, 0, 1);
}

void ui_job_list_wnd_show_jobs(struct UIJobListWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow **slot = self->boxes;
    struct TreeNode *node;
    int i, y;

    for (i = 0; i < JOB_LIST_SLOTS; i++) {
        if (self->boxes[i]) {
            ui_window_remove_child(wnd, self->boxes[i], 1);
            self->boxes[i] = NULL;
        }
    }
    for (node = tree_first(&job_map), y = 0x19; node != job_map.head; node = tree_next(node), y += 0x19, slot++) {
        const struct WantedJob *wanted = TREE_VALUE(node, struct WantedJob);
        struct UIWindow *box = mem_alloc(sizeof(struct UICheckBox));

        if (box)
            box = (struct UIWindow *)ui_check_box_construct(box);
        *slot = box;
        ui_window_create(box, 0x78, 0xf);
        if (wanted->job == -1)
            continue;
        ui_check_box_set_bitmaps(*slot, UI_SKIN "basic_interface\\grp_online.bmp",
                              UI_SKIN "basic_interface\\grp_offline.bmp",
                              msg_str(wanted->job + (wanted->job - 1 < 4 ? MSG_JOB_NAME_FIRST : MSG_JOB_NAME_FIRST_5)));
        ui_window_move(*slot, 0xa, y);
        ui_check_box_set_check(*slot, wanted->checked);
        ui_window_add_child(wnd, *slot);
    }
}

static int job_list_info_valid(const struct WindowInfo *info)
{
    return info->width >= 0x98 && info->height >= 0xc7 && info->x >= 0 &&
           info->x <= g_renderer->width - info->width && info->y >= 0 &&
           info->y <= g_renderer->height - info->height;
}

static void job_list_info_default(struct WindowInfo *info)
{
    info->x = 0x1fe;
    info->y = 0x168;
    info->width = 0x98;
    info->height = 0xc7;
}

#pragma pack(push, 1)
struct PartyJobsPacket {
    WORD id;
    short jobs[JOB_LIST_SLOTS];
};
#pragma pack(pop)
SIZE_ASSERT(PartyJobsPacket, 14);

static void send_jobs(struct UIJobListWnd *self)
{
    struct TreeNode *node;
    struct PartyJobsPacket packet;
    struct UIWindow **box = self->boxes;
    short *job = packet.jobs;
    int checked = 0, length, i;

    packet.id = PACKET_PARTY_JOBS;
    for (i = 0; i < JOB_LIST_SLOTS; i++)
        packet.jobs[i] = -1;
    for (node = tree_first(&job_map); node != job_map.head; node = tree_next(node), box++, job++) {
        struct WantedJob *wanted = TREE_VALUE(node, struct WantedJob);

        if (*box && (*box)->state) {
            *job = (short)wanted->job;
            wanted->checked = 1;
            checked++;
        } else {
            *job = -1;
            wanted->checked = 0;
        }
    }
    if (!checked) {
        ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_JOB_LIST_NONE_CHECKED), UI_MSGBOX_OK, 1, 0, 0,
                               "메시지", 0x118, 0x78, 0);
        return;
    }
    length = rag_connection_get_packet_length(rag_connection_instance(), (short)packet.id);
    rag_connection_send_packet(rag_connection_instance(), length, &packet);
    ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_JOB_LIST);
}

int ui_job_list_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                    int val3, int val4)
{
    struct UIJobListWnd *self = (struct UIJobListWnd *)window;
    if (msg == UI_MSG_COMMAND) {
        if (val1 == JOB_LIST_ID_OK)
            send_jobs(self);
        else if (val1 == UI_ID_CLOSE)
            ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_JOB_LIST);
        return 0;
    }
    if (msg == UI_MSG_SET_INFO) {
        self->info = (struct WindowInfo *)val1;
        if (!job_list_info_valid(self->info))
            job_list_info_default(self->info);
        ui_frame_wnd_move(&self->frame.base, self->info->x, self->info->y);
        ui_job_list_wnd_send_msg(&self->frame.base, NULL, UI_MSG_RESIZE, 3, self->info->width, self->info->height, 0);
        ui_job_list_wnd_send_msg(&self->frame.base, NULL, UI_MSG_REBUILD, 0, 0, 0, 0);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

void job_list_set_wanted_jobs(const short *jobs)
{
    int i;

    tree_clear(&job_map, NULL);
    for (i = 0; i < JOB_LIST_SLOTS && jobs[i] >= 0; i++) {
        struct WantedJob wanted;

        wanted.job = jobs[i];
        wanted.checked = 1;
        tree_add(&job_map, &wanted, sizeof wanted, tree_less_int, NULL);
    }
}

static void job_map_release(void)
{
    tree_free(&job_map, NULL);
}

void job_map_init(void)
{
    tree_init(&job_map);
    atexit(job_map_release);
}
