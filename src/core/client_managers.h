#ifndef CORE_CLIENT_MANAGERS_H
#define CORE_CLIENT_MANAGERS_H

#include "core/manager_table.h"

/* The client's managers in create order, with their destroy ranks (client_managers.c).
   app_init_managers creates them and app_destroy_singletons destroys them; manager_table.h
   says what each column and a missing row mean. */
extern const struct ManagerTable g_clientManagers;

#endif
