//
// Created by Junhao Wang (@Forkercat) on 2020/12/26.
//

#include "camera.h"

Camera::Camera(const Vector3f& position, const Vector3f& lookAtPos)
    : position(position), worldUp(WORLD_UP), lookAtPos(lookAtPos)
{
}

Camera::Camera(Float xPos, Float yPos, Float zPos, Float lookAtX, Float lookAtY,
               Float lookAtZ)
    : position(xPos, yPos, zPos), worldUp(WORLD_UP), lookAtPos(lookAtX, lookAtY, lookAtZ)
{
}

void Camera::SetPosition(Float x, Float y, Float z)
{
    position.x = x;
    position.y = y;
    position.z = z;
}

void Camera::SetLookAtPos(Float x, Float y, Float z)
{
    lookAtPos.x = x;
    lookAtPos.y = y;
    lookAtPos.z = z;
}

Matrix4f Camera::GetViewMatrix()
{
    // calculate lookAt matrix
    return MakeLookAtMatrix(position, lookAtPos, worldUp);
}

Matrix4f Camera::GetPerspectiveMatrix(Float fov, Float aspectRatio, Float n, Float f)
{
    return MakePerspectiveMatrix(fov, aspectRatio, n, f);
}

// 0 < n < f (right-handed) --> [-1, 1] NDC (left-handed)
Matrix4f Camera::GetPerspectiveMatrix(Float l, Float r, Float b, Float t, Float n, Float f)
{
    return MakePerspectiveMatrix(l, r, b, t, n, f);
}

// 0 < n < f (right-handed) --> [-1, 1] NDC (left-handed)
Matrix4f Camera::GetOrthographicMatrix(Float l, Float r, Float b, Float t, Float n, Float f)
{
    return MakeOrthographicMatrix(l, r, b, t, n, f);
}