#pragma once
#include <Utility/DirectXMath.h>
#include <WickedEngine.h>
#include "ImGuizmo.h"

namespace IMGUIZMO_NAMESPACE{
    void Mat4_ToImGuizmo(float* dest, const XMFLOAT4X4& src);
    void Mat4_FromImGuizmo(XMFLOAT4X4* dest, float* src);
}