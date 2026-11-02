#include "game/packet_handlers.h"
#include "game/gamemode.h"
#include "game/ui_bridge.h"
#include "core/msg_str.h"
#include "ui/items/refine_controls.h"

enum {
    WID_GRADE_ENCHANT = 0x157,
    MSG_GRADE_ENCHANT_NOT_POSSIBLE = 0xe88,
};
#define CHAT_COLOR_ENCHANT 0x0062b0ff

#pragma pack(push, 1)
struct PacketGradeEnchantMaterialList {
    unsigned short packet_type;
    short packet_length;
    unsigned short index;
    int success_chance;
    struct GradeBlessing blessing;
    int protect_item_id;
    int protect_amount;
    unsigned char materials[1];
};
#pragma pack(pop)

void game_mode_on_grade_enchant_material_list(struct GameMode *self,
                                                   const struct PacketGradeEnchantMaterialList *packet)
{
    unsigned material_count = (unsigned)(packet->packet_length - 0x22) / 17;
    struct UIWindow *wnd;

    (void)self;
    if (packet->index == 0xffff) {
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)msg_str(MSG_GRADE_ENCHANT_NOT_POSSIBLE),
                            CHAT_COLOR_ENCHANT, 0, 0);
        return;
    }
    wnd = ui_get_window(WID_GRADE_ENCHANT);
    if (wnd)
        ui_grade_enchant_wnd_set_materials((struct UIGradeEnchantWnd *)wnd, packet->index, packet->success_chance, material_count, packet->blessing,
                   packet->protect_item_id, packet->protect_amount, packet->materials);
}
