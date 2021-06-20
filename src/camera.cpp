//
// Created by Junhao Wang (@Forkercat) on 2020/12/26.
//

#include "camera.h"

Camera::Camera(const Point3f& lookFrom, const Point3f& lookAt)
    : m_EyePos(lookFrom), m_WorldUp(WORLD_UP), m_LookAtPos(lookAt)
{
}

Camera::Camera(Float lookFromX, Float lookFromY, Float lookFromZ, Float lookAtX,
               Float lookAtY, Float lookAtZ)
    : m_EyePos(lookFromX, lookFromY, lookFromZ),
      m_WorldUp(WORLD_UP),
      m_LookAtPos(lookAtX, lookAtY, lookAtZ)
{
}

void Camera::SetPosition(Float x, Float y, Float z)
{
    m_EyePos.x = x;
    m_EyePos.y = y;
    m_EyePos.z = z;
}

void Camera::SetLookAtPos(Float x, Float y, Float z)
{
    m_LookAtPos.x = x;
    m_LookAtPos.y = y;
    m_LookAtPos.z = z;
}

Matrix4x4f Camera::GetViewMatrix() const
{
    // calculate lookAt matrix
    return MakeLookAtMatrix(m_EyePos, m_LookAtPos, m_WorldUp);
}

Matrix4x4f Camera::GetPerspectiveMatrix(Float fov, Float aspectRatio, Float n, Float f) const
{
    return MakePerspectiveMatrix(fov, aspectRatio, n, f);
}

// 0 < n < f (right-handed) --> [-1, 1] NDC (left-handed)
Matrix4x4f Camera::GetPerspectiveMatrix(Float l, Float r, Float b, Float t, Float n,
                                        Float f) const
{
    return MakePerspectiveMatrix(l, r, b, t, n, f);
}

// 0 < n < f (right-handed) --> [-1, 1] NDC (left-handed)
Matrix4x4f Camera::GetOrthographicMatrix(Float l, Float r, Float b, Float t, Float n,
                                         Float f) const
{
    return MakeOrthographicMatrix(l, r, b, t, n, f);
}