#include "platform/language_api.h"
#include "std/std.h"
#include "session/state.h"
#include "core/table_reader.h"
#include "core/once.h"
#include "core/fatal.h"
#include <stdlib.h>

struct Session g_session1;
struct Language *g_language;

struct File *file_construct(struct File *file) { abort(); }
unsigned char file_open(struct File *file, const char *name, int mode, unsigned char a, unsigned char b) { abort(); }
unsigned char file_read(struct File *file, void *out, int size) { abort(); }
void file_close(struct File *file) { abort(); }
void file_destruct(struct File *file) { abort(); }
unsigned char file_make_file_name(const char *name, int flags) { abort(); }
unsigned char table_reader_read(struct TableReader *self, const char *name, enum TableReadMode mode) { abort(); }
void table_reader_release(struct TableReader *self) { abort(); }
void error_msg(const char *text, int flags) { abort(); }
void session_remove_underbar(const char *src, char *dst) { abort(); }
void string_free(struct String *text) { abort(); }
int string_compare(const struct String *a, const struct String *b) { abort(); }

void *std_allocate(size_t count, size_t size, size_t max_count) { abort(); }
