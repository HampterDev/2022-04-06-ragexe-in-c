#include "session/csession.h"
#include "session/session_talk.h"
#include "platform/filesystem.h"

#include <stdio.h>
#include <string.h>

static void log_path(char *path, const char *name)
{
    char directory[0x100], *data;

    platform_current_directory(directory, sizeof directory);
    data = strstr(directory, "\\data");
    if (data)
        memcpy(directory, directory, data - directory);
    platform_path_join(path, 0x100, directory, name);
}

void session_load_prohibition_log(struct Session *self)
{
    struct File file;
    char path[0x100];
    struct ProhibitionRecord record;
    int count, length, i;

    file_construct(&file);
    log_path(path, "ProhibitionLog.gd");
    if (file_open(&file, path, 0, 1, 0)) {
        count = 0;
        file_read(&file, &count, 4);
        for (i = 0; i < count; i++) {
            file_read(&file, &length, 4);
            file_read(&file, record.name, length);
            file_read(&file, &length, 4);
            file_read(&file, record.giver, length);
            file_read(&file, &record.aid, 4);
            file_read(&file, &record.actor_id, 4);
            file_read(&file, &record.minutes, 4);
            file_read(&file, &record.date, 4);
            file_read(&file, &record.time, 4);
            vec_push(&self->prohibitionLog, &record, sizeof record);
        }
        file_close(&file);
    }
    file_destruct(&file);
}
