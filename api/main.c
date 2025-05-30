#include <windows.h>

#include "dprintf.h"

BOOL WINAPI DllMain(HMODULE mod, DWORD cause, void *ctx) {

    if (cause != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    dprintf("segapi: DLL was loaded\n");

    return TRUE;
}
