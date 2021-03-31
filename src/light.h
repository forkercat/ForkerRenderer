//
// Created by Junhao Wang (@Forkercat) on 2021/1/4.
//

#ifndef _LIGHT_H_
#define _LIGHT_H_

#include "geometry.h"
#include "tgaimage.h"

class Light
{
public:
    Vector3f Color;

    Light(const Vector3f& color) : Color(color) { }
    virtual ~Light() = default;
};

class DirLight : public Light
{
public:
    Vector3f Direction;
    Vector3f Position;  // for shadow mapping

    explicit DirLight() : Light(Vector3f(1.f)), Direction(0, 0, -1), Position(0.f) { }

    explicit DirLight(const Vector3f& direction, const Vector3f& position,
                      const Vector3f& color = Vector3f(1.f))
        : Light(color), Position(position)
    {
        CHECK(direction != Vector3f(0.f, 0.f, 0.f));
        Direction = Normalize(direction);
    }

    explicit DirLight(Float dirX, Float dirY, Float dirZ, const Vector3f& position,
                      const Vector3f& color = Vector3f(1.f))
        : Light(color), Position(position)
    {
        Vector3f direction(dirX, dirY, dirZ);
        CHECK(direction != Vector3f(0.f, 0.f, 0.f));
        Direction = Normalize(direction);
    }
};

class PointLight : public Light
{
public:
    Vector3f Position;

    explicit PointLight() : Light(Vector3f(1.f)), Position(0.f) { }

    explicit PointLight(Float x, Float y, Float z, const Vector3f& color = Vector3f(1.f))
        : Light(color), Position(x, y, z)
    {
    }

    explicit PointLight(const Vector3f& position, const Vector3f& color = Vector3f(1.f))
        : Light(color), Position(position)
    {
    }
};

#endif  // _LIGHT_H_
