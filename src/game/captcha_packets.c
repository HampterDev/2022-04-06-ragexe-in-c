#include "game/captcha_packets.h"
#include "game/macro_detector.h"
#include "game/packets_macro_detector.h"
#include "game/packet_ids.h"
#include "ui/chat/captcha_data.h"
#include <stdint.h>
#include "base.h"

#pragma pack(push, 1)
struct PACKET_ZC_ACK_APPLY_MACRO_DETECTOR {
    unsigned short packet_type;
    int result;
};
struct PACKET_ZC_APPLY_MACRO_DETECTOR {
    unsigned short packet_type;
    unsigned short image_size;
    char key[4];
};
struct PACKET_ZC_APPLY_MACRO_DETECTOR_CAPTCHA {
    unsigned short packet_type;
    unsigned short packet_length;
    char key[4];
    unsigned char data[];
};
struct PACKET_ZC_REQ_ANSWER_MACRO_DETECTOR {
    unsigned short packet_type;
    unsigned char retry_count;
    int timeout;
};
struct PACKET_ZC_CLOSE_MACRO_DETECTOR {
    unsigned short packet_type;
    int result;
};
struct PACKET_ZC_ACK_PREVIEW_MACRO_DETECTOR {
    unsigned short packet_type;
    int result;
    unsigned short image_size;
    char key[4];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_APPLY_MACRO_DETECTOR, 6);
SIZE_ASSERT(PACKET_ZC_APPLY_MACRO_DETECTOR, 8);
SIZE_ASSERT(PACKET_ZC_APPLY_MACRO_DETECTOR_CAPTCHA, 8);
SIZE_ASSERT(PACKET_ZC_REQ_ANSWER_MACRO_DETECTOR, 7);
SIZE_ASSERT(PACKET_ZC_CLOSE_MACRO_DETECTOR, 6);
SIZE_ASSERT(PACKET_ZC_ACK_PREVIEW_MACRO_DETECTOR, 12);

void game_mode_dispatch_captcha_packet(struct GameMode *game, int id, const void *packet, size_t size)
{
    const unsigned char *p = packet;
    unsigned length;
    if (!p || size < 2) return;
    switch (id) {
    case HEADER_ZC_ACK_APPLY_MACRO_DETECTOR: {
        const struct PACKET_ZC_ACK_APPLY_MACRO_DETECTOR *apply = packet;

        if (size >= 6) game_mode_zc_ack_apply_macro_detector(game, apply->result);
        break;
    }
    case HEADER_ZC_APPLY_MACRO_DETECTOR: {
        const struct PACKET_ZC_APPLY_MACRO_DETECTOR *apply = packet;

        if (size >= 8) game_mode_zc_apply_macro_detector(game, apply->image_size, apply->key);
        break;
    }
    case HEADER_ZC_APPLY_MACRO_DETECTOR_CAPTCHA:
    case HEADER_ZC_PREVIEW_MACRO_DETECTOR_CAPTCHA: {
        const struct PACKET_ZC_APPLY_MACRO_DETECTOR_CAPTCHA *chunk = packet;

        if (size < 8) return;
        length = chunk->packet_length;
        if (length <= 8 || length > size || length - 8 > CAPTCHA_CHUNK_LIMIT) return;
        if (id == HEADER_ZC_APPLY_MACRO_DETECTOR_CAPTCHA)
            game_mode_zc_apply_macro_detector_captcha(game, chunk->key, chunk->data, length - 8);
        else
            game_mode_on_preview_macro_detector_captcha(game, chunk->key, chunk->data, (unsigned short)(length - 8));
        break;
    }
    case HEADER_ZC_REQ_ANSWER_MACRO_DETECTOR: {
        const struct PACKET_ZC_REQ_ANSWER_MACRO_DETECTOR *answer = packet;

        if (size >= 7) game_mode_on_req_answer_macro_detector(game, answer->retry_count, answer->timeout);
        break;
    }
    case HEADER_ZC_CLOSE_MACRO_DETECTOR: {
        const struct PACKET_ZC_CLOSE_MACRO_DETECTOR *close = packet;

        if (size >= 6) game_mode_zc_close_macro_detector(game, close->result);
        break;
    }
    case HEADER_ZC_ACK_PREVIEW_MACRO_DETECTOR: {
        const struct PACKET_ZC_ACK_PREVIEW_MACRO_DETECTOR *preview = packet;

        if (size >= 12) game_mode_zc_ack_preview_macro_detector(game, preview->result, preview->image_size, preview->key);
        break;
    }
    }
}
