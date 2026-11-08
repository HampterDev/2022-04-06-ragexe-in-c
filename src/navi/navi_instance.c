#include "navi/navi.h"

/* The client's navigation manager. Apart from navi/navi_manager.c, whose loading and
   arrival reach the windows and the session, so that the code that only reads it does
   not. */
struct NaviManager g_navi;
