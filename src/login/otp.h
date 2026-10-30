#ifndef LOGINMODE_OTP_H
#define LOGINMODE_OTP_H

struct LoginMode;

void login_set_otp_code(struct LoginMode *mode, const char *code);
int login_send_mobile_otp(struct LoginMode *mode);
int login_send_otp(struct LoginMode *mode);
unsigned long login_state_tick(const struct LoginMode *mode);

#endif
