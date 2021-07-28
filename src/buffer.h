//
// Created by Junhao Wang (@Forkercat) on 2021/1/5.
//

#pragma once

#include "color.h"
#include "geometry.h"
#include "tgaimage.h"

class Buffer
{
public:
    enum InitType
    {
        Zero,
        One,
        MaxPositive,
        MinNegative
    };

    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }

protected:
    Buffer(int w, int h) : m_Width(w), m_Height(h) { }
    virtual ~Buffer() = default;
    int m_Width;
    int m_Height;
};

class Buffer1f : public Buffer
{
public:
    Buffer1f() : Buffer(0, 0) { }
    explicit Buffer1f(int w, int h, InitType type);

    Float GetValue(int x, int y) const { return m_Data[x + y * m_Width].value; }
    void  SetValue(int x, int y, Float val) { m_Data[x + y * m_Width].value = val; }

    TGAImage GenerateImage(bool inverseColor = false) const;

    // Post-Processing
    void SimpleBlurDenoised();
    void TwoPassGaussianBlurDenoised();

private:
    struct Data
    {
        Data() : value(0.f) { }
        explicit Data(float v) : value(v) { }
        Float value;
    };
    std::vector<Data> m_Data;
};

class Buffer3f : public Buffer
{
public:
    Buffer3f() : Buffer(0, 0) { }
    explicit Buffer3f(int w, int h, InitType type);

    Vector3f GetValue(int x, int y) const
    {
        int index = x + y * m_Width;
        return Vector3f(m_Data[index].value);
    }

    void SetValue(int x, int y, const Vector3f& value)
    {
        int index = x + y * m_Width;
        m_Data[index].value = value;
    }

    TGAImage GenerateImage() const;

    // Paint Background Before Rendering
    void PaintColor(const Color3& color);

    // Post-Processing
    void SimpleBlurDenoised();
    void TwoPassGaussianBlurDenoised();

private:
    struct Data
    {
        Data() : value(0.f) { }
        explicit Data(Float v) : value(v) { }
        explicit Data(Float x, Float y, Float z) : value(x, y, z) { }
        Vector3f value;
    };
    std::vector<Data> m_Data;
};
