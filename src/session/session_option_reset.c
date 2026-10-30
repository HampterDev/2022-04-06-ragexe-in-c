#include "std/tree.h"
#include "std/integer_map.h"
#include "contents/connection_api.h"
#include "session/csession.h"
#include "session/session_options.h"

void session_reset_option2(struct Session *self)
{
    struct IntInt64Pair *var;
    int key = 2;
    DWORD packet;

    var = tree_find(&self->instantVars, &key, tree_less_int);
    if (!var || !(int)var->value)
        return;
    self->skillUseKey = 0;
    packet = 0x7f40b11;
    rag_connection_send_packet(rag_connection_instance(), 4, &packet);
    var = tree_get(&self->instantVars, &key, sizeof key, sizeof *var, tree_less_int);
    var->value = 0;
}
