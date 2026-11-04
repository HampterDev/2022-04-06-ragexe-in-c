#include "ui/window_access.h"
#include "ui/window_mgr.h"
#include "ui/window_manager_state.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include "platform/language.h"
#include "platform/window.h"
#include "platform/system.h"
#include "core/app.h"
#include "core/utf8.h"

/* The input editor every edit box shares, the same on every platform. The IME side is
   platform/language_win32.c on Windows and platform/language_posix.c elsewhere. */

struct Language *g_language;

#define KEY_DOWN(vk)  ((platform_key_state(vk) & 0xff00) != 0)

struct Language *language_construct(struct Language *self)
{
    memset(self, 0, sizeof *self);
    switch (platform_user_language() & 0x3ff) {
    case LANGUAGE_ID_GERMAN:     self->folder_lang = LANGUAGE_ID_GERMAN;     break;
    case LANGUAGE_ID_ITALIAN:    self->folder_lang = LANGUAGE_ID_ITALIAN;    break;
    case LANGUAGE_ID_PORTUGUESE: self->folder_lang = LANGUAGE_ID_PORTUGUESE; break;
    case LANGUAGE_ID_TURKISH:    self->folder_lang = LANGUAGE_ID_TURKISH;    break;
    default:                     self->folder_lang = LANGUAGE_ID_ENGLISH;    break;
    }
    return self;
}

void language_destruct(struct Language *self)
{
    free(self->input);
    free(self->display_text);
    memset(self, 0, sizeof *self);
}

void language_delete_selection(struct Language *self)
{
    language_replace_selection(self, "");
}

int language_replace_selection(struct Language *self, const char *text)
{
    int first = self->anchor < self->caret ? self->anchor : self->caret;
    int last = self->anchor < self->caret ? self->caret : self->anchor;
    size_t length = strlen(text), size, tail, capacity;
    uintptr_t source = (uintptr_t)text, start = (uintptr_t)self->input;
    int aliases = self->input && source >= start && source - start <= self->input_length;
    char *buffer;

    if (first < 0 || (size_t)last > self->input_length || length > INT_MAX)
        return 0;
    size = self->input_length - (last - first);
    if (length > INT_MAX - size)
        return 0;
    size += length;
    tail = self->input_length - last;
    if (!size) {
        if (self->input) self->input[0] = 0;
    } else if (size + 1 > self->input_capacity || aliases) {
        capacity = self->input_capacity ? self->input_capacity : 64;
        while (capacity < size + 1) {
            if (capacity > (size_t)INT_MAX / 2) { capacity = size + 1; break; }
            capacity *= 2;
        }
        buffer = malloc(capacity);
        if (!buffer) return 0;
        if (first) memcpy(buffer, self->input, first);
        if (length) memcpy(buffer + first, text, length);
        if (tail) memcpy(buffer + first + length, self->input + last, tail);
        buffer[size] = 0;
        free(self->input);
        self->input = buffer;
        self->input_capacity = capacity;
    } else {
        memmove(self->input + first + length, self->input + last, tail + 1);
        if (length) memcpy(self->input + first, text, length);
    }
    self->input_length = size;
    self->caret = self->anchor = first + (int)length;
    return 1;
}

const char *language_input_text(const struct Language *self)
{
    return self->input ? self->input : "";
}

/* The clipboard holds Unicode text; platform_clipboard_get gives it as UTF-8. */
void language_paste(struct Language *self)
{
    char *text;

    if (ui_window_mgr_get_focus_edit(&g_windowMgr) == NULL)
        return;
    text = platform_clipboard_get(g_hMainWnd);
    if (text) {
        language_replace_selection(self, text);
        free(text);
    }
}

int language_copy_text(const char *text, size_t length)
{
    if (length > INT_MAX)
        return 0;
    return platform_clipboard_set(g_hMainWnd, text, length);
}

void language_copy(struct Language *self)
{
    int first = self->anchor < self->caret ? self->anchor : self->caret;
    int last = self->anchor < self->caret ? self->caret : self->anchor;
    size_t length;
    char *masked;

    if (first < 0 || (unsigned)last > self->input_length || first == last)
        return;
    length = (size_t)(last - first);
    if (!self->masked) {
        language_copy_text(language_input_text(self) + first, length);
        return;
    }
    masked = malloc(length);
    if (!masked)
        return;
    memset(masked, '*', length);
    language_copy_text(masked, length);
    free(masked);
}

void language_caret_back(struct Language *self)
{
    const char *s;

    if (self->caret == 0)
        return;
    if (self->caret < 0) {
        self->caret = 0;
        return;
    }
    s = language_input_text(self);
    self->caret = (int)(utf8_prev(s, s + self->caret) - s);
}

void language_caret_forward(struct Language *self)
{
    const char *s;
    unsigned size = self->input_length;

    if ((unsigned)self->caret == size)
        return;
    if ((unsigned)self->caret > size) {
        self->caret = size;
        return;
    }
    s = language_input_text(self);
    self->caret = (int)(utf8_step(s + self->caret, s + size) - s);
}

int language_get_text(struct Language *self, struct TextView *out)
{
    size_t composition = strlen(self->composition), size, caret;
    char *buffer;
    if (!composition) {
        *out = text_view(language_input_text(self), self->input_length);
        return 1;
    }
    if (self->caret < 0 || (size_t)self->caret > self->input_length ||
        composition > INT_MAX - self->input_length)
        return 0;
    size = self->input_length + composition;
    caret = self->caret;
    if (size + 1 > self->display_capacity) {
        buffer = realloc(self->display_text, size + 1);
        if (!buffer) return 0;
        self->display_text = buffer;
        self->display_capacity = size + 1;
    }
    if (caret) memcpy(self->display_text, self->input, caret);
    memcpy(self->display_text + caret, self->composition, composition);
    if (self->input_length > caret)
        memcpy(self->display_text + caret + composition, self->input + caret, self->input_length - caret);
    self->display_text[size] = 0;
    *out = text_view(self->display_text, size);
    return 1;
}

void language_get_selection(struct Language *self, int *anchor, int *caret)
{
    *anchor = self->anchor;
    *caret = self->caret;
}

void language_set_masked(struct Language *self, BYTE masked)
{
    self->masked = masked;
}

void language_move_caret_forward(struct Language *self, int count)
{
    unsigned size = self->input_length;

    if ((unsigned)self->caret == size)
        return;
    if ((unsigned)self->caret > size)
        self->caret = size;
    else if ((unsigned)(count + self->caret) <= size)
        self->caret += count;
    if (!KEY_DOWN(VK_SHIFT))
        self->anchor = self->caret;
}

void language_move_caret_back(struct Language *self, int count)
{
    if (self->caret == 0)
        return;
    if (self->caret < 0)
        self->caret = 0;
    else if (self->caret - count >= 0)
        self->caret -= count;
    if (!KEY_DOWN(VK_SHIFT))
        self->anchor = self->caret;
}

static void insert_code_point(struct Language *self, unsigned long code_point)
{
    char text[UTF8_CHAR_MAX + 1];
    size_t length = utf8_encode(code_point, text);

    text[length] = 0;
    if (length)
        language_replace_selection(self, text);
}

/* ch is one UTF-16 unit from WM_CHAR; a surrogate pair arrives as two messages. */
void language_on_char(struct Language *self, int ch)
{
    unsigned long points[2];
    size_t count, i;

    switch (ch) {
    case VK_BACK:
        self->pending_unit = 0;
        if (self->caret == self->anchor)
            language_caret_back(self);
        language_delete_selection(self);
        return;
    case '\t': case '\n': case '\r': case 0x1b:
        self->pending_unit = 0;
        return;
    }
    if ((unsigned)ch <= 0x1f || (unsigned)ch > 0xffff) {
        self->pending_unit = 0;
        return;
    }
    count = utf16_feed(&self->pending_unit, (WCHAR)ch, points);
    for (i = 0; i < count; i++)
        insert_code_point(self, points[i]);
}

static void ctrl_key(struct Language *self, int key)
{
    if (key == 'C') {
        language_copy(self);
    } else if (key == 'V') {
        language_paste(self);
    } else if (key == 'X') {
        language_copy(self);
        language_delete_selection(self);
    }
}

static void plain_key(struct Language *self, int key)
{
    switch (key) {
    case VK_END:
        self->caret = self->input_length;
        self->anchor = self->input_length;
        break;
    case VK_HOME:
        self->anchor = 0;
        self->caret = 0;
        break;
    case VK_LEFT:
        language_caret_back(self);
        self->anchor = self->caret;
        break;
    case VK_RIGHT:
        language_caret_forward(self);
        self->anchor = self->caret;
        break;
    case VK_DELETE:
        if (self->caret == self->anchor)
            language_caret_forward(self);
        language_delete_selection(self);
        break;
    }
}

static void shift_key(struct Language *self, int key)
{
    switch (key) {
    case VK_END:
        self->caret = self->input_length;
        break;
    case VK_HOME:
        self->caret = 0;
        break;
    case VK_LEFT:
        language_caret_back(self);
        break;
    case VK_RIGHT:
        language_caret_forward(self);
        break;
    case VK_INSERT:
        language_paste(self);
        break;
    case VK_DELETE:
        language_copy(self);
        language_delete_selection(self);
        break;
    }
}

void language_on_key_down(struct Language *self, int key)
{
    if (KEY_DOWN(VK_SHIFT))
        shift_key(self, key);
    else if (KEY_DOWN(VK_CONTROL))
        ctrl_key(self, key);
    else
        plain_key(self, key);
}

void language_cancel_input(struct Language *self)
{
    platform_ime_cancel(g_hMainWnd);
    self->input_length = 0;
    if (self->input) self->input[0] = 0;
    self->caret = self->input_length;
    self->anchor = self->input_length;
    self->composition_length = 0;
    self->composition[0] = 0;
    self->pending_unit = 0;
}

void language_set_selection(struct Language *self, int anchor, int caret)
{
    unsigned size;

    self->anchor = anchor;
    self->caret = caret;
    if (anchor < 0) {
        self->anchor = 0;
        anchor = 0;
    }
    size = self->input_length;
    if ((unsigned)anchor > size)
        self->anchor = size;
    if (caret < 0) {
        self->caret = 0;
        return;
    }
    if ((unsigned)caret > size)
        self->caret = size;
}
