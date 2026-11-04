#include "contents/player_names.h"
#include "session/state.h"
#include "game/context.h"
#include "core/markup_tags.h"
#include "ui/social/party_windows.h"
#include "core/msg_str.h"
#include "session/session_items.h"
#include "session/session_names.h"
#include "session/session_party.h"
#include "session/session_state.h"

static struct BitmapRes *row_bitmap(const char *path)
{
    return ui_skin_bitmap(path);
}

static void draw_party_row(struct UIMessengerGroupWnd *self, const struct String *base,
                           const struct PartyMember *n, int index, int x, int y, COLORREF row_colour)
{
    struct UIWindow *wnd = &self->frame.base;
    struct PartyRowLayout *l = &PARTY_LAYOUT(&self->layouts, self->mode);
    struct String job_name, map;
    struct String fitted = {0};
    struct PartyMember m;
    struct PlateRecord rec;
    char text[0x100], path[0x180], hp[0x100], level[0x50];
    struct BitmapRes *bmp, *bg;
    COLORREF level_colour, name_colour;
    const char *s;
    int len;

    ui_messenger_group_wnd_get_member(self, &m, index);
    session_get_plate_record(&g_session1, &rec, m.aid);
    if (rec.found == 1)
        ui_window_fill_rect(wnd, 0, y + 2, wnd->width, self->row_height, 0xffe8dd);
    if (self->hover == index)
        ui_window_fill_rect(wnd, 0, y + 2, wnd->width, self->row_height, ui_skin_colour(6, 2));

    s = session_get_job_name(&g_session1, n->job, 0x63);
    string_init(&job_name);
    string_assign_cstr(&job_name, s);
    ui_bitmap_button_set_tool_tip(&self->job_icons[index]->bmp, string_cstr(&job_name));
    ui_window_move_base(&self->job_icons[index]->bmp.button.base, x, l->y + y);
    if (n->role == 0) {
        sprintf(path, "%sico_partyCrown.bmp", string_cstr(base));
        bmp = row_bitmap(path);
        ui_window_draw_bitmap(wnd, x + 2, PARTY_LAYOUT(&self->layouts, self->mode).y + y - 8, bmp, 1);
    }

    sprintf(level, "Lv.%d", n->level);
    if (n->map_name.size && n->state == 0) {
        s = session_map_display_name(string_cstr(&n->map_name));
        string_init(&map);
        string_assign_cstr(&map, s);
        sprintf(text, "%s(%s)", string_cstr(&n->name), string_cstr(&map));
        string_free(&map);
    } else {
        sprintf(text, "%s", string_cstr(&n->name));
    }
    ui_window_fit_text(wnd, &fitted, (int)((double)wnd->width * 0.8 - 80.0), text, 0, 0xc, 1);
    if (n->state == 1) {
        level_colour = name_colour = 0xa38a86;
    } else {
        level_colour = 0x493a37;
        name_colour = row_colour;
    }
    l = &PARTY_LAYOUT(&self->layouts, self->mode);
    ui_window_text_out(wnd, x + 0x1c, l->y + y + 3, level, (int)strlen(level), 0, 0xc, level_colour, 1, 0);
    len = (int)strlen(level);
    ui_window_text_out(wnd, x + 0x21 + ui_window_text_width(wnd, level, len, 0, 0xc, 1, 0), l->y + y + 3,
                     string_cstr(&fitted), fitted.size, 0, 0xc, name_colour, 1, 0);

    if (n->state == 0) {
        struct UIPcGage *gauge = self->gauges[index];

        if (gauge->gage.base.x != -100) {
            sprintf(hp, "%d/%d", gauge->hp, gauge->max_hp);
            ui_window_text_out(wnd, gauge->gage.base.x + 0x42, gauge->gage.base.y - 2, hp, (int)strlen(hp), 0, 0xa, 0, 0,
                             0);
        }
    }

    memset(text, 0, sizeof(text));
    if (n->state != 0)
        sprintf(text, "%sicon_party_off.bmp", string_cstr(base));
    else if (n->aid == g_session1.ownAid)
        sprintf(text, "%sicon_party_me.bmp", string_cstr(base));
    else
        sprintf(text, "%sicon_party_on.bmp", string_cstr(base));
    bmp = row_bitmap(text);
    ui_window_draw_bitmap(wnd, (int)((float)wnd->width * 0.8f),
                        (int)((float)self->row_height * 0.65f + (float)y), bmp, 1);

    memset(text, 0, sizeof(text));
    sprintf(text, "%sbg_partymember.bmp", string_cstr(base));
    bg = row_bitmap(text);
    ui_window_text_out(wnd, wnd->width - 0x60, wnd->height - 0x10, msg_str(0xc9f), 0, 0, 0xc, 0, 0, 0);
    ui_window_draw_bitmap(wnd, wnd->width - 0x3a, wnd->height - 0x12, bg, 1);
    memset(text, 0, sizeof(text));
    sprintf(text, "%d/%d", session_get_party_member_count(&g_session1), 0xc);
    ui_window_text_out(wnd, wnd->width - 0x33, wnd->height - 0x10, text, (int)strlen(text), 0, 0xc, 0xffffff,
                     0, 0);

    string_free(&fitted);
    string_free(&job_name);
    party_member_destroy(&m);
}

static void draw_friend_row(struct UIMessengerGroupWnd *self, const struct PartyMember *member, int index,
                            int x, int y, COLORREF row_colour)
{
    struct UIWindow *wnd = &self->frame.base;
    struct PartyMember f;
    struct String name, cut;
    char text[0x50], path[0x100];
    int pos;

    session_get_friend_by_index(&g_session1, &f, index);
    if (game_current_base_mode() && f.found) {
        struct TextView cached_name = player_name_lookup(f.char_id);
        string_assign(&f.name, cached_name.data, cached_name.size);
    }
    if (f.name.size == 0) {
        const char *unknown = msg_str(0x245);

        string_init(&name);
        string_reserve(&name, strlen(unknown) + text_tag_NAMELESS.size);
        string_append_cstr(&name, unknown);
        string_append(&name, string_cstr(&text_tag_NAMELESS), text_tag_NAMELESS.size);
        string_free(&f.name);
        f.name = name;
    } else if (!session_has_friend_named(&g_session1, string_cstr(&f.name))) {
        session_rename_friend(&g_session1, f.char_id, string_cstr(&f.name));
    }

    memset(text, 0, sizeof(text));
    string_copy(&cut, &f.name);
    pos = text_view_find(string_view(&f.name), string_view(&text_tag_NAMELESS), 0);
    if (pos != -1)
        string_assign(&cut, string_cstr(&f.name), f.name.size < (unsigned)pos ? f.name.size : pos);
    sprintf(text, "%s", string_cstr(&cut));
    if (self->hover == index)
        ui_window_fill_rect(wnd, x + 0x11, y + 4, ui_window_text_width(wnd, text, 0, 0, 0xc, 0, 0) + 3, 0x10,
                          ui_skin_colour(6, 2));
    ui_window_text_out(wnd, x + 0x13, y + 6, text, (int)strlen(text), 0, 0xc, row_colour, 0, 0);
    if (member->state == 0) {
        sprintf(path, "%s", UI_SKIN "basic_interface\\grp_online.bmp");
        ui_window_draw_bitmap(wnd, x + 2, y + 4, row_bitmap(path), 1);
    }
    string_free(&cut);
    party_member_destroy(&f);
}

void ui_messenger_group_wnd_on_draw(struct UIWindow *window)
{
    struct UIMessengerGroupWnd *self = (struct UIMessengerGroupWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct ListNode *node;
    struct BitmapRes *bmp;
    struct String base;
    char path[0x100];
    int tiles, tx, shown = 0, i = 0;

    ui_window_draw_title_bar(wnd, 1, "", 0);
    ui_window_fill_rect(wnd, 0, 0x11, wnd->width, wnd->height - 0x26, ui_skin_colour(2, 2));
    ui_window_draw_outline(wnd, 0, 0x11, wnd->width, wnd->height - 0x26, 0xc0c0c0);
    ui_window_draw_button_bar(wnd, 0, 0, 1);
    ui_window_draw_bitmap(wnd, 0, wnd->height - 0x28, ui_skin_bitmap(UI_SKIN "basic_interface\\mesbtn_left.bmp"),
                        0);
    bmp = ui_skin_bitmap(UI_SKIN "basic_interface\\mesbtn_right.bmp");
    ui_window_draw_bitmap(wnd, wnd->width - bmp->width, wnd->height - 0x28, bmp, 0);
    tiles = (wnd->width - 14) / 7 + ((wnd->width - 14) % 7 != 0);
    for (tx = 7; tiles > 0; tiles--, tx += 7)
        ui_window_draw_bitmap(wnd, tx, wnd->height - 0x28,
                            ui_skin_bitmap(UI_SKIN "basic_interface\\mesbtn_mid.bmp"), 0);

    string_init(&base);
    string_assign(&base, RENEWAL_PARTY, 0x1c);
    if (self->tab == 0) {
        memset(path, 0, sizeof(path));
        snprintf(path, sizeof(path), "%simg_friend1.bmp", string_cstr(&base));
        bmp = ui_skin_bitmap(path);
        if (self->mode == 0)
            ui_window_draw_bitmap(wnd, 0x93, 0x24, bmp, 1);
        snprintf(path, sizeof(path), "%simg_friend2.bmp", string_cstr(&base));
        bmp = ui_skin_bitmap(path);
        if (self->mode == 0)
            ui_window_draw_bitmap(wnd, 0x63, 0xc1, bmp, 1);
    }
    for (tx = 0; tx < PARTY_WND_ROWS; tx++)
        ui_window_move_base(&self->job_icons[tx]->bmp.button.base, -100, -100);

    for (node = self->rows.head->next; node != self->rows.head; node = node->next, i++) {
        const struct PartyMember *member = LIST_VALUE(node, struct PartyMember);
        unsigned c = member->color;
        COLORREF row_colour;
        int x, y;

        if (i < self->first * self->cols)
            continue;
        x = shown % self->cols * 32 + PARTY_LAYOUT(&self->layouts, self->mode).x;
        y = self->row_height * (shown / self->cols) + 0x11;
        row_colour = ((c >> 16) & 0xff) | ((c & 0xff) << 16) | (c & 0xff00);
        if (self->tab == 1)
            draw_party_row(self, &base, member, i, x, y, row_colour);
        else
            draw_friend_row(self, member, i, x, y, row_colour);
        shown++;
        if (shown == self->visible * self->cols)
            break;
    }
    string_free(&base);
}
