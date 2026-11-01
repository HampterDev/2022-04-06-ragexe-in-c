#ifndef CONTENTS_ACCOUNT_SETTINGS_INTERNAL_H
#define CONTENTS_ACCOUNT_SETTINGS_INTERNAL_H

#include "contents/account_settings.h"
#include "json/service_json.h"
#include <windows.h>

struct AccountUserDataMgr {
    struct JsonTree defaults;
    int have_defaults, loaded;
    char path[MAX_PATH];
};

int account_settings_capture(struct JsonTree *out);
int account_settings_apply(const struct JsonTree *data);

#endif
