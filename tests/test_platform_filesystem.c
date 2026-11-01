#include "platform/filesystem.h"
#include "core/unique_file.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    const char *name = ".\\port-\307\321\261\333.tmp";
    const char *alternate = "./port-\307\321\261\333(1).tmp";
    FILE *file;
    char content[16] = {0};
    assert(platform_path_is_directory("."));
    assert(!platform_file_open(NULL, "rb") && errno == EINVAL);
    assert(!platform_file_open("", "rb") && errno == EINVAL);
    assert(!platform_file_open("\xff", "rb"));
    assert(!platform_file_open(name, NULL) && errno == EINVAL);
    file = platform_file_open(name, "wbx");
    assert(file);
    assert(fwrite("original", 1, 8, file) == 8);
    assert(!fclose(file));
    assert(!platform_path_is_directory(name));
    assert(!platform_file_open(name, "wbx") && errno == EEXIST);
    file = file_open_unique(name);
    assert(file);
    assert(fputs("second", file) >= 0);
    assert(!fclose(file));
    file = platform_file_open("./port-\307\321\261\333.tmp", "rb");
    assert(file && fread(content, 1, sizeof content, file) == 8);
    assert(!memcmp(content, "original", 8));
    assert(!fclose(file));
    file = platform_file_open(alternate, "rb");
    assert(file && fread(content, 1, sizeof content, file) == 6);
    assert(!memcmp(content, "second", 6));
    assert(!fclose(file));
#ifndef _WIN32
    file = fopen("./port-\355\225\234\352\270\200.tmp", "rb");
    assert(file);
    assert(!fclose(file));
#endif
    assert(!platform_file_remove(name));
    assert(!platform_file_remove(alternate));
    assert(!platform_file_open(name, "rb") && errno == ENOENT);
    puts("Filesystem: CP949 names, UTF-8 disk names, separators, exclusive creation and unique output passed");
    return 0;
}
