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
    Vec3f Color;

    Light(const Vec3f& color) : Color(color) { }
    virtual ~Light() = default;
};

class DirLight : public Light
{
public:
    Vec3f Direction;
    Vec3f Position;  // for shadow mapping

    explicit DirLight() : Light(Vec3f(1.f)), Direction(0, 0, -1), Position(0.f) { }

    explicit DirLight(const Vec3f& direction, const Vec3f& position,
                      const Vec3f& color = Vec3f(1.f))
        : Light(color), Position(position)
    {
        CHECK(direction != Vec3f(0.f, 0.f, 0.f));
        Direction = Normalize(direction);
    }

    explicit DirLight(Float dirX, Float dirY, Float dirZ, const Vec3f& position,
                      const Vec3f& color = Vec3f(1.f))
        : Light(color), Position(position)
    {
        Vec3f direction(dirX, dirY, dirZ);
        CHECK(direction != Vec3f(0.f, 0.f, 0.f));
        Direction = Normalize(direction);
    }
};

class PointLight : public Light
{
public:
    Vec3f Position;

    explicit PointLight() : Light(Vec3f(1.f)), Position(0.f) { }

    explicit PointLight(Float x, Float y, Float z, const Vec3f& color = Vec3f(1.f))
        : Light(color), Position(x, y, z)
    {
    }

    explicit PointLight(const Vec3f& position, const Vec3f& color = Vec3f(1.f))
        : Light(color), Position(position)
    {
    }
};

#endif  // _LIGHT_H_
