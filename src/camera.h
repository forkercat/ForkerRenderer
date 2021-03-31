//
// Created by Junhao Wang (@Forkercat) on 2020/12/26.
//

#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "geometry.h"

const Vector3f WORLD_UP(0.f, 1.f, 0.f);
const Vector3f LOOK_AT_POS(0.f, 0.f, 0.f);

class Camera
{
public:
    enum ProjectionType
    {
        Orthographic,
        Perspective
    };

    explicit Camera(const Vector3f& position, const Vector3f& lookAtPos = LOOK_AT_POS);
    explicit Camera(Float xPos, Float yPos, Float zPos, Float lookAtX = LOOK_AT_POS.x,
           Float lookAtY = LOOK_AT_POS.y, Float lookAtZ = LOOK_AT_POS.z);

    void SetPosition(Float x, Float y, Float z);
    void SetLookAtPos(Float x, Float y, Float z);

    Matrix4f GetViewMatrix();
    Matrix4f GetPerspectiveMatrix(Float fov, Float aspectRatio, Float n, Float f);
    Matrix4f GetPerspectiveMatrix(Float l, Float r, Float b, Float t, Float n, Float f);
    Matrix4f GetOrthographicMatrix(Float l, Float r, Float b, Float t, Float n, Float f);

private:
    // camera attributes
    Vector3f position;
    Vector3f worldUp;
    Vector3f lookAtPos;
};

#endif  // MYSOFTWARERENDERER__CAMERA_H_
