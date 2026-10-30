#ifndef FILEIO_RESOURCE_ERRORS_H
#define FILEIO_RESOURCE_ERRORS_H

void resource_errors_init(void);
void resource_errors_destroy(void);
void resource_error_report(const char *name, const char *path);

#endif
