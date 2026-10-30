#include "platform/language.h"
#include "platform/language_api.h"
#include <stdlib.h>
#include "fileio/process.h"
#include "std/std.h"

#include <wincrypt.h>
#include <io.h>
#include <stdarg.h>
#include <string.h>

struct IProcess g_process;
INIT_ONCE g_processGuard;

static void remove_if_present(const char *name)
{
    if (_access(name, 6) == 0)
        _unlink(name);
}

void delete_hack_files(void)
{
    remove_if_present("nconfig.ini");
    remove_if_present("ffindhack.exe");
    remove_if_present("gfindhack.exe");
    remove_if_present("removenp.exe");
    remove_if_present("cfindhack.exe");
}
