#ifndef CRASH_REPORT_H
#define CRASH_REPORT_H

int crash_report_install(void (*on_crash)(void));
void crash_report_uninstall(void);

void crash_report_show_dialog(const char *text);

#endif
