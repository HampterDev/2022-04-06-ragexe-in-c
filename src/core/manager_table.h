#ifndef CORE_MANAGER_TABLE_H
#define CORE_MANAGER_TABLE_H

#include <stddef.h>

/* The manager table: one row per client manager that startup creates in app_init_managers
   (core/startup.c) and shutdown destroys in app_destroy_singletons (core/app.c). Before it,
   the two functions were two hand-written call sequences.

   Composition: g_clientManagers (core/client_managers.c) is the one list of rows. Startup
   runs it forwards with manager_table_create(); shutdown runs it in destroy-rank order with
   manager_table_destroy(). Shutdown is not the reverse of startup (the order was recorded as
   it was, see docs/purist-review.md), so each row says where its destroy comes.

   Order: the list order is the create order. destroy_rank is the destroy order: the rows are
   destroyed from the lowest rank to the highest; gaps between ranks are allowed. Two rows
   with a destroy and the same rank, or a stop or warn row without its text, stop the client
   (fatal_error) before the first create or destroy.

   Failure: a create that fails with MANAGER_STOP ends startup at that row: the rows after it
   are not created, and the rows before it are not destroyed, because client_run
   (core/winmain.c) shows the text and returns without calling app_destroy_singletons; the
   process ends anyway. That was the behaviour before the table and is kept as it was.

   A missing row: startup does not create the manager and shutdown does not destroy it. Code
   that uses it then finds no instance, or one that its *_instance() function makes on first
   use and that nothing destroys before the process ends.

   Columns:
   name          what the row is, in the error text of a broken table.
   create        creates (and, for the managers that read a file, loads) the manager and
                 returns nonzero on success. NULL: the manager is created elsewhere (the
                 skill-action manager, by client_init's skill_actions_register); startup
                 skips the row.
   destroy       destroys the manager. NULL: nothing to destroy; the rank is not used.
   destroy_rank  where the destroy comes at shutdown (lowest first).
   on_failure    what a create that returns 0 does:
                 MANAGER_STOP(text): startup stops; app_init_managers returns `text` as the
                 failure, which client_run shows in a message box.
                 MANAGER_WARN(text): a message box with `text` (caption "Error"); startup
                 goes on.
                 MANAGER_IGNORE: nothing; startup goes on. */

typedef int (*ManagerCreate)(void);
typedef void (*ManagerDestroy)(void);

enum ManagerFailureKind {
    MANAGER_FAILURE_IGNORE,
    MANAGER_FAILURE_WARN,
    MANAGER_FAILURE_STOP,
};

struct ManagerFailure {
    enum ManagerFailureKind kind;
    const char *text;
};

#define MANAGER_STOP(text) { MANAGER_FAILURE_STOP, (text) }
#define MANAGER_WARN(text) { MANAGER_FAILURE_WARN, (text) }
#define MANAGER_IGNORE { MANAGER_FAILURE_IGNORE, NULL }

struct ManagerRow {
    const char *name;
    ManagerCreate create;
    ManagerDestroy destroy;
    int destroy_rank;
    struct ManagerFailure on_failure;
};

struct ManagerTable {
    const struct ManagerRow *rows;
    size_t count;
};
#define MANAGER_TABLE(rows) { (rows), sizeof(rows) / sizeof((rows)[0]) }

/* Creates the rows in list order. Returns the MANAGER_STOP row whose create failed, or NULL
   when every row went through. */
const struct ManagerRow *manager_table_create(const struct ManagerTable *table);
/* Destroys the rows that have a destroy, from the lowest destroy_rank to the highest. */
void manager_table_destroy(const struct ManagerTable *table);

#endif
