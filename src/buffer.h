//
// Created by Junhao Wang (@Forkercat) on 2021/1/5.
//

#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <vector>

#include "geometry.h"
#include "tgaimage.h"

class Buffer
{
public:
    enum InitType
    {
        Zero,
        MaxFloat32,
        MinFloat32,
        MaxUInt8,
        MinUInt8
    };
    Buffer() = default;
    Buffer(int w, int h, InitType type);

    int GetWidth() const { return width; }
    int GetHeight() const { return height; }

    // Getters & Setters
    Vec3i Get(int x, int y) const
    {
        // For now, it ignores the fourth component
        int index = x + y * width;
        return Vec3i(data[index].r, data[index].g, data[index].b);
    }

    void Set(int x, int y, const Vec3i& val)
    {
        // For now, it ignores the fourth component
        int index = x + y * width;
        data[index].r = val.x;
        data[index].g = val.y;
        data[index].b = val.z;
    }

    float GetValue(int x, int y) const { return data[x + y * width].val; }
    void  SetValue(int x, int y, float val) { data[x + y * width].val = val; }

    // Generate TGAImage For Output Purpose
    TGAImage GenerateImage() const;
    TGAImage GenerateGrayImage(bool inverseColor = false) const;

    // Paint Background Before Rendering
    void PaintColor(const TGAColor& color);

private:
    union Data
    {
        explicit Data() : r(0), g(0), b(0), a(0) { }
        explicit Data(uint8_t vv) : r(vv), g(vv), b(vv), a(vv) { }
        explicit Data(float vv) : val(vv) { }
        struct
        {
            std::uint8_t r, g, b, a;
        };
        float val;
    };
    std::vector<Data> data;
    int               width;
    int               height;
};

#endif  // _BUFFER_H_