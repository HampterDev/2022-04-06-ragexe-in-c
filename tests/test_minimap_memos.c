#include "ui/controls/buttons.h"
#include "ui/controls/edit.h"
#include "ui/ui_std.h"
#include "ui/window_mgr.h"
#include "session/state.h"
#include "core/app.h"
#include "core/fatal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr); exit(1); \
} } while (0)

struct Session g_session1;
struct UIWindowMgr g_windowMgr;
char g_clientDir[0x88];
static enum { IO_OK, OPEN_FAIL, WRITE_FAIL, CLOSE_FAIL, SEEK_FAIL, TELL_FAIL, READ_FAIL } failure;
static int closes, reloads, editor_closed, errors;
static char error_text[1024], written[1024];
static const char *existing = "";
static const char expected_path[] = "C:\\memo-test\\Memo\\$$prontera.txt";
static char note_text[32] = "hello";

void *mem_alloc(size_t n) { void *p = malloc(n); CHECK(p != NULL); return p; }
void mem_free(void *p) { free(p); }
void *std_allocate_bytes(size_t n) { return mem_alloc(n); }
__declspec(noreturn) void fatal_error(const char *s) { fputs(s, stderr); abort(); }
const char *ui_edit_ctrl_get_text(struct UIWindow *w) { (void)w; return note_text; }
int ui_edit_ctrl_get_text_size(struct UIWindow *w) { (void)w; return (int)strlen(note_text); }
BYTE ui_window_mgr_post_quit1(struct UIWindowMgr *m, int id)
{
    CHECK(m == &g_windowMgr && id == 0x112);
    editor_closed++;
    return 1;
}
int ui_window_mgr_message_box(struct UIWindowMgr *m, const char *text, int type,
                             int a, int b, int c, const char *title, int w, int h, int d)
{
    CHECK(m == &g_windowMgr && type == UI_MSGBOX_OK);
    (void)a; (void)b; (void)c; (void)title; (void)w; (void)h; (void)d;
    CHECK(strlen(text) < sizeof error_text);
    strcpy(error_text, text);
    errors++;
    return 0;
}
int ui_frame_wnd_send_msg(struct UIWindow *w, struct UIWindow *sender, int msg,
                         int a, int b, int c, int d)
{
    (void)w; (void)sender; (void)msg; (void)a; (void)b; (void)c; (void)d;
    abort();
}

static FILE *test_fopen(const char *path, const char *mode)
{
    FILE *file;
    CHECK(strcmp(path, expected_path) == 0);
    CHECK(strcmp(mode, "ab") == 0 || strcmp(mode, "a+b") == 0 || strcmp(mode, "wb") == 0);
    if (failure == OPEN_FAIL)
        return NULL;
    file = tmpfile();
    CHECK(file != NULL);
    if (mode[0] == 'a')
        CHECK(fwrite(existing, 1, strlen(existing), file) == strlen(existing));
    return file;
}
static size_t test_fwrite(const void *bytes, size_t size, size_t count, FILE *file)
{
    size_t n = size * count;
    if (failure == WRITE_FAIL) {
        CHECK(n > 0);
        CHECK(fwrite(bytes, 1, n - 1, file) == n - 1);
        return (n - 1) / size;
    }
    return fwrite(bytes, size, count, file);
}
static int test_fseek(FILE *file, long offset, int origin)
{
    return failure == SEEK_FAIL ? -1 : fseek(file, offset, origin);
}
static long test_ftell(FILE *file)
{
    return failure == TELL_FAIL ? -1 : ftell(file);
}
static int test_fgetc(FILE *file)
{
    return failure == READ_FAIL ? EOF : fgetc(file);
}
static int test_fclose(FILE *file)
{
    size_t n;
    CHECK(fflush(file) == 0);
    CHECK(fseek(file, 0, SEEK_SET) == 0);
    n = fread(written, 1, sizeof written - 1, file);
    written[n] = '\0';
    CHECK(fclose(file) == 0);
    closes++;
    return failure == CLOSE_FAIL ? EOF : 0;
}

#define fopen test_fopen
#define fwrite test_fwrite
#define fclose test_fclose
#define fseek test_fseek
#define ftell test_ftell
#define fgetc test_fgetc
#include "minimap_memos.inc"
#undef fopen
#undef fwrite
#undef fclose
#undef fseek
#undef ftell
#undef fgetc

static int minimap_load_memos(struct UIMiniMapWnd *self)
{
    CHECK(self != NULL && closes == 1 && errors == 0);
    reloads++;
    return 1;
}
static void reset(void)
{
    closes = reloads = editor_closed = errors = 0;
    error_text[0] = written[0] = '\0';
    existing = "";
    failure = IO_OK;
}
static void store(struct UIEditctrlWnd *editor)
{
    ui_editctrl_wnd_send_msg(&editor->frame.base, NULL, UI_MSG_COMMAND, CMD_MEMO_OK, 0, 0, 0);
    CHECK(strcmp(note_text, "hello") == 0);
}
static void check_failure(int mode)
{
    CHECK(errors == 1);
    CHECK(strstr(error_text, expected_path) != NULL);
    CHECK(editor_closed == 0 && reloads == 0);
    CHECK(closes == (mode == OPEN_FAIL ? 0 : 1));
}
int main(void)
{
    struct UIEditctrlWnd editor = {0};
    struct UIMiniMapWnd map = {0};
    struct UIBitmapButton pins[MEMO_MAX] = {0};
    const char *endings[] = { "", "\n", "\r", "\r\n" };
    char manual[64], expected[128];
    int mode, i;
    strcpy(g_clientDir, "C:\\memo-test");
    strcpy(g_session1.mapName, "prontera");
    g_windowMgr.minimap_info.memo_x = 80;
    g_windowMgr.minimap_info.memo_y = 90;
    g_windowMgr.mini_map_wnd = &map.frame.base;
    for (mode = OPEN_FAIL; mode <= READ_FAIL; mode++) {
        reset();
        failure = mode;
        existing = "20\t30\tmanual note\r\n";
        store(&editor);
        check_failure(mode);
    }
    reset();
    existing = "20\t30\tmanual note";
    failure = WRITE_FAIL;
    store(&editor);
    check_failure(WRITE_FAIL);
    reset();
    store(&editor);
    CHECK(errors == 0 && editor_closed == 1 && reloads == 1 && closes == 1);
    CHECK(strcmp(written, "80\t90\thello\r\n") == 0);
    for (i = 0; i < sizeof endings / sizeof endings[0]; i++) {
        reset();
        sprintf(manual, "20\t30\tmanual note%s", endings[i]);
        sprintf(expected, "20\t30\tmanual note%s80\t90\thello\r\n",
                strcmp(endings[i], "\n") == 0 ? "\n" : "\r\n");
        existing = manual;
        store(&editor);
        CHECK(errors == 0 && editor_closed == 1 && reloads == 1 && closes == 1);
        CHECK(strcmp(written, expected) == 0);
    }
    for (i = 0; i < MEMO_MAX; i++) {
        map.memos[i] = &pins[i];
        string_init(&pins[i].button.tooltip);
    }
    pins[0].button.base.x = 20;
    pins[0].button.base.y = 30;
    string_assign_cstr(&pins[0].button.tooltip, "keep");
    pins[1].button.base.x = 40;
    pins[1].button.base.y = 50;
    string_assign_cstr(&pins[1].button.tooltip, "remove");
    for (mode = OPEN_FAIL; mode <= CLOSE_FAIL; mode++) {
        reset();
        failure = mode;
        CHECK(minimap_save_memos(&map, 1) == -1);
        check_failure(mode);
        CHECK(pins[1].button.base.x == 40 && pins[1].button.base.y == 50);
        CHECK(strcmp(string_cstr(&pins[1].button.tooltip), "remove") == 0);
    }
    reset();
    CHECK(minimap_save_memos(&map, 1) == 1);
    CHECK(errors == 0 && closes == 1);
    CHECK(strcmp(written, "20\t30\tkeep\r\n") == 0);
    pins[1].button.base.x = pins[1].button.base.y = -100;
    reset();
    CHECK(minimap_save_memos(&map, 0) == 0);
    CHECK(errors == 0 && closes == 1 && written[0] == '\0');
    for (i = 0; i < MEMO_MAX; i++)
        string_free(&pins[i].button.tooltip);
    puts("Minimap memo save tests passed.");
    return 0;
}
