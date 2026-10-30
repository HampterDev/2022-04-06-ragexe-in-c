#include "replay/replay.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static int fail_allocation, opened;
static char opened_path[MAX_PATH];
static void *resize(void *old, size_t size)
{ return fail_allocation ? NULL : realloc(old, size); }
BYTE replay_recorder_open(struct ReplayRecorder *self, const char *path, int mode)
{
    assert(self && mode == REPLAY_OPEN_READ);
    strcpy(opened_path, path); ++opened; return 1;
}
#define realloc resize
#include "../src/replay/file_list.c"
#undef realloc

static void create_file(const char *path)
{
    FILE *file = fopen(path, "wb"); assert(file); assert(!fclose(file));
}

int main(void)
{
    struct ReplayFileList list;
    struct ReplayRecorder recorder = {0};
    char cwd[MAX_PATH], temp[MAX_PATH], directory[MAX_PATH], long_name[300];
    unsigned i;
    assert(GetCurrentDirectoryA(sizeof cwd, cwd));
    assert(GetTempPathA(sizeof temp, temp));
    assert(GetTempFileNameA(temp, "rfl", 0, directory));
    assert(DeleteFileA(directory) && CreateDirectoryA(directory, NULL));
    assert(SetCurrentDirectoryA(directory));
    assert(CreateDirectoryA("Replay", NULL));
    replay_file_list_construct(&list);
    assert(!replay_file_list_select(&list, 0));
    assert(!replay_name_ends_with_rrf(NULL));
    assert(!replay_name_ends_with_rrf(""));
    assert(!replay_name_ends_with_rrf("r"));
    assert(!replay_name_ends_with_rrf("rrf"));
    assert(replay_name_ends_with_rrf("a.RRF"));
    assert(recording_number("RagnarokReplay_0001.rrf") == 1);
    assert(!recording_number("SomethingElse_0001.rrf"));
    assert(!recording_number("RagnarokReplay_x001.rrf"));
    assert(!recording_number("RagnarokReplay_99999999999.rrf"));
    assert(replay_file_list_scan(&list) && !list.count);
    create_file("Replay\\RagnarokReplay_0002.rrf");
    create_file("Replay\\RagnarokReplay_0001.rrf");
    create_file("Replay\\custom.RRF");
    assert(replay_file_list_scan(&list) && list.count == 3);
    assert(!replay_file_list_get_name(&list, 3));
    assert(!replay_file_list_select(&list, -1));
    assert(replay_file_list_select(&list, 100) && list.selected == 2);
    assert(replay_file_list_set_recorder(&list, &recorder));
    list.selected = 3;
    assert(!replay_file_list_open_selected(&list, REPLAY_OPEN_READ) && !opened);
    list.selected = 0;
    assert(replay_file_list_open_selected(&list, REPLAY_OPEN_READ) && opened == 1);
    assert(!strncmp(opened_path, "Replay\\", 7));
    assert(replay_file_list_set_name(&list, "custom"));
    assert(replay_file_list_custom_name_taken(&list));
    assert(replay_file_list_make_recording_name(&list) == -2 && !list.path[0]);
    assert(replay_file_list_set_name(&list, "new"));
    assert(replay_file_list_make_recording_name(&list) == 1 && !strcmp(list.path, "Replay\\new.rrf"));
    memset(long_name, 'x', sizeof long_name); long_name[sizeof long_name - 1] = 0;
    assert(!replay_file_list_set_name(&list, long_name));
    assert(!strcmp(list.path, "Replay\\new.rrf"));
    long_name[sizeof list.path - sizeof "Replay\\.rrf"] = 0;
    assert(replay_file_list_set_name(&list, long_name));
    assert(replay_file_list_make_recording_name(&list) == 1 && strlen(list.path) == MAX_PATH - 1);
    list.useCustomName = 0;
    assert(replay_file_list_make_recording_name(&list) == 1);
    assert(!strcmp(list.path, "Replay\\RagnarokReplay_0003.rrf"));
    replay_file_list_destruct(&list); replay_file_list_destruct(&list);
    replay_file_list_construct(&list);
    fail_allocation = 1;
    assert(!replay_file_list_scan(&list) && !list.count && !list.files);
    fail_allocation = 0;
    for (i = 0; i < 50; ++i) assert(append_file(&list, "capture.rrf"));
    assert(list.count == 50 && !strcmp(list.files[49].name, "capture.rrf"));
    replay_file_list_reset(&list); assert(!list.count && list.files);
    replay_file_list_destruct(&list);
    assert(DeleteFileA("Replay\\RagnarokReplay_0002.rrf"));
    assert(DeleteFileA("Replay\\RagnarokReplay_0001.rrf"));
    assert(DeleteFileA("Replay\\custom.RRF"));
    assert(RemoveDirectoryA("Replay"));
    assert(SetCurrentDirectoryA(cwd));
    assert(RemoveDirectoryA(directory));
    puts("Replay files: C storage, short names, numbering, selection bounds, maximum paths, allocation failure and repeat cleanup passed");
    return 0;
}
