#ifndef PLATFORM_LANGUAGE_H
#define PLATFORM_LANGUAGE_H

#include "platform/language_api.h"
#include "platform/code_page.h"
#include "core/text_view.h"
#include <windows.h>
#include <imm.h>

struct Language {
    CANDIDATELIST *candidates;
    int candidate_count;
    int candidate_page;
    int candidate_selection;
    char *input;
    size_t input_length, input_capacity;
    int caret, anchor;
    char composition[256];
    size_t composition_length;
    int composition_cursor;
    BYTE masked;
    WORD folder_lang;
    char *display_text;
    size_t display_capacity;
    char candidate_text[256];
};

struct Language *language_construct(struct Language *self);
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
int language_get_text(struct Language *self, struct TextView *out);
void language_get_selection(struct Language *self, int *anchor, int *caret);
void language_set_masked(struct Language *self, BYTE masked);
void language_move_caret_forward(struct Language *self, int count);
void language_move_caret_back(struct Language *self, int count);
void language_on_char(struct Language *self, int ch);
void language_on_key_down(struct Language *self, int key);
int language_on_ime_message(struct Language *self, HWND hwnd, UINT msg, WPARAM wparam,
                                      LPARAM lparam, BYTE *handled);
void language_cancel_input(struct Language *self);
void language_set_selection(struct Language *self, int anchor, int caret);

#endif
