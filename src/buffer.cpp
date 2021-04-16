//
// Created by Junhao Wang (@Forkercat) on 2021/1/5.
//

#include "buffer.h"

Buffer::Buffer(int w, int h, InitType type) : m_Width(w), m_Height(h)
{
    if (type == Zero)
        m_Data = std::vector<Data>(w * h, Data());
    else if (type == MaxFloat32)
        m_Data = std::vector<Data>(w * h, Data(std::numeric_limits<float>::max()));
    else if (type == MinFloat32)
        m_Data = std::vector<Data>(w * h, Data(std::numeric_limits<float>::min()));
    else if (type == MaxUInt8)
        m_Data = std::vector<Data>(w * h, Data(std::numeric_limits<uint8_t>::max()));
    else if (type == MinUInt8)
        m_Data = std::vector<Data>(w * h, Data(std::numeric_limits<uint8_t>::min()));
}

TGAImage Buffer::GenerateImage() const
{
    TGAImage image(m_Width, m_Height, TGAImage::RGB);
    for (int x = 0; x < m_Width; ++x)
    {
        for (int y = 0; y < m_Height; ++y)
        {
            const Vector3i& color = Get(x, y);
            image.Set(x, y, TGAColor(color.r, color.g, color.b));
        }
    }
    return image;
}

TGAImage Buffer::GenerateGrayImage(bool inverseColor) const
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

void Buffer::PaintColor(const Color3& color)
{

    for (int i = 0; i < m_Width; ++i)
        for (int j = 0; j < m_Height; ++j)
            Set(i, j, color);
}