#ifndef RENDER_PRESENT_FIT_H
#define RENDER_PRESENT_FIT_H

/* Window points the scene is raised by, so that the edit box being typed into stays above Android's
   on-screen keyboard; core/app_sdl.c sets it, render/gfx_gl.c holds it, and it is 0 elsewhere. */
extern int g_presentLift;

/* Where the scene lands in a window of another size: as large as fits with its aspect kept,
   centred, raised by lift, the rest black. The GL backend presents into it and the SDL layer maps
   the pointer back through it. */
static inline void present_fit(int window_w, int window_h, int scene_w, int scene_h, int lift, int *x, int *y,
                               int *w, int *h)
{
    if (window_w <= 0 || window_h <= 0 || scene_w <= 0 || scene_h <= 0) {
        *x = *y = 0;
        *w = window_w;
        *h = window_h;
        return;
    }
    if ((long long)window_w * scene_h > (long long)window_h * scene_w) {
        *h = window_h;
        *w = (int)((long long)window_h * scene_w / scene_h);
    } else {
        *w = window_w;
        *h = (int)((long long)window_w * scene_h / scene_w);
    }
    *x = (window_w - *w) / 2;
    *y = (window_h - *h) / 2 - lift;
}

#endif
