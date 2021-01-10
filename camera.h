//
// Created by Junhao Wang (@Forkercat) on 2020/12/26.
//

#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "geometry.h"

const Vec3f WORLD_UP(0.f, 1.f, 0.f);
const Vec3f LOOK_AT_POS(0.f, 0.f, 0.f);

class Camera
{
public:
    enum ProjectionType
    {
        Orthographic,
        Perspective
    };

    explicit Camera(const Vec3f& position, const Vec3f& lookAtPos = LOOK_AT_POS);
    explicit Camera(Float xPos, Float yPos, Float zPos, Float lookAtX = LOOK_AT_POS.x,
           Float lookAtY = LOOK_AT_POS.y, Float lookAtZ = LOOK_AT_POS.z);

    void SetPosition(Float x, Float y, Float z);
    void SetLookAtPos(Float x, Float y, Float z);

    Mat4f GetViewMatrix();
    Mat4f GetPerspectiveMatrix(Float fov, Float aspectRatio, Float n, Float f);
    Mat4f GetPerspectiveMatrix(Float l, Float r, Float b, Float t, Float n, Float f);
    Mat4f GetOrthographicMatrix(Float l, Float r, Float b, Float t, Float n, Float f);

private:
    // camera attributes
    Vec3f position;
    Vec3f worldUp;
    Vec3f lookAtPos;
};

#endif  // MYSOFTWARERENDERER__CAMERA_H_
