#include <windows.h>
#include <stdint.h>
#include <assert.h>

#include "util.h"

bool sj2utf8(const uint8_t* indata, int inlen, uint8_t* outdata, int* outlen){

    assert(indata != NULL);
    assert(outdata != NULL);
    assert(outlen != NULL);

    int utf16size = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, (char*)indata, -1, 0, 0);
    wchar_t pUTF16[utf16size];
    if (MultiByteToWideChar(CP_ACP, 0, (LPCCH)indata, -1, pUTF16, utf16size) != 0){
        int utf8size = WideCharToMultiByte(CP_UTF8, 0, pUTF16, -1, 0, 0, 0, 0);
        if (utf8size != 0) {
            uint8_t pUTF8[utf8size + 16];
            ZeroMemory(pUTF8, utf8size + 16);
            if (WideCharToMultiByte(CP_UTF8, 0, pUTF16, -1, (char*)pUTF8, utf8size, 0, 0) != 0) {
                if (utf8size + 16 <= *outlen) {
                    memcpy(outdata, pUTF8, utf8size + 16);
                    *outlen = utf8size + 16;
                    return true;
                }
            }
        }
    }
    return NULL;
}