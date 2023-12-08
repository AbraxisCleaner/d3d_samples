#ifndef _STDAFX_H_
#define _STDAFX_H_
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <memory.h>

#define ZeroThat(x) memset(x, 0, sizeof(*x))

typedef unsigned int uint;
typedef unsigned char uchar;

static void DebugPrintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char *buffer = NULL;
    int buffer_size = 512;
    int to_write = 0;
    int win = 0;

    while (!win)
    {
        buffer = (char *)malloc(buffer_size);
        memset(buffer, 0, buffer_size);

        if ((to_write = vsprintf_s(buffer, buffer_size, fmt, args)) <= 0)
        {
            free(buffer);
            buffer_size += 512;
            continue;
        }

        win = TRUE;
    }

    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), buffer, to_write, NULL, NULL);
    OutputDebugStringA(buffer);

    va_end(args);
    free(buffer);
}

#pragma warning(disable : 5103) // pasting '"*"' and '"*"' does not result in a valid preprocessing token
#define LOG(x) DebugPrintf("[%s]: " ## x, __FUNCTION__)
#define LOGF(x, ...) DebugPrintf("[%s]: " ## x, __FUNCTION__, __VA_ARGS__)
#define ASSERT(x) if (!(x)) { LOGF("Assertion Failed: %s\n\tFile: %s\n\tLine: %d\n", #x, __FILE__, __LINE__); __debugbreak(); *(int*)0 = 0; }

#endif