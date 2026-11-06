#include "../src/ui/items/item_collection_text.c"
#include <assert.h>
#include <stdio.h>

static int quit_calls, quit_id;

BYTE ui_window_mgr_post_quit1(struct UIWindowMgr *self, int window_id)
{
    assert(self == &g_windowMgr);
    ++quit_calls;
    quit_id = window_id;
    return 1;
}

static void set_item(struct ItemInfo *item, const char *id, int refine, int card)
{
    memset(item, 0, sizeof *item);
    string_init(&item->id_text);
    string_assign_cstr(&item->id_text, id);
    item->refine = refine;
    item->slot[0] = card;
}

int main(void)
{
    static struct UIItemCollectionWnd window;
    struct ItemInfo same;

    set_item(&window.item, "1201", 4, 4001);
    window.wnd_id = 0xc;
    set_item(&same, "1201", 4, 4001);
    assert(item_collection_set_item(&window, &same, 0, 0) == 0);
    assert(quit_calls == 1 && quit_id == 0xc);
    puts("Item collection: right-clicking the item a description window shows closes it passed.");
    return 0;
}
