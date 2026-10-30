#include "render/sprite_atlas.h"
#include "resources/sprite.h"
#include "ui/character/character.h"
#include "session/session_names.h"
#include "ui/character/sel_char_userver.h"

void ui_window_draw_robe(struct UIWindow *self, int job, int sex, int robe, int x, int y,
                                  BYTE scaled, int action, int motion)
{
    struct String spr_name, act_name;
    struct ActRes *act;
    struct SprRes *spr;
    struct SprClip *clip;
    struct SprImg *img;

    if (robe <= 0)
        return;
    session_get_robe_spr_name(&spr_name, job, sex, robe, 0);
    if (res_mgr_is_available(g_resMgr(), string_cstr(&spr_name))) {
        session_get_robe_act_name(&act_name, job, sex, robe, 0);
        if (res_mgr_is_available(g_resMgr(), string_cstr(&act_name))) {
            act = res_mgr_get(g_resMgr(), string_cstr(&act_name));
            spr = res_mgr_get(g_resMgr(), string_cstr(&spr_name));
            clip = motion_get_clip(act_res_get_motion(act, action, motion), 0);
            if (clip->sprIndex != -1) {
                struct SpriteRegion place = {0};
                void *tex;

                memset(&place, 0, sizeof place);
                img = spr_res_image(spr, clip);
                if (!img)
                    goto done_act;
                if (clip->clipType == 0) {
                    tex = sprite_atlas_resolve(g_renderer, img, spr->palette, &place);
                } else {
                    tex = img->texture;
                }
                if (!tex) goto done_act;
                if (scaled) {
                    DWORD colour = ui_window_get_color(self);

                    ui_window_draw_spr_clip(self, x, y, img, clip,
                                         session_get_baby_job_scale(&g_session1, job), 0, colour, spr->palette, 1.0f);
                } else {
                    ui_canvas_draw_texture(self->canvas, x + clip->x, y + clip->y, tex, place.x,
                                         place.y, img->width, img->height, clip->flags,
                                         img->widthHalved + 1, img->heightHalved + 1);
                }
            }
        }
done_act:
        string_free(&act_name);
    }
    string_free(&spr_name);
}
