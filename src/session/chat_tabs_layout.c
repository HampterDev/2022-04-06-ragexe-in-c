#include "session/chat_tabs_layout.h"
#include "fileio/file_api.h"
#include <stdio.h>
#include <string.h>

static void write_line(struct File *file, const char *line)
{
    file_write(file, line, (int)strlen(line));
}

static void write_name(struct File *file, struct TextView name)
{
    char buffer[256];
    size_t i, used = 0;

    write_line(file, "\"");
    for (i = 0; i < name.size; ++i) {
        unsigned byte = (unsigned char)name.data[i];
        if (used > sizeof buffer - 4) {
            file_write(file, buffer, (int)used);
            used = 0;
        }
        if (byte < 32 || byte > 126 || byte == '"' || byte == '\\') {
            /* Three decimal digits prevent a following digit joining the escape. */
            buffer[used++] = '\\';
            buffer[used++] = (char)('0' + byte / 100);
            buffer[used++] = (char)('0' + byte / 10 % 10);
            buffer[used++] = (char)('0' + byte % 10);
        } else {
            buffer[used++] = (char)byte;
        }
    }
    if (used) file_write(file, buffer, (int)used);
    write_line(file, "\"");
}

static void write_tab(struct File *file, unsigned number, const struct ChatTabSettings *tab, int in_window)
{
    char line[128];
    int i;

    snprintf(line, sizeof line, "ChatSubWnd_%u = {\r\n", number);
    write_line(file, line);
    snprintf(line, sizeof line, "\tXPos = %d,\r\n\tYPos = %d,\r\n", in_window ? 0 : tab->x, in_window ? 0 : tab->y);
    write_line(file, line);
    snprintf(line, sizeof line, "\tWidth = %d,\r\n\tHeight = %d,\r\n", in_window ? 0 : tab->width, in_window ? 0 : tab->height);
    write_line(file, line);
    write_line(file, in_window ? "\tTabState = \"On\",\r\n\tTabName = " : "\tTabState = \"Off\",\r\n\tTabName = ");
    write_name(file, owned_text_view(&tab->name));
    write_line(file, ",\r\n");
    for (i = 1; i <= CHAT_TAB_FLAG_COUNT; i++) {
        snprintf(line, sizeof line, "\toption%d = %d,\r\n", i, tab->shown[i - 1]);
        write_line(file, line);
    }
    write_line(file, "}\r\n\r\n");
}

void chat_tabs_save_layout(const struct ChatTabs *main_tabs, const struct ChatTabs *separate_tabs,
                          int editable, const char *path)
{
    struct File file;
    unsigned i;

    file_construct(&file);
    file_open(&file, path, 0x1001, 1, 0);
    write_line(&file, editable ? "CanDragWnd = 1\r\n\r\n" : "CanDragWnd = 0\r\n\r\n");
    for (i = 0; i < main_tabs->size; ++i)
        write_tab(&file, i + 1, &main_tabs->items[i].value, 1);
    for (i = 0; i < separate_tabs->size; ++i)
        write_tab(&file, main_tabs->size + i + 1, &separate_tabs->items[i].value, 0);
    file_close(&file);
    file_destruct(&file);
}
