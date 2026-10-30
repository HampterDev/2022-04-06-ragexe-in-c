#ifndef WORLD_VIEW_API_H
#define WORLD_VIEW_API_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
struct View;

struct View *view_construct(struct View *view);
void view_destruct(struct View *view);
void view_delete(struct View *view);
void view_reset(struct View *view);
void view_on_calc_view_info(struct View *view);
void view_on_render(struct View *view);

#endif
