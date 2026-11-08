#include <string.h>
#include "effects/rag/rag_effect.h"

/* The primitive functions that the master effect (rag_effect.c) calls too. They are apart
   from effect_prim.c, which reaches the actors and the session, so that the master does
   not reach them through its primitives. */

struct PrimTriangle *prim_triangle_construct(struct PrimTriangle *t)
{
    int i;

    for (i = 0; i < 3; i++) {
        memset(&t->v[i], 0, sizeof(t->v[i]));
        t->v[i].specular = 0xff000000;
    }
    return t;
}

void effect_prim_send_msg(struct EffectPrim *self, RenderMsgArg msg)
{
    if (msg == PRIM_MSG_MASTER_GONE)
        self->master = NULL;
}
