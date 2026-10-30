#include "session/hotkey_bindings.h"
#ifndef SESSION_SESSION_OPTIONS_H
#define SESSION_SESSION_OPTIONS_H

#include "session/csession.h"

void session_load_options(struct Session *self);

void session_save_options(struct Session *self);

void session_reset_option2(struct Session *self);

int session_get_option_int(struct Session *self, const char *name, int fallback);
float session_get_option_float(struct Session *self, const char *name, float fallback);
struct String *session_get_option_string(struct Session *self, struct String *out,
                                                     const char *name, const char *fallback);
void session_write_option_int(struct Session *self, struct File *file, const char *name, int value);
void session_write_option_float(struct Session *self, struct File *file, const char *name,
                                         float value);
void session_write_option_string(struct Session *self, struct File *file, const char *name,
                                          const char *value);

void session_set_bar_hotkey(struct Session *self, BYTE isSkill, int id, int level, int slot, int row);
void session_clear_hotkey_tab0(struct Session *self);
void session_clear_hotkey_tab1(struct Session *self);
void session_clear_shorten_skill_levels(struct Session *self);
void session_clear_quick_slots(struct Session *self);
void session_refresh_guild_skill_hotkeys(struct Session *self);

int session_hot_key_page_slot(struct Session *self, int slot, int bar);
void session_reset_the_hot_keys(struct Session *self);
int session_get_shorten_skill_id(struct Session *self, int index);
void session_set_shorten_skill_id(struct Session *self, int level, int index);
void session_set_shorten_skill_name(struct Session *self, const char *name, int level, int slot);

#include "session/chat_macros.h"

#endif
