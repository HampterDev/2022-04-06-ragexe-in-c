/* render/present_fit.h: where the scene lands in the window, and the scene pixel under a point of
   the window, also where SDL's window coordinates are points of several pixels (macOS, scaled
   Wayland) and the scene, drawn at the window's size in pixels, has more pixels than the window
   has points. */
#include <assert.h>
#include <stdio.h>
#include "render/present_fit.h"

static void point(int window_w, int window_h, int scene_w, int scene_h, const struct PresentCover *cover,
                  float x, float y, int want_x, int want_y)
{
    int got_x = -1, got_y = -1;
    present_point(window_w, window_h, scene_w, scene_h, cover, x, y, &got_x, &got_y);
    assert(got_x == want_x && got_y == want_y);
}

int main(void)
{
    static const struct PresentCover none = { 0, 0, 0, 0, 0 };
    /* an Android desktop's caption bar 50 points over the top, and the scene raised 30 for the
       on-screen keyboard */
    static const struct PresentCover caption = { 0, 50, 0, 0, 0 }, lifted = { 0, 50, 0, 0, 30 };
    int x, y, w, h;

    /* a point is a pixel (Windows, Android, X11) */
    point(1600, 900, 1600, 900, &none, 0.0f, 0.0f, 0, 0);
    point(1600, 900, 1600, 900, &none, 123.75f, 456.5f, 123, 456);
    point(1600, 900, 1600, 900, &none, 1599.9f, 899.9f, 1599, 899);

    /* two pixels a point: a 1600 by 900 scene in an 800 by 450 point window */
    present_fit(800, 450, 1600, 900, &none, &x, &y, &w, &h);
    assert(x == 0 && y == 0 && w == 800 && h == 450);
    point(800, 450, 1600, 900, &none, 0.0f, 0.0f, 0, 0);
    point(800, 450, 1600, 900, &none, 100.5f, 50.25f, 201, 100);
    point(800, 450, 1600, 900, &none, 799.9f, 449.9f, 1599, 899);
    point(800, 450, 1600, 900, &none, 400.0f, 225.0f, 800, 450);

    /* one and a half: a 1500 by 900 scene in a 1000 by 600 point window */
    point(1000, 600, 1500, 900, &none, 500.0f, 300.0f, 750, 450);
    point(1000, 600, 1500, 900, &none, 1.0f, 1.0f, 1, 1);
    point(1000, 600, 1500, 900, &none, 999.5f, 599.5f, 1499, 899);

    /* what covers the window is left out, in points, and the lift raises the scene */
    present_fit(800, 500, 1600, 900, &caption, &x, &y, &w, &h);
    assert(x == 0 && y == 50 && w == 800 && h == 450);
    point(800, 500, 1600, 900, &caption, 400.0f, 275.0f, 800, 450);
    point(800, 500, 1600, 900, &lifted, 400.0f, 245.0f, 800, 450);

    /* a scene of another aspect is centred with black bars, and a point on a bar falls outside */
    present_fit(1000, 450, 1600, 900, &none, &x, &y, &w, &h);
    assert(x == 100 && y == 0 && w == 800 && h == 450);
    point(1000, 450, 1600, 900, &none, 100.0f, 0.0f, 0, 0);
    point(1000, 450, 1600, 900, &none, 50.0f, 10.0f, -100, 20);

    /* no scene yet: the window's coordinates pass through */
    point(800, 450, 0, 0, &none, 12.5f, 34.5f, 12, 34);
    puts("Present fit: the pointer at one, two and one and a half pixels a point, covered windows, other aspects and no scene passed.");
    return 0;
}
