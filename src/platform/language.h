#ifndef PLATFORM_LANGUAGE_H
#define PLATFORM_LANGUAGE_H

#include "platform/language_api.h"
#include "platform/code_page.h"
#include "core/text_view.h"
#include "platform/win_types.h"
#include <imm.h>

enum LanguageInputMethod { LANGUAGE_INPUT_DEFAULT, LANGUAGE_INPUT_ZHUYIN };
struct Language {
    enum LanguageInputMethod input_method;
    CANDIDATELIST *candidates;
    int candidate_count;
    int candidate_page;
    int candidate_selection;
    char *input;
    size_t input_length, input_capacity;
    int caret, anchor;
    char reading[9];
    size_t reading_length;
    char composition[256];
    size_t composition_length;
    int composition_cursor;
    BYTE masked;
    WORD keyboard_lang;
    UINT code_page;
    const char *folder;
    char *owned_folder;
    WORD folder_lang;
    char *lookup_path;
    char *display_text;
    size_t display_capacity;
    char charset_tag[20];
    char candidate_text[256];
};

struct ZhuyinKey {
    BYTE kind;
    BYTE symbol[2];
    BYTE zero;
};

struct LanguageTC {
    struct Language base;
    BYTE keys[5];
    unsigned key_count;
    struct ZhuyinKey table[256];
};

extern int g_serviceType;

struct Language *language_construct(struct Language *self);
struct Language *language_th_construct(struct Language *self);
void language_destruct(struct Language *self);
int language_replace_selection(struct Language *self, const char *text);
const char *language_input_text(const struct Language *self);
void language_paste(struct Language *self);
void language_copy(struct Language *self);
void language_caret_back(struct Language *self);
void language_delete_selection(struct Language *self);
void language_caret_forward(struct Language *self);
const char *language_candidate_text(struct Language *self, int index);
void language_read_candidates(struct Language *self);
void language_read_composition(struct Language *self);
void language_read_cursor_pos(struct Language *self);
void language_insert_result(struct Language *self);
const char *language_reading(struct Language *self);
int language_reading_length(struct Language *self);
int language_get_text(struct Language *self, struct TextView *out);
void language_get_selection(struct Language *self, int *anchor, int *caret);
const char *language_char_next(const char *p);
void language_set_masked(struct Language *self, BYTE masked);
void language_move_caret_forward(struct Language *self, int count);
void language_move_caret_back(struct Language *self, int count);
void language_on_char(struct Language *self, int ch);
void language_on_key_down(struct Language *self, int key);
int language_on_ime_message(struct Language *self, HWND hwnd, UINT msg, WPARAM wparam,
                                      LPARAM lparam, BYTE *handled);
void language_cancel_input(struct Language *self);
void language_set_selection(struct Language *self, int anchor, int caret);

struct Language *language_tc_construct(struct LanguageTC *self);
const char *language_tc_reading(struct LanguageTC *self);
int language_tc_reading_length(struct LanguageTC *self);
void language_tc_on_key(struct LanguageTC *self, BYTE key);
void language_tc_read_composition(struct LanguageTC *self);
void language_tc_clear_keys(struct LanguageTC *self);
void language_tc_on_composition_changed(struct LanguageTC *self);

void language_on_reading_key(struct Language *self, BYTE key);
int language_set_folder(struct Language *self, const char *folder, WORD language);

struct ThaiBreakIterator;
extern struct ThaiBreakIterator g_thaiBreakIterator;

#endif
