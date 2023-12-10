#ifndef _TRANSFORM_H_
#define _TRANSFORM_H_
#include "stdafx.h"
#include "HandmadeMath.h"

struct Transform
{
    HMM_Vec3 position;
    HMM_Vec3 rotation;
    HMM_Vec3 scaling;

    HMM_Mat4 as_matrix()
    {
        auto tm = HMM_Translate(position);
        auto sm = HMM_Scale(scaling);
        auto rm = HMM_MulM4(HMM_Rotate_LH(rotation[0], HMM_V3(1, 0, 0)),
                            HMM_MulM4(HMM_Rotate_LH(rotation[1], HMM_V3(0, 1, 0)),
                                      HMM_Rotate_LH(rotation[2], HMM_V3(0, 0, 1))));
        return HMM_MulM4(tm, HMM_MulM4(rm, sm));
    }

    static Transform zero()
    {
        Transform tf;
        tf.position = HMM_V3(0, 0, 0);
        tf.rotation = HMM_V3(0, 0, 0);
        tf.scaling = HMM_V3(1, 1, 1);
        return tf;
    }
};

#endif
