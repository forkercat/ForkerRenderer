//
// Created by Junhao Wang (@Forkercat) on 2020/12/26.
//

#pragma once

#include "geometry.h"

const Vector3f WORLD_UP(0.f, 1.f, 0.f);
const Point3f  LOOK_AT_POS(0.f, 0.f, 0.f);

class Camera
{
public:
    enum ProjectionType
    {
        Orthographic,
        Perspective
    };

    explicit Camera(const Point3f& lookFrom, const Point3f& lookAt = LOOK_AT_POS);
    explicit Camera(Float lookFromX, Float lookFromY, Float lookFromZ,
                    Float lookAtX = LOOK_AT_POS.x, Float lookAtY = LOOK_AT_POS.y,
                    Float lookAtZ = LOOK_AT_POS.z);

    void SetPosition(Float x, Float y, Float z);
    void SetLookAtPos(Float x, Float y, Float z);

    Point3f GetPosition() const { return m_EyePos; }
    Point3f GetLookAt() const { return m_LookAtPos; }

    Matrix4x4f GetViewMatrix() const;
    Matrix4x4f GetPerspectiveMatrix(Float fov, Float aspectRatio, Float n, Float f) const;
    Matrix4x4f GetPerspectiveMatrix(Float l, Float r, Float b, Float t, Float n,
                                    Float f) const;
    Matrix4x4f GetOrthographicMatrix(Float l, Float r, Float b, Float t, Float n,
                                     Float f) const;

private:
    // camera attributes
    Point3f  m_EyePos;
    Point3f  m_LookAtPos;
    Vector3f m_WorldUp;
};
