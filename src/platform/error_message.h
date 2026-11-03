#ifndef PLATFORM_ERROR_MESSAGE_H
#define PLATFORM_ERROR_MESSAGE_H

/* Shows `text` in a modal "Error" message box; `flags` are the MessageBox flags. */
void error_msg(const char *text, int flags);

/* Called by error_msg before it shows the box, so that a full-screen display hands the
   screen back to GDI and the box is visible. client_install_hooks() (core/client_init.c)
   installs device_flip_to_gdi (render/c3ddevice.c). NULL: the box is shown without that
   step, which is what device_flip_to_gdi does while no DirectDraw device exists. */
extern void (*g_errorMsgPrepareDisplay)(void);

#endif
