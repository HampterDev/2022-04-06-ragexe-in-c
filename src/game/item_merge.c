#include "game/packet_handlers.h"
#include "contents/connection_api.h"
#include "game/gamemode.h"
#include "game/ui_bridge.h"
#include "game/session_bridge.h"

enum {
    WID_ITEM_MERGE = 0xe3,
    MERGE_MSG_SET_ITEMS = 0x22,
    HEADER_CZ_CANCEL_MERGE_ITEM = 0x974,
};

#pragma pack(push, 1)
struct PacketMergeItemOpen {
    unsigned short packet_type;
    short packet_length;
    unsigned short indexes[1];
};
#pragma pack(pop)

void game_mode_on_merge_item_open(struct GameMode *self, const struct PacketMergeItemOpen *packet)
{
    int count = (unsigned)(packet->packet_length - 4) >> 1;
    struct Vec indexes;
    struct UIWindow *wnd;

    (void)self;
    if (count == 0) {
        unsigned short cancel = HEADER_CZ_CANCEL_MERGE_ITEM;
        int size = rag_connection_get_packet_length(rag_connection_instance(), HEADER_CZ_CANCEL_MERGE_ITEM);
        rag_connection_send_packet(rag_connection_instance(), size, &cancel);
        return;
    }
    indexes.first = indexes.last = indexes.end = 0;
    vec_insert_range(&indexes, indexes.last, packet->indexes, packet->indexes + count, 1);
    wnd = ui_make_window(WID_ITEM_MERGE);
    if (wnd)
        ui_window_send(wnd, NULL, MERGE_MSG_SET_ITEMS, (int)indexes.first, count, 0, 0);
    vec_free(&indexes);
}
