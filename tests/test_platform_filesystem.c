#include "platform/filesystem.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    const char *name = ".\\port-\307\321\261\333.tmp";
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
    file = platform_file_open("./port-\307\321\261\333.tmp", "rb");
    assert(file && fread(content, 1, sizeof content, file) == 8);
    assert(!memcmp(content, "original", 8));
    assert(!fclose(file));
#ifndef _WIN32
    file = fopen("./port-\355\225\234\352\270\200.tmp", "rb");
    assert(file);
    assert(!fclose(file));
#endif
    assert(!platform_file_remove(name));
    assert(!platform_file_open(name, "rb") && errno == ENOENT);
    puts("Filesystem: CP949 names, UTF-8 disk names, separators and exclusive creation passed");
    return 0;
}
