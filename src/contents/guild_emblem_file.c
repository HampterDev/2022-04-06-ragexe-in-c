#include <zlib.h>
#include "session/state.h"
#include "game/context.h"
#include "ui/window_access.h"
#include <stdio.h>
#include <io.h>
#include <direct.h>
#include <sys/stat.h>
#include "contents/guild_mgr.h"
#include "contents/guild_emblem_name.h"
#include "contents/ui_window.h"
#include "contents/ui_window_mgr.h"
#include "contents/game_session.h"
#include "core/file_util.h"
#include "core/text_util.h"
#include "world/mode.h"
#include "core/app.h"

#define UIW_GUILD 0x3c

struct String *guild_mgr_emblem_path(struct GuildMgr *self, struct String *out, unsigned guild,
                                                   int version)
{
    (void)self;
    guild_emblem_name(out, &g_session1.worldName, guild, version);
    return out;
}

static void destroy_string(void *value) { string_free(value); }

static void delete_old_files(const struct String *folder, unsigned guild)
{
    struct String pattern;
    struct List files;
    struct ListNode *node;

    guild_emblem_pattern(&pattern, &g_session1.worldName, guild);
    list_init(&files);
    dir_list_names_list(&files, string_cstr(folder), string_cstr(&pattern));
    for (node = files.head->next; node != files.head; node = node->next) {
        struct String path;

        string_copy(&path, folder);
        string_append(&path, string_cstr(LIST_VALUE(node, struct String)), (LIST_VALUE(node, struct String))->size);
        string_free(&pattern);
        pattern = path;
        _chmod(string_cstr(&pattern), _S_IWRITE);
        _unlink(string_cstr(&pattern));
    }
    list_free(&files, destroy_string);
    string_free(&pattern);
}

static int write_emblem_file(const struct String *folder, const struct String *name,
                             const unsigned char *packed, unsigned long packed_size, unsigned guild)
{
    struct String path;
    FILE *fp;

    if (_access(string_cstr(folder), 0) && _mkdir(string_cstr(folder)) == -1)
        return 0;
    delete_old_files(folder, guild);
    string_copy(&path, folder);
    string_append(&path, string_cstr(name), name->size);
    fp = fopen(string_cstr(&path), "wb");
    if (fp) {
        fwrite(packed, packed_size, 1, fp);
        fclose(fp);
    }
    string_free(&path);
    return 1;
}

void guild_emblem_save(const unsigned char *data, unsigned size, unsigned guild, int version)
{
    struct String folder, name;
    unsigned long packed_size;
    unsigned char *packed;

    string_init(&folder);
    string_format(&folder, "%s\\_tmpEmblem\\", g_clientDir);
    guild_mgr_emblem_path(g_guildMgr, &name, guild, version);
    packed_size = (unsigned long)((double)g_guildMgr->emblem_max_size * 1.2);
    packed = (unsigned char *)mem_alloc(packed_size);
    if (compress(packed, &packed_size, data, size) == 0) {
        if (write_emblem_file(&folder, &name, packed, packed_size, guild))
            mem_free(packed);
    } else {
        struct String arg;

        string_copy(&arg, &name);
        guild_mgr_mark_bad_emblem(g_guildMgr, arg);
        mem_free(packed);
    }
    string_free(&name);
    string_free(&folder);
}

void guild_emblem_changed(unsigned guild)
{
    void *wnd;

    if (game_current_mode())
        game_mode_notify_listed(game_current_mode(), guild);
    wnd = ui_window_find(UIW_GUILD);
    if (wnd)
        ui_window_invalidate(wnd);
}
