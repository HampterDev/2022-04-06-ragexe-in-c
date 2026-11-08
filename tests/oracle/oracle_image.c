#include "oracle_image.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char *g_base;
static unsigned g_preferred;
static unsigned g_size;
static int g_verbose;
#define TRACE(...) do { if (g_verbose) { fprintf(stderr, __VA_ARGS__); fflush(stderr); } } while (0)

static void __cdecl unresolved_import(void)
{
    fprintf(stderr, "oracle: the original called an import the harness could not bind\n");
    ExitProcess(3);
}

void *oracle_addr(unsigned original_va)
{
    return g_base + (original_va - g_preferred);
}

void oracle_patch(unsigned original_va, const void *bytes, unsigned size)
{
    memcpy(oracle_addr(original_va), bytes, size);
}

void oracle_patch_return(unsigned original_va, int value, unsigned stack_bytes)
{
    unsigned char code[8];
    code[0] = 0xb8;                                 /* mov eax, imm32 */
    memcpy(code + 1, &value, 4);
    if (stack_bytes) {
        code[5] = 0xc2;                             /* ret imm16 */
        code[6] = (unsigned char)stack_bytes;
        code[7] = (unsigned char)(stack_bytes >> 8);
        oracle_patch(original_va, code, 8);
    } else {
        code[5] = 0xc3;                             /* ret */
        oracle_patch(original_va, code, 6);
    }
}

void oracle_patch_jump(unsigned original_va, void *fn)
{
    unsigned char code[5];
    int rel = (int)((unsigned char *)fn - ((unsigned char *)oracle_addr(original_va) + 5));
    code[0] = 0xe9;
    memcpy(code + 1, &rel, 4);
    oracle_patch(original_va, code, 5);
}

unsigned oracle_call_target(unsigned call_va)
{
    const unsigned char *p = oracle_addr(call_va);
    int rel;
    if (p[0] != 0xe8) {
        fprintf(stderr, "oracle: no call instruction at 0x%x\n", call_va);
        ExitProcess(3);
    }
    memcpy(&rel, p + 1, 4);
    return call_va + 5 + (unsigned)rel;
}

static int apply_relocations(const IMAGE_NT_HEADERS *nt, unsigned delta)
{
    const IMAGE_DATA_DIRECTORY *dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    const unsigned char *p, *end;

    if (!dir->Size) {
        fprintf(stderr, "oracle: the image has no relocations and its base is taken\n");
        return 0;
    }
    p = g_base + dir->VirtualAddress;
    end = p + dir->Size;
    while (p + sizeof(IMAGE_BASE_RELOCATION) <= end) {
        const IMAGE_BASE_RELOCATION *block = (const IMAGE_BASE_RELOCATION *)p;
        const unsigned short *entry = (const unsigned short *)(block + 1);
        unsigned count, i;

        if (block->SizeOfBlock < sizeof *block)
            break;
        count = (block->SizeOfBlock - sizeof *block) / 2;
        for (i = 0; i < count; i++) {
            unsigned type = entry[i] >> 12, offset = entry[i] & 0xfff;
            if (type == IMAGE_REL_BASED_HIGHLOW)
                *(unsigned *)(g_base + block->VirtualAddress + offset) += delta;
            else if (type != IMAGE_REL_BASED_ABSOLUTE) {
                fprintf(stderr, "oracle: relocation type %u not handled\n", type);
                return 0;
            }
        }
        p += block->SizeOfBlock;
    }
    return 1;
}

static int game_dll(const char *name)
{
    static const char *const skip[] = { "granny2.dll", "mss32.dll", "binkw32.dll", "cps.dll", "cpsa.dll", "ijl15.dll" };
    unsigned i;
    for (i = 0; i < sizeof skip / sizeof *skip; i++)
        if (!_stricmp(name, skip[i]))
            return 1;
    return 0;
}

static int bind_imports(const IMAGE_NT_HEADERS *nt)
{
    const IMAGE_DATA_DIRECTORY *dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    const IMAGE_IMPORT_DESCRIPTOR *desc;
    unsigned unresolved = 0, bound = 0;

    if (!dir->Size)
        return 1;
    for (desc = (const IMAGE_IMPORT_DESCRIPTOR *)(g_base + dir->VirtualAddress); desc->Name; desc++) {
        const char *dll = (const char *)(g_base + desc->Name);
        /* the game's own DLLs (video, sound, model, compression, JPEG) are not loaded: their
         * DllMains start threads and devices, and no routine an oracle test calls needs them */
        HMODULE mod = game_dll(dll) ? NULL : LoadLibraryA(dll);
        TRACE("oracle: import %s -> %p\n", dll, (void *)mod);
        const unsigned *lookup = (const unsigned *)(g_base + (desc->OriginalFirstThunk ? desc->OriginalFirstThunk : desc->FirstThunk));
        unsigned *thunk = (unsigned *)(g_base + desc->FirstThunk);

        for (; *lookup; lookup++, thunk++) {
            FARPROC fn = NULL;
            if (mod) {
                if (*lookup & IMAGE_ORDINAL_FLAG32)
                    fn = GetProcAddress(mod, (LPCSTR)(UINT_PTR)(*lookup & 0xffff));
                else
                    fn = GetProcAddress(mod, ((const IMAGE_IMPORT_BY_NAME *)(g_base + *lookup))->Name);
            }
            if (fn) {
                bound++;
            } else {
                unresolved++;
                fn = (FARPROC)unresolved_import;
            }
            *thunk = (unsigned)(UINT_PTR)fn;
        }
    }
    if (unresolved)
        fprintf(stderr, "oracle: %u imports bound, %u left pointing at the trap\n", bound, unresolved);
    return 1;
}

int oracle_load(const char *exe_path)
{
    HANDLE file;
    DWORD size, read, old;
    unsigned char *data;
    const IMAGE_DOS_HEADER *dos;
    const IMAGE_NT_HEADERS *nt;
    const IMAGE_SECTION_HEADER *sec;
    unsigned i;
    char dir[MAX_PATH];
    const char *slash;

    g_verbose = getenv("ORACLE_VERBOSE") != NULL;
    file = CreateFileA(exe_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "oracle: cannot open %s (error %lu)\n", exe_path, GetLastError());
        return 0;
    }
    size = GetFileSize(file, NULL);
    data = malloc(size);
    if (!data || !ReadFile(file, data, size, &read, NULL) || read != size) {
        fprintf(stderr, "oracle: cannot read %s\n", exe_path);
        CloseHandle(file);
        return 0;
    }
    CloseHandle(file);

    dos = (const IMAGE_DOS_HEADER *)data;
    nt = (const IMAGE_NT_HEADERS *)(data + dos->e_lfanew);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || nt->Signature != IMAGE_NT_SIGNATURE
        || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386) {
        fprintf(stderr, "oracle: %s is not a 32-bit PE\n", exe_path);
        return 0;
    }
    g_preferred = nt->OptionalHeader.ImageBase;
    g_size = nt->OptionalHeader.SizeOfImage;
    g_base = VirtualAlloc((void *)(UINT_PTR)g_preferred, g_size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (!g_base)
        g_base = VirtualAlloc(NULL, g_size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (!g_base) {
        fprintf(stderr, "oracle: cannot reserve %u bytes for the image\n", g_size);
        return 0;
    }
    TRACE("oracle: image base %p (preferred %x), size %x\n", (void *)g_base, g_preferred, g_size);
    memcpy(g_base, data, nt->OptionalHeader.SizeOfHeaders);
    sec = IMAGE_FIRST_SECTION(nt);
    for (i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        unsigned raw = sec[i].SizeOfRawData, virt = sec[i].Misc.VirtualSize;
        if (raw > virt && virt)
            raw = virt;
        memcpy(g_base + sec[i].VirtualAddress, data + sec[i].PointerToRawData, raw);
    }
    TRACE("oracle: sections copied\n");
    nt = (const IMAGE_NT_HEADERS *)(g_base + dos->e_lfanew);
    free(data);

    if ((unsigned)(UINT_PTR)g_base != g_preferred && !apply_relocations(nt, (unsigned)(UINT_PTR)g_base - g_preferred))
        return 0;

    /* the game's own DLLs sit beside the executable */
    slash = strrchr(exe_path, '\\');
    if (!slash)
        slash = strrchr(exe_path, '/');
    if (slash && (size_t)(slash - exe_path) < sizeof dir) {
        memcpy(dir, exe_path, slash - exe_path);
        dir[slash - exe_path] = 0;
        SetDllDirectoryA(dir);
    }
    TRACE("oracle: binding imports\n");
    if (!bind_imports(nt))
        return 0;
    TRACE("oracle: imports bound\n");
    VirtualProtect(g_base, g_size, PAGE_EXECUTE_READWRITE, &old);
    return 1;
}
