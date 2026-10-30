#include "ui/window_messages.h"
#include "fileio/res_mgr.h"
#include "ui/window_manager_state.h"
#include "core/fatal.h"
#include "core/abstract_method.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

struct UIWindowMgr g_windowMgr;
int uncompress(unsigned char *dest, unsigned long *dest_len, const unsigned char *source, unsigned long source_len) { abort(); }
void abstract_method_called(void) { abort(); }
struct ResMgr *g_resMgr(void) { abort(); }
BYTE file_make_file_name(const char *name, int flags) { abort(); }
char resource_test_error[260];
void error_msg(const char *text, int flags)
{
    assert(flags == 0 && strlen(text) < sizeof resource_test_error);
    strcpy(resource_test_error, text);
}
int ui_window_mgr_send_msg_(struct UIWindowMgr *mgr, int msg, int a, int b, int c, int d) { abort(); }
void fatal_error(const char *text) { abort(); }

void std_vector_bytes_resize(struct Vec *v, size_t count) { abort(); }
void std_sort(void *first, void *last, size_t size, StdSortLessFn less, void *pred) { abort(); }
struct Dec *dec_construct(struct Dec *self, const BYTE *key) { abort(); }
void dec_destruct(struct Dec *self) { abort(); }
BYTE dec_decode(struct Dec *self, const BYTE *src, int len, BYTE *dst, int *outLen, int cycle) { abort(); }
BYTE dec_decode_header(struct Dec *self, const BYTE *src, int len, BYTE *dst, int *outLen) { abort(); }
