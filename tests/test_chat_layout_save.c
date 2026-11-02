#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "session/chat_tabs_layout.h"

enum { NONE, PATH_FAIL, LONG_PATH, TEMP_FAIL, OPEN_FAIL, WRITE_FAIL,
       SHORT_WRITE, FLUSH_FAIL, CLOSE_FAIL, MOVE_FAIL };
static int fault, fail_write, writes, moves, deletes, live_handles, warnings, cleanup;
static int failed_delete;
static char directory[MAX_PATH], target[MAX_PATH], temporary[MAX_PATH];
static const char old_layout[] = "previous complete layout\r\n";
static struct { struct ChatTabs chatWndTabs, subChatWnds; int canDragWnd; } g_session1;

static DWORD WINAPI get_path(LPCSTR path, DWORD capacity, LPSTR out, LPSTR *name)
{
    if (fault == PATH_FAIL) return 0;
    if (fault == LONG_PATH) return capacity + 1;
    return GetFullPathNameA(path, capacity, out, name);
}
static UINT WINAPI get_temp(LPCSTR folder, LPCSTR prefix, UINT unique, LPSTR out)
{
    UINT result;
    if (fault == TEMP_FAIL) return 0;
    result = GetTempFileNameA(folder, prefix, unique, out);
    if (result) strcpy(temporary, out);
    return result;
}
static HANDLE WINAPI open_temp(LPCSTR path, DWORD access, DWORD share,
    LPSECURITY_ATTRIBUTES security, DWORD creation, DWORD flags, HANDLE template_file)
{
    HANDLE file;
    assert(!strcmp(path, temporary) && strcmp(path, target));
    assert(creation == TRUNCATE_EXISTING && access == GENERIC_WRITE);
    if (fault == OPEN_FAIL) return INVALID_HANDLE_VALUE;
    file = CreateFileA(path, access, share, security, creation, flags, template_file);
    if (file != INVALID_HANDLE_VALUE) ++live_handles;
    return file;
}
static BOOL WINAPI write_temp(HANDLE file, LPCVOID bytes, DWORD size,
                            LPDWORD written, LPOVERLAPPED overlap)
{
    ++writes;
    if (writes == fail_write) {
        if (fault == WRITE_FAIL) { *written = 0; return FALSE; }
        if (fault == SHORT_WRITE) return WriteFile(file, bytes, size / 2, written, overlap);
    }
    return WriteFile(file, bytes, size, written, overlap);
}
static BOOL WINAPI flush_temp(HANDLE file)
{
    if (fault == FLUSH_FAIL) return FALSE;
    return FlushFileBuffers(file);
}
static BOOL WINAPI close_temp(HANDLE file)
{
    BOOL result = CloseHandle(file);
    assert(result && live_handles == 1);
    --live_handles;
    return fault == CLOSE_FAIL ? FALSE : result;
}
static BOOL WINAPI replace_temp(LPCSTR from, LPCSTR to, DWORD flags)
{
    ++moves;
    assert(!live_handles && !strcmp(from, temporary) && !strcmp(to, target));
    assert(flags == (MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH));
    if (fault == MOVE_FAIL) return FALSE;
    return MoveFileExA(from, to, flags);
}
static BOOL WINAPI delete_temp(LPCSTR path)
{
    ++deletes;
    assert(!strcmp(path, temporary) && strcmp(path, target) && !live_handles);
    if (failed_delete) return FALSE;
    return DeleteFileA(path);
}

#define GetFullPathNameA get_path
#define GetTempFileNameA get_temp
#define CreateFileA open_temp
#define WriteFile write_temp
#define FlushFileBuffers flush_temp
#define CloseHandle close_temp
#define MoveFileExA replace_temp
#define DeleteFileA delete_temp
#include "../src/session/chat_tabs_layout.c"
#undef GetFullPathNameA
#undef GetTempFileNameA
#undef CreateFileA
#undef WriteFile
#undef FlushFileBuffers
#undef CloseHandle
#undef MoveFileExA
#undef DeleteFileA

/* Actual destruction function, with only UI cleanup and notification substituted. */
struct UIWindow { const int *ops; };
struct UIFrameWnd { struct UIWindow base; };
struct UINewChatWnd { struct UIFrameWnd frame; int boxes; };
static int ui_new_chat_wnd_ops;
static void vec_free(int *boxes) { (void)boxes; assert(cleanup == 0); cleanup = 1; }
static void ui_frame_wnd_destruct(struct UIFrameWnd *frame)
{ assert(frame->base.ops == &ui_new_chat_wnd_ops && cleanup == 1); cleanup = 2; }
static void mem_free(void *memory) { assert(cleanup == 2); cleanup = 3; free(memory); }
static void warn(HWND owner, const char *text, const char *title, unsigned flags)
{
    assert(!owner && cleanup == 3 && !live_handles);
    assert(strstr(text, "Could not save") && strstr(text, "not be restored"));
    assert(!strcmp(title, "Chat layout") && flags == (MB_OK | MB_ICONWARNING | MB_TASKMODAL));
    ++warnings;
}
#define CHAT_LAYOUT_FILE target
#include "platform/window.h"
#define MessageBoxA warn
#define platform_message_box warn
#include "chat_layout_destroy.inc"
#undef MessageBoxA
#undef platform_message_box

static void reset(int failure, int at)
{
    assert(!live_handles);
    fault = failure; fail_write = at;
    writes = moves = deletes = warnings = cleanup = failed_delete = 0;
    temporary[0] = 0;
}
static void seed(void)
{
    DWORD written;
    HANDLE file = CreateFileA(target, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    assert(file != INVALID_HANDLE_VALUE);
    assert(WriteFile(file, old_layout, sizeof old_layout - 1, &written, NULL));
    assert(written == sizeof old_layout - 1 && CloseHandle(file));
}
static void check_old(void)
{
    char bytes[sizeof old_layout];
    DWORD read;
    HANDLE file = CreateFileA(target, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    assert(file != INVALID_HANDLE_VALUE);
    assert(GetFileSize(file, NULL) == sizeof old_layout - 1);
    assert(ReadFile(file, bytes, sizeof bytes, &read, NULL) && read == sizeof old_layout - 1);
    assert(!memcmp(bytes, old_layout, read) && CloseHandle(file));
}
static void check_clean(void)
{
    assert(!live_handles);
    if (*temporary) assert(GetFileAttributesA(temporary) == INVALID_FILE_ATTRIBUTES);
}
static void destroy_window(void)
{
    struct UINewChatWnd *window = calloc(1, sizeof *window);
    assert(window);
    ui_new_chat_wnd_destroy(&window->frame.base);
    assert(cleanup == 3);
}

int main(void)
{
    struct ChatTabSettings tab = {0};
    char base[MAX_PATH], previous_directory[MAX_PATH], missing[MAX_PATH], too_long[MAX_PATH + 5];
    HANDLE held;
    int i, count;
    assert(GetTempPathA(MAX_PATH, base) > 0);
    assert(GetTempFileNameA(base, "cst", 0, directory));
    assert(DeleteFileA(directory) && CreateDirectoryA(directory, NULL));
    assert(snprintf(target, sizeof target, "%s\\layout.lua", directory) < MAX_PATH);
    assert(owned_text_assign(&tab.name, text_view_cstr("Party\\chat \"quoted\" ]]]")));
    assert(chat_tabs_set(&g_session1.chatWndTabs, 0, &tab));
    assert(chat_tabs_set(&g_session1.subChatWnds, 0, &tab));
    chat_tab_settings_clear(&tab);
    g_session1.canDragWnd = 1;
    seed(); reset(NONE, 0); destroy_window();
    assert(!warnings && moves == 1 && !deletes); check_clean();
    count = writes;
    assert(count > 20);

    /* Every serializer write can fail or succeed with a short byte count. */
    for (i = 1; i <= count; ++i) {
        seed(); reset(WRITE_FAIL, i); destroy_window();
        assert(warnings == 1 && writes == i && !moves && deletes == 1);
        check_old(); check_clean();
        seed(); reset(SHORT_WRITE, i); destroy_window();
        assert(warnings == 1 && writes == i && !moves && deletes == 1);
        check_old(); check_clean();
    }
    for (i = PATH_FAIL; i <= MOVE_FAIL; ++i) {
        if (i == WRITE_FAIL || i == SHORT_WRITE) continue;
        seed(); reset(i, 0); destroy_window();
        assert(warnings == 1);
        check_old(); check_clean();
    }
    seed(); reset(NONE, 0);
    held = CreateFileA(target, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    assert(held != INVALID_HANDLE_VALUE);
    destroy_window(); assert(warnings == 1 && moves == 1);
    check_old(); check_clean(); assert(CloseHandle(held));
    seed(); assert(SetFileAttributesA(target, FILE_ATTRIBUTE_READONLY));
    reset(NONE, 0); destroy_window(); assert(warnings == 1);
    check_old(); check_clean(); assert(SetFileAttributesA(target, FILE_ATTRIBUTE_NORMAL));
    assert(DeleteFileA(target));
    reset(WRITE_FAIL, 2); destroy_window(); assert(warnings == 1);
    assert(GetFileAttributesA(target) == INVALID_FILE_ATTRIBUTES); check_clean();
    reset(NONE, 0); destroy_window(); assert(!warnings && moves == 1); check_clean();

    seed(); reset(MOVE_FAIL, 0); failed_delete = 1; destroy_window();
    assert(warnings == 1 && deletes == 1); check_old();
    assert(DeleteFileA(temporary)); /* Simulated cleanup denial: remove our fixture. */
    reset(NONE, 0);
    assert(!chat_tabs_save_layout(&g_session1.chatWndTabs, &g_session1.subChatWnds, 1, NULL));
    assert(!chat_tabs_save_layout(&g_session1.chatWndTabs, &g_session1.subChatWnds, 1, ""));
    memset(too_long, 'x', sizeof too_long - 1); too_long[sizeof too_long - 1] = 0;
    assert(!chat_tabs_save_layout(&g_session1.chatWndTabs, &g_session1.subChatWnds, 1, too_long));
    check_old(); check_clean();
    assert(snprintf(missing, sizeof missing, "%s\\missing\\layout.lua", directory) < MAX_PATH);
    assert(!chat_tabs_save_layout(&g_session1.chatWndTabs, &g_session1.subChatWnds, 1, missing));
    check_old(); check_clean();
    assert(GetCurrentDirectoryA(MAX_PATH, previous_directory) > 0);
    assert(SetCurrentDirectoryA(directory));
    reset(NONE, 0);
    assert(chat_tabs_save_layout(&g_session1.chatWndTabs, &g_session1.subChatWnds, 1, "layout.lua"));
    assert(SetCurrentDirectoryA(previous_directory));
    assert(moves == 1); check_clean();
    assert(DeleteFileA(target)); assert(RemoveDirectoryA(directory));
    chat_tabs_clear(&g_session1.chatWndTabs); chat_tabs_clear(&g_session1.subChatWnds);
    printf("Chat saves: %d failed/short write positions, path/temp/open/flush/close/replace failures, real locked/read-only targets, first save and one warning after cleanup passed.\n", count * 2);
    return 0;
}
