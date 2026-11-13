#ifndef RENDER_DIRTY_RECTS_H
#define RENDER_DIRTY_RECTS_H

/* The parts of a texture written since it last went up to the GPU, as pure logic: a few
   rectangles kept apart, so that what was written far apart (the cells of a sprite atlas's page)
   goes up alone and not as the rectangle around all of it. Uses nothing else of the client. */
#include "platform/win_types.h"

#define DIRTY_RECTS_MOST 8

/* At most DIRTY_RECTS_MOST rectangles (left and top within, right and bottom past the edge), no
   two of which touch: they neither overlap nor share an edge or a corner. */
struct DirtyRects {
    int count;
    RECT rect[DIRTY_RECTS_MOST];
};

/* r written: it takes in every kept rectangle it touches, and again those the union touches; where
   DIRTY_RECTS_MOST are kept and it touches none, it takes in the one whose union with it covers
   the least area that neither covered, and goes on as before. Every texel of r and of the
   rectangles kept before lies within one kept after. An empty r (right <= left or bottom <= top)
   changes nothing. */
void dirty_rects_add(struct DirtyRects *d, const RECT *r);

#endif
