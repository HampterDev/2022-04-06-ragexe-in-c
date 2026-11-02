#ifndef LOGINMODE_OTP_H
#define LOGINMODE_OTP_H

#include "platform/win_types.h"

struct LoginMode;

void login_set_otp_code(struct LoginMode *mode, const char *code);
int login_send_mobile_otp(struct LoginMode *mode);
int login_send_otp(struct LoginMode *mode);
DWORD login_state_tick(const struct LoginMode *mode);

#endif
