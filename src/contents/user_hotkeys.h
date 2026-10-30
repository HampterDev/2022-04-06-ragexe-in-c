#ifndef CONTENTS_USER_HOTKEYS_H
#define CONTENTS_USER_HOTKEYS_H

#include "core/owned_text.h"

struct UserHotkey {
    int group, id, key1, key2;
    struct OwnedText key_name;
};

struct UserHotkeyList {
    struct UserHotkey *items;
    size_t size, capacity;
};

struct UserHotkeyMgr {
    int settings_read;
    int apply_count;
    int lacks_v2_save;
    struct UserHotkeyList keys;
};

extern struct UserHotkeyMgr *g_userHotkeyMgr;

void user_hotkey_mgr_init(struct UserHotkeyMgr *self);
void user_hotkey_mgr_reset(struct UserHotkeyMgr *self);
int user_hotkey_mgr_create(void);
void user_hotkey_mgr_release(void);

int user_hotkey_tab_group(struct UserHotkeyMgr *self, int tab);
void user_hotkey_mgr_clear_saved(struct UserHotkeyMgr *self);
void user_hotkey_mgr_save(struct UserHotkeyMgr *self);
void user_hotkey_mgr_change(struct UserHotkeyMgr *self, int group, int id,
                           int key1, int key2, const char *key_name);

int user_hotkey_mgr_collect(struct UserHotkeyMgr *self);
char user_hotkey_mgr_has_keys_to_save(struct UserHotkeyMgr *self);

#endif
