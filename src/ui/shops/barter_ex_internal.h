#include "session/state.h"
#ifndef UI_SHOPS_BARTER_EX_INTERNAL_H
#define UI_SHOPS_BARTER_EX_INTERNAL_H
#include "ui/shops/barter_internal.h"
#include "web/barter_expanded.h"
#include "std/list.h"

#define MSG_EXB_COUNT_0_REFUSED 0xde5
#define MSG_EXB_TIP_AMOUNT 0xe73

#define DRAG_EXB_CART 0x20
#define DRAG_EXB_GOODS 0x1f

#define EXB_ROW 0x40
#define EXB_PAY_X 0x48
#define EXB_PAY_STEP 0x2a
#define EXB_BOTTOM 0x2e
#define EXB_REFINE_COLOUR 0xffffff
#define EXB_REFINE_OUTLINE 0xc8

struct ExpandedBarterWnd {
    struct UIFrameWnd frame;
    struct List goods;
    struct UIResizer *resizer;
    struct UIScrollBar *scroll_bar;
    int scroll;
    int rows;
    int hover;
    BYTE scrollable;
};
struct UIExpandedBarterItemPurchaseWnd {
    struct ExpandedBarterWnd list;
    struct UIBitmapButton *buy;
    struct UIBitmapButton *cancel;
};

#define WND_ID_EXB_MARKET 0x155
#define WND_ID_EXB_PURCHASE 0x156
#define COUNT_FOR_EXB_MARKET 0x19
#define COUNT_FOR_EXB_PURCHASE 0x1a
#define EXB_MARKET_RESIZE_EDGE 7
#define EXB_PURCHASE_RESIZE_EDGE 3
#define MSG_EXB_HELD_OVER_MAX 0xe6b
#define MSG_EXB_OVER_MAX 0xe6c
#define MSG_EXB_NO_ZENY 0x37
#define MODE_MSG_EXB_BUY 0x141

void exb_construct(struct ExpandedBarterWnd *self, const struct UIWindowOps *ops);
void exb_destruct(struct ExpandedBarterWnd *self, const struct UIWindowOps *ops);
struct BarterExFound *exb_goods_at(const struct ExpandedBarterWnd *self, struct BarterExFound *out, int x, int y);
void exb_append_refine(struct String *text, int refine);
void exb_add_resizer_and_bar(struct ExpandedBarterWnd *self, int edge);
void exb_finish_create(struct ExpandedBarterWnd *self, int height);
void exb_draw(struct ExpandedBarterWnd *self, int market);
void exb_press(struct ExpandedBarterWnd *self, int x, int y);
void exb_show_pay_tooltip(struct ExpandedBarterWnd *self, int x, int y, int times_count);
void exb_show_item_at(const struct ExpandedBarterWnd *self, int x, int y);
void exb_wheel(struct ExpandedBarterWnd *self, int delta);
int exb_scroll(struct ExpandedBarterWnd *self, int msg, int val1);
void exb_refresh(struct ExpandedBarterWnd *self, const struct Vec *goods);
void exb_resize(struct ExpandedBarterWnd *self, int width, int height);
int exb_snap_height(int height);

struct ExpandedBarterWnd *ui_expanded_bartermarket_wnd_construct(struct ExpandedBarterWnd *self);
struct UIWindow *ui_expanded_bartermarket_wnd_new(void);

struct UIExpandedBarterItemPurchaseWnd *ui_expanded_barter_item_purchase_wnd_construct(
    struct UIExpandedBarterItemPurchaseWnd *self);
struct UIWindow *ui_expanded_barter_item_purchase_wnd_new(void);

#endif
