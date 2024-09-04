#include <windows.h>

#include "dprintf.h"

BOOL WINAPI DllMain(__attribute__((unused)) HMODULE mod, DWORD cause, __attribute__((unused)) void *ctx) {

    if (cause != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    dprintf("segapi: DLL was loaded\n");

    return TRUE;
}
