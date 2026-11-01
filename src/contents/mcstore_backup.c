#include "json/service_json.h"
#include "session/state.h"
#include "session/session_items.h"
#include "ui/window_manager_state.h"
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "contents/game_session.h"
#include "iteminfo/item_api.h"
#include "contents/mcstore_backup.h"
#include "contents/client_res.h"
#include "contents/ui_window_mgr.h"
#include "core/msg_str.h"

#define ANSWER_OK 1
#define ANSWER_LOAD_EMPTY 4
#define WM_MSG_SHOW_CHAT 3
#define MSG_SAVED_0 0xe65
#define MSG_SAVED_1 0xe66
#define COLOR_YELLOW 0xffff

static void build_urls(struct McStoreBackup *self)
{
    struct OwnedText save = {0}, load = {0};
    if (session_service_urls(&g_session1, &save, "/MerchantStore/save",
            &load, "/MerchantStore/load")) {
        owned_text_clear(&self->save_url);
        owned_text_clear(&self->load_url);
        self->save_url = save;
        self->load_url = load;
    } else {
        owned_text_clear(&save);
        owned_text_clear(&load);
    }
}

void mcstore_init(struct McStoreBackup *self)
{
    memset(self, 0, sizeof *self);
}

void mcstore_clear_items(struct McStoreBackup *self)
{
    free(self->items);
    self->items = NULL;
    self->item_count = 0;
}

void mcstore_destroy(struct McStoreBackup *self)
{
    mcstore_clear_items(self);
    owned_text_clear(&self->title);
    owned_text_clear(&self->save_url);
    owned_text_clear(&self->load_url);
    mcstore_init(self);
}

static void inventory_item(struct ItemInfo *out, const struct McStoreItem *saved)
{
    int i;
    item_info_construct(out);
    item_info_set_item_id(out, saved->id);
    out->count = saved->count;
    out->price = saved->price;
    out->identified = (unsigned char)saved->identified;
    out->refine = saved->refine;
    memcpy(out->slot, saved->slot, sizeof out->slot);
    out->option_count = saved->option_count;
    for (i = 0; i < MCSTORE_OPTION_COUNT; ++i) {
        out->options[i].id = saved->options[i].id;
        out->options[i].value = saved->options[i].value;
        out->options[i].param = saved->options[i].param;
    }
}

void mcstore_restore_shop(struct McStoreBackup *self, char mode, int count)
{
    size_t at, kept = 0;
    if (count <= 0) return;
    for (at = 0; at < self->item_count && kept < (size_t)count; ++at) {
        struct ItemInfo item;
        struct InventoryMatch match;
        inventory_item(&item, &self->items[at]);
        session_find_inventory_item(&g_session1, &match, &item);
        item_info_destruct(&item);
        if (match.found && match.item.index != 0 && match.item.count > 0 &&
            !session_get_store_setup_count(&g_session1, match.item.index)) {
            if (!mode && match.item.count > self->items[at].count)
                match.item.count = self->items[at].count;
            if (session_sell_list_add(&g_session1, &match.item, (unsigned char)mode, 0)) {
                session_mirror_remove(&g_session1, &match.item, (unsigned char)mode, 0);
                self->items[kept] = self->items[at];
                if (!mode) self->items[kept].count = match.item.count;
                ++kept;
            }
        }
        item_info_destruct(&match.item);
    }
    self->item_count = kept;
}

void mcstore_reset(struct McStoreBackup *self)
{
    self->loaded = 0;
    owned_text_clear(&self->title);
    mcstore_clear_items(self);
    build_urls(self);
}

static char parse_answer(struct JsonTree *root, struct TextView response)
{
    return json_parse_text(root, response.data, response.size) && json_value_is_object(root);
}

int mcstore_read_items(struct McStoreBackup *self, const struct JsonTree *data)
{
    const struct JsonTree *items = json_tree_find(data, "Item", 4);
    struct OwnedText title = {0};
    struct McStoreItem *copy = NULL;
    size_t i, count = 0, total = json_value_size(items);
    int notify;

    if (!owned_text_assign(&title, json_value_text(json_tree_find(data, "Title", 5))))
        return 0;
    for (i = 0; i < total; ++i) {
        const struct JsonTree *item = items->type == JSON_TREE_ARRAY ? &items->u.items[i] : &items->u.members[i].value;
        if (json_value_is_object(item)) ++count;
    }
    if (count) {
        if (count > SIZE_MAX / sizeof *copy) goto failed;
        copy = malloc(count * sizeof *copy);
        if (!copy) goto failed;
        count = 0;
        for (i = 0; i < total; ++i) {
            const struct JsonTree *item = items->type == JSON_TREE_ARRAY ? &items->u.items[i] : &items->u.members[i].value;
            if (json_value_is_object(item)) mcstore_read_item(&copy[count++], item);
        }
    }
    notify = !json_value_is_null(items) || title.size != 0;
    mcstore_clear_items(self);
    owned_text_clear(&self->title);
    self->title = title;
    self->items = copy;
    self->item_count = count;
    if (notify) {
        if (g_windowMgr.merchant_item_wnd)
            ui_merchant_shop_make_wnd_apply_backup((struct UIMerchantShopMakeWnd *)g_windowMgr.merchant_item_wnd);
        else if (g_windowMgr.hot_key_wnd)
            ui_merchant_shop_make_wnd_apply_backup((struct UIMerchantShopMakeWnd *)g_windowMgr.hot_key_wnd);
    }
    return 1;

failed:
    owned_text_clear(&title);
    return 0;
}

void mcstore_on_loaded(struct McStoreBackup *self, int succeeded, struct TextView response)
{
    if (succeeded) {
        struct JsonTree root;

        json_tree_init(&root);

        if (parse_answer(&root, response)) {
            switch (json_value_as_int(json_value_member(&root, "Type"))) {
            case ANSWER_OK:
                if (mcstore_read_items(self, json_tree_find(&root, "data", 4)))
                    self->loaded = 1;
                break;
            case ANSWER_LOAD_EMPTY:
                self->loaded = 1;
                break;
            }
        }

        json_tree_free(&root);
    }
}

void mcstore_on_saved(struct McStoreBackup *self, int succeeded,
                                 struct TextView response)
{
    self->saves_pending--;
    if (succeeded) {
        struct JsonTree root;

        json_tree_init(&root);

        if (parse_answer(&root, response)
            && json_value_as_int(json_value_member(&root, "Type")) == ANSWER_OK) {
            ui_window_mgr_send_msg_(&g_windowMgr, WM_MSG_SHOW_CHAT, 0, 0, 0, 0);
            if (self->save_type == 0)
                ui_window_mgr_send_msg_(&g_windowMgr, UIWM_CHAT_LINE, (int)msg_str(MSG_SAVED_0), COLOR_YELLOW, 0, 0);
            else if (self->save_type == 1)
                ui_window_mgr_send_msg_(&g_windowMgr, UIWM_CHAT_LINE, (int)msg_str(MSG_SAVED_1), COLOR_YELLOW, 0, 0);
        }

        json_tree_free(&root);
    }
}
