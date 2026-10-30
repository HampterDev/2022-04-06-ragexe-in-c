#include "login/loginmode.h"

const struct ModeOps g_CLoginMode_ops = {
    login_mode_on_update,
    login_mode_send_msg,
};
