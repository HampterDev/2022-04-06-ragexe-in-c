
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winternl.h>
#include <stdio.h>
#include <string.h>

typedef NTSTATUS(NTAPI *NtQueryInformationProcess_fn)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

static int fail(const char *what, int code)
{
    fprintf(stderr, "parity_launch: %s (error %lu)\n", what, GetLastError());
    return code;
}

static int read_mem(HANDLE proc, const void *at, void *out, SIZE_T n)
{
    SIZE_T done = 0;
    return ReadProcessMemory(proc, at, out, n, &done) && done == n;
}

static int write_code(HANDLE proc, void *at, const void *bytes, SIZE_T n)
{
    DWORD old;
    SIZE_T done = 0;
    if (!VirtualProtectEx(proc, at, n, PAGE_EXECUTE_READWRITE, &old))
        return 0;
    if (!WriteProcessMemory(proc, at, bytes, n, &done) || done != n)
        return 0;
    VirtualProtectEx(proc, at, n, old, &old);
    return FlushInstructionCache(proc, at, n);
}


static DWORD remote_call(HANDLE proc, LPTHREAD_START_ROUTINE fn, void *arg)
{
    HANDLE t = CreateRemoteThread(proc, NULL, 0, fn, arg, 0, NULL);
    DWORD code = (DWORD)-1;
    if (!t)
        return code;
    if (WaitForSingleObject(t, 60000) == WAIT_OBJECT_0)
        GetExitCodeThread(t, &code);
    CloseHandle(t);
    return code;
}

static void append_quoted(wchar_t *cmd, size_t cap, const wchar_t *arg)
{
    size_t n = wcslen(cmd);
    _snwprintf(cmd + n, cap - n - 1, n ? L" \"%s\"" : L"\"%s\"", arg);
    cmd[cap - 1] = 0;
}

int wmain(int argc, wchar_t **argv)
{
    static wchar_t cmd[32768];
    wchar_t dll[MAX_PATH];
    STARTUPINFOW si = { sizeof si };
    PROCESS_INFORMATION pi;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION lim = { 0 };
    PROCESS_BASIC_INFORMATION pbi;
    NtQueryInformationProcess_fn query;
    HANDLE job;
    BYTE *image, *entry;
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS32 nt;
    BYTE saved[2], spin[2] = { 0xEB, 0xFE };
    CONTEXT ctx;
    HMODULE local;
    void *remote_path, *remote_init;
    DWORD remote_dll, ok, code, waited;
    int i;

    if (argc < 4) {
        fprintf(stderr, "usage: parity_launch <parity.dll> <working folder> <client exe> [arguments...]\n");
        return 90;
    }
    if (!GetFullPathNameW(argv[1], MAX_PATH, dll, NULL))
        return fail("parity.dll path", 90);
    for (i = 3; i < argc; i++)
        append_quoted(cmd, sizeof cmd / sizeof cmd[0], argv[i]);

    job = CreateJobObjectW(NULL, NULL);
    lim.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!job || !SetInformationJobObject(job, JobObjectExtendedLimitInformation, &lim, sizeof lim))
        return fail("job object", 91);
    if (!CreateProcessW(argv[3], cmd, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, argv[2], &si, &pi))
        return fail("CreateProcess", 92);
    if (!AssignProcessToJobObject(job, pi.hProcess))
        return fail("job assignment", 91);

    query = (NtQueryInformationProcess_fn)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQueryInformationProcess");
    if (!query || query(pi.hProcess, ProcessBasicInformation, &pbi, sizeof pbi, NULL) != 0 ||
        !read_mem(pi.hProcess, (BYTE *)pbi.PebBaseAddress + 8, &image, sizeof image) ||
        !read_mem(pi.hProcess, image, &dos, sizeof dos) ||
        !read_mem(pi.hProcess, image + dos.e_lfanew, &nt, sizeof nt))
        return fail("reading the client's image header", 93);
    entry = image + nt.OptionalHeader.AddressOfEntryPoint;
    if (!read_mem(pi.hProcess, entry, saved, 2) || !write_code(pi.hProcess, entry, spin, 2))
        return fail("parking the entry point", 93);

    ResumeThread(pi.hThread);
    ctx.ContextFlags = CONTEXT_CONTROL;
    for (waited = 0;; waited += 5) {
        if (waited > 30000)
            return fail("the client never reached its entry point", 94);
        Sleep(5);
        SuspendThread(pi.hThread);
        if (GetThreadContext(pi.hThread, &ctx) && (BYTE *)(UINT_PTR)ctx.Eip == entry)
            break;
        ResumeThread(pi.hThread);
    }

    remote_path = VirtualAllocEx(pi.hProcess, NULL, sizeof dll, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote_path || !WriteProcessMemory(pi.hProcess, remote_path, dll, sizeof dll, NULL))
        return fail("writing the DLL path", 95);
    remote_dll = remote_call(pi.hProcess,
                             (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"),
                             remote_path);
    if (!remote_dll || remote_dll == (DWORD)-1)
        return fail("LoadLibrary of parity.dll in the client", 95);

    
    local = LoadLibraryExW(dll, NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (!local)
        return fail("loading parity.dll here", 95);
    remote_init = (BYTE *)(UINT_PTR)remote_dll + ((BYTE *)GetProcAddress(local, "parity_init") - (BYTE *)local);
    ok = remote_call(pi.hProcess, (LPTHREAD_START_ROUTINE)remote_init, (void *)(UINT_PTR)pi.dwThreadId);
    if (ok != 1)
        return fail("parity_init in the client", 96);

    if (!write_code(pi.hProcess, entry, saved, 2))
        return fail("restoring the entry point", 93);
    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);

    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    return (int)code;
}
