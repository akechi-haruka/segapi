#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "api/config.h"


void api_config_load(
        struct api_config *cfg,
        const wchar_t *filename)
{

    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"api", L"enable", 0, filename);

    wchar_t tmp[32];
	GetPrivateProfileStringW(
            L"api",
            L"bindAddr",
            L"255.255.255.255",
            tmp,
            _countof(tmp),
            filename);
    wcstombs(cfg->bindAddr, tmp, sizeof(cfg->bindAddr));

    cfg->log = GetPrivateProfileIntW(L"api", L"log", 1, filename);
    cfg->port = GetPrivateProfileIntW(L"api", L"port", 5364, filename);
    cfg->groupId = GetPrivateProfileIntW(L"api", L"groupId", 1, filename);
    cfg->deviceId = GetPrivateProfileIntW(L"api", L"deviceId", 1, filename);

}
