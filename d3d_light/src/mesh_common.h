#ifndef _MESH_COMMON_H_
#define _MESH_COMMON_H_
#include "stdafx.h"

#include "transform.h"

// @TODO: Rearrange vertex parameters.
struct Vertex {
    float position[3];
    float color[3];
    float texcoord[2];
    float normal[3];
};

union Face {
    struct {
        unsigned int v0, v1, v2;
    };
    uint elements[3];
};

#endif 
