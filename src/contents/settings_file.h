#ifndef CONTENTS_SETTINGS_FILE_H
#define CONTENTS_SETTINGS_FILE_H

#include "core/owned_text.h"
#include "platform/win_types.h"

int settings_file_path(char out[MAX_PATH], const char *root, const char *address,
                       const char *port, const char *world, unsigned aid, unsigned gid);
int settings_file_read(const char *path, struct OwnedText *out);
int settings_file_write(const char *path, struct TextView text);

#endif
