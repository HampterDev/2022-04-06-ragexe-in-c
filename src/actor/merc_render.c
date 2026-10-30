#include "actor/render_ops.h"
#include "render/renderer.h"
#include "resources/palette.h"
#include <stdio.h>
#include "contents/body_layer.h"
#include "actor/sprite_render.h"

static const char *const merc_hats[30] = {
    "여\\여_털모자"  ,
    "여\\여_골든헤드기어"  ,
    "여\\여_대형금방울"  ,
    "여\\여_엘리스모자"  ,
    "여\\여_가부키가면"  ,
    "여\\여_여왕의투구"  ,
    "여\\여_지르타스가면"  ,
    "여\\여_실버티아라"  ,
    "여\\여_고양이모자"  ,
    "여\\여_예쁜리본"  ,
    "남\\남_간호모"  ,
    "남\\남_검은눈가리개"  ,
    "남\\남_고블린가면""4호"  ,
    "남\\남_양모자"  ,
    "남\\남_귀찮은너구리"  ,
    "남\\남_돈잃은자의마음"  ,
    "남\\남_마리오넷모자"  ,
    "남\\남_라이온마스크"  ,
    "남\\남_클로스헬멧"  ,
    "남\\남_로드카호의뿔"  ,
    "남\\남_반창고"  ,
    "남\\남_분노입"  ,
    "남\\남_코끼리모자"  ,
    "남\\남_야구모자"  ,
    "남\\남_장례건"  ,
    "남\\남_부용"  ,
    "남\\남_타이거마스크"  ,
    "남\\남_초승달투구"  ,
    "남\\남_엔젤링모자"  ,
    "남\\남_태양신의모자"  ,
};

#define MERC_RES(path) res_mgr_get(g_resMgr(), (path))

static struct Motion *merc_motion(struct RenderObject *self, struct ActRes *act)
{
    struct Motion *motion = act_res_get_motion(act, self->action, self->motion);

    if (self->action < 8)
        motion = act_res_get_motion(act, self->action, 0);
    return motion;
}

static int merc_attach_match(const struct Motion *a, const struct Motion *b)
{
    return a->attachment_count && b->attachment_count &&
           ((const struct AttachPointInfo *)a->attachments)->attr == ((const struct AttachPointInfo *)b->attachments)->attr;
}

void render_object_render_mercenary(struct RenderObject *self, const float *matrix, int job)
{
    struct ScreenBounds bounds;
    struct SqQuad quad;
    struct ActRes *act;
    struct SprRes *spr;
    struct Motion *motion, *body_motion = NULL, *head_motion = NULL;
    struct SprClip *clip;
    void *image, *palette;
    char path[256], name[256];
    float view[3], sx, sy, rhw, screen_scale, screen_shift, size, depth_a, depth_b;
    int rect[4];
    unsigned colour;
    int head_dx = 0, head_dy = 0, acc_dx = 0, acc_dy = 0;
    int dx, dy, flags, i;
    unsigned char drew;
    signed char layer;

    view[0] = self->pos.y * matrix[3] + self->pos.x * matrix[0] + matrix[6] * self->pos.z + matrix[9];
    view[1] = matrix[1] * self->pos.x + matrix[4] * self->pos.y + matrix[7] * self->pos.z + matrix[10];
    view[2] = matrix[2] * self->pos.x + matrix[5] * self->pos.y + matrix[8] * self->pos.z + matrix[11];
    renderer_project_point(g_renderer, &self->pos, matrix, &sx, &sy, &rhw);
    screen_scale = (float)renderer_screen_scale(g_renderer, rhw);
    self->scale = screen_scale / g_spriteScaleDivisor;
    size = (float)self->shift_x * self->zoom;
    size = (float)(renderer_scale_x(g_renderer) * size);
    screen_shift = size * self->scale;

    for (layer = 0; layer < 8; layer++) {
        dx = 0;
        dy = 0;
        if (layer >= 5 && job != 0x178a && job != 0x1794 && job != 0x179e)
            continue;
        if (layer == 0) {
            act = self->act;
            spr = self->spr;
            motion = body_motion = merc_motion(self, act);
        } else if (layer == 1) {
            int head = job % 23 + 1;
            const char *spr_fmt, *act_fmt;

            if ((unsigned)(job - 0x1781) <= 9) {
                spr_fmt = "인간족\\머리통\\여\\%d_여.spr";
                act_fmt = "인간족\\머리통\\여\\%d_여.act";
            } else {
                spr_fmt = "인간족\\머리통\\남\\%d_남.spr";
                act_fmt = "인간족\\머리통\\남\\%d_남.act";
            }
            sprintf(path, act_fmt, head);
            act = (struct ActRes *)MERC_RES(path);
            sprintf(path, spr_fmt, head);
            spr = (struct SprRes *)MERC_RES(path);
            motion = head_motion = merc_motion(self, act);
            if (merc_attach_match(head_motion, body_motion)) {
                const struct AttachPointInfo *h = (const struct AttachPointInfo *)head_motion->attachments;
                const struct AttachPointInfo *b = (const struct AttachPointInfo *)body_motion->attachments;

                head_dx = b->x - h->x;
                head_dy = b->y - h->y;
            }
            dx = head_dx;
            dy = head_dy;
        } else if (layer == 2 || layer == 3) {
            const char *act_name, *spr_name;

            if (layer == 2) {
                if ((unsigned)(job - 0x1781) <= 9) {
                    spr_name = "인간족\\용병\\활용병_활.spr";
                    act_name = "인간족\\용병\\활용병_활.act";
                } else if ((unsigned)(job - 0x178b) <= 9) {
                    spr_name = "인간족\\용병\\창용병_창.spr";
                    act_name = "인간족\\용병\\창용병_창.act";
                } else {
                    spr_name = "인간족\\용병\\검용병_검.spr";
                    act_name = "인간족\\용병\\검용병_검.act";
                }
            } else {
                if (job >= 0x1781 && job <= 0x178a)
                    continue;
                if ((unsigned)(job - 0x178b) <= 9) {
                    spr_name = "인간족\\용병\\창용병_창_검광.spr";
                    act_name = "인간족\\용병\\창용병_창_검광.act";
                } else {
                    spr_name = "인간족\\용병\\검용병_검_검광.spr";
                    act_name = "인간족\\용병\\검용병_검_검광.act";
                }
            }
            sprintf(path, act_name);
            act = (struct ActRes *)MERC_RES(path);
            sprintf(path, spr_name);
            spr = (struct SprRes *)MERC_RES(path);
            motion = act_res_get_motion(act, self->action, self->motion);
        } else {
            if (layer == 4) {
                if ((unsigned)(job - 0x1781) <= 0x1d)
                    sprintf(name, merc_hats[job - 0x1781]);
            } else if (layer == 5) {
                if (job == 0x178a)
                    sprintf(name, "여\\여_꽃잎");
                else if (job == 0x1794)
                    sprintf(name, "남\\남_토스트");
            } else if (layer == 6) {
                if (job == 0x178a)
                    sprintf(name, "여\\여_요정의귀");
                else if (job == 0x1794 || job == 0x179e)
                    sprintf(name, "남\\남_요정의귀");
            } else {
                if (job == 0x178a)
                    sprintf(name, "여\\여_미스틱로즈");
                else if (job == 0x1794)
                    sprintf(name, "남\\남_타키우스눈가리개");
                else if (job == 0x179e)
                    sprintf(name, "남\\남_사이클롭스아이");
            }
            sprintf(path, "악세사리\\%s.act", name);
            act = (struct ActRes *)MERC_RES(path);
            sprintf(path, "악세사리\\%s.spr", name);
            spr = (struct SprRes *)MERC_RES(path);
            motion = merc_motion(self, act);
            if (merc_attach_match(motion, head_motion)) {
                const struct AttachPointInfo *a = (const struct AttachPointInfo *)motion->attachments;
                const struct AttachPointInfo *h = (const struct AttachPointInfo *)head_motion->attachments;

                acc_dx = h->x - a->x;
                acc_dy = h->y - a->y;
            }
            dx = head_dx + acc_dx;
            dy = head_dy + acc_dy;
        }

        drew = 0;
        bounds.left = (float)g_renderer->width;
        bounds.top = (float)g_renderer->height;
        bounds.right = 0.0f;
        bounds.bottom = 0.0f;
        bounds.z = rhw;
        for (i = 0; i < motion->clip_count; i++) {
            clip = motion_get_clip(motion, i);
            if (clip->sprIndex == -1)
                continue;
            image = sprite_resource_image(spr, clip->clipType, clip->sprIndex);
            if (!image) continue;
            if (!render_object_clip_to_quad(self, i, i, screen_scale, clip, sx, sy, view, screen_shift, &depth_a, &depth_b,
                                                    image, &quad, rect, &colour, dx, dy, 0))
                continue;
            quad.image = image;
            flags = (colour >> 24) < 0xff ? 1 : 0x101;
            drew = 1;
            if (layer >= 4) {
                float bias = (float)(layer - 4) * 4.0e-5f + 0.0001f;

                flags = 1;
                quad.depth_a = quad.depth_a + bias;
                quad.depth_b = quad.depth_b + bias;
            }
            if (self->pickable) {
                float lo = quad.x0 < quad.x1 ? quad.x0 : quad.x1;
                float hi = quad.x0 > quad.x1 ? quad.x0 : quad.x1;

                if (bounds.left > lo)
                    bounds.left = lo;
                if (bounds.top > quad.top)
                    bounds.top = quad.top;
                if (hi > bounds.right)
                    bounds.right = hi;
                if (quad.bottom > bounds.bottom)
                    bounds.bottom = quad.bottom;
            }
            palette = spr->palette;
            if (self->body_palette)
                palette = palette_resource_colors(MERC_RES(string_cstr(&self->body_pal_name)), spr->palette);
            if (layer <= 1)
                render_object_draw_pulse_outline(self, clip, rect, screen_shift, depth_a, depth_b, palette, image, colour);
            QUAD_BUILDER(self, &quad, palette, flags, 0.0f, 0.0f);
        }
        if (drew)
            render_object_ops_of(self)->render_body(self, &bounds, sx, sy);
        else
            render_object_set_screen_position(self, sx, sy);
    }
    self->drawn = 1;
}

void render_object_submit_flipped_quad(struct RenderObject *pc, struct SqQuad *quad,
                                          void *palette, int flags, float x, float y)
{
    quad->angle = 180.0f;
    QUAD_BUILDER(pc, quad, palette, flags, (x - quad->x1) + (x - quad->x0),
                 (y - quad->bottom) + (y - quad->top));
}
