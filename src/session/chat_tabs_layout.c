#include "session/chat_tabs_layout.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>

struct LayoutWriter {
    HANDLE file;
    int ok;
};

static void write_bytes(struct LayoutWriter *writer, const void *data, size_t size)
{
    DWORD written = 0;
    if (!writer->ok) return;
    writer->ok = size <= MAXDWORD &&
        WriteFile(writer->file, data, (DWORD)size, &written, NULL) && written == size;
}

static void write_line(struct LayoutWriter *writer, const char *line)
{
    write_bytes(writer, line, strlen(line));
}

static void write_name(struct LayoutWriter *file, struct TextView name)
{
    char buffer[256];
    size_t i, used = 0;

    write_line(file, "\"");
    for (i = 0; i < name.size; ++i) {
        unsigned byte = (unsigned char)name.data[i];
        if (used > sizeof buffer - 4) {
            write_bytes(file, buffer, used);
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
    if (used) write_bytes(file, buffer, used);
    write_line(file, "\"");
}

static void write_tab(struct LayoutWriter *file, unsigned number, const struct ChatTabSettings *tab, int in_window)
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

int chat_tabs_save_layout(const struct ChatTabs *main_tabs, const struct ChatTabs *separate_tabs,
                         int editable, const char *path)
{
    struct LayoutWriter writer;
    char target[MAX_PATH], directory[MAX_PATH], temporary[MAX_PATH], *name;
    DWORD length;
    unsigned i;

    if (!path || !*path) return 0;
    length = GetFullPathNameA(path, MAX_PATH, target, &name);
    if (!length || length >= MAX_PATH || !name || !*name) return 0;
    memcpy(directory, target, (size_t)(name - target));
    directory[name - target] = 0;
    if (!GetTempFileNameA(directory, "cht", 0, temporary)) return 0;
    writer.file = CreateFileA(temporary, GENERIC_WRITE, 0, NULL, TRUNCATE_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    writer.ok = writer.file != INVALID_HANDLE_VALUE;
    if (writer.ok) {
        write_line(&writer, editable ? "CanDragWnd = 1\r\n\r\n" : "CanDragWnd = 0\r\n\r\n");
        for (i = 0; writer.ok && i < main_tabs->size; ++i)
            write_tab(&writer, i + 1, &main_tabs->items[i].value, 1);
        for (i = 0; writer.ok && i < separate_tabs->size; ++i)
            write_tab(&writer, main_tabs->size + i + 1, &separate_tabs->items[i].value, 0);
        if (writer.ok) writer.ok = FlushFileBuffers(writer.file) != 0;
        if (!CloseHandle(writer.file)) writer.ok = 0;
    }
    if (writer.ok)
        writer.ok = MoveFileExA(temporary, target,
                               MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
    if (!writer.ok) DeleteFileA(temporary);
    return writer.ok;
}
