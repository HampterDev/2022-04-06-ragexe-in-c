#include "login/otp.h"
#include "login/loginmode.h"
#include <string.h>
#include "base.h"
#include "core/msg_str.h"
#include "ui/character/character.h"

static void open_wait_window(struct LoginMode *self)
{
    self->waitWnd = ui_window_open(6);
    ui_wait_wnd_set_text(self->waitWnd, msg_str(0x7a));
}

#pragma pack(push, 1)
struct PACKET_CA_ACK_MOBILE_OTP {
    WORD packet_type;
    WORD packet_length;
    DWORD otp_id;
    BYTE code[6];
};
struct PACKET_CT_MOTP {
    WORD packet_type;
    BYTE code[9];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_CA_ACK_MOBILE_OTP, 14);
SIZE_ASSERT(PACKET_CT_MOTP, 11);

int login_send_mobile_otp(struct LoginMode *self)
{
    struct PACKET_CA_ACK_MOBILE_OTP packet = {0x9a3, sizeof packet};
    size_t i;

    packet.otp_id = (DWORD)self->mobile_otp_id;
    for (i = 0; i < 6 && self->otp_code[i]; ++i)
        packet.code[i] = self->otp_code[i];
    rag_connection_send_packet(rag_connection_instance(), sizeof(packet), &packet);
    open_wait_window(self);
    return 0;
}

int login_send_otp(struct LoginMode *self)
{
    struct PACKET_CT_MOTP packet = {0xad0};

    memcpy(packet.code, self->otp_code, sizeof packet.code);
    login_send_packet(0xad0, &packet);
    open_wait_window(self);
    return 0;
}

void login_set_otp_code(struct LoginMode *self, const char *code)
{
    BYTE text[sizeof(self->otp_code)] = {0};
    size_t i;

    for (i = 0; i + 1 < sizeof(text) && code && code[i]; ++i)
        text[i] = (BYTE)code[i];
    memcpy(self->otp_code, text, sizeof(text));
}

unsigned long login_state_tick(const struct LoginMode *mode)
{
    return mode->stateTick;
}
