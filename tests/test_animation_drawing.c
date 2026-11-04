#include "render/rp_pool.h"
#define main animation_owner_fixture_main
#include "test_animation_owner.c"
#undef main
#include "ui/draw.h"
#include "ui/render/render.h"
#include "resources/bitmap.h"
#include "render/sprite_atlas.h"
struct Renderer *g_renderer;
static unsigned atlas_requests;
float get_cos_deg(int angle) { (void)angle;abort();return 0; }
float get_sin_deg(int angle) { (void)angle;abort();return 0; }
BYTE bitmap_res_make_gray_copy(struct BitmapRes *bitmap,int mode)
{ (void)bitmap;(void)mode;abort();return 0; }
void bitmap_res_free_gray_copy(struct BitmapRes *bitmap) { (void)bitmap;abort(); }
struct Texture *sprite_atlas_resolve(struct Renderer *renderer, const struct SprImg *image,
    const DWORD *palette, struct SpriteRegion *region)
{ (void)renderer; assert(image && palette && region); memset(region,0,sizeof *region); ++atlas_requests; return NULL; }
struct TexDrawPrim *renderer_new_prim(struct Renderer *renderer) { (void)renderer;abort();return NULL; }
static struct SpriteAction *draw_action;
static unsigned draws;
void dc_bitmap_draw_sprite(struct DcBitmap *canvas,int x,int y,int width,int height,
                           void *spr,void *motion,int ox,int oy,float scale,DWORD *palette)
{
    assert(canvas&&spr==&sprite&&x==7&&y==9&&width==100&&height==100);
    assert(motion==&draw_action->motions[0]||motion==&draw_action->motions[1]);
    assert(ox==41&&oy==32&&scale==1.0f&&!palette);++draws;
}
void dc_bitmap_update(struct DcBitmap *b,int x,int y,int w,int h,const DWORD *pixels,char keyed)
{ (void)b;(void)x;(void)y;(void)w;(void)h;(void)pixels;(void)keyed;abort(); }
void dc_bitmap_fill_triangle(struct DcBitmap *b,const POINT points[3],COLORREF colour)
{ (void)b;(void)points;(void)colour;abort(); }
void dc_bitmap_fill(struct DcBitmap *b,const RECT *r,DWORD color)
{ (void)b;(void)r;(void)color;abort(); }
int main(void)
{
    BYTE data[1024],empty[20]={0};struct Locations loc;size_t size=make_fixture(data,0x205,2,&loc);
    struct Res *base=animation_resource_create();struct UIWindow window={0};
    struct DcBitmap canvas={0};
    int ox,oy;float scale;unsigned i;
    window.canvas=&canvas;setup_sprite(0);
    assert(load(base,data,size));draw_action=act_res_get_action((struct ActRes*)base,0);
    ui_window_draw_spr_clip(&window,0,0,&fixture_image,
        &draw_action->motions[0].clips[0],1.0f,0,0xffffffff,sprite.palette,1.0f);
    assert(atlas_requests==1);
    assert(ui_window_fit_motion(&draw_action->motions[0],&sprite,100,100,&ox,&oy,&scale));
    ui_window_draw_actor_sprite(&window,&sprite,base,7,9,100,100,0,1,NULL);assert(draws==1);
    for(i=1;i<32;++i)ui_window_draw_actor_sprite(&window,&sprite,base,7,9,100,100,(int)i,1,NULL);
    assert(draws==32);
    ui_window_draw_actor_sprite(&window,&sprite,base,7,9,0,100,0,1,NULL);assert(draws==32);
    fixture_image.width=0;ui_window_draw_actor_sprite(&window,&sprite,base,7,9,100,100,0,1,NULL);assert(draws==32);
    setup_sprite(0);put16(empty,0x4341);put16(empty+2,0x205);assert(load(base,empty,sizeof empty));
    ui_window_draw_actor_sprite(&window,&sprite,base,7,9,100,100,5,1,NULL);assert(draws==32);
    assert(!ui_window_fit_motion(act_res_get_motion((struct ActRes*)base,0,0),&sprite,100,100,&ox,&oy,&scale));
    resource_destroy(base);clean();puts("Animation drawing: real UI bounds, typed frame indexing, missing images and empty actions passed.");return 0;
}
