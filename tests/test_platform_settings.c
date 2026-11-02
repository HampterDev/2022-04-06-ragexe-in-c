#include "platform/settings.h"
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define FILE_NAME "client_settings.ini"

static char *read_file(void)
{
    static char text[8192];
    FILE *file = fopen(FILE_NAME, "rb");
    size_t count;
    assert(file);
    count = fread(text, 1, sizeof text - 1, file);
    assert(!ferror(file) && count < sizeof text - 1);
    fclose(file);
    text[count] = 0;
    return text;
}

static void write_file(const char *text)
{
    FILE *file = fopen(FILE_NAME, "wb");
    assert(file && fputs(text, file) >= 0 && !fclose(file));
}

static int directory_entries(void)
{
    DIR *directory = opendir(".");
    struct dirent *entry;
    int count = 0;
    assert(directory);
    while ((entry = readdir(directory)) != NULL)
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
            ++count;
    closedir(directory);
    return count;
}

static uint32_t read_dword(const char *path, const char *name, uint32_t fallback)
{
    struct PlatformSettingsKey *key;
    uint32_t type = 0, size = 4, value = fallback;
    if (!platform_settings_open(path, PLATFORM_SETTINGS_QUERY, &key))
        return fallback;
    if (!platform_settings_read(key, name, &type, &value, &size) || type != PLATFORM_SETTING_DWORD || size != 4)
        value = fallback;
    platform_settings_close(key);
    return value;
}

static void missing_store(void)
{
    struct PlatformSettingsKey *key = NULL;
    assert(!platform_settings_open("Software\\Client", PLATFORM_SETTINGS_QUERY, &key) && !key);
    assert(directory_entries() == 0);
}

static void round_trip(void)
{
    static const unsigned char guid[16] = {0x78, 0x56, 0x34, 0x12, 0, 0xff, 0x10, 0x20, 1, 2, 3, 4, 5, 6, 7, 8};
    static const char id[0x33] = "player%one";
    static const char odd[] = "line\nbreak \x80\xbf=[x]";
    struct PlatformSettingsKey *key;
    unsigned char bytes[64];
    uint32_t type, size, number = 0xdeadbeef;
    char text[8192];

    assert(platform_settings_create("Software\\Client", PLATFORM_SETTINGS_ALL, &key));
    assert(platform_settings_write(key, "CHECKSAVE", PLATFORM_SETTING_DWORD, &number, 4));
    assert(platform_settings_write(key, "ID", PLATFORM_SETTING_STRING, id, sizeof id));
    assert(platform_settings_write(key, "GUIDDRIVER", PLATFORM_SETTING_BINARY, guid, sizeof guid));
    assert(platform_settings_write(key, "odd=[name]", PLATFORM_SETTING_STRING, odd, sizeof odd));
    assert(platform_settings_write(key, "EMPTY", PLATFORM_SETTING_BINARY, guid, 0));
    assert(!platform_settings_write(key, "SHORT", PLATFORM_SETTING_DWORD, &number, 2));
    platform_settings_close(key);

    strcpy(text, read_file());
    assert(strstr(text, "[Software\\Client]\n"));
    assert(strstr(text, "\nCHECKSAVE=dword:3735928559\n"));
    assert(strstr(text, "\nID=sz:51:player%25one\n"));
    assert(strstr(text, "\nGUIDDRIVER=hex:7856341200ff102001020304050607" "08\n"));
    assert(strstr(text, "\nodd%3D%5Bname]=sz:18:line%0Abreak %80%BF=[x]\n"));
    assert(strstr(text, "\nEMPTY=hex:\n"));
    assert(!strstr(text, "SHORT"));
    assert(directory_entries() == 1);

    assert(platform_settings_open("software\\client\\", PLATFORM_SETTINGS_QUERY, &key));
    type = 0;
    size = 4;
    number = 0;
    assert(platform_settings_read(key, "checksave", &type, &number, &size));
    assert(type == PLATFORM_SETTING_DWORD && size == 4 && number == 0xdeadbeef);
    memset(bytes, 0x55, sizeof bytes);
    size = sizeof bytes;
    assert(platform_settings_read(key, "ID", &type, bytes, &size));
    assert(type == PLATFORM_SETTING_STRING && size == sizeof id && !memcmp(bytes, id, sizeof id));
    size = sizeof bytes;
    assert(platform_settings_read(key, "GUIDDRIVER", &type, bytes, &size));
    assert(type == PLATFORM_SETTING_BINARY && size == sizeof guid && !memcmp(bytes, guid, sizeof guid));
    size = sizeof bytes;
    assert(platform_settings_read(key, "ODD=[NAME]", &type, bytes, &size));
    assert(type == PLATFORM_SETTING_STRING && size == sizeof odd && !memcmp(bytes, odd, sizeof odd));
    size = sizeof bytes;
    assert(platform_settings_read(key, "EMPTY", &type, bytes, &size) && type == PLATFORM_SETTING_BINARY && !size);

    type = 77;
    size = 4;
    assert(!platform_settings_read(key, "ID", &type, bytes, &size));
    assert(type == PLATFORM_SETTING_STRING && size == sizeof id);
    type = 77;
    size = 4;
    assert(!platform_settings_read(key, "MISSING", &type, bytes, &size) && type == 77 && size == 4);
    assert(!platform_settings_write(key, "CHECKSAVE", PLATFORM_SETTING_DWORD, &number, 4));
    platform_settings_close(key);
    assert(!strcmp(read_file(), text));
}

static void key_tree(void)
{
    struct PlatformSettingsKey *key;
    uint32_t type = 0, size = 4, value = 7;

    assert(!platform_settings_open("Software\\Client\\UIRectInfo", PLATFORM_SETTINGS_QUERY, &key));
    assert(platform_settings_create("Software\\Client\\ShortcutItem\\Server One", PLATFORM_SETTINGS_ALL, &key));
    platform_settings_close(key);
    assert(strstr(read_file(), "\n[Software\\Client\\ShortcutItem\\Server One]\n"));
    assert(platform_settings_open("Software\\Client\\ShortcutItem", PLATFORM_SETTINGS_QUERY, &key));
    assert(!platform_settings_read(key, "CHECKSAVE", &type, &value, &size));
    platform_settings_close(key);
    assert(!platform_settings_open("Software\\Client\\Shortcut", PLATFORM_SETTINGS_QUERY, &key));
    assert(platform_settings_open("Software\\Client\\ShortcutItem", PLATFORM_SETTINGS_ALL, &key));
    assert(platform_settings_write(key, "POS", PLATFORM_SETTING_DWORD, &value, 4));
    platform_settings_close(key);
    assert(read_dword("Software\\Client\\ShortcutItem", "POS", 0) == 7);

    assert(platform_settings_create("Software\\Client", PLATFORM_SETTINGS_SET, &key));
    assert(!platform_settings_read(key, "CHECKSAVE", &type, &value, &size));
    value = 1;
    assert(platform_settings_write(key, "CheckSave", PLATFORM_SETTING_DWORD, &value, 4));
    platform_settings_close(key);
    assert(strstr(read_file(), "\nCHECKSAVE=dword:1\n"));
    assert(read_dword("Software\\Client", "CHECKSAVE", 0) == 1);
}

static void external_edits(void)
{
    struct PlatformSettingsKey *key;
    uint32_t type, size, value;
    char text[16];

    write_file("ORPHAN=dword:5\r\n"
               "[Software\\Client]\r\n"
               "GOOD=dword:42\r\n"
               "NEGATIVE=dword:-1\r\n"
               "LARGE=dword:4294967296\r\n"
               "BADHEX=hex:0g\r\n"
               "ODDHEX=hex:123\r\n"
               "LONG=sz:2:abc\r\n"
               "BADESCAPE=sz:4:a%4\r\n"
               "NOEQUALS\r\n"
               "UNKNOWN=qword:1\r\n"
               "\r\n"
               "[SOFTWARE\\CLIENT\\]\r\n"
               "TEXT=sz:6:a%00b\r\n"
               "last=dword:9");
    assert(read_dword("Software\\Client", "GOOD", 0) == 42);
    assert(read_dword("Software\\Client", "last", 0) == 9);
    assert(read_dword("Software\\Client", "ORPHAN", 0) == 0);
    assert(read_dword("Software\\Client", "NEGATIVE", 3) == 3);
    assert(read_dword("Software\\Client", "LARGE", 3) == 3);
    assert(read_dword("Software\\Client", "UNKNOWN", 3) == 3);
    assert(platform_settings_open("Software\\Client", PLATFORM_SETTINGS_QUERY, &key));
    size = sizeof text;
    assert(!platform_settings_read(key, "BADHEX", &type, text, &size));
    assert(!platform_settings_read(key, "ODDHEX", &type, text, &size));
    assert(!platform_settings_read(key, "LONG", &type, text, &size));
    assert(!platform_settings_read(key, "BADESCAPE", &type, text, &size));
    assert(!platform_settings_read(key, "NOEQUALS", &type, text, &size));
    assert(platform_settings_read(key, "TEXT", &type, text, &size));
    assert(type == PLATFORM_SETTING_STRING && size == 6 && !memcmp(text, "a\0b\0\0\0", 6));
    platform_settings_close(key);

    assert(platform_settings_create("Software\\Client", PLATFORM_SETTINGS_ALL, &key));
    value = 43;
    assert(platform_settings_write(key, "GOOD", PLATFORM_SETTING_DWORD, &value, 4));
    platform_settings_close(key);
    assert(!strcmp(read_file(), "[Software\\Client]\nGOOD=dword:43\nTEXT=sz:6:a%00b\nlast=dword:9\n"));
}

static void atomic_rewrite(void)
{
    struct PlatformSettingsKey *key;
    struct stat before, after;
    uint32_t value = 11;
    FILE *old;
    char line[64];

    assert(!stat(FILE_NAME, &before));
    old = fopen(FILE_NAME, "rb");
    assert(old);
    assert(platform_settings_create("Software\\Client", PLATFORM_SETTINGS_ALL, &key));
    assert(platform_settings_write(key, "GOOD", PLATFORM_SETTING_DWORD, &value, 4));
    platform_settings_close(key);
    assert(!stat(FILE_NAME, &after) && after.st_ino != before.st_ino);
    assert(fgets(line, sizeof line, old) && fgets(line, sizeof line, old) && !strcmp(line, "GOOD=dword:43\n"));
    fclose(old);
    assert(directory_entries() == 1);
    assert(read_dword("Software\\Client", "GOOD", 0) == 11);

    if (geteuid() == 0)
        return;
    assert(!chmod(".", 0500));
    assert(platform_settings_create("Software\\Client", PLATFORM_SETTINGS_ALL, &key));
    value = 12;
    assert(platform_settings_write(key, "GOOD", PLATFORM_SETTING_DWORD, &value, 4));
    platform_settings_close(key);
    assert(strstr(read_file(), "\nGOOD=dword:11\n") && directory_entries() == 1);
    assert(read_dword("Software\\Client", "GOOD", 0) == 12);
    assert(!chmod(".", 0700));
    assert(platform_settings_open("Software\\Client", PLATFORM_SETTINGS_QUERY, &key));
    platform_settings_close(key);
    assert(strstr(read_file(), "\nGOOD=dword:12\n") && directory_entries() == 1);

    assert(!chmod(FILE_NAME, 0));
    assert(!platform_settings_open("Software\\Client", PLATFORM_SETTINGS_QUERY, &key));
    assert(!platform_settings_create("Software\\Client", PLATFORM_SETTINGS_ALL, &key));
    assert(!chmod(FILE_NAME, 0600));
    assert(read_dword("Software\\Client", "GOOD", 0) == 12);
}

int main(void)
{
    char directory[64];
    snprintf(directory, sizeof directory, "/tmp/platform_settings_%ld", (long)getpid());
    assert(!mkdir(directory, 0700) && !chdir(directory));
    missing_store();
    round_trip();
    key_tree();
    external_edits();
    atomic_rewrite();
    assert(!remove(FILE_NAME) && !chdir("/") && !rmdir(directory));
    puts("Platform settings: round trip, missing values, key tree, external edits and atomic rewrite passed");
    return 0;
}
