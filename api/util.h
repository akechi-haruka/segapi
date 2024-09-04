#pragma once
#include <stdint.h>

bool sj2utf8(const uint8_t* indata, int inlen, uint8_t* outdata, int* outlen);