#include "ui/window_access.h"
#include "ui/window_mgr.h"
#include "ui/window_manager_state.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include "platform/language.h"
#include "core/app.h"
#include "core/utf8.h"

struct Language *g_language;

#define KEY_DOWN(vk)  ((GetKeyState(vk) & 0xff00) != 0)

struct Language *language_construct(struct Language *self)
{
    memset(self, 0, sizeof *self);
    switch (GetUserDefaultLangID() & 0x3ff) {
    case LANG_GERMAN:     self->folder_lang = LANG_GERMAN;     break;
    case LANG_ITALIAN:    self->folder_lang = LANG_ITALIAN;    break;
    case LANG_PORTUGUESE: self->folder_lang = LANG_PORTUGUESE; break;
    case LANG_TURKISH:    self->folder_lang = LANG_TURKISH;    break;
    default:              self->folder_lang = LANG_ENGLISH;    break;
    }
    return self;
}

void language_destruct(struct Language *self)
{
    free(self->input);
    free(self->candidates);
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

/* Converts count UTF-16 units to a new NUL-terminated UTF-8 string; NULL if out of memory. */
static char *utf8_copy_of(const WCHAR *wide, size_t count)
{
    char *text;

    if (count > (SIZE_MAX - 1) / 3)
        return NULL;
    text = malloc(count * 3 + 1);
    if (text)
        utf8_from_utf16(text, count * 3 + 1, wide, count);
    return text;
}

void language_paste(struct Language *self)
{
    HANDLE data;
    const WCHAR *wide;
    char *text;

    if (ui_window_mgr_get_focus_edit(&g_windowMgr) == NULL)
        return;
    if (!OpenClipboard(g_hMainWnd))
        return;
    data = GetClipboardData(CF_UNICODETEXT);
    if (data) {
        wide = (const WCHAR *)GlobalLock(data);
        if (wide) {
            text = utf8_copy_of(wide, wcsnlen(wide, GlobalSize(data) / sizeof(WCHAR)));
            if (text) {
                language_replace_selection(self, text);
                free(text);
            }
            GlobalUnlock(data);
        }
    }
    CloseClipboard();
}

int language_copy_text(const char *text, size_t length)
{
    int units = 0, copied = 0;
    HGLOBAL block;
    WCHAR *wide;

    if (length > INT_MAX)
        return 0;
    if (length && !(units = MultiByteToWideChar(CP_UTF8, 0, text, (int)length, NULL, 0)))
        return 0;
    if (!OpenClipboard(g_hMainWnd))
        return 0;
    block = GlobalAlloc(GMEM_MOVEABLE, ((size_t)units + 1) * sizeof(WCHAR));
    if (block) {
        wide = GlobalLock(block);
        if (wide) {
            if (units)
                MultiByteToWideChar(CP_UTF8, 0, text, (int)length, wide, units);
            wide[units] = 0;
            GlobalUnlock(block);
            if (EmptyClipboard() && SetClipboardData(CF_UNICODETEXT, block)) {
                block = NULL;
                copied = 1;
            }
        }
        if (block)
            GlobalFree(block);
    }
    CloseClipboard();
    return copied;
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

const char *language_candidate_text(struct Language *self, int index)
{
    const WCHAR *text;

    if (index < 0 || index >= self->candidate_count)
        return "";
    text = (const WCHAR *)((const BYTE *)self->candidates + self->candidates->dwOffset[index]);
    utf8_from_utf16(self->candidate_text, sizeof self->candidate_text, text, wcslen(text));
    return self->candidate_text;
}

static int candidate_list_valid(const CANDIDATELIST *list, size_t bytes)
{
    size_t header = offsetof(CANDIDATELIST, dwOffset), strings, offset, limit;
    DWORD i;
    const WCHAR *text;
    if (bytes < header || list->dwSize < header || list->dwSize > bytes)
        return 0;
    if (list->dwCount > (list->dwSize - header) / sizeof(DWORD))
        return 0;
    if (list->dwCount && list->dwSelection >= list->dwCount)
        return 0;
    if (list->dwStyle == IME_CAND_CODE && list->dwCount == 1)
        return 0;
    strings = header + list->dwCount * sizeof(DWORD);
    for (i = 0; i < list->dwCount; ++i) {
        offset = list->dwOffset[i];
        if (offset < strings || offset > list->dwSize || offset % sizeof(WCHAR))
            return 0;
        limit = (list->dwSize - offset) / sizeof(WCHAR);
        text = (const WCHAR *)((const BYTE *)list + offset);
        while (limit && *text) { ++text; --limit; }
        if (!limit) return 0;
    }
    return 1;
}

void language_read_candidates(struct Language *self)
{
    HIMC imc = ImmGetContext(g_hMainWnd);
    DWORD size, copied;
    CANDIDATELIST *list = NULL;

    self->candidate_count = self->candidate_selection = self->candidate_page = 0;
    if (!imc) return;
    size = ImmGetCandidateListW(imc, 0, NULL, 0);
    if (size < offsetof(CANDIDATELIST, dwOffset) || size > INT_MAX)
        goto done;
    list = malloc(size);
    if (!list) goto done;
    copied = ImmGetCandidateListW(imc, 0, list, size);
    if (copied > size || !candidate_list_valid(list, copied))
        goto done;
    free(self->candidates);
    self->candidates = list;
    list = NULL;
    self->candidate_count = self->candidates->dwCount;
    self->candidate_selection = self->candidates->dwSelection;
    self->candidate_page = self->candidates->dwPageSize < 9 ? self->candidates->dwPageSize : 9;
 done:
    free(list);
    ImmReleaseContext(g_hMainWnd, imc);
}

/* Reads an IME string of any length; NULL if the IME reports an invalid size. */
static WCHAR *ime_string(HIMC imc, DWORD kind, size_t *count)
{
    LONG bytes = ImmGetCompositionStringW(imc, kind, NULL, 0);
    WCHAR *wide;

    if (bytes < 0 || bytes % sizeof(WCHAR))
        return NULL;
    wide = malloc((size_t)bytes + sizeof(WCHAR));
    if (!wide)
        return NULL;
    if (bytes && ImmGetCompositionStringW(imc, kind, wide, (DWORD)bytes) != bytes) {
        free(wide);
        return NULL;
    }
    *count = (size_t)bytes / sizeof(WCHAR);
    wide[*count] = 0;
    return wide;
}

void language_read_composition(struct Language *self)
{
    HIMC imc = ImmGetContext(g_hMainWnd);
    WCHAR *wide;
    size_t count;

    if (!imc) {
        self->composition_length = 0;
        self->composition[0] = 0;
        return;
    }
    wide = ime_string(imc, GCS_COMPSTR, &count);
    ImmReleaseContext(g_hMainWnd, imc);
    if (!wide)
        return;
    self->composition_length = utf8_from_utf16(self->composition, sizeof self->composition, wide, count);
    free(wide);
}

/* The IME reports the cursor in UTF-16 units; the editor needs a byte offset. */
void language_read_cursor_pos(struct Language *self)
{
    HIMC imc = ImmGetContext(g_hMainWnd);
    LONG cursor;
    WCHAR *wide = NULL;
    char *text;
    size_t count, units;

    if (!imc) {
        self->composition_length = 0;
        self->composition[0] = 0;
        return;
    }
    cursor = ImmGetCompositionStringW(imc, GCS_CURSORPOS, NULL, 0);
    if (cursor >= 0)
        wide = ime_string(imc, GCS_COMPSTR, &count);
    ImmReleaseContext(g_hMainWnd, imc);
    if (!wide)
        return;
    units = (size_t)cursor < count ? (size_t)cursor : count;
    if (units && units < count && wide[units - 1] >= 0xd800 && wide[units - 1] <= 0xdbff)
        units++;
    text = utf8_copy_of(wide, units);
    if (text) {
        self->composition_cursor = (int)strlen(text);
        free(text);
    }
    free(wide);
}

void language_insert_result(struct Language *self)
{
    HIMC imc = ImmGetContext(g_hMainWnd);
    WCHAR *wide;
    char *text;
    size_t count;

    if (!imc) {
        self->composition_length = 0;
        self->composition[0] = 0;
        return;
    }
    wide = ime_string(imc, GCS_RESULTSTR, &count);
    ImmReleaseContext(g_hMainWnd, imc);
    if (!wide)
        return;
    text = utf8_copy_of(wide, count);
    if (text) {
        language_replace_selection(self, text);
        free(text);
    }
    free(wide);
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

int language_on_ime_message(struct Language *self, HWND hwnd, UINT msg, WPARAM wparam,
                                      LPARAM lparam, BYTE *handled)
{
    switch (msg) {
    case WM_IME_STARTCOMPOSITION:
        language_delete_selection(self);
        break;
    case WM_INPUTLANGCHANGE:
        break;
    case WM_IME_ENDCOMPOSITION:
        self->composition[0] = 0;
        self->composition_length = 0;
        break;
    case WM_IME_COMPOSITION:
        if (lparam & GCS_COMPSTR)
            language_read_composition(self);
        if (lparam & GCS_CURSORPOS)
            language_read_cursor_pos(self);
        if (lparam & GCS_RESULTSTR)
            language_insert_result(self);
        break;
    case WM_IME_NOTIFY:
        if (wparam == IMN_OPENCANDIDATE || wparam == IMN_CHANGECANDIDATE)
            language_read_candidates(self);
        else if (wparam == IMN_CLOSECANDIDATE)
            self->candidate_count = 0;
        break;
    default:
        *handled = 0;
        return 0;
    }
    *handled = 1;
    return 0;
}

void language_cancel_input(struct Language *self)
{
    HIMC imc = ImmGetContext(g_hMainWnd);

    if (imc) {
        ImmNotifyIME(imc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
        ImmReleaseContext(g_hMainWnd, imc);
    }
    self->input_length = 0;
    if (self->input) self->input[0] = 0;
    self->caret = self->input_length;
    self->anchor = self->input_length;
    self->composition_length = 0;
    self->composition[0] = 0;
    self->candidate_count = 0;
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
