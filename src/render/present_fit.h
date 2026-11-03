#ifndef RENDER_PRESENT_FIT_H
#define RENDER_PRESENT_FIT_H

/* What covers the window, in window points: the system's bars along each edge (on Android, the caption
   bar a desktop draws over a maximised window), and how far the scene is raised so that the edit box
   being typed into stays above the on-screen keyboard. core/app_sdl.c sets it on Android,
   render/gfx_gl.c holds it, and it is all 0 elsewhere. */
struct PresentCover {
    int left, top, right, bottom, lift;
};

extern struct PresentCover g_presentCover;

/* Where the scene lands in a window of another size: as large as fits in the part of the window
   nothing covers, with its aspect kept, centred there and raised by the lift, the rest black. The GL
   backend presents into it and the SDL layer maps the pointer back through it. */
static inline void present_fit(int window_w, int window_h, int scene_w, int scene_h,
                               const struct PresentCover *cover, int *x, int *y, int *w, int *h)
{
    int area_w = window_w - cover->left - cover->right, area_h = window_h - cover->top - cover->bottom;
    if (area_w <= 0 || area_h <= 0 || scene_w <= 0 || scene_h <= 0) {
        *x = *y = 0;
        *w = window_w;
        *h = window_h;
        return;
    }
    if ((long long)area_w * scene_h > (long long)area_h * scene_w) {
        *h = area_h;
        *w = (int)((long long)area_h * scene_w / scene_h);
    } else {
        *w = area_w;
        *h = (int)((long long)area_w * scene_h / scene_w);
    }
    *x = cover->left + (area_w - *w) / 2;
    *y = cover->top + (area_h - *h) / 2 - cover->lift;
}

#endif
