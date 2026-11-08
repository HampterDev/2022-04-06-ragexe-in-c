#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "actor/object_ops.h"
#include "core/object_type.h"
#include "effects/skills/skill_state.h"
#include "parity/parity_world.h"
#include "game/context.h"
#include "game/ground_item_state.h"
#include "iteminfo/item_api.h"
#include "resources/attr_types.h"
#include "std/list.h"
#include "std/string.h"
#include "ui/chat/balloon.h"
#include "ui/controls/buttons.h"
#include "ui/controls/check_radio.h"
#include "ui/controls/combo_box.h"
#include "ui/controls/edit_types.h"
#include "ui/controls/line_info.h"
#include "ui/controls/listbox.h"
#include "ui/controls/scroll.h"
#include "ui/controls/static.h"
#include "ui/controls/tabs.h"
#include "ui/controls/text_viewer.h"
#include "ui/quests/tag_text_viewer.h"
#include "ui/render/graphic_setting.h"
#include "ui/window_access.h"
#include "ui/window_manager_state.h"
#include "ui/window_messages.h"
#include "ui/window_types.h"
#include "world/mode_manager.h"
#include "world/pick_node.h"
#include "world/state.h"

#define MAX_DEPTH 6
#define MAX_NODES 4096

static void text_piece(struct ParityControl *c, const char *text, size_t length)
{
    if (c->pieces < PARITY_TEXT_PIECES) {
        c->text[c->pieces] = text ? text : "";
        c->length[c->pieces] = text ? length : 0;
        c->pieces++;
    }
}

static void write_control(FILE *f, const struct UIWindow *w, int kind)
{
    struct ParityControl c;
    size_t i;
    if (kind == PARITY_PLAIN)
        return;
    memset(&c, 0, sizeof c);
    c.kind = kind;
    switch (kind) {
    case PARITY_CHECK_BOX:
        c.value[0] = w->state;
        c.value[1] = ((const struct UICheckBox *)w)->disabled;
        break;
    case PARITY_RADIO:
    case PARITY_TAB:
        c.value[0] = w->state;
        break;
    case PARITY_SCROLL_BAR: {
        const struct UIScrollBar *bar = (const struct UIScrollBar *)w;
        c.value[0] = bar->pos;
        c.value[1] = bar->max_pos;
        c.value[2] = bar->page;
        c.value[3] = bar->total;
        c.timed = w->parent && parity_timed_bar_parent(object_type_name(w->parent->ops)) &&
                  ((const struct UIGraphicSettingWnd *)w->parent)->bgm_volume == bar;
        break;
    }
    case PARITY_EDIT: {
        const struct UIEditCtrl *edit = (const struct UIEditCtrl *)w;
        c.masked = edit->mask_char != 0;
        text_piece(&c, edit->text.data, edit->text.size);
        c.value[0] = edit->sel_start;
        c.value[1] = edit->sel_end;
        break;
    }
    case PARITY_TAB_CONTROL:
        c.value[0] = ((const struct UITabControl *)w)->selected;
        break;
    case PARITY_LIST_BOX:
        c.value[0] = ((const struct UIListBox *)w)->selected;
        c.value[1] = ((const struct UIListBox *)w)->top;
        break;
    case PARITY_LIST_BOX_EX:
        c.value[0] = ((const struct UIListBoxEx *)w)->selected;
        c.value[1] = ((const struct UIListBoxEx *)w)->top;
        break;
    case PARITY_COMBO:
        c.value[0] = ((const struct UIComboBoxDefWnd *)w)->open;
        break;
    case PARITY_STATIC_TEXT:
        text_piece(&c, ((const struct UIStaticText *)w)->text.data, ((const struct UIStaticText *)w)->text.size);
        break;
    case PARITY_BALLOON: {
        const struct TextArray *lines = &((const struct UIBalloonText *)w)->lines;
        for (i = 0; i < lines->size; i++)
            text_piece(&c, lines->items[i].data, lines->items[i].size);
        break;
    }
    case PARITY_TEXT_BUTTON: {

        const struct UITextButton *button = (const struct UITextButton *)w;
        text_piece(&c, button->label.data, button->label.size);
        for (i = 0; i < button->lines.size; i++)
            text_piece(&c, button->lines.items[i].data, button->lines.items[i].size);
        break;
    }
    case PARITY_BMP_TEXT_BUTTON:
        text_piece(&c, ((const struct UIBmpTextButton *)w)->caption.data,
                   ((const struct UIBmpTextButton *)w)->caption.size);
        break;
    case PARITY_LINE_TEXT:
        text_piece(&c, ((const struct LineCutterText *)w)->text.data, ((const struct LineCutterText *)w)->text.size);
        break;
    case PARITY_TEXT_VIEWER: {
        const struct UITextViewer *viewer = (const struct UITextViewer *)w;
        size_t count = VEC_COUNT(&viewer->list.items, struct String);
        c.hash = PARITY_HASH_START;
        for (i = 0; i < count; i++) {
            const struct String *line = &VEC_AT(&viewer->list.items, struct String, i);
            text_piece(&c, line->data, line->size);
            c.hash = parity_text_hash(c.hash, (int)i, line->data ? line->data : "", line->data ? line->size : 0);
        }
        c.value[0] = (int)count;
        c.value[1] = (int)VEC_COUNT(&viewer->embeds, struct UIWindow *);
        break;
    }
    case PARITY_TAG_TEXT_VIEWER: {

        const struct String *text = &((const struct UITagTextViewer *)w)->text;
        text_piece(&c, text->data, text->size);
        c.size = text->data ? text->size : 0;
        c.hash = parity_text_hash(PARITY_HASH_START, 0, text->data ? text->data : "", c.size);
        break;
    }
    }
    parity_control_write(f, &c);
}

static void write_window(FILE *f, struct UIWindow *w, int depth, int *budget)
{
    struct ListNode *head, *n;
    const char *cls;
    if (--*budget < 0 || !w)
        return;
    cls = object_type_name(w->ops);
    fprintf(f, "%*s%s@@ id=%u rect=%d,%d,%d,%d shown=%d%s%s", depth * 2, "", cls, (unsigned)w->id, w->x, w->y,
            w->width, w->height, w->visible ? 1 : 0, w == g_windowMgr.modal_wnd ? " modal" : "",
            w == g_windowMgr.new_chat_wnd ? " chat" : "");
    write_control(f, w, parity_control_kind(cls));
    fputc('\n', f);
    head = w->children.head;
    if (depth >= MAX_DEPTH || !head)
        return;
    for (n = head->next; n && n != head && *budget > 0; n = n->next)
        write_window(f, *LIST_VALUE(n, struct UIWindow *), depth + 1, budget);
}

static void write_picks(FILE *f, const struct ActorPickNode *node, int depth, int *budget)
{
    const struct ListNode *head, *n;
    int i;
    if (depth > 4 || !node || !(head = node->boxes.head))
        return;
    for (n = head->next; n && n != head && --*budget > 0; n = n->next) {
        const struct PickInfo *p = *LIST_VALUE(n, struct PickInfo *const);
        if (!p)
            continue;
        fprintf(f, "actor id=%u job=%d kind=%d rect=%d,%d,%d,%d\n", (unsigned)p->aid, p->job, p->kind,
                (int)p->left, (int)p->top, (int)(p->right - p->left), (int)(p->bottom - p->top));
    }
    for (i = 0; i < 4; i++)
        if (node->child[i])
            write_picks(f, node->child[i], depth + 1, budget);
}

static struct ParityWorld g_world;

static void at_cell(const struct C3dAttr *attr, const Vec3 *pos, int *cell_x, int *cell_y)
{
    parity_world_cell(pos->x, pos->z, attr ? attr->width : 0, attr ? attr->height : 0, attr ? attr->zoom : 0,
                      cell_x, cell_y);
}

static void actor_record(struct ParityActor *r, const struct GameActor *a, const struct C3dAttr *attr)
{
    r->id = (unsigned)a->aid;
    r->job = a->job;
    r->kind = a->object_type;
    r->cell_x = a->cell_x;
    r->cell_y = a->cell_y;
    at_cell(attr, &a->pos, &r->at_x, &r->at_y);
    r->state = a->state;
    r->action = a->action;
    r->held = a->dies_after_hits;
    r->opt = (unsigned)a->effect_state;
    r->vanishing = a->fade_until != 0;
    r->fx = a->effects.size;
}

static void write_world(FILE *f, const struct World *world)
{
    struct ParityWorld *w = &g_world;
    const struct C3dAttr *attr = world->attr;
    const struct ListNode *head, *n;
    w->objects = world->gameObjects.size;
    w->effects = 0;
    if ((head = world->gameObjects.head) != NULL)
        for (n = head->next; n && n != head; n = n->next) {
            const struct GameObject *o = *LIST_VALUE(n, const struct GameObject *const);

            if (o && o->ops->destroy == rag_effect_delete)
                w->effects++;
        }
    w->has_player = world->player != NULL;
    if (world->player)
        actor_record(&w->player, (const struct GameActor *)world->player, attr);
    w->n_actors = w->n_units = w->n_items = w->n_pending = 0;
    if ((head = world->actors.head) != NULL)
        for (n = head->next; n && n != head; n = n->next, w->n_actors++)
            if (w->n_actors < PARITY_WORLD_MAX)
                actor_record(&w->actors[w->n_actors], *LIST_VALUE(n, const struct GameActor *const), attr);
    if ((head = world->items.head) != NULL)
        for (n = head->next; n && n != head; n = n->next, w->n_items++)
            if (w->n_items < PARITY_WORLD_MAX) {
                const struct Item *item = *LIST_VALUE(n, const struct Item *const);
                struct ParityItem *r = &w->items[w->n_items];
                r->id = (unsigned)item->aid;
                r->item_id = item->item_id;
                at_cell(attr, &item->base.pos, &r->cell_x, &r->cell_y);
            }
    if ((head = world->skills.head) != NULL)
        for (n = head->next; n && n != head; n = n->next, w->n_units++)
            if (w->n_units < PARITY_WORLD_MAX) {
                const struct Skill *unit = *LIST_VALUE(n, const struct Skill *const);
                struct ParityUnit *r = &w->units[w->n_units];
                r->id = (unsigned)unit->unit_aid;
                r->type = unit->base.job;
                r->creator = (unsigned)unit->creator_aid;
                at_cell(attr, &unit->base.pos, &r->cell_x, &r->cell_y);
                r->shown = unit->base.revealed;
                r->fx = unit->base.effects.size;
            }
    if ((head = world->pendingSkills.head) != NULL)
        for (n = head->next; n && n != head; n = n->next, w->n_pending++)
            if (w->n_pending < PARITY_WORLD_MAX)
                w->pending[w->n_pending] = *LIST_VALUE(n, const unsigned);
    parity_world_write(f, w);
}

__declspec(dllexport) void __cdecl parity_probe(const char *path)
{
    struct ListNode *head = g_windowMgr.windows.head, *n;
    int budget = MAX_NODES;
    FILE *f = fopen(path, "w");
    if (!f)
        return;
    fprintf(f, "mode=%s\n", g_modeMgr.curModeType == MODE_TYPE_GAME && g_modeMgr.curMode ? "game" : "login");
    fprintf(f, "ui=%d,%d\n", g_windowMgr.screen_width, g_windowMgr.screen_height);
    if (head)
        for (n = head->next; n && n != head && budget > 0; n = n->next)
            write_window(f, *LIST_VALUE(n, struct UIWindow *), 0, &budget);
    budget = MAX_NODES;
    write_picks(f, &g_actorPickNode, 0, &budget);
    if (game_current_world())
        write_world(f, game_current_world());
    parity_calls_write(f);
    fclose(f);
}

static int item_record(void)
{
    static const int ids[] = { PARITY_ITEM_WINDOW, PARITY_ITEM_CMP_WINDOW };
    FILE *f = parity_call_records();
    int i, n = 0, budget = MAX_NODES;
    for (i = 0; i < 2; i++) {
        struct UIWindow *w = ui_window_mgr_get_window(&g_windowMgr, ids[i]);
        if (!w)
            continue;
        if (f) {
            fprintf(f, "item window=%d\n", ids[i]);
            write_window(f, w, 0, &budget);
        }
        ui_window_mgr_post_quit1(&g_windowMgr, ids[i]);
        n++;
    }
    return n;
}

static int item_show(const unsigned *w)
{
    struct ItemInfo item;
    struct String name;
    struct UIWindow *wnd;
    FILE *f = parity_call_records();
    int i;
    item_info_construct(&item);
    item_info_set_item_id(&item, (int)w[PARITY_ITEM_ID]);
    item.type = (int)w[PARITY_ITEM_TYPE];
    item.refine = (int)w[PARITY_ITEM_REFINE];
    for (i = 0; i < ITEM_CARD_SLOTS; i++)
        item.slot[i] = (int)w[PARITY_ITEM_CARDS + i];
    item.identified = (unsigned char)w[PARITY_ITEM_IDENTIFIED];
    item.damaged = (unsigned char)w[PARITY_ITEM_DAMAGED];
    item.expire_time = (int)w[PARITY_ITEM_EXPIRE];
    item.expire_in_hours = (unsigned char)w[PARITY_ITEM_EXPIRE_HOURS];
    item.grade = (short)w[PARITY_ITEM_GRADE];
    item.option_count = (int)w[PARITY_ITEM_OPTION_COUNT];
    for (i = 0; i < PARITY_ITEM_OPTION_SLOTS; i++) {
        item.options[i].id = (unsigned short)w[PARITY_ITEM_OPTIONS + 3 * i];
        item.options[i].value = (short)w[PARITY_ITEM_OPTIONS + 3 * i + 1];
        item.options[i].param = (unsigned char)w[PARITY_ITEM_OPTIONS + 3 * i + 2];
    }
    string_init(&name);
    item_info_get_display_name(&item, &name);
    if (f)
        parity_item_write(f, w, string_cstr(&name), name.size);
    string_free(&name);
    wnd = ui_window_mgr_make_window(&g_windowMgr, PARITY_ITEM_WINDOW);
    if (wnd)
        ui_window_send_msg(wnd, NULL, 0x18, (int)&item, 0, 0, 0);
    item_info_destruct(&item);
    if (!(w[PARITY_ITEM_FLAGS] & PARITY_ITEM_HOLD))
        item_record();
    return wnd != NULL;
}

static int call_op(int op, const unsigned *arg, unsigned count)
{
    switch (op) {
    case PARITY_CALL_MAKE_WINDOW:
        return count >= 1 && ui_window_mgr_make_window(&g_windowMgr, (int)arg[0]) != NULL;
    case PARITY_CALL_CLOSE_WINDOW:
        return count >= 1 ? ui_window_mgr_post_quit1(&g_windowMgr, (int)arg[0]) : 0;
    case PARITY_CALL_ITEM_SHOW:
        return count >= PARITY_ITEM_WORDS ? item_show(arg) : -1;
    case PARITY_CALL_ITEM_RECORD:
        return item_record();
    }
    return -1;
}

__declspec(dllexport) int __cdecl parity_call(const unsigned *words, unsigned count)
{
    int op, result;
    if (!count)
        return -1;
    op = (int)(words[0] & 0xff);
    if (words[0] & PARITY_CALL_GUARD) {
        __try {
            result = call_op(op, words + 1, count - 1);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            result = PARITY_CALL_FAULT;
        }
    } else {
        result = call_op(op, words + 1, count - 1);
    }
    parity_call_note(op, count > 1 ? (int)words[1] : 0, result);
    return result;
}

