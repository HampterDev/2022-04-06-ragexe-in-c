#include "ui/chat/captcha.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include <string.h>
#include "game/macro_detector.h"

enum {
    WID_MACRO_DETECTOR = 0x11d,
    WID_MACRO_CAPTCHA_PREVIEW = 0x120,
    HEADER_CZ_ACK_AU_BOT = 0xa2a,
};

void game_mode_on_preview_macro_detector_captcha(struct GameMode *self, const char *key, const void *image,
                                                       unsigned short image_size)
{
    struct UIWindow *wnd = ui_window_find(WID_MACRO_CAPTCHA_PREVIEW);

    (void)self;
    captcha_preview_append(wnd, key, image, image_size);
}

void game_mode_on_req_answer_macro_detector(struct GameMode *self, int retry_count, int timeout)
{
    struct UIWindow *wnd = ui_window_find(WID_MACRO_DETECTOR);

    (void)self;
    if (wnd == 0)
        return;
    ui_window_show(wnd, 1);
    captcha_detector_set_timer(wnd, (unsigned char)retry_count, timeout > 0 ? (unsigned long)timeout : 0);
}

#pragma pack(push, 1)
struct PacketAckAuBot {
    unsigned short packet_type;
    int aid;
};
#pragma pack(pop)

void game_mode_on_req_au_bot(struct GameMode *self, int aid)
{
    struct PacketAckAuBot ack;
    int size;

    (void)self;
    ack.packet_type = HEADER_CZ_ACK_AU_BOT;
    ack.aid = aid;
    size = rag_connection_get_packet_length(rag_connection_instance(), HEADER_CZ_ACK_AU_BOT);
    rag_connection_send_packet(rag_connection_instance(), size, &ack);
}
