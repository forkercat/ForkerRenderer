//
// Created by Junhao Wang (@Forkercat) on 2020/12/26.
//

#include "camera.h"

Camera::Camera(const Point3f& lookFrom, const Point3f& lookAt)
    : eyePos(lookFrom), worldUp(WORLD_UP), lookAtPos(lookAt)
{
}

Camera::Camera(Float lookFromX, Float lookFromY, Float lookFromZ, Float lookAtX,
               Float lookAtY, Float lookAtZ)
    : eyePos(lookFromX, lookFromY, lookFromZ),
      worldUp(WORLD_UP),
      lookAtPos(lookAtX, lookAtY, lookAtZ)
{
}

void Camera::SetPosition(Float x, Float y, Float z)
{
    eyePos.x = x;
    eyePos.y = y;
    eyePos.z = z;
}

void Camera::SetLookAtPos(Float x, Float y, Float z)
{
    lookAtPos.x = x;
    lookAtPos.y = y;
    lookAtPos.z = z;
}

Matrix4x4f Camera::GetViewMatrix()
{
    // calculate lookAt matrix
    return MakeLookAtMatrix(eyePos, lookAtPos, worldUp);
}

Matrix4x4f Camera::GetPerspectiveMatrix(Float fov, Float aspectRatio, Float n, Float f)
{
    return MakePerspectiveMatrix(fov, aspectRatio, n, f);
}

// 0 < n < f (right-handed) --> [-1, 1] NDC (left-handed)
Matrix4x4f Camera::GetPerspectiveMatrix(Float l, Float r, Float b, Float t, Float n,
                                        Float f)
{
    return MakePerspectiveMatrix(l, r, b, t, n, f);
}

// 0 < n < f (right-handed) --> [-1, 1] NDC (left-handed)
Matrix4x4f Camera::GetOrthographicMatrix(Float l, Float r, Float b, Float t, Float n,
                                         Float f)
{
    return MakeOrthographicMatrix(l, r, b, t, n, f);
}