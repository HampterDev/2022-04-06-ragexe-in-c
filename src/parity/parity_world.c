#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "parity_world.h"

static const struct {
    const char *name;
    int kind;
} control_classes[] = {
    {"UICheckBox", PARITY_CHECK_BOX},
    {"UI2ndCostumeCheckBox", PARITY_CHECK_BOX},
    {"UIRadioBtn", PARITY_RADIO},
    {"UIBmpRadioBtn", PARITY_RADIO},
    {"UIScrollBar", PARITY_SCROLL_BAR},
    {"UIScrollBar2", PARITY_SCROLL_BAR},
    {"UIScrollBar3", PARITY_SCROLL_BAR},
    {"UIScrollBar_Navi", PARITY_SCROLL_BAR},
    {"UIRodexScrollBar", PARITY_SCROLL_BAR},
    {"UIRenewQuestScrollBar", PARITY_SCROLL_BAR},
    {"UIEditCtrl", PARITY_EDIT},
    {"UIEditCtrl2", PARITY_EDIT},
    {"UIEditCtrl3", PARITY_EDIT},
    {"UIEditCtrl5", PARITY_EDIT},
    {"UIChatEditCtrl", PARITY_EDIT},
    {"UIWhisperEditCtrl", PARITY_EDIT},
    {"UIEditCtrlWheel", PARITY_EDIT},
    {"UIBitmapEditCtrl", PARITY_EDIT},
    {"UIEditCtrlVarId", PARITY_EDIT},
    {"UIEditCtrl_Custom", PARITY_EDIT},
    {"UIEditCtrl_Currency", PARITY_EDIT},
    {"UIRodexEditCtrl3", PARITY_EDIT},
    {"UITabControl", PARITY_TAB_CONTROL},
    {"UITab", PARITY_TAB},
    {"UIListBox", PARITY_LIST_BOX},
    {"UIServerListBox", PARITY_LIST_BOX},
    {"UIChatHisBox", PARITY_LIST_BOX},
    {"UIChatHisBox2", PARITY_LIST_BOX},
    {"UIListBoxAgit", PARITY_LIST_BOX},
    {"UIListBoxAgitMenu", PARITY_LIST_BOX},
    {"UIComboBoxEx", PARITY_LIST_BOX_EX},
    {"UIComboBoxEx2", PARITY_LIST_BOX_EX},
    {"UIListBoxTitle", PARITY_LIST_BOX_EX},
    {"UIItemStoreComboBox", PARITY_LIST_BOX_EX},
    {"UIComboBoxDefWnd", PARITY_COMBO},
    {"UIComboBoxDef2Wnd", PARITY_COMBO},
    {"UIComboBox_BmpWnd", PARITY_COMBO},
    {"UIItemStoreComboBoxWnd", PARITY_COMBO},
    {"UIStaticText", PARITY_STATIC_TEXT},
    {"UIBalloonText", PARITY_BALLOON},
    {"UITransBalloonText", PARITY_BALLOON},
    {"UINameBalloonText", PARITY_BALLOON},
    {"UITextButton", PARITY_TEXT_BUTTON},
    {"UINaviLinkButton", PARITY_TEXT_BUTTON},
    {"UIItemTagButton", PARITY_TEXT_BUTTON},
    {"UITagButton", PARITY_TEXT_BUTTON},
    {"UIItemMoveInfo", PARITY_TEXT_BUTTON},
    {"UIBmpTextButton", PARITY_BMP_TEXT_BUTTON},
    {"UIText@LineCutter", PARITY_LINE_TEXT},
    {"UITextViewer", PARITY_TEXT_VIEWER},
    {"UITagTextViewer", PARITY_TAG_TEXT_VIEWER},
};

int parity_control_kind(const char *name)
{
    size_t i, n;
    if (!name)
        return PARITY_PLAIN;
    n = strlen(name);
    if (n >= 2 && name[n - 1] == '@' && name[n - 2] == '@')
        n -= 2;
    for (i = 0; i < sizeof control_classes / sizeof control_classes[0]; i++)
        if (strlen(control_classes[i].name) == n && strncmp(control_classes[i].name, name, n) == 0)
            return control_classes[i].kind;
    return PARITY_PLAIN;
}

static int class_is(const char *name, const char *cls)
{
    size_t n;
    if (!name)
        return 0;
    n = strlen(name);
    if (n >= 2 && name[n - 1] == '@' && name[n - 2] == '@')
        n -= 2;
    return strlen(cls) == n && strncmp(cls, name, n) == 0;
}

int parity_timed_bar_parent(const char *parent)
{
    return class_is(parent, "UIGraphicSettingWnd");
}

int parity_check_box_class(const char *name)
{
    return parity_control_kind(name) == PARITY_CHECK_BOX;
}

static void write_bytes(FILE *f, const char *text, size_t length)
{
    size_t i;
    if (!text) {
        fputc('?', f);
        return;
    }
    for (i = 0; i < length; i++) {
        unsigned char c = (unsigned char)text[i];
        if (c > ' ' && c < 0x7f && c != '\\' && c != '=' && c != '@')
            fputc(c, f);
        else
            fprintf(f, "\\x%02x", c);
    }
}

static void write_pieces(FILE *f, const struct ParityControl *c, int first, size_t limit)
{
    int i;
    for (i = first; i < c->pieces && i < PARITY_TEXT_PIECES && limit; i++) {
        size_t n = c->text[i] ? c->length[i] : 1;
        if (i > first) {
            write_bytes(f, "\n", 1);
            if (!--limit)
                break;
        }
        if (n > limit)
            n = limit;
        write_bytes(f, c->text[i], n);
        limit -= n;
    }
}

static void write_text(FILE *f, const struct ParityControl *c)
{
    fputs(" text=", f);
    write_pieces(f, c, 0, (size_t)-1);
}

unsigned parity_text_hash(unsigned hash, int index, const char *text, size_t length)
{
    size_t i;
    if (index)
        hash = (hash ^ '\n') * 0x01000193u;
    if (!text) {
        text = "?";
        length = 1;
    }
    for (i = 0; i < length; i++)
        hash = (hash ^ (unsigned char)text[i]) * 0x01000193u;
    return hash;
}

void parity_control_write(FILE *f, const struct ParityControl *c)
{
    const int *v = c->value;
    switch (c->kind) {
    case PARITY_CHECK_BOX:
        fprintf(f, " on=%d dim=%d", v[0], v[1]);
        break;
    case PARITY_RADIO:
        fprintf(f, " on=%d", v[0]);
        break;
    case PARITY_SCROLL_BAR:
        if (c->timed)
            fprintf(f, " pos=* max=%d page=%d total=%d", v[1], v[2], v[3]);
        else
            fprintf(f, " pos=%d max=%d page=%d total=%d", v[0], v[1], v[2], v[3]);
        break;
    case PARITY_EDIT:

        if (c->masked)
            fprintf(f, " masked=%u", c->pieces > 0 ? (unsigned)c->length[0] : 0u);
        else
            write_text(f, c);
        fprintf(f, " sel=%d,%d", v[0], v[1]);
        break;
    case PARITY_TAB_CONTROL:
    case PARITY_TAB:
        fprintf(f, " tab=%d", v[0]);
        break;
    case PARITY_LIST_BOX:
    case PARITY_LIST_BOX_EX:
        fprintf(f, " row=%d top=%d", v[0], v[1]);
        break;
    case PARITY_COMBO:
        fprintf(f, " open=%d", v[0]);
        break;
    case PARITY_STATIC_TEXT:
    case PARITY_BALLOON:
    case PARITY_BMP_TEXT_BUTTON:
    case PARITY_LINE_TEXT:
        write_text(f, c);
        break;
    case PARITY_TEXT_BUTTON:
        fputs(" text=", f);
        if (c->pieces > 0)
            write_bytes(f, c->text[0], c->length[0]);
        fputs(" args=", f);
        write_pieces(f, c, 1, (size_t)-1);
        break;
    case PARITY_TEXT_VIEWER:
        fprintf(f, " lines=%d hash=%08x links=%d", v[0], c->hash, v[1]);
        write_text(f, c);
        break;
    case PARITY_TAG_TEXT_VIEWER:
        fprintf(f, " size=%u hash=%08x text=", (unsigned)c->size, c->hash);
        write_pieces(f, c, 0, PARITY_TEXT_BYTES);
        break;
    }
}

void parity_world_cell(float x, float z, int width, int height, int zoom, int *cell_x, int *cell_y)
{
    if (zoom <= 0) {
        *cell_x = *cell_y = -1;
        return;
    }
    *cell_x = (int)(float)floor((double)((float)(width / 2) + x / (float)zoom));
    *cell_y = (int)(float)floor((double)((float)(height / 2) + z / (float)zoom));
}

static int compare_fields(const void *a, const void *b, size_t size)
{
    const int *x = a, *y = b;
    size_t i, n = size / sizeof(int);
    if ((unsigned)x[0] != (unsigned)y[0])
        return (unsigned)x[0] < (unsigned)y[0] ? -1 : 1;
    for (i = 1; i < n; i++)
        if (x[i] != y[i])
            return x[i] < y[i] ? -1 : 1;
    return 0;
}

static int compare_actors(const void *a, const void *b) { return compare_fields(a, b, sizeof(struct ParityActor)); }
static int compare_units(const void *a, const void *b) { return compare_fields(a, b, sizeof(struct ParityUnit)); }
static int compare_items(const void *a, const void *b) { return compare_fields(a, b, sizeof(struct ParityItem)); }
static int compare_ids(const void *a, const void *b) { return compare_fields(a, b, sizeof(unsigned)); }

static unsigned kept(unsigned n)
{
    return n < PARITY_WORLD_MAX ? n : PARITY_WORLD_MAX;
}

static void write_actor(FILE *f, const char *tag, const struct ParityActor *a)
{
    fprintf(f, "%s id=%u job=%d kind=%d cell=%d,%d at=%d,%d state=%d action=%d held=%d opt=%u vanish=%d fx=%u\n",
            tag, a->id, a->job, a->kind, a->cell_x, a->cell_y, a->at_x, a->at_y, a->state, a->action, a->held,
            a->opt, a->vanishing, a->fx);
}

void parity_world_write(FILE *f, struct ParityWorld *w)
{
    unsigned i;
    qsort(w->actors, kept(w->n_actors), sizeof w->actors[0], compare_actors);
    qsort(w->units, kept(w->n_units), sizeof w->units[0], compare_units);
    qsort(w->items, kept(w->n_items), sizeof w->items[0], compare_items);
    qsort(w->pending, kept(w->n_pending), sizeof w->pending[0], compare_ids);
    fprintf(f, "world objects=%u effects=%u actors=%u items=%u units=%u pending=%u\n", w->objects, w->effects,
            w->n_actors, w->n_items, w->n_units, w->n_pending);
    if (w->has_player)
        write_actor(f, "wplayer", &w->player);
    for (i = 0; i < kept(w->n_actors); i++)
        write_actor(f, "wactor", &w->actors[i]);
    for (i = 0; i < kept(w->n_items); i++)
        fprintf(f, "witem id=%u item=%d cell=%d,%d\n", w->items[i].id, w->items[i].item_id, w->items[i].cell_x,
                w->items[i].cell_y);
    for (i = 0; i < kept(w->n_units); i++) {
        const struct ParityUnit *u = &w->units[i];
        fprintf(f, "wunit id=%u type=%d creator=%u cell=%d,%d shown=%d fx=%u\n", u->id, u->type, u->creator,
                u->cell_x, u->cell_y, u->shown, u->fx);
    }
    for (i = 0; i < kept(w->n_pending); i++)
        fprintf(f, "wpending id=%u\n", w->pending[i]);
}

#define CALL_RECORDS "parity_calls.tmp"
static FILE *g_records;

FILE *parity_call_records(void)
{
    if (!g_records)
        g_records = fopen(CALL_RECORDS, "w+b");
    return g_records;
}

void parity_call_note(int op, int arg, int result)
{
    FILE *f = parity_call_records();
    if (!f)
        return;
    fprintf(f, "call op=%d arg=%d ret=", op, arg);
    if (result == PARITY_CALL_FAULT)
        fputs("fault\n", f);
    else
        fprintf(f, "%d\n", result);
}

void parity_calls_write(FILE *f)
{
    char buf[4096];
    size_t n;
    if (!g_records)
        return;
    fflush(g_records);
    rewind(g_records);
    while ((n = fread(buf, 1, sizeof buf, g_records)) > 0)
        fwrite(buf, 1, n, f);
    fclose(g_records);
    g_records = NULL;
    remove(CALL_RECORDS);
}

void parity_item_write(FILE *f, const unsigned *w, const char *name, size_t name_length)
{
    int i;
    fprintf(f, "item id=%u type=%u refine=%u cards=%u,%u,%u,%u ident=%u broken=%u expire=%u hours=%u grade=%u options=%u",
            w[PARITY_ITEM_ID], w[PARITY_ITEM_TYPE], w[PARITY_ITEM_REFINE], w[PARITY_ITEM_CARDS], w[PARITY_ITEM_CARDS + 1],
            w[PARITY_ITEM_CARDS + 2], w[PARITY_ITEM_CARDS + 3], w[PARITY_ITEM_IDENTIFIED], w[PARITY_ITEM_DAMAGED],
            w[PARITY_ITEM_EXPIRE], w[PARITY_ITEM_EXPIRE_HOURS], w[PARITY_ITEM_GRADE], w[PARITY_ITEM_OPTION_COUNT]);
    for (i = 0; i < PARITY_ITEM_OPTION_SLOTS; i++)
        fprintf(f, "%s%u:%d:%u", i ? "," : " opt=", w[PARITY_ITEM_OPTIONS + 3 * i], (int)w[PARITY_ITEM_OPTIONS + 3 * i + 1],
                w[PARITY_ITEM_OPTIONS + 3 * i + 2]);
    fputs(" name=", f);
    if (name)
        write_bytes(f, name, name_length);
    else
        fputc('?', f);
    fputc('\n', f);
}
