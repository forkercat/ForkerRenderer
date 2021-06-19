//
// Created by Junhao Wang (@Forkercat) on 2021/1/5.
//

#pragma once

#include <vector>

#include "geometry.h"
#include "tgaimage.h"
#include "color.h"

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

    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }

    // Getters & Setters
    Vector3i Get(int x, int y) const
    {
        // For now, it ignores the fourth component
        int index = x + y * m_Width;
        return Vector3i(m_Data[index].r, m_Data[index].g, m_Data[index].b);
    }

    void Set(int x, int y, const Color3& val)
    {
        // For now, it ignores the fourth component
        int index = x + y * m_Width;
        m_Data[index].r = val.x * 254.999;
        m_Data[index].g = val.y * 254.999;
        m_Data[index].b = val.z * 254.999;
    }

    float GetValue(int x, int y) const { return m_Data[x + y * m_Width].val; }
    void  SetValue(int x, int y, float val) { m_Data[x + y * m_Width].val = val; }

    // Generate TGAImage For Output Purpose
    TGAImage GenerateImage() const;
    TGAImage GenerateGrayImage(bool inverseColor = false) const;

    // Paint Background Before Rendering
    void PaintColor(const Color3& color);

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
    std::vector<Data> m_Data;
    int               m_Width;
    int               m_Height;
};
