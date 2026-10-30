#include "replay/replay.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

struct ReplayFileList g_replayFileList;
INIT_ONCE g_replayFileListGuard;

static void file_list_atexit(void)
{
    replay_file_list_destruct(&g_replayFileList);
}

struct ReplayFileList *replay_file_list_instance(void)
{
    if (once_begin(&g_replayFileListGuard)) {
        replay_file_list_construct(&g_replayFileList);
        atexit(file_list_atexit);
        once_complete(&g_replayFileListGuard);
    }
    return &g_replayFileList;
}

struct ReplayFileList *replay_file_list_construct(struct ReplayFileList *self)
{
    memset(self, 0, sizeof *self);
    self->selected = -1;
    self->useCustomName = self->customNameSet = 1;
    return self;
}

void replay_file_list_destruct(struct ReplayFileList *self)
{
    free(self->files);
    self->files = NULL;
    self->count = self->capacity = 0;
    self->recorder = NULL;
    self->selected = -1;
    self->serverCount = 0;
    memset(self->servers, 0, sizeof self->servers);
}

void replay_file_list_reset(struct ReplayFileList *self)
{
    self->selected = -1;
    self->count = 0;
}

BYTE replay_file_list_set_recorder(struct ReplayFileList *self, struct ReplayRecorder *recorder)
{
    if (!recorder)
        return 0;
    self->recorder = recorder;
    return 1;
}

int replay_name_ends_with_rrf(const char *name)
{
    size_t size;
    if (!name)
        return 0;
    size = strlen(name);
    return size >= 4 && _stricmp(name + size - 4, ".rrf") == 0;
}

static int recording_number(const char *name)
{
    const char prefix[] = "RagnarokReplay_";
    const char *p;
    unsigned number = 0;
    if (strncmp(name, prefix, sizeof prefix - 1))
        return 0;
    p = name + sizeof prefix - 1;
    if (*p < '0' || *p > '9')
        return 0;
    for (; *p >= '0' && *p <= '9'; ++p) {
        unsigned digit = (unsigned)(*p - '0');
        if (number > (INT_MAX - digit) / 10)
            return 0;
        number = number * 10 + digit;
    }
    return _stricmp(p, ".rrf") ? 0 : (int)number;
}

static int append_file(struct ReplayFileList *self, const char *name)
{
    struct ReplayFileEntry *file;
    if (self->count == self->capacity) {
        size_t capacity = self->capacity ? self->capacity * 2 : 16;
        void *files;
        if (capacity <= self->capacity || capacity > INT_MAX / sizeof *self->files)
            return 0;
        files = realloc(self->files, capacity * sizeof *self->files);
        if (!files)
            return 0;
        self->files = files;
        self->capacity = capacity;
    }
    file = &self->files[self->count++];
    file->number = recording_number(name);
    strcpy(file->name, name);
    return 1;
}

BYTE replay_file_list_scan(struct ReplayFileList *self)
{
    WIN32_FIND_DATAA found;
    HANDLE find;
    DWORD error;
    replay_file_list_reset(self);
    find = FindFirstFileA("Replay\\*.rrf", &found);
    if (find == INVALID_HANDLE_VALUE) {
        error = GetLastError();
        return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
    }
    do {
        if (!(found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            replay_name_ends_with_rrf(found.cFileName) && !append_file(self, found.cFileName)) {
            FindClose(find);
            replay_file_list_reset(self);
            return 0;
        }
    } while (FindNextFileA(find, &found));
    error = GetLastError();
    FindClose(find);
    if (error != ERROR_NO_MORE_FILES) {
        replay_file_list_reset(self);
        return 0;
    }
    return 1;
}

const char *replay_file_list_get_name(struct ReplayFileList *self, unsigned index)
{
    return index < self->count ? self->files[index].name : NULL;
}

BYTE replay_file_list_select(struct ReplayFileList *self, int index)
{
    if (index < 0 || !self->count)
        return 0;
    self->selected = (size_t)index < self->count ? index : (int)self->count - 1;
    return 1;
}

char *replay_file_list_set_name(struct ReplayFileList *self, const char *name)
{
    size_t size;
    if (!name || !self->useCustomName)
        return NULL;
    size = strlen(name);
    if (size > sizeof self->path - sizeof "Replay\\.rrf")
        return NULL;
    memmove(self->path, name, size + 1);
    return self->path;
}

BYTE replay_file_list_custom_name_taken(struct ReplayFileList *self)
{
    char wanted[MAX_PATH];
    size_t i;
    int length = snprintf(wanted, sizeof wanted, "%s.rrf", self->path);
    if (length < 0 || length >= sizeof wanted)
        return 1;
    for (i = 0; i < self->count; ++i)
        if (!_stricmp(wanted, self->files[i].name))
            return 1;
    return 0;
}

int replay_file_list_make_recording_name(struct ReplayFileList *self)
{
    char path[MAX_PATH];
    int length;
    if (!replay_file_list_scan(self))
        return 0;
    if (self->useCustomName) {
        if (self->customNameSet && replay_file_list_custom_name_taken(self)) {
            self->path[0] = 0;
            return -2;
        }
        length = snprintf(path, sizeof path, "Replay\\%s.rrf", self->path);
    } else {
        size_t number, i;
        for (number = 1; number <= self->count + 1; ++number) {
            for (i = 0; i < self->count; ++i)
                if (self->files[i].number == (int)number)
                    break;
            if (i == self->count)
                break;
        }
        length = snprintf(path, sizeof path, "Replay\\RagnarokReplay_%04u.rrf", (unsigned)number);
    }
    if (length < 0 || length >= sizeof path)
        return 0;
    memcpy(self->path, path, length + 1);
    return 1;
}

int replay_file_list_open_selected(struct ReplayFileList *self, int mode)
{
    int named, length;
    char path[MAX_PATH];
    if (!self->recorder)
        return 0;
    if (mode == REPLAY_OPEN_READ) {
        if (self->selected < 0 || (size_t)self->selected >= self->count)
            return 0;
        length = snprintf(path, sizeof path, "Replay\\%s", self->files[self->selected].name);
        if (length < 0 || length >= sizeof path || !replay_recorder_open(self->recorder, path, mode))
            return 0;
        memcpy(self->path, path, length + 1);
        return 1;
    }
    if (mode != REPLAY_OPEN_WRITE)
        return 0;
    named = replay_file_list_make_recording_name(self);
    if (named != 1)
        return named;
    return replay_recorder_open(self->recorder, self->path, mode);
}
