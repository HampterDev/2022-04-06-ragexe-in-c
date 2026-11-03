#ifdef _WIN32
#include "platform/language.h"
#include "core/app.h"
#include <imm.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

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
