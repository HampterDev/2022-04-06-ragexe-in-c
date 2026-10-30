#ifndef FILEIO_PROCESS_H
#define FILEIO_PROCESS_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include "base.h"

typedef BOOL (WINAPI *EnumProcessModulesFn)(HANDLE process, HMODULE *modules, DWORD bytes, DWORD *needed);
typedef DWORD (WINAPI *GetModuleFileNameExAFn)(HANDLE process, HMODULE module, char *name, DWORD size);
typedef HANDLE (WINAPI *CreateToolhelp32SnapshotFn)(DWORD flags, DWORD pid);
typedef BOOL (WINAPI *ProcessWalkFn)(HANDLE snapshot, PROCESSENTRY32 *entry);
typedef BOOL (WINAPI *ModuleWalkFn)(HANDLE snapshot, MODULEENTRY32 *entry);

struct IProcess {
    EnumProcessModulesFn enumProcessModules;
    GetModuleFileNameExAFn getModuleFileNameEx;
    CreateToolhelp32SnapshotFn createSnapshot;
    ProcessWalkFn process32First;
    ProcessWalkFn process32Next;
    ModuleWalkFn module32First;
    ModuleWalkFn module32Next;
    HMODULE library;
    int failed;
    int usePsapi;
};
OFFSET_ASSERT(IProcess, library, 0x1c);
SIZE_ASSERT(IProcess, 0x28);

void delete_hack_files(void);

#endif
