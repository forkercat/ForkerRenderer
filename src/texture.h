//
// Created by Junhao Wang (@Forkercat) on 2021/4/3.
//

#ifndef SRC_TEXTURE_H_
#define SRC_TEXTURE_H_

#include <cmath>

#include "color.h"
#include "geometry.h"
#include "tgaimage.h"

class Texture
{
public:
    enum WrapMode
    {
        NoWrap,
        Repeat,
        MirroredRepeat,
        ClampToEdge
    };

    enum FilterMode
    {
        Nearest,
        Linear  // Bilinear
    };

    Texture(const TGAImage& img, WrapMode wrap = WrapMode::NoWrap,
            FilterMode filter = FilterMode::Nearest)
        : m_Width(img.GetWidth()),
          m_Height(img.GetHeight()),
          m_Image(img),
          m_WrapMode(wrap),
          m_FilterMode(filter)
    {
    }

    inline int GetWidth() const { return m_Width; }
    inline int GetHeight() const { return m_Height; }

    inline Color3 Sample(const Vector2f& coord) const
    {
        Vector2f wrapUV = wrapCoord(coord);
        return colorFromFiltering(wrapUV) / 255.f;
    }

    inline Float SampleFloat(const Vector2f& coord) const
    {
        Vector2f wrapUV = wrapCoord(coord);
        return colorFromFiltering(wrapUV)[2] / 255.f;
    }

private:
    // Private Data
    int        m_Width;
    int        m_Height;
    TGAImage   m_Image;
    WrapMode   m_WrapMode;
    FilterMode m_FilterMode;

    // Texture Wrapping
    inline Vector2f wrapCoord(const Vector2f& coord) const
    {
        if (m_WrapMode == WrapMode::Repeat)
        {
            return Vector2f(coord.x - std::floor(coord.x), coord.y - std::floor(coord.y));
        }
        else if (m_WrapMode == WrapMode::MirroredRepeat)
        {
            int      xi = std::floor(coord.x), yi = std::floor(coord.y);
            Vector2f repeat = Vector2f(coord.x - xi, coord.y - yi);
            return Vector2f(xi % 2 == 0 ? repeat.x : 1.f - repeat.x,
                            yi % 2 == 0 ? repeat.y : 1.f - repeat.y);
        }
        else if (m_WrapMode == WrapMode::ClampToEdge)
        {
            return Clamp01(coord);
        }
        else  // No Wrap (Default)
        {
            return coord;  // TGAImage::Get will return a specific color
        }
    }

    // Texture Filtering
    inline Color3 colorFromFiltering(const Vector2f& coord) const
    {
        Float w = m_Width - 0.001, h = m_Height - 0.001;
        if (m_FilterMode == FilterMode::Linear)
        {
            // Point in image space (not truncated)
            Vector2f p = Vector2f(coord.u * w, coord.v * h);

            // Find the top-left pixel in the sample region
            Vector2f topLeft = Vector2f(std::floor(p.x - 0.5f), std::floor(p.y - 0.5f));

            // Bilinear Interpolation
            Float tx = p.x - (topLeft.x + 0.5f);  // sample point is at center (+ 0.5f)
            Float ty = p.y - (topLeft.y + 0.5f);

            // Sample Points
            Vector2i s0 = Vector2i(topLeft.x, topLeft.y);
            Vector2i s1 = Vector2i(topLeft.x + 1.f, topLeft.y);
            Vector2i s2 = Vector2i(topLeft.x, topLeft.y + 1.f);
            Vector2i s3 = Vector2i(topLeft.x + 1.f, topLeft.y + 1.f);

            if (m_WrapMode != WrapMode::NoWrap)
            {
                // For modes except NoWrap, we need to manually clamp the image
                // coordinate. For NoWrap, don't clamp it to retrieve black color when out
                // of image region.
                s0 = clampImageCoord(s0);
                s1 = clampImageCoord(s1);
                s2 = clampImageCoord(s2);
                s3 = clampImageCoord(s3);
            }

            Color3 c0 = getColorFromImage(s0);
            Color3 c1 = getColorFromImage(s1);
            Color3 c2 = getColorFromImage(s2);
            Color3 c3 = getColorFromImage(s3);

            Color3 cx1 = Lerp(tx, c0, c1);  // Horizontal Interpolation
            Color3 cx2 = Lerp(tx, c2, c3);
            return Lerp(ty, cx1, cx2);  // Vertical Interpolation
        }
        else  // Nearest (Default)
        {
            Vector2i imageUV = Vector2i(std::floor(coord.u * w), std::floor(coord.v * h));
            return getColorFromImage(imageUV);
        }
    }

    // Return Color3 [0, 255]
    inline Color3 getColorFromImage(const Vector2i& imageUV) const
    {
        const TGAColor& color = m_Image.Get(imageUV.u, imageUV.v);
        return Color3(color.r, color.g, color.b);
    }

    inline Vector2i clampImageCoord(const Vector2i& imageUV) const
    {
        return Vector2i(Clamp(imageUV.u, 0, m_Image.GetWidth() - 1),
                        Clamp(imageUV.v, 0, m_Image.GetHeight() - 1));
    }
};

#endif  // SRC_TEXTURE_H_
