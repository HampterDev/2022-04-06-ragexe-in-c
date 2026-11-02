#include "ui/window_access.h"
#include "ui/window_mgr.h"
#include "fileio/resource_api.h"
#include "ui/window_manager_state.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include "platform/language.h"
#include "platform/window.h"
#ifdef _WIN32
#include <imm.h>
#endif
#include "platform/system.h"
#include "core/app.h"
#include "ui/misc/thai_lexicon.h"
#include "platform/window.h"

struct ThaiBreakIterator g_thaiBreakIterator;

struct Language *g_language;

#define KEY_DOWN(vk)  ((platform_key_state(vk) & 0xff00) != 0)

enum {
    LANGUAGE_ID_GERMAN = 0x07,
    LANGUAGE_ID_ENGLISH = 0x09,
    LANGUAGE_ID_ITALIAN = 0x10,
    LANGUAGE_ID_PORTUGUESE = 0x16,
    LANGUAGE_ID_TURKISH = 0x1f
};

struct Language *language_construct(struct Language *self)
{
    memset(self, 0, sizeof *self);
    self->input_method = LANGUAGE_INPUT_DEFAULT;
    self->keyboard_lang = platform_keyboard_language();
    self->code_page = g_useCharsetTag ? code_page_from_lang(self->keyboard_lang) : g_codePage;
    switch (platform_user_language() & 0x3ff) {
    case LANGUAGE_ID_GERMAN:     self->folder_lang = LANGUAGE_ID_GERMAN;     self->folder = "german\\";     break;
    case LANGUAGE_ID_ITALIAN:    self->folder_lang = LANGUAGE_ID_ITALIAN;    self->folder = "italian\\";    break;
    case LANGUAGE_ID_PORTUGUESE: self->folder_lang = LANGUAGE_ID_PORTUGUESE; self->folder = "portuguese\\"; break;
    case LANGUAGE_ID_TURKISH:    self->folder_lang = LANGUAGE_ID_TURKISH;    self->folder = "turkish\\";    break;
    default:                    self->folder_lang = LANGUAGE_ID_ENGLISH;    self->folder = "english\\";    break;
    }
    if (g_serviceType == 0xf)
        self->code_page = 1258;
    return self;
}

struct Language *language_th_construct(struct Language *self)
{
    language_construct(self);
    thai_break_iterator_initialize_lexicon(&g_thaiBreakIterator);
    return self;
}

void language_destruct(struct Language *self)
{
    free(self->input);
    free(self->candidates);
    free(self->owned_folder);
    free(self->lookup_path);
    free(self->display_text);
    memset(self, 0, sizeof *self);
    self->folder = "";
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

void language_paste(struct Language *self)
{
    char *text;

    if (g_windowMgr.macro_detector_edit == ui_window_mgr_get_focus_edit(&g_windowMgr))
        return;
    text = platform_clipboard_get(g_hMainWnd);
    if (text) {
        language_replace_selection(self, text);
        free(text);
    }
}

void language_copy(struct Language *self)
{
    int first = self->anchor < self->caret ? self->anchor : self->caret;
    int last = self->anchor < self->caret ? self->caret : self->anchor;
    size_t length;
    char *text;

    if (first < 0 || (unsigned)last > self->input_length || first == last)
        return;
    length = (size_t)(last - first);
    text = malloc(length);
    if (!text)
        return;
    if (self->masked)
        memset(text, '*', length);
    else
        memcpy(text, language_input_text(self) + first, length);
    platform_clipboard_set(g_hMainWnd, text, length);
    free(text);
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
    self->caret = (int)(platform_code_page_prev(self->code_page, s, s + self->caret) - s);
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
    self->caret = (int)(platform_code_page_next(self->code_page, s + self->caret) - s);
}

#ifdef _WIN32
const char *language_candidate_text(struct Language *self, int index)
{
    int length;
    if (index < 0 || index >= self->candidate_count)
        return "";
    length = WideCharToMultiByte(self->code_page, 0,
        (LPCWSTR)((const BYTE *)self->candidates + self->candidates->dwOffset[index]), -1,
        self->candidate_text, sizeof self->candidate_text, NULL, NULL);
    if ((unsigned)length >= sizeof self->candidate_text)
        length = sizeof self->candidate_text - 1;
    self->candidate_text[length] = 0;
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

void language_read_composition(struct Language *self)
{
    WCHAR wide[0x100];
    char text[0x100];
    HIMC imc = ImmGetContext(g_hMainWnd);
    LONG bytes;
    int length;

    if (!imc) {
        self->composition_length = 0;
        self->composition[0] = 0;
        return;
    }
    bytes = ImmGetCompositionStringW(imc, GCS_COMPSTR, wide, sizeof wide);
    if (bytes < 0 || (size_t)bytes > sizeof wide || (size_t)bytes % sizeof(WCHAR)) {
        ImmReleaseContext(g_hMainWnd, imc);
        return;
    }
    length = bytes ? WideCharToMultiByte(self->code_page, 0, wide, (int)(bytes / sizeof(WCHAR)),
                                        text, sizeof text - 1, NULL, NULL) : 0;
    if (bytes && !length) {
        ImmReleaseContext(g_hMainWnd, imc);
        return;
    }
    ImmReleaseContext(g_hMainWnd, imc);
    if (length) memcpy(self->composition, text, length);
    self->composition[length] = 0;
    self->composition_length = length;
}

void language_read_cursor_pos(struct Language *self)
{
    HIMC imc = ImmGetContext(g_hMainWnd);
    LONG cursor;

    if (!imc) {
        self->composition_length = 0;
        self->composition[0] = 0;
        return;
    }
    cursor = ImmGetCompositionStringA(imc, GCS_CURSORPOS, NULL, 0);
    if (cursor >= 0) self->composition_cursor = cursor;
    ImmReleaseContext(g_hMainWnd, imc);
}

void language_insert_result(struct Language *self)
{
    WCHAR wide[0x100];
    char text[0x100];
    HIMC imc = ImmGetContext(g_hMainWnd);
    LONG bytes;
    int length;

    if (!imc) {
        self->composition_length = 0;
        self->composition[0] = 0;
        return;
    }
    bytes = ImmGetCompositionStringW(imc, GCS_RESULTSTR, wide, sizeof wide);
    if (bytes < 0 || (size_t)bytes > sizeof wide || (size_t)bytes % sizeof(WCHAR)) {
        ImmReleaseContext(g_hMainWnd, imc);
        return;
    }
    length = bytes ? WideCharToMultiByte(self->code_page, 0, wide, (int)(bytes / sizeof(WCHAR)),
                                        text, sizeof text - 1, NULL, NULL) : 0;
    if (bytes && !length) {
        ImmReleaseContext(g_hMainWnd, imc);
        return;
    }
    text[length] = 0;
    ImmReleaseContext(g_hMainWnd, imc);
    language_replace_selection(self, text);
}

#else
const char *language_candidate_text(struct Language *self, int index)
{
    (void)index;
    self->candidate_text[0] = 0;
    return self->candidate_text;
}

void language_read_candidates(struct Language *self)
{
    self->candidate_count = self->candidate_selection = self->candidate_page = 0;
}

void language_set_composition(struct Language *self, const char *text, int cursor)
{
    size_t length = strlen(text);
    if (length >= sizeof self->composition)
        length = sizeof self->composition - 1;
    memcpy(self->composition, text, length);
    self->composition[length] = 0;
    self->composition_length = length;
    self->composition_cursor = cursor;
}

void language_commit(struct Language *self, const char *text)
{
    self->composition[0] = 0;
    self->composition_length = 0;
    language_replace_selection(self, text);
}

#endif

const char *language_reading(struct Language *self)
{
    if (self->input_method == LANGUAGE_INPUT_ZHUYIN)
        return language_tc_reading((struct LanguageTC *)self);
    return self->reading;
}

int language_reading_length(struct Language *self)
{
    if (self->input_method == LANGUAGE_INPUT_ZHUYIN)
        return language_tc_reading_length((struct LanguageTC *)self);
    return self->reading_length;
}

const char *language_charset_tag(struct Language *self, BYTE folder_lang)
{
    if (!g_useCharsetTag) {
        self->charset_tag[0] = 0;
        return self->charset_tag;
    }
    snprintf(self->charset_tag, sizeof self->charset_tag, "|%02x",
             get_charset_from_lang(folder_lang ? self->folder_lang : self->keyboard_lang));
    return self->charset_tag;
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

const char *language_char_next(const char *p)
{
    return platform_code_page_next(g_codePage, p);
}

void language_set_masked(struct Language *self, BYTE masked)
{
    self->masked = masked;
}

const char *language_language_file(struct Language *self, const char *name)
{
    size_t folder, length;
    char *path;
    BYTE found;
    if (g_serviceType == 0 || g_serviceType == 2 || g_serviceType == 0x14)
        return name;
    folder = strlen(self->folder);
    length = strlen(name);
    if (length == SIZE_MAX || folder > SIZE_MAX - length - 1)
        return name;
    path = malloc(folder + length + 1);
    if (!path) return name;
    memcpy(path, self->folder, folder);
    memcpy(path + folder, name, length + 1);
    if (strstr(name, ".txt") || strstr(name, ".xml"))
        found = file_make_file_name(path, 0);
    else
        found = res_mgr_is_available(g_resMgr(), path);
    if (!found) { free(path); return name; }
    free(self->lookup_path);
    self->lookup_path = path;
    return path;
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

static void insert_char(struct Language *self, const int *ch)
{
    char text[sizeof *ch + 1];

    memcpy(text, ch, sizeof *ch);
    text[sizeof *ch] = 0;

    language_replace_selection(self, text);
}

void language_on_char(struct Language *self, int ch)
{
    switch (ch) {
    case VK_BACK:
        if (self->caret == self->anchor)
            language_caret_back(self);
        language_delete_selection(self);
        return;
    case '\t': case '\n': case '\r': case 0x1b:
        return;
    }
    if ((unsigned)ch <= 0x1f)
        return;
    insert_char(self, &ch);
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

#ifdef _WIN32
int language_on_ime_message(struct Language *self, HWND hwnd, UINT msg, WPARAM wparam,
                                      LPARAM lparam, BYTE *handled)
{
    switch (msg) {
    case WM_IME_STARTCOMPOSITION:
        language_delete_selection(self);
        break;
    case WM_INPUTLANGCHANGE:
        self->keyboard_lang = (WORD)lparam;
        self->code_page = g_useCharsetTag ? code_page_from_lang(self->keyboard_lang) : g_codePage;
        break;
    case WM_IME_ENDCOMPOSITION:
        if (self->input_method == LANGUAGE_INPUT_ZHUYIN)
            language_tc_clear_keys((struct LanguageTC *)self);
        self->composition[0] = 0;
        self->composition_length = 0;
        break;
    case WM_IME_COMPOSITION:
        if (lparam & GCS_COMPSTR) {
            if (self->input_method == LANGUAGE_INPUT_ZHUYIN) {
                language_tc_read_composition((struct LanguageTC *)self);
                language_tc_on_composition_changed((struct LanguageTC *)self);
            } else {
                language_read_composition(self);
            }
        }
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
#endif

void language_cancel_input(struct Language *self)
{
    platform_ime_cancel(g_hMainWnd);
    self->input_length = 0;
    if (self->input) self->input[0] = 0;
    self->caret = self->input_length;
    self->anchor = self->input_length;
    self->reading_length = 0;
    self->reading[0] = 0;
    self->composition_length = 0;
    self->composition[0] = 0;
    self->candidate_count = 0;
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

void language_on_reading_key(struct Language *self, BYTE key)
{
    if (self->input_method == LANGUAGE_INPUT_ZHUYIN)
        language_tc_on_key((struct LanguageTC *)self, key);
}

int language_set_folder(struct Language *self, const char *folder, WORD language)
{
    size_t length = strlen(folder);
    char *copy;
    if (length == SIZE_MAX) return 0;
    copy = malloc(length + 1);
    if (!copy) return 0;
    memcpy(copy, folder, length + 1);
    free(self->owned_folder);
    self->owned_folder = copy;
    self->folder = copy;
    self->folder_lang = language;
    return 1;
}
