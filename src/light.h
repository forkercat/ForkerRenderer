//
// Created by Junhao Wang (@Forkercat) on 2021/1/4.
//

#ifndef LIGHT_H_
#define LIGHT_H_

#include "geometry.h"
#include "tgaimage.h"

class Light
{
public:
    Color3 color;

    Light(const Color3& c) : color(c) { }
    virtual ~Light() = default;
};

class DirLight : public Light
{
public:
    Point3f  position;  // for shadow mapping
    Vector3f direction;

    explicit DirLight() : Light(Color3(1.f)), direction(0, 0, -1), position(0.f) { }

    explicit DirLight(const Vector3f& dir, const Point3f& pos,
                      const Color3& c = Color3(1.f))
        : Light(c), position(pos)
    {
        CHECK(dir != Vector3f(0.f, 0.f, 0.f));
        direction = Normalize(dir);
    }

    explicit DirLight(Float dirX, Float dirY, Float dirZ, const Point3f& position,
                      const Color3& color = Vector3f(1.f))
        : Light(color), position(position)
    {
        Vector3f dir(dirX, dirY, dirZ);
        CHECK(dir != Vector3f(0.f, 0.f, 0.f));
        direction = Normalize(dir);
    }
};

class PointLight : public Light
{
public:
    Vector3f position;

    explicit PointLight() : Light(Vector3f(1.f)), position(0.f) { }

    explicit PointLight(Float x, Float y, Float z, const Vector3f& c = Vector3f(1.f))
        : Light(c), position(x, y, z)
    {
    }

    explicit PointLight(const Vector3f& pos, const Vector3f& c = Vector3f(1.f))
        : Light(c), position(pos)
    {
    }
};

#endif  // LIGHT_H_
