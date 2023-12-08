// NOTE: Not actually a precompiled header.
#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>
#include <assert.h>
#include <string.h>

#include <Windows.h>
#define ZeroThat(x) memset(x, 0, sizeof(*x))

typedef unsigned int uint;
typedef unsigned char uchar;

#ifdef _DEBUG

static void DebugPrintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    
    size_t total_size = 1024;
    char *buffer = (char *)malloc(total_size);
    memset(buffer, 0, total_size);
    
    int chars = 0;
    while (!(chars = vsprintf_s(buffer, 1024, fmt, args)))
    {
        free(buffer);
        total_size += 1024;
        buffer = (char *)malloc(total_size);
        memset(buffer, 0, total_size);
    }
    
    static auto std_out = GetStdHandle(STD_OUTPUT_HANDLE);
    WriteFile(std_out, buffer, chars, nullptr, nullptr);
    OutputDebugStringA(buffer);
 
    va_end(args); 
}

#define LOG(x) DebugPrintf("[%s]: " ## x, __FUNCTION__)
#define LOGF(x, ...) DebugPrintf("[%s]: " ## x, __FUNCTION__, __VA_ARGS__)
#define ASSERT(x) assert(x)

#else

#define LOG(x)
#define LOGF(x, ...)
#define ASSERT(x) x

#endif