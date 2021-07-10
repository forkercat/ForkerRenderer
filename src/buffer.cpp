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

// Post-Processing
void Buffer1f::SimpleBlurDenoised()
{
    const Float scale = 1 / 9.f;

    for (int h = 0; h < m_Height; ++h)
    {
        for (int w = 0; w < m_Width; ++w)
        {
            Float result = 0.f;

            for (int xOffset = -1; xOffset <= 1; ++xOffset)
            {
                for (int yOffset = -1; yOffset <= 1; ++yOffset)
                {
                    int newW = Clamp(w + xOffset, 0, m_Width - 1);
                    int newH = Clamp(h + yOffset, 0, m_Height - 1);
                    result += GetValue(newW, newH) * scale;
                }
            }
            SetValue(w, h, result);
        }
    }
}

void Buffer1f::TwoPassGaussianBlurDenoised()
{
    const Float weights[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };

    // Horizontal Pass
    for (int h = 0; h < m_Height; ++h)
    {
        for (int w = 0; w < m_Width; ++w)
        {
            Float result = GetValue(w, h) * weights[0];

            for (int i = 1; i < 5; ++i)
            {
                int w1 = Clamp(w + i, 0, m_Width - 1);
                int w2 = Clamp(w - i, 0, m_Width - 1);
                result += GetValue(w1, h) * weights[i];
                result += GetValue(w2, h) * weights[i];
            }
            SetValue(w, h, result);
        }
    }

    // Vertical Pass
    for (int h = 0; h < m_Height; ++h)
    {
        for (int w = 0; w < m_Width; ++w)
        {
            Float result = GetValue(w, h) * weights[0];

            for (int i = 1; i < 5; ++i)
            {
                int h1 = Clamp(h + i, 0, m_Height - 1);
                int h2 = Clamp(h - i, 0, m_Height - 1);
                result += GetValue(w, h1) * weights[i];
                result += GetValue(w, h2) * weights[i];
            }
            SetValue(w, h, result);
        }
    }
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

// Post-Processing
void Buffer3f::SimpleBlurDenoised()
{
    const Float scale = 1 / 9.f;

    for (int h = 0; h < m_Height; ++h)
    {
        for (int w = 0; w < m_Width; ++w)
        {
            Vector3f result(0.f);

            for (int xOffset = -1; xOffset <= 1; ++xOffset)
            {
                for (int yOffset = -1; yOffset <= 1; ++yOffset)
                {
                    int newW = Clamp(w + xOffset, 0, m_Width - 1);
                    int newH = Clamp(h + yOffset, 0, m_Height - 1);
                    result += GetValue(newW, newH) * scale;
                }
            }
            SetValue(w, h, result);
        }
    }
}

void Buffer3f::TwoPassGaussianBlurDenoised()
{
    const Float weights[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };

    // Horizontal Pass
    for (int h = 0; h < m_Height; ++h)
    {
        for (int w = 0; w < m_Width; ++w)
        {
            Vector3f result = GetValue(w, h) * weights[0];

            for (int i = 1; i < 5; ++i)
            {
                int w1 = Clamp(w + i, 0, m_Width - 1);
                int w2 = Clamp(w - i, 0, m_Width - 1);
                result += GetValue(w1, h) * weights[i];
                result += GetValue(w2, h) * weights[i];
            }
            SetValue(w, h, result);
        }
    }

    // Vertical Pass
    for (int h = 0; h < m_Height; ++h)
    {
        for (int w = 0; w < m_Width; ++w)
        {
            Vector3f result = GetValue(w, h) * weights[0];

            for (int i = 1; i < 5; ++i)
            {
                int h1 = Clamp(h + i, 0, m_Height - 1);
                int h2 = Clamp(h - i, 0, m_Height - 1);
                result += GetValue(w, h1) * weights[i];
                result += GetValue(w, h2) * weights[i];
            }
            SetValue(w, h, result);
        }
    }
}