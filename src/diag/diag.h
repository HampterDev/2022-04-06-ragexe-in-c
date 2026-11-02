#ifndef DIAG_H
#define DIAG_H

#ifndef _WINDOWS_
#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"
#endif

DWORD image_timestamp(void);

extern unsigned g_launchedEffectIds[32];
extern DWORD g_renderCheckpointLine;
extern const char *g_renderCheckpointFile;

#endif
