#ifdef _WIN32
#include "platform/language.h"
#include "core/window_state.h"
#include "core/utf8.h"
#include <imm.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* The Windows IME side of platform/language.c: composition and result text from the input
   context, read in UTF-16 and kept as UTF-8, and the place of the IME's own candidate list. */

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

void language_place_candidates(struct Language *self, int x, int top, int bottom)
{
    struct LanguageCandidatePos *pos = &self->candidate_pos;
    CANDIDATEFORM form;
    HIMC imc;

    if (pos->placed && pos->x == x && pos->top == top && pos->bottom == bottom)
        return;
    imc = ImmGetContext(g_hMainWnd);
    if (!imc)
        return;
    /* Some IMEs take only the list's position, others only the area to keep clear. */
    form.dwIndex = 0;
    form.dwStyle = CFS_CANDIDATEPOS;
    form.ptCurrentPos.x = x;
    form.ptCurrentPos.y = bottom;
    SetRectEmpty(&form.rcArea);
    ImmSetCandidateWindow(imc, &form);
    form.dwStyle = CFS_EXCLUDE;
    form.ptCurrentPos.y = top;
    SetRect(&form.rcArea, x, top, x + 1, bottom);
    ImmSetCandidateWindow(imc, &form);
    ImmReleaseContext(g_hMainWnd, imc);
    pos->x = x;
    pos->top = top;
    pos->bottom = bottom;
    pos->placed = 1;
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

int language_on_ime_message(struct Language *self, HWND hwnd, UINT msg, WPARAM wparam,
                                      LPARAM lparam, BYTE *handled)
{
    (void)hwnd;
    (void)wparam;
    switch (msg) {
    case WM_IME_STARTCOMPOSITION:
        language_delete_selection(self);
        self->candidate_pos.placed = 0;
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
    default:
        *handled = 0;
        return 0;
    }
    *handled = 1;
    return 0;
}
#endif
