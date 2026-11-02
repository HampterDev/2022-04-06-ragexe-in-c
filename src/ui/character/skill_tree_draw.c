#include "core/thread_text.h"
#include "ui/character/skill_tree.h"
#include "render/sprite_atlas.h"
#include "resources/palette.h"
#include "resources/sprite.h"
#include "skills/skill_info.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "platform/win_types.h"
#include "ui/character/skill_tree_internal.h"
#include "core/msg_str.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_talk.h"
#include "session/skill_levels.h"
#include "ui/draw_key.h"
#include "platform/clock.h"

#define MSG_SKILL_TREE_TITLE 0x11b
#define MSG_POINTS_LEFT_LIST 0x11c
#define MSG_PASSIVE 0x11e
#define MSG_POINTS_LEFT 0x927

#define TINT_GREY 0
#define TINT_RED 1
#define TINT_BLUE 3

#define ICON_FALLBACK UI_SKIN "item\\am_berserkpitcher.bmp"

static struct ResMgr *res(void)
{
    return g_resMgr();
}

static struct String *required_level_text(struct UINewSkillListWnd *self, struct String *out, int skill_id)
{
    struct SkillRequirement *required = skill_tree_level_list_find(&self->required, skill_id);
    char buf[0x10];

    if (required && required->level > 0) {
        _itoa(required->level, buf, 10);
        string_init(out);
        string_assign_cstr(out, buf);
        return out;
    }
    string_init(out);
    return out;
}

static int character_frame(DWORD anim_time, int frame_count)
{
    return (int)((float)(double)(DWORD)(platform_ticks_ms() - anim_time) / 300.0f) / frame_count;
}

static int character_due(struct UINewSkillListWnd *self)
{
    int i;

    for (i = 0; i < 2; i++) {
        int frame;

        if (!self->anim_frame_counts[i])
            continue;
        frame = character_frame(self->anim_time, self->anim_frame_counts[i]);
        if (frame >= 3 || frame != self->anim_frames[i])
            return 1;
    }
    return 0;
}

static void draw_character(struct UINewSkillListWnd *self)
{
    struct String imf, act_path[2], spr_path[2], body_pal, head_pal, tmp;
    struct SpriteRegion place = {0};
    int job, sex, body_palette, head_palette, head, i;

    string_init(&imf);
    for (i = 0; i < 2; i++) {
        string_init(&act_path[i]);
        string_init(&spr_path[i]);
    }
    string_init(&body_pal);
    string_init(&head_pal);
    memset(&place, 0, sizeof place);
    job = g_session1.job;
    sex = session_get_sex(&g_session1);
    head = g_session1.hairStyle;
    body_palette = g_session1.clothesColor;
    head_palette = g_session1.hairColor;
    session_get_body_spr_name_variant0(&tmp, job, sex, g_session1.body, 0);
    string_assign(&act_path[0], string_cstr(&tmp), tmp.size);
    string_free(&tmp);
    session_get_body_spr_name_variant1(&tmp, job, sex, g_session1.body, 0);
    string_assign(&spr_path[0], string_cstr(&tmp), tmp.size);
    string_free(&tmp);
    session_get_head_spr_name0(&tmp, job, sex, &head);
    string_assign(&act_path[1], string_cstr(&tmp), tmp.size);
    string_free(&tmp);
    session_get_head_spr_name1(&tmp, job, sex, &head);
    string_assign(&spr_path[1], string_cstr(&tmp), tmp.size);
    string_free(&tmp);
    if (body_palette) {
        session_get_body_pal_name(&tmp, job, sex, body_palette, g_session1.body, 0);
        string_assign(&body_pal, string_cstr(&tmp), tmp.size);
        string_free(&tmp);
    }
    if (head_palette) {
        session_get_head_pal_name(&tmp, head, job, sex, head_palette);
        string_assign(&head_pal, string_cstr(&tmp), tmp.size);
        string_free(&tmp);
    }
    session_get_imf_name(&tmp, job, sex);
    string_assign(&imf, string_cstr(&tmp), tmp.size);
    string_free(&tmp);

    self->anim_frame_counts[0] = self->anim_frame_counts[1] = 0;
    for (i = 0; i < 2; i++) {
        struct ActRes *act;
        struct SprRes *spr;
        struct Motion *motion;
        struct SprClip *clip;
        struct SprImg *img;
        DWORD *palette;
        void *tex;
        int frame, dx = 0, dy = 0, y, clip_top;

        res_mgr_get(res(), string_cstr(&imf));
        if (!act_path[i].size || !spr_path[i].size)
            continue;
        act = res_mgr_get(res(), string_cstr(&act_path[i]));
        spr = res_mgr_get(res(), string_cstr(&spr_path[i]));
        self->anim_frame_counts[i] = (int)actres_frame_count(act, 0x17);
        frame = character_frame(self->anim_time, self->anim_frame_counts[i]);
        if (frame >= 3) {
            frame = 0;
            self->anim_time = platform_ticks_ms();
        }
        self->anim_frames[i] = frame;
        motion = act_res_get_motion(act, 0x17, frame);
        if (session_is_doram_job(&g_session1, g_session1.job) == 1) {
            int accessory = 0, ox = 0, oy = 0;

            if (i == 4)
                accessory = g_session1.headBottom;
            else if (i == 3)
                accessory = g_session1.headTop;
            else if (i == 2)
                accessory = g_session1.headMid;

            client_lua_call(g_session1.luaState, "OffsetItemPos_GetOffsetForDoram", "ddd>dd", accessory, 0x17, session_get_sex(&g_session1), &ox, &oy);
            dx = ox;
            dy = oy;
        }
        if (i == 1) {
            struct Motion *body = act_res_get_motion(res_mgr_get(res(), string_cstr(&act_path[0])), 0x17, frame);

            if (animation_attachment_count(motion) && animation_attachment_count(body)) {
                const struct AttachPointInfo *b = animation_attachment(body, 0), *h = animation_attachment(motion, 0);

                if (h->attr == b->attr) {
                    dx += b->x - h->x;
                    dy += b->y - h->y;
                    g_skillWndHeadDx = dx;
                    g_skillWndHeadDy = dy;
                }
            }
        }
        palette = spr->palette;
        if (i == 0) {
            if (body_palette)
                palette = palette_resource_colors(res_mgr_get(res(), string_cstr(&body_pal)), spr->palette);
        } else if (i == 1) {
            if (head_palette)
                palette = palette_resource_colors(res_mgr_get(res(), string_cstr(&head_pal)), spr->palette);
        }
        clip = motion_get_clip(motion, i <= 1 ? i : 0);
        if (clip->sprIndex == -1)
            continue;
        img = spr_res_image(spr, clip);
        if (!img)
            continue;
        if (clip->clipType == 0) {
            tex = sprite_atlas_resolve(g_renderer, img, palette, &place);
        } else {
            tex = img->texture;
            place.x = place.y = 0;
        }
        if (!tex) continue;
        y = dy - 0x19 + WND(self)->height + clip->y;
        clip_top = y < 0 ? y : 0;
        ui_canvas_draw_texture(WND(self)->canvas, clip->x + dx + 0x30, y >= 0 ? y : 0, tex,
                             place.x, (int)((float)place.y - (float)clip_top), img->width,
                             img->height + clip_top, clip->flags, img->widthHalved + 1, img->heightHalved + 1);
    }
    string_free(&head_pal);
    string_free(&body_pal);
    for (i = 1; i >= 0; i--)
        string_free(&spr_path[i]);
    for (i = 1; i >= 0; i--)
        string_free(&act_path[i]);
    string_free(&imf);
}

static struct BitmapRes *skill_icon(struct SkillInfo *info, char *buf)
{
    struct BitmapRes *icon;

    session_format_item_bitmap_path(SKILL_INFO_ID_NAME(info), buf);
    icon = res_mgr_get(res(), buf);
    if (icon)
        return icon;
    sprintf(buf, ICON_FALLBACK);
    return res_mgr_get(res(), buf);
}

#define GREY(v) RGB((v), (v), (v))

static void draw_rows(struct UINewSkillListWnd *self)
{
    struct UIWindow *wnd = WND(self);
    struct List *list = skill_tree_shown_list(self);
    struct ListNode *node;
    char buf[0x50];
    int drawn = 0, row = 0;

    for (node = list->head->next; node != list->head; node = node->next, row++) {
        struct SkillInfo *info = LIST_VALUE(node, struct SkillInfo);
        struct BitmapRes *icon;
        struct SkillInfo at;
        int level, x, y, text_x, grey = 0, blue, red_green, known_level, sp, ap;
        BYTE level_use;

        if (row < self->top * self->columns)
            continue;
        level = info->level;
        y = drawn / self->columns * ROW_H;
        x = drawn % self->columns * ROW_H;
        if (self->hover == row) {
            int r, g, b;

            ui_skin_color_chip(&g_skinMgr, 6, 2, &r, &g, &b);
            if (info->level == 0)
                ui_skin_color_chip(&g_skinMgr, 0xa, 2, &r, &g, &b);
            else if (info->sp_cost == 0)
                ui_skin_color_chip(&g_skinMgr, 0xe, 2, &r, &g, &b);
            ui_window_fill_rect(wnd, x + 0x68, y + 0x20, 0xa4, 0x1c, RGB((BYTE)r, (BYTE)g, (BYTE)b));
        }
        icon = skill_icon(info, buf);
        if (icon) {
            if (level == 0) {
                grey = 0x78;
                ui_window_draw_bitmap_tinted(wnd, TINT_GREY, x + 0x2c, y + 0x22, icon, 1, 0);
            } else {
                ui_window_draw_bitmap(wnd, x + 0x2c, y + 0x22, icon, 1);
            }
        }
        skill_tree_skill_at(self, &at, row, 1);
        known_level = at.level;
        skill_info_destruct(&at);
        blue = info->level > known_level ? 0xff : grey;
        red_green = info->level <= known_level ? grey : 0;
        text_x = x + 0x70;
        level_use = skill_info_is_level_used(info->id);
        if (level) {
            int lx;

            sprintf(buf, g_serviceType == 0xc ? "Nv :" : "Lv :");
            lx = !self->pending && level_use ? text_x + 0xc : text_x + 1;
            ui_window_text_out(wnd, lx, y + 0x2f, buf, 0, 0, 0xc, GREY(grey), 0, 0);
            if (level_use) {
                int chosen = session_get_skill_table_value(&g_session1, info->id), cx;

                if (chosen <= 0 || chosen > info->level)
                    chosen = info->level;
                cx = text_x + (self->pending == 0 ? 0x25 : 0x19);
                sprintf(buf, "%2d", chosen);
                ui_window_text_out(wnd, cx, y + 0x2f, buf, 0, 0, 0xc, chosen != info->level ? 0xff : GREY(grey),
                                 0, 0);
                cx += ui_window_text_width(wnd, buf, 0, 0, 0xc, 0, 0) + 7;
                ui_window_text_out(wnd, cx, y + 0x2f, "/ ", 0, 0, 0xc, GREY(grey), 0, 0);
                cx += ui_window_text_width(wnd, "/ ", 0, 0, 0xc, 0, 0);
                sprintf(buf, "%2d", info->level);
                ui_window_text_out(wnd, cx, y + 0x2f, buf, 0, 0, 0xc, RGB(red_green, red_green, blue), 0, 0);
            } else {
                sprintf(buf, "%d", info->level);
                ui_window_text_out(wnd, text_x + 0x19, y + 0x2f, buf, 0, 0, 0xc,
                                 RGB(red_green, red_green, blue), 0, 0);
            }
        }
        if (level) {
            if (info->type == 0) {
                sprintf(buf, msg_str(MSG_PASSIVE));
            } else {
                int found = 0;

                sp = info->sp_cost;
                if (level_use) {
                    struct List *own = skill_tree_tab_count(self) - 1 == self->tab ? &g_session1.skills.extra_skills
                                                                            : player_skill_bundle_tree_tab(&g_session1.skills, self->tab);
                    struct ListNode *n;

                    for (n = own->head->next; n != own->head; n = n->next) {
                        const struct SkillInfo *have = LIST_VALUE(n, struct SkillInfo);

                        if (have->id == info->id && have->level) {
                            found = 1;
                            break;
                        }
                    }
                }
                if (found) {
                    int chosen = session_get_skill_table_value(&g_session1, info->id);
                    int amount = skill_info_get_sp_amount(info, chosen);

                    if (amount && chosen <= info->level)
                        sp = amount;
                } else {
                    sp = skill_info_get_sp_amount(info, info->level);
                }
                ap = skill_info_get_ap_amount(info, info->level);
                if (session_is_space(&g_session1, info->id)) {
                    if (ap)
                        sprintf(buf, "Sp : %d%%, Ap : %d", sp, ap);
                    else
                        sprintf(buf, "Sp : %d%%", sp);
                } else {
                    if (ap)
                        sprintf(buf, "Sp : %d, Ap : %d", sp, ap);
                    else
                        sprintf(buf, "Sp : %d", sp);
                }
            }
            ui_window_text_out(wnd, text_x + 0x64, y + 0x2f, buf, 0, 0, 0xc, GREY(grey), 0, 0);
        }
        ui_window_text_out(wnd, text_x, y + 0x23, skill_info_get_name(info), 0, 0, 0xc, GREY(grey), 0, 0);
        if (++drawn == self->rows * self->columns)
            return;
    }
}

static void draw_cells(struct UINewSkillListWnd *self)
{
    struct UIWindow *wnd = WND(self);
    struct BitmapRes *frame = ui_skin_bitmap(BASIC_INTERFACE "no_skill.bmp");
    struct List *list;
    struct ListNode *node;
    char buf[0x50];
    int i;

    for (i = 0; i < SKILL_TREE_CELLS; i++)
        ui_window_draw_bitmap(wnd, i % self->columns * CELL_W + 0x4c, i / self->columns * CELL_H + 0x2c, frame, 1);
    list = skill_tree_shown_list(self);
    for (node = list->head->next; node != list->head; node = node->next) {
        struct SkillInfo *info = LIST_VALUE(node, struct SkillInfo), *known, at;
        struct BitmapRes *icon;
        int slot = SKILL_INFO_SLOT(info), first, cell, x, y, level, grey = 0x66, max_level;
        int red, green, blue, icon_x, text_y, w, offset;
        int marked = 0;
        BYTE level_use;

        first = self->top * self->columns;
        if (slot == -1 || slot < first || slot - first >= SKILL_TREE_CELLS)
            continue;
        cell = slot - first;
        level = info->level;
        x = cell % self->columns * CELL_W + 0x48;
        y = cell / self->columns * CELL_H;
        if (!skill_tree_is_last_tab(self, self->tab))
            marked = skill_tree_level_list_find(&self->required, info->id) != NULL;
        if (marked) {
            ui_window_fill_rect(wnd, x + 3, y + 0x2b, 0x1a, 0x1a, 0xccc0ff);
            ui_window_draw_round_frame(wnd, x + 2, y + 0x2a, 0x1c, 0x1c, 0xc1b6ff);
        } else if (self->points_left && info->found && SKILL_INFO_UPGRADABLE(info)) {
            ui_window_fill_rect(wnd, x + 3, y + 0x2b, 0x1a, 0x1a, 0xf0c4af);
            ui_window_draw_round_frame(wnd, x + 2, y + 0x2a, 0x1c, 0x1c, 0xf0c4af);
        } else {
            ui_window_fill_rect(wnd, x + 3, y + 0x2b, 0x1a, 0x1a, 0xffffff);
        }
        icon = skill_icon(info, buf);
        if (icon) {
            icon_x = x + 4;
            if (!skill_tree_is_last_tab(self, self->tab)
                && skill_tree_level_list_find(&self->required, info->id) != NULL) {
                struct String need;

                ui_window_draw_bitmap_tinted(wnd, TINT_RED, icon_x, y + 0x2c, icon, 1, 1);
                required_level_text(self, &need, info->id);
                if (need.size)
                    ui_window_text_out_outlined(wnd, x + 0x1e, y + 0x3a, string_cstr(&need), 0, 0xffffff, 0, 0,
                                             0xb, 1, 0);
                string_free(&need);
            } else if (self->points_left && info->found && SKILL_INFO_UPGRADABLE(info)) {
                ui_window_draw_bitmap_tinted(wnd, TINT_BLUE, icon_x, y + 0x2c, icon, 1, 1);
            } else if (level == 0) {
                grey = 0x78;
                ui_window_draw_bitmap_tinted(wnd, TINT_GREY, icon_x, y + 0x2c, icon, 1, 1);
            } else {
                ui_window_draw_bitmap(wnd, icon_x, y + 0x2c, icon, 1);
            }
            {
                int planned = skill_tree_planned_level(self, info->id);

                known = player_skill_bundle_find_tree_skill(&g_session1.skills, info->id);
                if (known) {
                    int extra = planned - SKILL_INFO_BASE_LEVEL(known);

                    extra = extra >= 0 ? extra : 0;
                    if (extra > 0)
                        ui_window_text_out(wnd, x + 0x1e, y + 0x2c, thread_text_format("+%d", extra), 0, 0, 0xb,
                                         0xff, 0, 0);
                }
            }
        }
        skill_tree_skill_at(self, &at, slot, 1);
        max_level = at.level;
        skill_info_destruct(&at);
        red = green = blue = grey;
        if (info->level > max_level) {
            red = 0x7f;
            blue = 0xff;
            green = 0x3c;
        }
        icon_x = x + 4;
        text_y = y + 0x46;
        level_use = skill_info_is_level_used(info->id);
        if (level) {
            if (level_use) {
                int chosen = session_get_skill_table_value(&g_session1, info->id), shown, tx;
                struct SkillInfo *last;

                if (chosen <= 0 || chosen > info->level)
                    chosen = info->level;
                sprintf(buf, "%d / %d", chosen, info->level);
                w = ui_window_text_width(wnd, buf, 0, 0, 0xb, 1, 0);
                tx = (0x18 - w) / 2 + icon_x;
                sprintf(buf, "%d", chosen);
                ui_window_text_out_outlined(wnd, tx, text_y, buf, 0,
                                         chosen == info->level ? GREY(grey) : 0x7f3cff, 0xffffff, 0, 0xb, 1, 0);
                tx += ui_window_text_width(wnd, buf, 0, 0, 0xb, 1, 0);
                ui_window_text_out_outlined(wnd, tx, text_y, " / ", 0, GREY(grey), 0xffffff, 0, 0xb, 1, 0);
                tx += ui_window_text_width(wnd, " / ", 0, 0, 0xb, 1, 0);
                shown = skill_tree_planned_level(self, info->id);
                if (shown <= player_skill_bundle_tree_skill_level(&g_session1.skills, info->id))
                    shown = player_skill_bundle_tree_skill_level(&g_session1.skills, info->id);
                last = player_skill_bundle_find_extra_skill(&g_session1.skills, info->id);
                if (last && shown <= last->level)
                    shown = last->level;
                _snprintf_s(buf, 0x50, (size_t)-1, "%d", shown);
                ui_window_text_out_outlined(wnd, tx, text_y, buf, 0, RGB(red, green, blue), 0xffffff, 0, 0xb, 1,
                                         0);
            } else {
                sprintf(buf, "%d", info->level);
                w = ui_window_text_width(wnd, buf, 0, 0, 0xb, 1, 0);
                ui_window_text_out_outlined(wnd, (0x18 - w) / 2 + icon_x, text_y, buf, 0, RGB(red, green, blue),
                                         0xffffff, 0, 0xb, 1, 0);
            }
        }
        w = ui_window_text_width(wnd, skill_info_get_name(info), 0, 0, 0xb, 0, 0);
        offset = w < 0x30 ? (0x30 - w) / 2 : 0;
        {
            struct String shortened = {0};
            ui_window_text_out(wnd, x - 8 + offset, y + 0x1e,
                             ui_window_shorten_text(wnd, &shortened, skill_info_get_name(info), 0x30, 0, 0xb, 0),
                             0, 0, 0xb, GREY(grey), 0, 0);
            string_free(&shortened);
        }
        if (player_skill_bundle_is_level_boosted(&g_session1.skills, info->id) == 1)
            ui_window_draw_bitmap(wnd, x + 8, y + 0x3a, ui_skin_bitmap(BASIC_INTERFACE "item_skill.bmp"), 1);
    }
}

static unsigned long long skill_tree_key(struct UINewSkillListWnd *self)
{
    struct UIWindow *wnd = WND(self);
    struct List *list = skill_tree_shown_list(self);
    struct ListNode *node;
    unsigned long long key = DRAW_KEY_START;
    int first = self->top * self->columns;

    key = draw_key_int(key, wnd->width);
    key = draw_key_int(key, wnd->height);
    key = draw_key_int(key, self->top_margin);
    key = draw_key_int(key, self->bottom_margin);
    key = draw_key_int(key, self->tab);
    key = draw_key_int(key, self->top);
    key = draw_key_int(key, self->columns);
    key = draw_key_int(key, self->rows);
    key = draw_key_int(key, self->hover);
    key = draw_key_int(key, self->pending);
    key = draw_key_int(key, self->points_left);
    key = draw_key_int(key, g_session1.skillPoints);
    key = draw_key_int(key, g_session1.job);
    key = draw_key_int(key, session_get_sex(&g_session1));
    key = draw_key_int(key, g_session1.hairStyle);
    key = draw_key_int(key, g_session1.body);
    key = draw_key_int(key, g_session1.clothesColor);
    key = draw_key_int(key, g_session1.hairColor);
    for (node = list->head->next; node != list->head; node = node->next) {
        struct SkillInfo *info = LIST_VALUE(node, struct SkillInfo), *known, *extra, at;
        struct SkillRequirement *required;
        int slot = SKILL_INFO_SLOT(info);

        if (slot == -1 || slot < first || slot - first >= SKILL_TREE_CELLS)
            continue;
        required = skill_tree_level_list_find(&self->required, info->id);
        known = player_skill_bundle_find_tree_skill(&g_session1.skills, info->id);
        extra = player_skill_bundle_find_extra_skill(&g_session1.skills, info->id);
        key = draw_key_skill(key, info);
        key = draw_key_int(key, required ? required->level : -1);
        key = draw_key_int(key, skill_tree_planned_level(self, info->id));
        key = draw_key_int(key, known ? SKILL_INFO_BASE_LEVEL(known) : -1);
        key = draw_key_int(key, extra ? extra->level : -1);
        key = draw_key_int(key, player_skill_bundle_tree_skill_level(&g_session1.skills, info->id));
        key = draw_key_int(key, session_get_skill_table_value(&g_session1, info->id));
        key = draw_key_int(key, player_skill_bundle_is_level_boosted(&g_session1.skills, info->id));
        skill_tree_skill_at(self, &at, slot, 1);
        key = draw_key_int(key, at.level);
        skill_info_destruct(&at);
    }
    return draw_key_children(key, wnd);
}

BYTE ui_new_skill_list_wnd_render(struct UIWindow *window, int force)
{
    struct UINewSkillListWnd *self = (struct UINewSkillListWnd *)window;

    window->redraws_tree = !g_session1.simpleSkillWnd;
    if (window->redraws_tree && (skill_tree_key(self) != self->drawn_key || character_due(self)))
        ui_window_invalidate_base(window);
    return ui_window_render_base(window, force);
}

void ui_new_skill_list_wnd_on_draw(struct UIWindow *window)
{
    struct UINewSkillListWnd *self = (struct UINewSkillListWnd *)window;
    struct UIWindow *wnd = WND(self);
    int left = self->tabs->base.width, n, x;
    struct BitmapRes *bmp;
    char buf[0x50];

    ui_window_clear_canvas(wnd, UI_TRANSPARENT);
    ui_window_fill_rect(wnd, left, self->top_margin, wnd->width - left,
                      wnd->height - self->bottom_margin - self->top_margin, 0xffffff);
    ui_window_draw_round_frame(wnd, left, 0, wnd->width - left, wnd->height, 0xc3c3c3);
    ui_window_draw_round_frame(wnd, left + 2, 2, wnd->width - left - 4, wnd->height - 4, 0xc3c3c3);
    ui_window_draw_bitmap(wnd, left, 0, ui_skin_bitmap(BASIC_INTERFACE "skill_upbar_left.bmp"), 0);
    bmp = ui_skin_bitmap(BASIC_INTERFACE "skill_upbar_right.bmp");
    ui_window_draw_bitmap(wnd, wnd->width - bmp->width, 0, bmp, 0);
    n = (wnd->width - left - 0x18) / 5 + ((wnd->width - left - 0x18) % 5 != 0);
    for (x = left + 0xc; n > 0; n--, x += 5)
        ui_window_draw_bitmap(wnd, x, 0, ui_skin_bitmap(BASIC_INTERFACE "skill_upbar_mid.bmp"), 0);
    bmp = ui_skin_bitmap(BASIC_INTERFACE "skill_botbar_left.bmp");
    ui_window_draw_bitmap(wnd, left, wnd->height - bmp->height, bmp, 0);
    bmp = ui_skin_bitmap(BASIC_INTERFACE "skill_botbar_right.bmp");
    ui_window_draw_bitmap(wnd, wnd->width - bmp->width, wnd->height - bmp->height, bmp, 0);
    bmp = ui_skin_bitmap(BASIC_INTERFACE "skill_botbar_mid.bmp");
    n = (wnd->width - left - 0x18) / bmp->width + ((wnd->width - left - 0x18) % bmp->width != 0);
    for (x = left + 0xc; n > 0; n--, x += 5)
        ui_window_draw_bitmap(wnd, x, wnd->height - bmp->height, bmp, 0);
    ui_window_text_out(wnd, 0x29, 6, msg_str(MSG_SKILL_TREE_TITLE), 0, 0, 0xc, 0xffffff, 0, 0);
    ui_window_text_out(wnd, 0x28, 5, msg_str(MSG_SKILL_TREE_TITLE), 0, 0, 0xc, 0, 0, 0);
    memset(buf, 0, sizeof buf);
    if (g_session1.simpleSkillWnd) {
        sprintf(buf, msg_str(MSG_POINTS_LEFT_LIST), self->points_left);
        ui_window_text_width(wnd, buf, 0, 0, 0xc, 0, 0);
        ui_window_text_out(wnd, 0x1e, wnd->height - 0x17, buf, 0, 0, 0xc, 0, 0, 0);
        draw_rows(self);
    } else {
        struct String text;
        int total = g_session1.skillPoints, left_points = self->points_left;

        if (left_points < total)
            _snprintf_s(buf, 0x50, (size_t)-1, "%s^0000ff%d^000000(^ff0000%d^000000) / %d",
                           msg_str(MSG_POINTS_LEFT), left_points, total - left_points, total);
        else
            _snprintf_s(buf, 0x50, (size_t)-1, "%s%d", msg_str(MSG_POINTS_LEFT), total);
        string_init(&text);
        string_assign_cstr(&text, buf);
        ui_window_text_out_wrapped(wnd, 0x50, wnd->height - 0x17, &text, 0, 0, 0, 0, 0xc, 0, 0);
        string_free(&text);
        draw_cells(self);
        draw_character(self);
        self->drawn_key = skill_tree_key(self);
    }
    self->dirty = 0;
}
