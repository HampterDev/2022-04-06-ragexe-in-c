#ifndef KAFRA_SERVICES_H
#define KAFRA_SERVICES_H

#include <stddef.h>
struct ClientLua;
struct lua_State;

size_t kafra_destination_count(const char *map);
const char *kafra_destination(const char *map, size_t index);
int kafra_services_load(struct ClientLua *lua);
void kafra_positions_clear(void);
void kafra_destinations_clear(void);
void map_list_register_lua_functions(struct lua_State *L);

#endif
