#ifndef RENDER_FONT_FAMILY_H
#define RENDER_FONT_FAMILY_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int font_backend_start(uintptr_t *token);
void font_backend_stop(uintptr_t token);

int font_file_read_family(const wchar_t *path, wchar_t *name, size_t capacity);

#ifdef __cplusplus
}
#endif
#endif
