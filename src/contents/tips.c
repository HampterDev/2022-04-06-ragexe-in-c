#include "ui/window_manager_state.h"
#include <stdio.h>
#include <string.h>
#include "contents/client_res.h"
#include "std/std.h"
#include "contents/tips.h"
#include "platform/language.h"
#include "ui/ui_std.h"

struct TipFile g_tipOfTheDay;
struct TipFile g_guildTips;

static size_t tip_count(const struct Vec *tips)
{
    return VEC_COUNT(tips, struct Vec);
}

void tip_list_free(struct Vec *tips)
{
    struct Vec *tip;

    for (tip = tips->first; tip != tips->last; tip++)
        string_vec_free(tip);
    vec_free(tips);
}

static void tip_file_parse(struct TipFile *self, char *text, int size)
{
    struct Vec lines = { 0, 0, 0 }, copy;
    char *line = text;
    int i;

    text[size] = 0;
    for (i = 0; i < size; i++) {
        if (text[i] == '\n') {
            text[i] = 0;
            string_vec_push_text(&lines, line, strlen(line));
            line = text + i + 1;
        } else if (text[i] == '#') {
            text[i] = 0;
            string_vec_push_text(&lines, line, strlen(line));
            string_vec_copy(&copy, &lines);
            vec_push(&self->tips, &copy, sizeof copy);
            string_vec_clear(&lines);
            line = text + i + 1;
        }
    }
    if (text)
        mem_free(text);
    string_vec_free(&lines);
}

static char *read_disk_file(const char *name, int *size)
{
    FILE *fp = fopen(name, "r");
    char *text;

    if (!fp)
        return 0;
    fseek(fp, 0, SEEK_END);
    *size = ftell(fp);
    text = (char *)mem_alloc(*size + 1);
    fseek(fp, 0, SEEK_SET);
    fread(text, *size, 1, fp);
    fclose(fp);
    return text;
}

static char *read_game_file(const char *name, int *size)
{
    struct File file;
    char *text;

    if (!file_make_file_name(name, 0))
        return 0;
    file_construct(&file);
    if (!file_open(&file, name, 0, 1, 0)) {
        file_destruct(&file);
        return 0;
    }
    *size = file.size;
    text = (char *)mem_alloc(*size + 1);
    file_read(&file, text, *size);
    file_close(&file);
    file_destruct(&file);
    return text;
}

int tip_of_the_day_read(struct TipFile *self)
{
    char *text;
    int size;

    self->loaded = 1;
    if (g_serviceType == 0)
        text = read_disk_file("tipOfTheday.txt", &size);
    else if (g_serviceType == 19)
        text = read_disk_file(g_windowMgr.arabic_text ? "tipoftheday.txt" : "tipOfTheday_Eng.txt", &size);
    else
        text = read_game_file(language_language_file(g_language, "tipofday.txt"), &size);
    if (!text)
        return 0;
    tip_file_parse(self, text, size);
    return 1;
}

struct Vec *tip_of_the_day_get(struct TipFile *self, int index)
{
    struct Vec *tips;

    if (!self->loaded)
        tip_of_the_day_read(self);
    tips = self->tips.first;
    if (index >= 0 && (size_t)index < tip_count(&self->tips))
        return &tips[index];
    return tips;
}

int guild_tips_read(struct TipFile *self)
{
    char *text;
    int size;

    self->loaded = 1;
    if (g_serviceType == 0)
        text = read_disk_file("GuildTip.txt", &size);
    else
        text = read_game_file(language_language_file(g_language, "GuildTip.txt"), &size);
    if (!text)
        return 0;
    tip_file_parse(self, text, size);
    return 1;
}

struct Vec *guild_tips_copy(struct TipFile *self, struct Vec *out)
{
    const struct Vec *tip;
    struct Vec *copy;

    if (!self->loaded)
        guild_tips_read(self);
    vec_init(out);
    copy = vec_add(out, tip_count(&self->tips), sizeof *copy);
    for (tip = self->tips.first; tip != self->tips.last; tip++)
        string_vec_copy(copy++, tip);
    return out;
}
