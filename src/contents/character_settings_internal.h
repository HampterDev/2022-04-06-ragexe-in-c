#ifndef CONTENTS_CHARACTER_SETTINGS_INTERNAL_H
#define CONTENTS_CHARACTER_SETTINGS_INTERNAL_H

#include "contents/character_settings.h"
#include "json/service_json.h"
#include "platform/win_types.h"

struct CharUserDataMgr {
    int loaded;
    char path[MAX_PATH];
};

void character_settings_seed(void);
int character_settings_clear(void);
int character_settings_capture(struct JsonTree *out);
int character_settings_apply(const struct JsonTree *data);

#endif
