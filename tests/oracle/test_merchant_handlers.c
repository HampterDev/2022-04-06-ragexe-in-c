#include <windows.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "oracle_image.h"

static SHORT WINAPI fake_key_state(int vk);
#define GetAsyncKeyState fake_key_state
#include "ui/shops/merchant_messages.c"
#undef GetAsyncKeyState
#include "game/gamemode.h"

#define ORIG_PURCHASE_R_BTN   0x6156f0u
#define ORIG_SHOP_R_BTN       0x615890u
#define ORIG_CART_R_BTN       0x6159d0u
#define ORIG_MIRROR_R_BTN     0x615b30u
#define ORIG_INDEX_AT         0x601f50u
#define ORIG_GET_ITEM         0x6020d0u
#define ORIG_ITEM_AT          0x602160u
#define ORIG_GET_GAME_MODE    0x6617e0u
#define ORIG_PURCHASE_BY_ID   0x8548b0u
#define ORIG_MIRROR_BY_INDEX  0x8544c0u
#define ORIG_ITEM_ASSIGN      0x92cfa0u
#define ORIG_DRAG_ITEM        0x795300u
#define ORIG_WINDOW_OPEN      0x6349a0u

#define ORIG_WND_ITEM             0xee8984u
#define ORIG_WND_EXCHANGE         0xee8a30u
#define ORIG_WND_ITEM_STORE       0xee8a38u
#define ORIG_WND_SHOP_MAKE        0xee8aacu
#define ORIG_WND_MERCHANT_SHOP    0xee8ab0u
#define ORIG_WND_MERCHANT_BUY     0xee8ab8u
#define ORIG_WND_BUYING_MAKE      0xee8b98u

#define ORIG_ITEM_SIZE        0xe8
#define ORIG_ITEM_INDEX       0x04
#define ORIG_ITEM_COUNT       0x10
#define ORIG_ITEM_ID_TEXT     0x2c
#define ORIG_ITEM_NAME        0x44
#define ORIG_MODE_ITEM_MOVE   0x5f0
#define ORIG_PURCHASE_HOVER   0xe0
#define ORIG_SHOP_HOVER       0xdc
#define VT_WINDOW_SEND        (0x88 / 4)
#define VT_WINDOW_INVALIDATE  (0x8c / 4)
#define VT_MODE_SEND          (0x18 / 4)

#define HOVER_ANSWER 3
#define ITEM_COUNT_GIVEN 5
#define ITEM_ID_TEXT "Apple"
#define EVENTS 24
#define EVENT_LEN 96
#define MAX_REPORTED 40

enum { ROLE_ITEM, ROLE_EXCHANGE, ROLE_ITEM_STORE, ROLE_SHOP_MAKE, ROLE_MERCHANT_SHOP, ROLE_MERCHANT_BUY,
       ROLE_BUYING_MAKE, ROLE_COUNT, ROLE_ITEM_DESC = ROLE_COUNT, ROLE_ALL };

static const char *const role_names[ROLE_ALL] = { "item", "exchange", "item_store", "shop_make", "merchant_shop",
                                                  "merchant_buy", "buying_make", "item_desc" };
static const unsigned role_addrs[ROLE_COUNT] = { ORIG_WND_ITEM, ORIG_WND_EXCHANGE, ORIG_WND_ITEM_STORE, ORIG_WND_SHOP_MAKE,
                                                 ORIG_WND_MERCHANT_SHOP, ORIG_WND_MERCHANT_BUY, ORIG_WND_BUYING_MAKE };
static const size_t role_fields[ROLE_COUNT] = {
    offsetof(struct UIWindowMgr, item_wnd), offsetof(struct UIWindowMgr, exchange_wnd),
    offsetof(struct UIWindowMgr, item_store_wnd), offsetof(struct UIWindowMgr, merchant_shop_make_wnd),
    offsetof(struct UIWindowMgr, merchant_item_shop_wnd), offsetof(struct UIWindowMgr, merchant_item_purchase_wnd),
    offsetof(struct UIWindowMgr, buying_store_make_wnd) };

struct UIWindowMgr g_windowMgr;
struct Session g_session1;

static char events[EVENTS][EVENT_LEN];
static int event_count;
static int key_held, key_low_bits, item_index, hover_old;
static unsigned compared, mismatches, reported, faults, fault_code, fault_va;

static void event(const char *fmt, ...)
{
    va_list ap;
    if (event_count >= EVENTS)
        return;
    va_start(ap, fmt);
    vsnprintf(events[event_count++], EVENT_LEN, fmt, ap);
    va_end(ap);
}

static void joined(char *out, size_t size)
{
    int i;
    out[0] = 0;
    for (i = 0; i < event_count; i++) {
        strncat(out, events[i], size - strlen(out) - 1);
        strncat(out, "; ", size - strlen(out) - 1);
    }
}

static int key_bit(int vk)
{
    switch (vk) {
    case VK_SHIFT: return 1;
    case VK_CONTROL: return 2;
    case VK_MENU: return 4;
    }
    return 0;
}

static SHORT WINAPI fake_key_state(int vk)
{
    int down = (key_held & key_bit(vk)) != 0;
    event("key %x", vk);
    return (SHORT)((down ? 0x8000 : 0) | (key_low_bits ? 1 : 0));
}

static struct UIWindow lift_windows[ROLE_ALL];
static struct UIWindowOps lift_window_ops;
static struct GameMode lift_mode;
static struct ItemInfo *lift_shown;

static int lift_role(const struct UIWindow *w)
{
    int i;
    for (i = 0; i < ROLE_ALL; i++)
        if (w == &lift_windows[i])
            return i;
    return -1;
}

void *mem_alloc(size_t bytes) { return calloc(1, bytes ? bytes : 1); }
void mem_free(void *memory) { free(memory); }

static void lift_fill(struct ItemInfo *out, int index, int count)
{
    memset(out, 0, sizeof *out);
    out->index = index;
    out->count = count;
    string_assign_cstr(&out->id_text, ITEM_ID_TEXT);
}

int ui_item_base_wnd_index_at(struct UIItemBaseWnd *self, int x, int y)
{
    (void)self;
    event("index_at %d %d", x, y);
    return HOVER_ANSWER;
}

struct ItemInfo *ui_item_base_wnd_get_item(struct UIItemBaseWnd *self, struct ItemInfo *out, int index)
{
    (void)self;
    event("get_item %d", index);
    lift_fill(out, item_index, ITEM_COUNT_GIVEN);
    return out;
}

struct ItemInfo *ui_item_base_wnd_item_at(struct UIItemBaseWnd *self, struct ItemInfo *out, int x, int y)
{
    (void)self;
    event("item_at %d %d", x, y);
    lift_fill(out, item_index, ITEM_COUNT_GIVEN);
    return out;
}

void ui_window_invalidate_base(struct UIWindow *self)
{
    (void)self;
    event("invalidate");
}

struct ItemInfo *session_purchase_item_by_id(struct Session *self, struct ItemInfo *out, const char *name)
{
    (void)self;
    event("purchase %s", name);
    lift_fill(out, item_index + 100, ITEM_COUNT_GIVEN + 1);
    return out;
}

struct ItemInfo *session_mirror_item_by_index(struct Session *self, struct ItemInfo *out, int index)
{
    (void)self;
    event("mirror %d", index);
    lift_fill(out, index + 200, ITEM_COUNT_GIVEN + 2);
    return out;
}

struct ItemInfo *item_info_assign(struct ItemInfo *self, const struct ItemInfo *src)
{
    event("assign %d %d", src->index, src->count);
    self->index = src->index;
    self->count = src->count;
    return self;
}

struct GameMode *game_current_mode(void) { return &lift_mode; }
struct Mode *game_current_base_mode(void) { return (struct Mode *)&lift_mode; }

int game_take_item_move(void)
{
    struct GameMode *mode = game_current_mode();
    if (!mode || !mode->item_move_ready)
        return 0;
    mode->item_move_ready = 0;
    return 1;
}

void game_mode_drag_item(struct GameMode *self, int source, const struct ItemInfo *item, int count, int last)
{
    event("drag %x item %d count %d last %d%s", source, item->index, count, last, self == &lift_mode ? "" : " other mode");
}

int mode_send_message(struct Mode *mode, int message, int a, int b, int c, int d)
{
    event("request %x %d %d %d %d%s", message, a, b, c, d, mode == (struct Mode *)&lift_mode ? "" : " other mode");
    return 0;
}

struct UIWindow *ui_window_open(int id)
{
    event("open %x", id);
    return &lift_windows[ROLE_ITEM_DESC];
}

static void lift_send(struct UIWindow *window, struct UIWindow *sender, int msg, int a, int b, int c, int d)
{
    int role = lift_role(window);
    if (msg == UI_MSG_SHOW_ITEM)
        event("send %s %x item %d%s", role >= 0 ? role_names[role] : "?", msg, ((struct ItemInfo *)a)->index,
              sender || b || c || d ? " extra" : "");
    else
        event("send %s %x %d%s", role >= 0 ? role_names[role] : "?", msg, a, sender || b || c || d ? " extra" : "");
}

int ui_window_send(struct UIWindow *window, struct UIWindow *sender, int message, int a, int b, int c, int d)
{
    lift_send(window, sender, message, a, b, c, d);
    return 0;
}

static int lift_window_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int a, int b, int c, int d)
{
    lift_send(self, sender, msg, a, b, c, d);
    return 0;
}

static unsigned char orig_self[0x200], orig_mode[0x800];
static unsigned char orig_windows[ROLE_ALL][0x40];
static void *orig_self_vt[64], *orig_window_vt[64], *orig_mode_vt[64];

static int orig_role(const void *w)
{
    int i;
    for (i = 0; i < ROLE_ALL; i++)
        if (w == orig_windows[i])
            return i;
    return -1;
}

static void orig_string(unsigned char *s, const char *text)
{
    size_t n = strlen(text);
    memset(s, 0, 0x18);
    memcpy(s, text, n);
    *(unsigned *)(s + 0x10) = (unsigned)n;
    *(unsigned *)(s + 0x14) = 15;
}

static void orig_fill(unsigned char *out, int index, int count)
{
    memset(out, 0, ORIG_ITEM_SIZE);
    *(int *)(out + ORIG_ITEM_INDEX) = index;
    *(int *)(out + ORIG_ITEM_COUNT) = count;
    orig_string(out + ORIG_ITEM_ID_TEXT, ITEM_ID_TEXT);
    orig_string(out + ORIG_ITEM_NAME, "");
}

static int __fastcall o_index_at(void *self, void *edx, int x, int y)
{
    (void)self; (void)edx;
    event("index_at %d %d", x, y);
    return HOVER_ANSWER;
}

static void *__fastcall o_get_item(void *self, void *edx, unsigned char *out, int index)
{
    (void)self; (void)edx;
    event("get_item %d", index);
    orig_fill(out, item_index, ITEM_COUNT_GIVEN);
    return out;
}

static void *__fastcall o_item_at(void *self, void *edx, unsigned char *out, int x, int y)
{
    (void)self; (void)edx;
    event("item_at %d %d", x, y);
    orig_fill(out, item_index, ITEM_COUNT_GIVEN);
    return out;
}

static void __fastcall o_invalidate(void *self)
{
    (void)self;
    event("invalidate");
}

static void *__fastcall o_get_game_mode(void *self)
{
    (void)self;
    return orig_mode;
}

static void *__fastcall o_purchase_by_id(void *self, void *edx, unsigned char *out, const char *name)
{
    (void)self; (void)edx;
    event("purchase %s", name);
    orig_fill(out, item_index + 100, ITEM_COUNT_GIVEN + 1);
    return out;
}

static void *__fastcall o_mirror_by_index(void *self, void *edx, unsigned char *out, int index)
{
    (void)self; (void)edx;
    event("mirror %d", index);
    orig_fill(out, index + 200, ITEM_COUNT_GIVEN + 2);
    return out;
}

static void *__fastcall o_item_assign(unsigned char *self, void *edx, const unsigned char *src)
{
    (void)edx;
    event("assign %d %d", *(const int *)(src + ORIG_ITEM_INDEX), *(const int *)(src + ORIG_ITEM_COUNT));
    *(int *)(self + ORIG_ITEM_INDEX) = *(const int *)(src + ORIG_ITEM_INDEX);
    *(int *)(self + ORIG_ITEM_COUNT) = *(const int *)(src + ORIG_ITEM_COUNT);
    return self;
}

static void *__fastcall o_drag_item(void *self, void *edx, int source, const unsigned char *item, int count, int last)
{
    (void)edx;
    event("drag %x item %d count %d last %d%s", source, *(const int *)(item + ORIG_ITEM_INDEX), count, last,
          self == orig_mode ? "" : " other mode");
    return NULL;
}

static int __fastcall o_mode_send(void *self, void *edx, int message, int a, int b, int c, int d)
{
    (void)edx;
    event("request %x %d %d %d %d%s", message, a, b, c, d, self == orig_mode ? "" : " other mode");
    return 0;
}

static void *__fastcall o_window_open(void *self, void *edx, int id)
{
    (void)self; (void)edx;
    event("open %x", id);
    return orig_windows[ROLE_ITEM_DESC];
}

static int __fastcall o_window_send(void *self, void *edx, void *sender, int msg, int a, int b, int c, int d)
{
    int role = orig_role(self);
    (void)edx;
    if (msg == UI_MSG_SHOW_ITEM)
        event("send %s %x item %d%s", role >= 0 ? role_names[role] : "?", msg,
              *(const int *)((const unsigned char *)a + ORIG_ITEM_INDEX), sender || b || c || d ? " extra" : "");
    else
        event("send %s %x %d%s", role >= 0 ? role_names[role] : "?", msg, a, sender || b || c || d ? " extra" : "");
    return 0;
}

static void __cdecl o_trap(void)
{
    fprintf(stderr, "  the original called a method the test does not provide\n");
    ExitProcess(3);
}

typedef void(__fastcall *handler_fn)(void *self, void *edx, int x, int y);
typedef void (*lift_handler_fn)(struct UIWindow *window, int x, int y);

struct Handler {
    const char *name;
    unsigned original;
    lift_handler_fn lift;
    int hover_offset;
};

static int note_fault(EXCEPTION_POINTERS *ep)
{
    fault_code = ep->ExceptionRecord->ExceptionCode;
    fault_va = (unsigned)((unsigned char *)ep->ExceptionRecord->ExceptionAddress - (unsigned char *)oracle_addr(0x400000u)) + 0x400000u;
    return EXCEPTION_EXECUTE_HANDLER;
}

static int call_original(handler_fn f)
{
    __try { f(orig_self, NULL, 10, 20); return 1; }
    __except (note_fault(GetExceptionInformation())) { return 0; }
}

static int layout_matches(void)
{
    int ok = 1, i;
    for (i = 0; i < ROLE_COUNT; i++)
        if (role_fields[i] - role_fields[ROLE_EXCHANGE] != role_addrs[i] - ORIG_WND_EXCHANGE) {
            fprintf(stderr, "layout: g_windowMgr's %s window is not at the original's 0x%x\n", role_names[i], role_addrs[i]);
            ok = 0;
        }
    if (offsetof(struct UIMerchantItemPurchaseWnd, hover) != ORIG_PURCHASE_HOVER ||
        offsetof(struct UIMerchantItemShopWnd, hover) != ORIG_SHOP_HOVER) {
        fprintf(stderr, "layout: purchase hover at 0x%x, vendor hover at 0x%x; the original's are 0x%x, 0x%x\n",
                (unsigned)offsetof(struct UIMerchantItemPurchaseWnd, hover), (unsigned)offsetof(struct UIMerchantItemShopWnd, hover),
                ORIG_PURCHASE_HOVER, ORIG_SHOP_HOVER);
        ok = 0;
    }
    return ok;
}

static void set_windows(int present)
{
    int i;
    memset(&g_windowMgr, 0, sizeof g_windowMgr);
    for (i = 0; i < ROLE_COUNT; i++) {
        int here = (present >> i) & 1;
        *(void **)((unsigned char *)&g_windowMgr + role_fields[i]) = here ? &lift_windows[i] : NULL;
        *(void **)oracle_addr(role_addrs[i]) = here ? orig_windows[i] : NULL;
    }
}

int main(int argc, char **argv)
{
    const char *exe = argc > 1 ? argv[1] : getenv("ORACLE_EXE");
    static const int indexes[] = { 0, 7, -1 };
    const struct Handler handlers[] = {
        { "purchase list", ORIG_PURCHASE_R_BTN, ui_merchant_item_purchase_wnd_on_r_btn_down, ORIG_PURCHASE_HOVER },
        { "vendor list", ORIG_SHOP_R_BTN, ui_merchant_item_shop_wnd_on_r_btn_down, ORIG_SHOP_HOVER },
        { "cart", ORIG_CART_R_BTN, ui_merchant_item_wnd_on_r_btn_down, -1 },
        { "shop setup list", ORIG_MIRROR_R_BTN, ui_merchant_mirror_item_wnd_on_r_btn_down, -1 },
    };
    union {
        struct UIMerchantItemPurchaseWnd purchase;
        struct UIMerchantItemShopWnd shop;
        struct UIMerchantItemWnd cart;
        struct UIMerchantMirrorItemWnd mirror;
    } lift_self;
    int h, keys, low, present, ii, take, moved, i;

    if (!exe) {
        fprintf(stderr, "usage: test_merchant_handlers <original exe>\n");
        return 2;
    }
    if (!layout_matches())
        return 1;
    if (!oracle_load(exe))
        return 2;
    if (!oracle_bind_import("user32.dll", "GetAsyncKeyState", (void *)fake_key_state)) {
        fprintf(stderr, "the original imports no GetAsyncKeyState from user32.dll\n");
        return 2;
    }

    oracle_patch_jump(ORIG_INDEX_AT, (void *)o_index_at);
    oracle_patch_jump(ORIG_GET_ITEM, (void *)o_get_item);
    oracle_patch_jump(ORIG_ITEM_AT, (void *)o_item_at);
    oracle_patch_jump(ORIG_GET_GAME_MODE, (void *)o_get_game_mode);
    oracle_patch_jump(ORIG_PURCHASE_BY_ID, (void *)o_purchase_by_id);
    oracle_patch_jump(ORIG_MIRROR_BY_INDEX, (void *)o_mirror_by_index);
    oracle_patch_jump(ORIG_ITEM_ASSIGN, (void *)o_item_assign);
    oracle_patch_jump(ORIG_DRAG_ITEM, (void *)o_drag_item);
    oracle_patch_jump(ORIG_WINDOW_OPEN, (void *)o_window_open);

    for (i = 0; i < 64; i++)
        orig_self_vt[i] = orig_window_vt[i] = orig_mode_vt[i] = (void *)o_trap;
    orig_self_vt[VT_WINDOW_INVALIDATE] = (void *)o_invalidate;
    orig_window_vt[VT_WINDOW_SEND] = (void *)o_window_send;
    orig_mode_vt[VT_MODE_SEND] = (void *)o_mode_send;
    *(void ***)orig_mode = orig_mode_vt;
    for (i = 0; i < ROLE_ALL; i++)
        *(void ***)orig_windows[i] = orig_window_vt;
    lift_window_ops.send_msg = lift_window_send_msg;
    for (i = 0; i < ROLE_ALL; i++)
        lift_windows[i].ops = &lift_window_ops;

    for (h = 0; h < (int)(sizeof handlers / sizeof handlers[0]); h++)
        for (keys = 0; keys < 8; keys++)
            for (low = 0; low < 2; low++)
                for (present = 0; present < (1 << ROLE_COUNT); present++)
                    for (ii = 0; ii < (int)(sizeof indexes / sizeof indexes[0]); ii++)
                        for (take = 0; take < 2; take++)
                            for (moved = 0; moved < 2; moved++) {
                                char want[EVENTS * EVENT_LEN], got[EVENTS * EVENT_LEN];
                                int orig_take, orig_hover = 0, lift_hover = 0;

                                if (handlers[h].hover_offset < 0 && moved)
                                    continue;
                                key_held = keys;
                                key_low_bits = low;
                                item_index = indexes[ii];
                                hover_old = moved ? HOVER_ANSWER + 6 : HOVER_ANSWER;
                                set_windows(present);

                                memset(orig_self, 0, sizeof orig_self);
                                *(void ***)orig_self = orig_self_vt;
                                if (handlers[h].hover_offset >= 0)
                                    *(int *)(orig_self + handlers[h].hover_offset) = hover_old;
                                orig_mode[ORIG_MODE_ITEM_MOVE] = (unsigned char)take;
                                event_count = 0;
                                if (!call_original((handler_fn)oracle_addr(handlers[h].original))) {
                                    faults++;
                                    if (reported++ < MAX_REPORTED)
                                        printf("FAULT %s keys %d low %d windows %02x index %d: 0x%08x at original 0x%x\n",
                                               handlers[h].name, keys, low, present, item_index, fault_code, fault_va);
                                    continue;
                                }
                                joined(want, sizeof want);
                                orig_take = orig_mode[ORIG_MODE_ITEM_MOVE];
                                if (handlers[h].hover_offset >= 0)
                                    orig_hover = *(int *)(orig_self + handlers[h].hover_offset);

                                memset(&lift_self, 0, sizeof lift_self);
                                if (handlers[h].hover_offset == ORIG_PURCHASE_HOVER)
                                    lift_self.purchase.hover = hover_old;
                                else if (handlers[h].hover_offset == ORIG_SHOP_HOVER)
                                    lift_self.shop.hover = hover_old;
                                lift_mode.item_move_ready = (unsigned char)take;
                                event_count = 0;
                                handlers[h].lift((struct UIWindow *)&lift_self, 10, 20);
                                joined(got, sizeof got);
                                if (handlers[h].hover_offset == ORIG_PURCHASE_HOVER)
                                    lift_hover = lift_self.purchase.hover;
                                else if (handlers[h].hover_offset == ORIG_SHOP_HOVER)
                                    lift_hover = lift_self.shop.hover;

                                compared++;
                                if (strcmp(want, got) != 0 || orig_take != lift_mode.item_move_ready || orig_hover != lift_hover) {
                                    mismatches++;
                                    if (reported++ < MAX_REPORTED)
                                        printf("MISMATCH %s keys %d low %d windows %02x index %d take %d hover %d\n"
                                               "  original: %s(take %d hover %d)\n  lift:     %s(take %d hover %d)\n",
                                               handlers[h].name, keys, low, present, item_index, take, hover_old,
                                               want, orig_take, orig_hover, got, lift_mode.item_move_ready, lift_hover);
                                }
                            }

    printf("merchant handlers: %u calls, %u mismatches, %u faults\n", compared, mismatches, faults);
    return mismatches || faults ? 1 : 0;
}
