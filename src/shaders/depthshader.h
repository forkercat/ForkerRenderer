//
// Created by Junhao Wang (@Forkercat) on 2021/5/5.
//

#pragma once

#include "shader.h"

// Depth Shader For Shadow Mapping
struct DepthShader : public Shader
{
    // Interpolation
    Matrix3x3f vPositionNDC;

    // Transformations
    Matrix4x4f uModelMatrix;
    Matrix4x4f uLightSpaceMatrix;

    DepthShader() : Shader() { }

    Point4f ProcessVertex(int faceIdx, int vertIdx) override
    {
        Point4f positionCS =
            uLightSpaceMatrix * uModelMatrix * Point4f(mesh->Vert(faceIdx, vertIdx), 1.f);
        Point4f positionNDC = positionCS / positionCS.w;
        vPositionNDC.SetCol(vertIdx, positionNDC.xyz);
        return positionNDC;
    }

    bool ProcessFragment(const Vector3f& baryCoord, Color3& gl_Color) override
    {
        // Interpolation
        Point3f positionNDC = vPositionNDC * baryCoord;
        gl_Color.z = positionNDC.z * 0.5f + 0.5f;  // to [0, 1]
        return false;
    }
};