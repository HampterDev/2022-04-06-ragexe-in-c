#ifndef UI_CONTROLS_URL_LINK_H
#define UI_CONTROLS_URL_LINK_H

#include "core/text_view.h"

/* Opens the target of a <URL> text link in the user's browser. Only an absolute
   http:// or https:// URL is opened; files, UNC paths and other protocol
   handlers are ignored. The target must be NUL-terminated at its size, as
   text-button lines are. Returns 1 when the target was handed to the shell. */
int ui_open_url_link(struct TextView target);

#endif
