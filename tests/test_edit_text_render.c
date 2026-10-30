#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/ui/controls/edit_text_render.c"

static int created, destroyed, drawn, plain, colours, font_selected;
static int positions[2], lengths[2];
static const char *expected_text;
static struct UIEditCtrlCustom *expected_edit;
struct TextDC *text_dc_construct(struct TextDC *dc, struct DcBitmap *target)
{
    assert(target == expected_edit->edit.base.canvas); memset(dc,0,sizeof *dc); ++created; return dc;
}
void text_dc_destruct(struct TextDC *dc) { assert(dc); ++destroyed; }
void text_dc_select_font(struct TextDC *dc,int face,int height,int bold,int italic)
{ assert(dc && !face && height==EDIT_FONT_HEIGHT && bold==expected_edit->font_flag && !italic); ++font_selected; }
COLORREF text_dc_set_text_color(struct TextDC *dc,COLORREF colour)
{
    assert(dc && colour==(colours ? (COLORREF)expected_edit->edit.double_colour : expected_edit->edit.text_colour));
    ++colours; return 0;
}
void text_dc_text_out(struct TextDC *dc,int x,int y,const char *text,int length)
{
    assert(dc && y==20 && text==expected_text && length==(int)strlen(text) && drawn<2);
    positions[drawn]=x; lengths[drawn++]=length;
}
void ui_window_text_out(struct UIWindow *wnd,int x,int y,const char *text,int length,int face,int height,
                     COLORREF colour,int bold,int italic)
{
    assert(wnd==&expected_edit->edit.base && x==10 && y==20 && text==expected_text && !length && !face);
    assert(height==EDIT_FONT_HEIGHT && colour==expected_edit->edit.text_colour && bold==expected_edit->font_flag && !italic);
    ++plain;
}
int main(void)
{
    struct UIEditCtrlCustom edit={0};
    expected_edit=&edit; edit.edit.base.canvas=(void *)1; edit.edit.text_colour=0x123456; edit.font_flag=1;
    expected_text="한국 text"; edit.edit.double_colour=-1;
    ui_edit_ctrl_custom_draw_string(&edit,10,20,expected_text); assert(plain==1 && !created && !drawn);
    edit.edit.double_colour=0x654321;
    ui_edit_ctrl_custom_draw_string(&edit,10,20,expected_text);
    assert(created==1 && destroyed==1 && font_selected==1 && colours==2 && drawn==2);
    assert(positions[0]==10 && positions[1]==9 && lengths[0]>0 && lengths[0]==lengths[1]);
    puts("Edit text rendering: shared context, real draw helper, Korean byte lengths, outline order and release passed.");
    return 0;
}
