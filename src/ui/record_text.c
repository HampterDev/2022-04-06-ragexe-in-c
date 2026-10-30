#include "ui/window_records.h"
#include "ui/controls/text_viewer.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

int ui_dialog_records_save(struct UIWindow *viewer, const char *question, uint16_t begin, uint16_t name,
                           uint16_t end, struct ReplayBuffer *out)
{
    struct SnapshotWriter writer = {0};
    char *text;
    int length;
    size_t framing;
    if (!viewer || !question || !out)
        return 0;
    length = ui_text_viewer_text_length(viewer);
    framing = strlen(question);
    if (length < 2 || length == INT_MAX || framing > (size_t)length - 2)
        return 0;
    framing += 2;
    if ((size_t)length - framing >= UI_RECORD_TEXT_CAPACITY)
        return 0;
    text = malloc((size_t)length + 1);
    if (!text)
        return 0;
    text[length] = 0;
    if (!ui_text_viewer_get_text(viewer, text, length)) {
        free(text);
        return 0;
    }
    snapshot_write(&writer, begin, NULL, 0);
    snapshot_write(&writer, name, text + 1, (size_t)length - framing);
    snapshot_write(&writer, end, NULL, 0);
    free(text);
    return snapshot_finish(&writer, out);
}
