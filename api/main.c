#include <windows.h>

#include "dprintf.h"

BOOL WINAPI DllMain([[maybe_unused]] HMODULE mod, DWORD cause, [[maybe_unused]] void *ctx) {

    if (cause != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    dprintf("segapi: DLL was loaded\n");

    return TRUE;
}
