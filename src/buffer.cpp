//
// Created by Junhao Wang (@Forkercat) on 2021/1/5.
//

#include "buffer.h"

Buffer::Buffer(int w, int h, InitType type) : width(w), height(h)
{
    if (type == Zero)
        data = std::vector<Data>(w * h, Data());
    else if (type == MaxFloat32)
        data = std::vector<Data>(w * h, Data(std::numeric_limits<float>::max()));
    else if (type == MinFloat32)
        data = std::vector<Data>(w * h, Data(std::numeric_limits<float>::min()));
    else if (type == MaxUInt8)
        data = std::vector<Data>(w * h, Data(std::numeric_limits<uint8_t>::max()));
    else if (type == MinUInt8)
        data = std::vector<Data>(w * h, Data(std::numeric_limits<uint8_t>::min()));
}

TGAImage Buffer::GenerateImage() const
{
    TGAImage image(width, height, TGAImage::RGB);
    for (int x = 0; x < width; ++x)
    {
        for (int y = 0; y < height; ++y)
        {
            const Vec3i& color = Get(x, y);
            image.Set(x, y, TGAColor(color.r, color.g, color.b));
        }
    }
    return image;
}

TGAImage Buffer::GenerateGrayImage(bool inverseColor) const
{
    TGAImage image(width, height, TGAImage::GRAYSCALE);
    for (int x = 0; x < width; ++x)
    {
        for (int y = 0; y < height; ++y)
        {
            Float val = (inverseColor) ? 1.f - GetValue(x, y) : GetValue(x, y);
            image.Set(x, y, TGAColor(val * 255));
        }
    }
    return image;
}

void Buffer::PaintColor(const TGAColor& color)
{
    for (int i = 0; i < width; ++i)
        for (int j = 0; j < height; ++j)
            Set(i, j, Vec3i(color.r, color.g, color.b));
}