#ifndef _CAMERA_H_
#define _CAMERA_H_
#include <pch.h>
#include <vendor/HandmadeMath.h>

struct Camera
{
    float fov;
    HMM_Vec3 position;
    HMM_Vec3 orbit;
    HMM_Vec3 target;
    HMM_Vec2 view_plane_distance;
    float speed;

    void Tick(float timestep);
    HMM_Mat4 GetMatrix();
};

#endif