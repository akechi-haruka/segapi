#pragma once
#include <stdint.h>

/**
 * Converts a SHIFT-JIS string to UTF8.
 * @param indata The input string
 * @param inlen The input string length
 * @param outdata A pre-allocated array holding the converted string.
 * @param outlen The length of the outdata. Will be set to the actual string length afterwards.
 * @return true if the conversion was successful.
 */
bool sj2utf8(const uint8_t* indata, int inlen, uint8_t* outdata, int* outlen);