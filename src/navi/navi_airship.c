#include "contents/connection_api.h"
#include "navi/navi_manager.h"
#include "game/cells.h"
#include "render/texture_cache.h"
#include "std/string.h"
#include "contents/client_lua_calls.h"
#include "world/pathfinder_state.h"
#include "navi/navi_search.h"
#include "resources/attr.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "platform/win_types.h"
#include "fileio/resource_api.h"
#include <string.h>
#include "fileio/res_mgr.h"
#include "ui/navigation_view.h"
#include "core/cstr.h"

#pragma pack(push, 1)
struct PacketPrivateAirshipRequest {
    short id;
    char mapName[16];
    int itemId;
};
#pragma pack(pop)
OFFSET_ASSERT(PacketPrivateAirshipRequest, itemId, 0x12);
SIZE_ASSERT(PacketPrivateAirshipRequest, 0x16);

void navi_manager_request_airship(struct NaviManager *self, struct String mapName,
        int itemId)
{
    if (mapName.size != 0) {
        struct PacketPrivateAirshipRequest pkt;
        int len;

        pkt.id = 0xa49;
        cstr_copy(pkt.mapName, 0x10, string_cstr(&mapName));
        pkt.itemId = itemId;
        len = rag_connection_get_packet_length(rag_connection_instance(), pkt.id);
        rag_connection_send_packet(rag_connection_instance(), len, &pkt);
    }
    string_free(&mapName);
}
