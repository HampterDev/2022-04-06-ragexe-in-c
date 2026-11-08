
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "herc_ui.h"
#include "parity_world.h"


#define MGR_WINDOWS 0x174
#define MGR_MODAL 0x19c
#define MGR_CHAT 0x1b0
#define MGR_WIDTH 0x440


#define BOX_ON 0x30
#define BOX_DIM 0x7c


#define STATE 0x30


#define BAR_MAX 0x80
#define BAR_POS 0x84
#define BAR_TOTAL 0x98
#define BAR_PAGE 0x9c


#define GRAPHIC_BGM_BAR 0x140
#define WND_PARENT 0x10


#define EDIT_SEL_START 0x7c
#define EDIT_SEL_END 0x80
#define EDIT_MASK 0x84
#define EDIT_TEXT 0xd8


#define TABS_SELECTED 0x7c


#define LIST_ROW 0x94
#define LIST_TOP 0x98
#define LIST_EX_ROW 0x90
#define LIST_EX_TOP 0x94


#define COMBO_OPEN 0x89


#define STATIC_TEXT 0xa8


#define BALLOON_LINES 0x84


#define BUTTON_LABEL 0xd4
#define BUTTON_LINES 0xec


#define BMP_CAPTION 0x120


#define LINE_TEXT 0x90


#define VIEWER_LINES 0x88
#define VIEWER_LINKS 0xd0


#define TAG_TEXTS 0xb8


#define STR_SIZE 0x10
#define STR_CAP 0x14
#define STR_BYTES 0x18

extern BYTE g_windowMgr[];
extern void *data_d2a7f8[];             
extern BYTE g_modeMgr[];
void *__fastcall CModeMgr__GetGameMode(void *mgr, void *edx);

void *__fastcall sub_6349a0(void *mgr, void *edx, int id);
char __fastcall UIWindowMgr__PostQuit1(void *mgr, void *edx, int id);

struct UIWindow *__fastcall sub_6409d0(void *mgr, void *edx, int id);


#define ITEM_INFO_SIZE 0xe8
#define ITEM_TYPE 0x00
#define ITEM_CARDS 0x1c
#define ITEM_IDENTIFIED 0x5c
#define ITEM_DAMAGED 0x5d
#define ITEM_REFINE 0x60
#define ITEM_EXPIRE_HOURS 0x66
#define ITEM_EXPIRE 0x68
#define ITEM_GRADE 0x88
#define ITEM_OPTION_COUNT 0x8c
#define ITEM_OPTIONS 0x90
#define WND_SEND_MSG 34
void *__fastcall sub_92c970(void *item, void *edx);
void __fastcall sub_9305f0(void *item, void *edx, int id);
void __fastcall sub_50b2b0(void *item, void *edx);
void *__fastcall sub_92d720(void *item, void *edx, void *out);
void __fastcall sub_4771c0(void *s, void *edx);


extern BYTE data_e8e5a0[];
#define PICK_ID 0x18
#define PICK_JOB 0x1c
#define PICK_KIND 0x20
#define PICK_DRAWN 0x28

struct PickListNode {
    struct PickListNode *next, *prev;
    BYTE *entry;
};

struct PickNode {
    void *vtable;
    int left, top, right, bottom;
    struct PickNode *child[4];
    struct PickListNode *head;
    DWORD count;
};


#define MODE_WORLD 0xcc
#define WORLD_OBJECTS 0x8
#define WORLD_ACTORS 0x10
#define WORLD_ITEMS 0x18
#define WORLD_UNITS 0x20
#define WORLD_PLAYER 0x2c
#define WORLD_ATTR 0x30
#define WORLD_PENDING 0xd8
#define ATTR_WIDTH 0x110
#define ATTR_HEIGHT 0x114
#define ATTR_ZOOM 0x118


#define OBJ_POS_X 0x10
#define OBJ_POS_Z 0x18
#define OBJ_ACTION 0x38             
#define OBJ_STATE 0x70              
#define OBJ_SHOWN 0xa1              
#define OBJ_ID 0x110                
#define ACTOR_EFFECTS 0x144         
#define ACTOR_CELL_X 0x16c          
#define ACTOR_CELL_Y 0x170
#define ACTOR_JOB 0x260             
#define ACTOR_HELD 0x2b4            
#define ACTOR_OPT 0x2c4             
#define ACTOR_VANISH 0x2e0          
#define ACTOR_KIND 0x318            

#define UNIT_CREATOR 0x3f0
#define UNIT_ID 0x3f8

#define ITEM_ITEM_ID 0x178
#define ITEM_ID 0x17c

#define MAX_DEPTH 6
#define MAX_NODES 4096
#define MAX_TEXTS 0x10000           

static int readable(const void *p, size_t n)
{
    return p && !IsBadReadPtr(p, n);
}

#define AT(p, off, type) (*(const type *)((const BYTE *)(p) + (off)))


static void text_piece(struct ParityControl *c, const BYTE *s)
{
    size_t size, cap;
    const char *text = NULL;
    if (c->pieces >= PARITY_TEXT_PIECES)
        return;
    size = readable(s, STR_BYTES) ? AT(s, STR_SIZE, size_t) : 0;
    if (readable(s, STR_BYTES)) {
        cap = AT(s, STR_CAP, size_t);
        text = cap >= 16 ? AT(s, 0, char *) : (const char *)s;
        if (size > cap || !readable(text, size + 1))
            text = NULL;
    }
    c->text[c->pieces] = text;
    c->length[c->pieces] = size;
    c->pieces++;
}


static void write_control(FILE *f, const BYTE *w, int kind)
{
    
    static const size_t ends[] = {
        [PARITY_CHECK_BOX] = BOX_DIM + 4,         [PARITY_RADIO] = STATE + 4,
        [PARITY_SCROLL_BAR] = BAR_PAGE + 4,       [PARITY_EDIT] = EDIT_TEXT + STR_BYTES,
        [PARITY_TAB_CONTROL] = TABS_SELECTED + 4, [PARITY_TAB] = STATE + 4,
        [PARITY_LIST_BOX] = LIST_TOP + 4,         [PARITY_LIST_BOX_EX] = LIST_EX_TOP + 4,
        [PARITY_COMBO] = COMBO_OPEN + 1,          [PARITY_STATIC_TEXT] = STATIC_TEXT + STR_BYTES,
        [PARITY_BALLOON] = BALLOON_LINES + 8,     [PARITY_TEXT_BUTTON] = BUTTON_LINES + 8,
        [PARITY_BMP_TEXT_BUTTON] = BMP_CAPTION + STR_BYTES, [PARITY_LINE_TEXT] = LINE_TEXT + STR_BYTES,
        [PARITY_TEXT_VIEWER] = VIEWER_LINKS + 8,  [PARITY_TAG_TEXT_VIEWER] = TAG_TEXTS + 8,
    };
    struct ParityControl c;
    const BYTE *s, *end;
    int i;
    if (kind <= PARITY_PLAIN || kind >= (int)(sizeof ends / sizeof ends[0]) || !readable(w, ends[kind]))
        return;
    memset(&c, 0, sizeof c);
    c.kind = kind;
    switch (kind) {
    case PARITY_CHECK_BOX:
        c.value[0] = AT(w, BOX_ON, int);
        c.value[1] = AT(w, BOX_DIM, int);
        break;
    case PARITY_RADIO:
    case PARITY_TAB:
        c.value[0] = AT(w, STATE, int);
        break;
    case PARITY_SCROLL_BAR:
        c.value[0] = AT(w, BAR_POS, int);
        c.value[1] = AT(w, BAR_MAX, int);
        c.value[2] = AT(w, BAR_PAGE, int);
        c.value[3] = AT(w, BAR_TOTAL, int);
        {
            const BYTE *parent = AT(w, WND_PARENT, BYTE *);
            c.timed = readable(parent, GRAPHIC_BGM_BAR + 4) &&
                      parity_timed_bar_parent(herc_ui_class_of(AT(parent, 0, void **))) &&
                      AT(parent, GRAPHIC_BGM_BAR, BYTE *) == w;
        }
        break;
    case PARITY_EDIT:
        c.masked = AT(w, EDIT_MASK, int) != 0;
        text_piece(&c, w + EDIT_TEXT);
        c.value[0] = AT(w, EDIT_SEL_START, int);
        c.value[1] = AT(w, EDIT_SEL_END, int);
        break;
    case PARITY_TAB_CONTROL:
        c.value[0] = AT(w, TABS_SELECTED, int);
        break;
    case PARITY_LIST_BOX:
        c.value[0] = AT(w, LIST_ROW, int);
        c.value[1] = AT(w, LIST_TOP, int);
        break;
    case PARITY_LIST_BOX_EX:
        c.value[0] = AT(w, LIST_EX_ROW, int);
        c.value[1] = AT(w, LIST_EX_TOP, int);
        break;
    case PARITY_COMBO:
        c.value[0] = AT(w, COMBO_OPEN, BYTE);
        break;
    case PARITY_STATIC_TEXT:
        text_piece(&c, w + STATIC_TEXT);
        break;
    case PARITY_BALLOON: {
        const BYTE *line = AT(w, BALLOON_LINES, BYTE *), *end = AT(w, BALLOON_LINES + 4, BYTE *);
        for (; line && line < end && c.pieces < PARITY_TEXT_PIECES; line += STR_BYTES)
            text_piece(&c, line);
        break;
    }
    case PARITY_TEXT_BUTTON:
        text_piece(&c, w + BUTTON_LABEL);
        s = AT(w, BUTTON_LINES, BYTE *);
        end = AT(w, BUTTON_LINES + 4, BYTE *);
        for (; s && s < end && c.pieces < PARITY_TEXT_PIECES; s += STR_BYTES)
            text_piece(&c, s);
        break;
    case PARITY_BMP_TEXT_BUTTON:
        text_piece(&c, w + BMP_CAPTION);
        break;
    case PARITY_LINE_TEXT:
        text_piece(&c, w + LINE_TEXT);
        break;
    case PARITY_TEXT_VIEWER:
        
        s = AT(w, VIEWER_LINES, BYTE *);
        end = AT(w, VIEWER_LINES + 4, BYTE *);
        c.hash = PARITY_HASH_START;
        for (i = 0; s && s < end && i < MAX_TEXTS; s += STR_BYTES, i++) {
            struct ParityControl one;
            memset(&one, 0, sizeof one);
            text_piece(&one, s);
            c.hash = parity_text_hash(c.hash, i, one.text[0], one.length[0]);
            if (c.pieces < PARITY_TEXT_PIECES) {
                c.text[c.pieces] = one.text[0];
                c.length[c.pieces++] = one.length[0];
            }
        }
        c.value[0] = i;
        s = AT(w, VIEWER_LINKS, BYTE *);
        end = AT(w, VIEWER_LINKS + 4, BYTE *);
        c.value[1] = s && end > s ? (int)((end - s) / 4) : 0;
        break;
    case PARITY_TAG_TEXT_VIEWER:
        s = AT(w, TAG_TEXTS, BYTE *);
        end = AT(w, TAG_TEXTS + 4, BYTE *);
        c.hash = PARITY_HASH_START;
        for (i = 0; s && s < end && i < MAX_TEXTS; s += STR_BYTES, i++) {
            struct ParityControl one;
            memset(&one, 0, sizeof one);
            text_piece(&one, s);
            c.hash = parity_text_hash(c.hash, i, one.text[0], one.length[0]);
            c.size += (i ? 1 : 0) + (one.text[0] ? one.length[0] : 1);
            if (c.pieces < PARITY_TEXT_PIECES) {
                c.text[c.pieces] = one.text[0];
                c.length[c.pieces++] = one.length[0];
            }
        }
        break;
    }
    parity_control_write(f, &c);
}

static void write_window(FILE *f, struct UIWindow *w, int depth, struct UIWindow *modal, struct UIWindow *chat,
                         int *budget)
{
    struct UIWindowNode *head, *n;
    const char *cls;
    if (--*budget < 0 || !readable(w, sizeof *w))
        return;
    cls = herc_ui_class_of(w->vtable);
    fprintf(f, "%*s%s id=%u rect=%d,%d,%d,%d shown=%d%s%s", depth * 2, "", cls, w->id, w->x, w->y, w->width,
            w->height, w->visible ? 1 : 0, w == modal ? " modal" : "", w == chat ? " chat" : "");
    write_control(f, (const BYTE *)w, parity_control_kind(cls));
    fputc('\n', f);
    head = w->children;
    if (depth >= MAX_DEPTH || !readable(head, sizeof *head))
        return;
    for (n = head->next; n && n != head && readable(n, sizeof *n) && *budget > 0; n = n->next)
        write_window(f, n->window, depth + 1, modal, chat, budget);
}

static void write_picks(FILE *f, const struct PickNode *node, int depth, int *budget)
{
    const struct PickListNode *n;
    int i;
    if (depth > 4 || !readable(node, sizeof *node) || !readable(node->head, sizeof *node->head))
        return;
    for (n = node->head->next; n && n != node->head && readable(n, sizeof *n) && --*budget > 0; n = n->next) {
        const BYTE *e = n->entry;
        const float *d;
        if (!readable(e, PICK_DRAWN + 16))
            continue;
        d = (const float *)(e + PICK_DRAWN);
        fprintf(f, "actor id=%u job=%d kind=%d rect=%d,%d,%d,%d\n", *(const DWORD *)(e + PICK_ID),
                *(const int *)(e + PICK_JOB), *(const int *)(e + PICK_KIND), (int)d[0], (int)d[1],
                (int)(d[2] - d[0]), (int)(d[3] - d[1]));
    }
    for (i = 0; i < 4; i++)
        if (node->child[i])
            write_picks(f, node->child[i], depth + 1, budget);
}

static struct ParityWorld g_world;

struct ListNode {
    struct ListNode *next, *prev;
    BYTE *value;
};


static const struct ListNode *list_head(const BYTE *owner, size_t off)
{
    const struct ListNode *head = AT(owner, off, struct ListNode *);
    return readable(head, sizeof *head) ? head : NULL;
}

static void at_cell(const BYTE *attr, const BYTE *obj, int *cell_x, int *cell_y)
{
    parity_world_cell(AT(obj, OBJ_POS_X, float), AT(obj, OBJ_POS_Z, float), attr ? AT(attr, ATTR_WIDTH, int) : 0,
                      attr ? AT(attr, ATTR_HEIGHT, int) : 0, attr ? AT(attr, ATTR_ZOOM, int) : 0, cell_x, cell_y);
}

static void actor_record(struct ParityActor *r, const BYTE *a, const BYTE *attr)
{
    r->id = AT(a, OBJ_ID, unsigned);
    r->job = AT(a, ACTOR_JOB, int);
    r->kind = AT(a, ACTOR_KIND, BYTE);
    r->cell_x = AT(a, ACTOR_CELL_X, int);
    r->cell_y = AT(a, ACTOR_CELL_Y, int);
    at_cell(attr, a, &r->at_x, &r->at_y);
    r->state = AT(a, OBJ_STATE, int);
    r->action = AT(a, OBJ_ACTION, int);
    r->held = AT(a, ACTOR_HELD, int);
    r->opt = AT(a, ACTOR_OPT, unsigned);
    r->vanishing = AT(a, ACTOR_VANISH, unsigned) != 0;
    r->fx = AT(a, ACTOR_EFFECTS + 4, unsigned);
}

static void write_world(FILE *f, const BYTE *world)
{
    struct ParityWorld *w = &g_world;
    const BYTE *attr = AT(world, WORLD_ATTR, BYTE *), *player = AT(world, WORLD_PLAYER, BYTE *);
    const struct ListNode *head, *n;
    if (!readable(attr, ATTR_ZOOM + 4))
        attr = NULL;
    w->objects = AT(world, WORLD_OBJECTS + 4, unsigned);
    w->effects = 0;
    if ((head = list_head(world, WORLD_OBJECTS)) != NULL)
        for (n = head->next; n && n != head && readable(n, sizeof *n); n = n->next)
            if (readable(n->value, 4) && *(void ***)n->value == data_d2a7f8)
                w->effects++;
    w->has_player = readable(player, ACTOR_KIND + 1);
    if (w->has_player)
        actor_record(&w->player, player, attr);
    w->n_actors = w->n_units = w->n_items = w->n_pending = 0;
    if ((head = list_head(world, WORLD_ACTORS)) != NULL)
        for (n = head->next; n && n != head && readable(n, sizeof *n); n = n->next, w->n_actors++)
            if (w->n_actors < PARITY_WORLD_MAX && readable(n->value, ACTOR_KIND + 1))
                actor_record(&w->actors[w->n_actors], n->value, attr);
    if ((head = list_head(world, WORLD_ITEMS)) != NULL)
        for (n = head->next; n && n != head && readable(n, sizeof *n); n = n->next, w->n_items++)
            if (w->n_items < PARITY_WORLD_MAX && readable(n->value, ITEM_ID + 4)) {
                struct ParityItem *r = &w->items[w->n_items];
                r->id = AT(n->value, ITEM_ID, unsigned);
                r->item_id = AT(n->value, ITEM_ITEM_ID, int);
                at_cell(attr, n->value, &r->cell_x, &r->cell_y);
            }
    if ((head = list_head(world, WORLD_UNITS)) != NULL)
        for (n = head->next; n && n != head && readable(n, sizeof *n); n = n->next, w->n_units++)
            if (w->n_units < PARITY_WORLD_MAX && readable(n->value, UNIT_ID + 4)) {
                struct ParityUnit *r = &w->units[w->n_units];
                r->id = AT(n->value, UNIT_ID, unsigned);
                r->type = AT(n->value, ACTOR_JOB, int);
                r->creator = AT(n->value, UNIT_CREATOR, unsigned);
                at_cell(attr, n->value, &r->cell_x, &r->cell_y);
                r->shown = AT(n->value, OBJ_SHOWN, BYTE);
                r->fx = AT(n->value, ACTOR_EFFECTS + 4, unsigned);
            }
    if ((head = list_head(world, WORLD_PENDING)) != NULL)
        for (n = head->next; n && n != head && readable(n, sizeof *n); n = n->next, w->n_pending++)
            if (w->n_pending < PARITY_WORLD_MAX)
                w->pending[w->n_pending] = (unsigned)(UINT_PTR)n->value;
    parity_world_write(f, w);
}

__declspec(dllexport) void __cdecl parity_probe(const char *path)
{
    struct UIWindowNode *head = *(struct UIWindowNode **)(g_windowMgr + MGR_WINDOWS), *n;
    struct UIWindow *modal = *(struct UIWindow **)(g_windowMgr + MGR_MODAL);
    struct UIWindow *chat = *(struct UIWindow **)(g_windowMgr + MGR_CHAT);
    BYTE *mode = CModeMgr__GetGameMode(g_modeMgr, NULL);
    int budget = MAX_NODES;
    FILE *f = fopen(path, "w");
    if (!f)
        return;
    fprintf(f, "mode=%s\n", mode ? "game" : "login");
    fprintf(f, "ui=%d,%d\n", *(int *)(g_windowMgr + MGR_WIDTH), *(int *)(g_windowMgr + MGR_WIDTH + 4));
    if (readable(head, sizeof *head))
        for (n = head->next; n && n != head && readable(n, sizeof *n) && budget > 0; n = n->next)
            write_window(f, n->window, 0, modal, chat, &budget);
    budget = MAX_NODES;
    write_picks(f, (const struct PickNode *)data_e8e5a0, 0, &budget);
    if (mode && readable(mode + MODE_WORLD, 4) && readable(AT(mode, MODE_WORLD, BYTE *), WORLD_PENDING + 8))
        write_world(f, AT(mode, MODE_WORLD, BYTE *));
    parity_calls_write(f);
    fclose(f);
}


static int item_record(void)
{
    static const int ids[] = { PARITY_ITEM_WINDOW, PARITY_ITEM_CMP_WINDOW };
    struct UIWindow *modal = *(struct UIWindow **)(g_windowMgr + MGR_MODAL);
    struct UIWindow *chat = *(struct UIWindow **)(g_windowMgr + MGR_CHAT);
    FILE *f = parity_call_records();
    int i, n = 0, budget = MAX_NODES;
    for (i = 0; i < 2; i++) {
        struct UIWindow *w = sub_6409d0(g_windowMgr, NULL, ids[i]);
        if (!w)
            continue;
        if (f) {
            fprintf(f, "item window=%d\n", ids[i]);
            write_window(f, w, 0, modal, chat, &budget);
        }
        UIWindowMgr__PostQuit1(g_windowMgr, NULL, ids[i]);
        n++;
    }
    return n;
}


static int item_show(const unsigned *w)
{
    __declspec(align(4)) BYTE item[ITEM_INFO_SIZE];
    BYTE name[STR_BYTES];
    struct ParityControl text;
    void *wnd;
    FILE *f = parity_call_records();
    int i;
    memset(item, 0, sizeof item);
    sub_92c970(item, NULL);
    sub_9305f0(item, NULL, (int)w[PARITY_ITEM_ID]);
    *(int *)(item + ITEM_TYPE) = (int)w[PARITY_ITEM_TYPE];
    *(int *)(item + ITEM_REFINE) = (int)w[PARITY_ITEM_REFINE];
    for (i = 0; i < 4; i++)
        *(int *)(item + ITEM_CARDS + 4 * i) = (int)w[PARITY_ITEM_CARDS + i];
    item[ITEM_IDENTIFIED] = (BYTE)w[PARITY_ITEM_IDENTIFIED];
    item[ITEM_DAMAGED] = (BYTE)w[PARITY_ITEM_DAMAGED];
    *(int *)(item + ITEM_EXPIRE) = (int)w[PARITY_ITEM_EXPIRE];
    item[ITEM_EXPIRE_HOURS] = (BYTE)w[PARITY_ITEM_EXPIRE_HOURS];
    *(WORD *)(item + ITEM_GRADE) = (WORD)w[PARITY_ITEM_GRADE];
    *(int *)(item + ITEM_OPTION_COUNT) = (int)w[PARITY_ITEM_OPTION_COUNT];
    for (i = 0; i < PARITY_ITEM_OPTION_SLOTS; i++) {
        BYTE *o = item + ITEM_OPTIONS + 5 * i;
        *(WORD *)o = (WORD)w[PARITY_ITEM_OPTIONS + 3 * i];
        *(WORD *)(o + 2) = (WORD)w[PARITY_ITEM_OPTIONS + 3 * i + 1];
        o[4] = (BYTE)w[PARITY_ITEM_OPTIONS + 3 * i + 2];
    }
    sub_92d720(item, NULL, name);
    memset(&text, 0, sizeof text);
    text_piece(&text, name);
    if (f)
        parity_item_write(f, w, text.text[0], text.length[0]);
    sub_4771c0(name, NULL);
    wnd = sub_6349a0(g_windowMgr, NULL, PARITY_ITEM_WINDOW);
    if (wnd)
        ((int(__fastcall *)(void *, void *, void *, int, int, int, int, int))(*(void ***)wnd)[WND_SEND_MSG])(
            wnd, NULL, NULL, 0x18, (int)item, 0, 0, 0);
    sub_50b2b0(item, NULL);
    if (!(w[PARITY_ITEM_FLAGS] & PARITY_ITEM_HOLD))
        item_record();
    return wnd != NULL;
}

static int call_op(int op, const unsigned *arg, unsigned count)
{
    switch (op) {
    case PARITY_CALL_MAKE_WINDOW:
        return count >= 1 && sub_6349a0(g_windowMgr, NULL, (int)arg[0]) != NULL;
    case PARITY_CALL_CLOSE_WINDOW:
        return count >= 1 ? (BYTE)UIWindowMgr__PostQuit1(g_windowMgr, NULL, (int)arg[0]) : 0;
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
