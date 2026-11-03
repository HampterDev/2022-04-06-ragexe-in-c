#ifndef CORE_OBJECT_TYPE_H
#define CORE_OBJECT_TYPE_H

#include <stddef.h>
#include <stdint.h>

enum ObjectType {
    OBJECT_BITMAP_RESOURCE,
    OBJECT_SPRITE_RESOURCE,
    OBJECT_ACTION_RESOURCE,
    OBJECT_EMBLEM_RESOURCE,
    OBJECT_GIF_RESOURCE,
    OBJECT_PALETTE_RESOURCE,
    OBJECT_PC,
    OBJECT_GAME_ACTOR,
    OBJECT_TETRA_BALL_EFFECT,
    OBJECT_FRAME_WINDOW,
    OBJECT_BASIC_BUTTONS_WINDOW,
    OBJECT_PARA_RESULT_WINDOW,
    OBJECT_MY_SHOP_WINDOW,
    OBJECT_GRAPHICS_SETTINGS_WINDOW,
    OBJECT_ITEM_COUNT_WINDOW,
    OBJECT_PARA_PURCHASE_WINDOW,
    OBJECT_PARA_SHOP_WINDOW,
    OBJECT_TYPE_COUNT
};

/* `object` if its operation table answers `type`, else NULL (also for NULL, an out-of-range
   type or a table without a row): the original client's dynamic_cast. */
void *object_as_type(void *object, enum ObjectType type);

/* The class name of an operation table for diagnostics, or "?" for a table without a row. */
const char *object_type_name(const void *table);

/* The object types: one row per class, keyed by its operation table, with the ObjectType
   queries the class answers and its name.

   Who owns the rows: the directory that defines the classes, in its *_object_types.c (for
   example ui/controls/controls_object_types.c and actor/actor_object_types.c; the NPC actor
   class of core/character_npc.c is with the actor rows).

   Composition: g_objectTypes (core/object_type_registry.c) lists those tables;
   client_install_hooks() installs it with object_types_install() before anything asks.

   Order: an operation table has at most one row in all the tables; installing a second row
   for a table stops the client (fatal_error). Apart from that the order of the tables and of
   their rows decides nothing. Within a file the rows are in the order of the class names.

   A missing row, or no installed table: object_as_type returns NULL for every type, and
   object_type_name returns "?", as for the classes the client no longer has. */
struct ObjectTypeRow {
    const void *table;
    uint32_t types;
    const char *name;
};

/* One directory's rows. */
struct ObjectTypeRows {
    const struct ObjectTypeRow *rows;
    size_t count;
};
#define OBJECT_TYPE_ROWS(rows) { (rows), sizeof(rows) / sizeof((rows)[0]) }

/* The tables object_as_type and object_type_name use, installed at startup. */
struct ObjectTypeTable {
    const struct ObjectTypeRows *const *parts;
    size_t count;
};

/* The bit of one ObjectType query in a row's `types`. */
#define OBJECT_TYPE(kind) (UINT32_C(1) << OBJECT_##kind)

void object_types_install(const struct ObjectTypeTable *table);

#endif
