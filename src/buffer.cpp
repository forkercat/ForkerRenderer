//
// Created by Junhao Wang (@Forkercat) on 2021/1/5.
//

#include "buffer.h"

// Buffer1f
Buffer1f::Buffer1f(int w, int h, InitType type) : Buffer(w, h)
{
    if (type == Zero)
        m_Data = std::vector<Data>(w * h, Data(0.f));
    else if (type == One)
        m_Data = std::vector<Data>(w * h, Data(1.f));
    else if (type == MaxPositive)
        m_Data = std::vector<Data>(w * h, Data(MaxFloat));
    else if (type == MinNegative)
        m_Data = std::vector<Data>(w * h, Data(MinFloat));
}

TGAImage Buffer1f::GenerateImage(bool inverseColor) const
{
    TGAImage image(m_Width, m_Height, TGAImage::GRAYSCALE);
    for (int x = 0; x < m_Width; ++x)
    {
        for (int y = 0; y < m_Height; ++y)
        {
            Float val = (inverseColor) ? 1.f - GetValue(x, y) : GetValue(x, y);
            image.Set(x, y, TGAColor(val * 255));
        }
    }
    return image;
}

// Buffer3f
Buffer3f::Buffer3f(int w, int h, InitType type) : Buffer(w, h)
{
    if (type == Zero)
        m_Data = std::vector<Data>(w * h, Data(0.f));
    else if (type == One)
        m_Data = std::vector<Data>(w * h, Data(1.f));
    else if (type == MaxPositive)
        m_Data = std::vector<Data>(w * h, Data(MaxFloat));
    else if (type == MinNegative)
        m_Data = std::vector<Data>(w * h, Data(MinFloat));
}

TGAImage Buffer3f::GenerateImage() const
{
    TGAImage image(m_Width, m_Height, TGAImage::RGB);
    for (int x = 0; x < m_Width; ++x)
    {
        for (int y = 0; y < m_Height; ++y)
        {
            const Vector3f& color = GetValue(x, y);
            image.Set(x, y,
                      TGAColor(color.r * 254.99f, color.g * 254.99f, color.b * 254.99f));
        }
    }
    return image;
}

void Buffer3f::PaintColor(const Color3& color)
{
    for (int x = 0; x < m_Width; ++x)
    {
        for (int y = 0; y < m_Height; ++y)
        {
            SetValue(x, y, color);
        }
    }
}