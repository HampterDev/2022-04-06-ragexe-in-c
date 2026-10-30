#ifndef UI_TIPS_TIPBOX_MGR_H
#define UI_TIPS_TIPBOX_MGR_H
#include <stddef.h>

struct TipEffect {
    int kind, blinks;
    int x0, y0, x1, y1;
    float duration;
};

struct TipboxTip {
    const char *title, *image;
    const char *const *pages;
    size_t page_count;
    int image_kind, image_arg;
    const struct TipEffect *effects;
    size_t effect_count;
};

enum TipPicture { TIP_PICTURE_TEXTURE, TIP_PICTURE_SPRITE, TIP_PICTURE_NONE };

struct TipboxView {
    struct TipboxTip tip;
    enum TipPicture picture;
    float left, top, right, bottom;
    int tex_arg1, tex_arg2;
    float scale;
    const char *sprite;
    int page;
};

int tipbox_load(void);
void tipbox_clear(void);
const struct TipboxView *tipbox_view(void);
void tipbox_show(int id);
const struct TipboxTip *tipbox_pick_random(void);
const char *tipbox_title(int id);
size_t tipbox_search(const char *words);
size_t tipbox_search_count(void);
int tipbox_search_id(size_t index);
void tipbox_set_page(int page);
const char *tipbox_page_text(void);
int tipbox_on_last_page(void);

#endif
