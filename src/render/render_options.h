#ifndef RENDER_RENDER_OPTIONS_H
#define RENDER_RENDER_OPTIONS_H

/* Nonzero: ground and water are drawn with their lightmaps. Defined in
   render/render_options.c; the graphics settings window and /lightmap change it. */
extern int g_lightmap;

/* Display options: loaded with the saved options (session/session_option_load.c) and changed by
   the graphics settings window (ui/render/graphic_setting.c); the device, the sprite loader and the
   frame windows read them. Defined in render/render_options.c. */
extern int g_textureMode;
extern int g_displayHeight;
extern int g_displayWidth;
extern int g_sprHalfHeight;
extern int g_sprHalfWidth;
extern int g_spriteMode;
extern int g_trilinearFilter;

#endif
