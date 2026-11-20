#ifndef ACTOR_LIVENESS_H
#define ACTOR_LIVENESS_H

struct UIWindow;

int actor_is_live(const void *actor);

void actors_forget_window(const struct UIWindow *wnd);

#endif
