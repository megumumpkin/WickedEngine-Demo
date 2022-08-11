#include "ImGuizmo_Converter.h"

namespace IMGUIZMO_NAMESPACE{
    void Mat4_ToImGuizmo(float* dest, const XMFLOAT4X4& src)
    {
        for (auto row = 0; row < 4; row++)
        {
            for (auto col = 0; col < 4; col++)
                dest[row * 4 + col] = src.m[col][row];
        }
    }

    void Mat4_FromImGuizmo(XMFLOAT4X4* dest, float* src)
    {
        for (auto row = 0; row < 4; row++)
        {
            for (auto col = 0; col < 4; col++)
                dest->m[col][row] = src[row * 4 + col];
        }
    }
}