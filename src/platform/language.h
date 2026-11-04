#ifndef PLATFORM_LANGUAGE_H
#define PLATFORM_LANGUAGE_H

#include "platform/language_api.h"
#include "platform/code_page.h"
#include "core/text_view.h"
#include "platform/win_types.h"

/* IME composition text in UTF-8: 256 UTF-16 units of up to three bytes. */
#define LANGUAGE_TEXT_MAX (0x100 * 3 + 1)

/* Primary language IDs of the user's Windows language (platform_user_language), as LANG_* has them. */
enum {
    LANGUAGE_ID_GERMAN = 0x07,
    LANGUAGE_ID_ENGLISH = 0x09,
    LANGUAGE_ID_ITALIAN = 0x10,
    LANGUAGE_ID_PORTUGUESE = 0x16,
    LANGUAGE_ID_TURKISH = 0x1f
};

/* Where the IME's own candidate list was last placed, in main-window client coordinates. */
struct LanguageCandidatePos {
    int x, top, bottom;
    BYTE placed;
};

struct Language {
    char *input;
    size_t input_length, input_capacity;
    int caret, anchor;
    char composition[LANGUAGE_TEXT_MAX];
    size_t composition_length;
    int composition_cursor;
    BYTE masked;
    WCHAR pending_unit;
    WORD folder_lang;
    char *display_text;
    size_t display_capacity;
    struct LanguageCandidatePos candidate_pos;
};

struct Language *language_construct(struct Language *self);
void language_destruct(struct Language *self);
int language_replace_selection(struct Language *self, const char *text);
const char *language_input_text(const struct Language *self);
void language_paste(struct Language *self);
void language_copy(struct Language *self);
int language_copy_text(const char *text, size_t length);
void language_caret_back(struct Language *self);
void language_delete_selection(struct Language *self);
void language_caret_forward(struct Language *self);
/* The client draws the composition itself and leaves the candidate list to the IME's own
   window. x is where the composition starts and top..bottom the line it is on, in client
   coordinates of the main window; the IME opens the list below that line, or above it when
   there is no room. Windows: platform/language_win32.c; elsewhere platform/language_posix.c keeps
   the place for the SDL layer (core/app_sdl.c), which hands it to SDL's text input area. */
void language_place_candidates(struct Language *self, int x, int top, int bottom);
int language_get_text(struct Language *self, struct TextView *out);
void language_get_selection(struct Language *self, int *anchor, int *caret);
void language_set_masked(struct Language *self, BYTE masked);
void language_move_caret_forward(struct Language *self, int count);
void language_move_caret_back(struct Language *self, int count);
void language_on_char(struct Language *self, int ch);
void language_on_key_down(struct Language *self, int key);
#ifdef _WIN32
void language_read_composition(struct Language *self);
void language_read_cursor_pos(struct Language *self);
void language_insert_result(struct Language *self);
int language_on_ime_message(struct Language *self, HWND hwnd, UINT msg, WPARAM wparam,
                                      LPARAM lparam, BYTE *handled);
#else
/* SDL's composition (UTF-8, cursor in characters, -1 for the end) and committed text. */
void language_set_composition(struct Language *self, const char *text, int cursor);
void language_commit(struct Language *self, const char *text);
#endif
void language_cancel_input(struct Language *self);
void language_set_selection(struct Language *self, int anchor, int caret);

#endif
