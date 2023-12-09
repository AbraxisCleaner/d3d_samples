#ifndef _OBJ_IMPORT_H_
#define _OBJ_IMPORT_H_
#include "stdafx.h"

#define IS_LETTER(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
#define IS_NUMBER(c) (c >= '0' && c <= '9')

bool import_obj(char *path) {
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 128, NULL);
    if (file == (HANDLE)-1) {
        LOGF("Failed: %s\n", path);
        return false;
    }

    LARGE_INTEGER source_size;
    GetFileSizeEx(file, &source_size);

    char *source_buffer = (char *)malloc(source_size.QuadPart + 1);
    source_buffer[source_size.QuadPart] = 0;

    ReadFile(file, source_buffer, source_size.LowPart, NULL, NULL);
    CloseHandle(file);

    auto c = source_buffer;
    auto temp_c = c;

    uint num_vertices = 0;
    uint num_texcoords = 0;
    uint num_normals = 0;
    uint num_faces = 0;
    
    while (*c)
    {
        if (*c == 'v') {
            if (*(c + 1) == 't') {
                num_texcoords++;
            }
            else {
                num_vertices++;
            }
        }
        if (*c == 'f') {
            num_faces++;
        }
        if (*c == 'n') {
            num_faces++;
        }
        
        ++c;
    }

    printf("%d vertices, %d texcoords, %d faces.\n", num_vertices, num_texcoords, num_faces);

    free(source_buffer);
    return true;
}

#endif
