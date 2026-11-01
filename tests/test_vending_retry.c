#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "game/mode_msg.h"
#include "session/session_items.h"
#include "iteminfo/item_api.h"
#include "vending_retry_unreached.h"

struct Session g_session1;
int g_replayPlaying;
static int connection;
static unsigned sends;
static BYTE sent[0x800];
static int sent_length;

struct RagConnection *rag_connection_instance(void)
{
    return (struct RagConnection *)&connection;
}

char rag_connection_send_packet(struct RagConnection *self, int length, const void *packet)
{
    assert(self == (struct RagConnection *)&connection);
    assert(length > 0 && length <= sizeof sent);
    memcpy(sent, packet, length);
    sent_length = length;
    ++sends;
    return 1;
}

int main(void)
{
    const BYTE entries[] = {7, 0, 3, 0, 0xe8, 3, 0, 0, 9, 0, 2, 0, 0xd0, 7, 0, 0};
    struct ItemInfo item;
    int attempt;
    list_init(&g_session1.storeSetupItems);
    string_init(&g_session1.shopTitle);
    item_info_construct(&item);
    item.index = 7;
    item.count = 3;
    item.charged_price = 1000;
    item_info_copy(list_push_back(&g_session1.storeSetupItems, NULL, sizeof item), &item);
    item.index = 9;
    item.count = 2;
    item.charged_price = 2000;
    item_info_copy(list_push_back(&g_session1.storeSetupItems, NULL, sizeof item), &item);
    item_info_destruct(&item);

    for (attempt = 0; attempt < 2; ++attempt) {
        assert(game_mode_msg_open_vending("Retry shop", 1) == 0);
        assert(sent_length == 101 && sent[0] == 0xb2 && sent[1] == 1);
        assert(sent[2] == 101 && sent[3] == 0 && sent[84] == 1);
        assert(strcmp((const char *)sent + 4, "Retry shop") == 0);
        assert(memcmp(sent + 85, entries, sizeof entries) == 0);
        assert(session_get_store_setup_size(&g_session1) == 2);
    }
    assert(sends == 2);
    assert(strcmp(string_cstr(&g_session1.shopTitle), "Retry shop") == 0);
    session_clear_store_setup(&g_session1);
    assert(session_get_store_setup_size(&g_session1) == 0);
    game_mode_msg_open_vending("Empty shop", 1);
    assert(sends == 3 && sent_length == 85 && sent[84] == 0);
    list_free(&g_session1.storeSetupItems, NULL);
    string_free(&g_session1.shopTitle);
    puts("Vending retry: repeated requests retain item indexes, quantities, prices and title; clearing the setup still sends no open shop passed.");
    return 0;
}
