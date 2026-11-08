/* Windows scene runs: the crash report goes to stderr instead of the client's crash dialog
   (src/ui/misc/crash_dialog.c), so that a crash ends the run at once instead of waiting for a
   click. The report is still written beside the executable as usual. */
#include <stdio.h>

void crash_report_show_dialog(const char *text)
{
    fflush(stdout);
    fprintf(stderr, "scene: crashed\n%s\n", text ? text : "");
    fflush(stderr);
}
