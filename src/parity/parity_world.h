#ifndef RAGEXE_PARITY_WORLD_H
#define RAGEXE_PARITY_WORLD_H

#include <stdio.h>

struct ParityActor {
    unsigned id;
    int job;
    int kind;
    int cell_x, cell_y;
    int at_x, at_y;
    int state;
    int action;
    int held;
    unsigned opt;
    int vanishing;
    unsigned fx;
};

struct ParityUnit {
    unsigned id;
    int type;
    unsigned creator;
    int cell_x, cell_y;
    int shown;
    unsigned fx;
};

struct ParityItem {
    unsigned id;
    int item_id;
    int cell_x, cell_y;
};

#define PARITY_WORLD_MAX 1024

struct ParityWorld {
    unsigned objects;
    unsigned effects;
    int has_player;
    struct ParityActor player;
    unsigned n_actors, n_units, n_items, n_pending;
    struct ParityActor actors[PARITY_WORLD_MAX];
    struct ParityUnit units[PARITY_WORLD_MAX];
    struct ParityItem items[PARITY_WORLD_MAX];
    unsigned pending[PARITY_WORLD_MAX];
};

void parity_world_cell(float x, float z, int width, int height, int zoom, int *cell_x, int *cell_y);

void parity_world_write(FILE *f, struct ParityWorld *w);

enum ParityControlKind {
    PARITY_PLAIN,
    PARITY_CHECK_BOX,
    PARITY_RADIO,
    PARITY_SCROLL_BAR,
    PARITY_EDIT,
    PARITY_TAB_CONTROL,
    PARITY_TAB,
    PARITY_LIST_BOX,
    PARITY_LIST_BOX_EX,
    PARITY_COMBO,
    PARITY_STATIC_TEXT,
    PARITY_BALLOON,
    PARITY_TEXT_BUTTON,
    PARITY_BMP_TEXT_BUTTON,
    PARITY_LINE_TEXT,
    PARITY_TEXT_VIEWER,
    PARITY_TAG_TEXT_VIEWER
};

int parity_control_kind(const char *name);

int parity_check_box_class(const char *name);

#define PARITY_TEXT_PIECES 32
#define PARITY_TEXT_BYTES 512

struct ParityControl {
    int kind;
    int value[4];
    int masked;
    int timed;
    int pieces;
    const char *text[PARITY_TEXT_PIECES];
    size_t length[PARITY_TEXT_PIECES];
    unsigned hash;
    size_t size;
};

#define PARITY_HASH_START 0x811c9dc5u

unsigned parity_text_hash(unsigned hash, int index, const char *text, size_t length);

int parity_timed_bar_parent(const char *parent);

void parity_control_write(FILE *f, const struct ParityControl *c);

enum ParityCallOp {
    PARITY_CALL_MAKE_WINDOW = 1,
    PARITY_CALL_CLOSE_WINDOW = 2,
    PARITY_CALL_ITEM_SHOW = 3,
    PARITY_CALL_ITEM_RECORD = 4
};

#define PARITY_CALL_GUARD 0x100
#define PARITY_CALL_FAULT (-0x7fffffff)

enum ParityItemWord {
    PARITY_ITEM_ID,
    PARITY_ITEM_TYPE,
    PARITY_ITEM_REFINE,
    PARITY_ITEM_CARDS,
    PARITY_ITEM_IDENTIFIED = PARITY_ITEM_CARDS + 4,
    PARITY_ITEM_DAMAGED,
    PARITY_ITEM_EXPIRE,
    PARITY_ITEM_EXPIRE_HOURS,
    PARITY_ITEM_GRADE,
    PARITY_ITEM_OPTION_COUNT,
    PARITY_ITEM_OPTIONS,
    PARITY_ITEM_FLAGS = PARITY_ITEM_OPTIONS + 3 * 5,
    PARITY_ITEM_WORDS
};
#define PARITY_ITEM_OPTION_SLOTS 5
#define PARITY_ITEM_HOLD 1
#define PARITY_ITEM_WINDOW 0xc
#define PARITY_ITEM_CMP_WINDOW 0xea

FILE *parity_call_records(void);

void parity_call_note(int op, int arg, int result);

void parity_calls_write(FILE *f);

void parity_item_write(FILE *f, const unsigned *w, const char *name, size_t name_length);

#endif
