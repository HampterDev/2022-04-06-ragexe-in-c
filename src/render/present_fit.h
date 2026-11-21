#ifndef RENDER_PRESENT_FIT_H
#define RENDER_PRESENT_FIT_H

/* What covers the window, in window points: the system's bars along each edge (on Android, the caption
   bar a desktop draws over a maximised window), and how far the scene is raised so that the edit box
   being typed into stays above the on-screen keyboard. It is all 0 but on Android. */
struct PresentCover {
    int left, top, right, bottom, lift;
};

/* core/app_sdl.c, which learns what covers the window from the system, tells the device, which
   presents the scene around it (render/gpu_device.c). */
void gfx_set_cover(const struct PresentCover *cover);

/* Where the scene lands in a window of another size: as large as fits in the part of the window
   nothing covers, with its aspect kept, centred there and raised by the lift, the rest black. The
   device presents into it and the SDL layer maps the pointer back through it. */
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

/* The scene pixel under the point x, y of a window window_w by window_h, all in the window's own
   coordinates (SDL's: pixels on Windows and Android, points on macOS and scaled Wayland, where the
   scene has more pixels than the window has points), through where present_fit puts the scene. */
static inline void present_point(int window_w, int window_h, int scene_w, int scene_h,
                                 const struct PresentCover *cover, float x, float y, int *scene_x, int *scene_y)
{
    int fx, fy, fw, fh;
    present_fit(window_w, window_h, scene_w, scene_h, cover, &fx, &fy, &fw, &fh);
    if (fw > 0 && fh > 0 && scene_w > 0 && scene_h > 0) {
        *scene_x = (int)((x - fx) * scene_w / fw);
        *scene_y = (int)((y - fy) * scene_h / fh);
    } else {
        *scene_x = (int)x;
        *scene_y = (int)y;
    }
}

#endif
