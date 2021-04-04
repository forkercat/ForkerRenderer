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
        NoFilter,
        Nearest,
        Linear
    };

    Texture(const TGAImage& img, WrapMode wrap = WrapMode::Repeat,
            FilterMode filter = FilterMode::NoFilter)
        : width(img.GetWidth()),
          height(img.GetHeight()),
          image(img),
          wrapMode(wrap),
          filterMode(filter)
    {
    }

    inline int GetWidth() const { return width; }
    inline int GetHeight() const { return height; }

    inline Color3 Sample(const Vector2f& coord) const
    {
        Vector2f        wrap = wrapCoord(coord);
        int             w = width - 1, h = height - 1;
        const TGAColor& color = image.Get(wrap.s * w, wrap.t * h);
        return Color3(color.r / 255.f, color.g / 255.f, color.b / 255.f);
    }

    inline Float SampleFloat(const Vector2f& coord) const
    {
        Vector2f        wrap = wrapCoord(coord);
        int             w = width - 1, h = height - 1;
        const TGAColor& color = image.Get(wrap.s * w, wrap.t * h);
        return color[0] / 255.f;
    }

private:
    // Private Data
    int        width;
    int        height;
    TGAImage   image;
    WrapMode   wrapMode;
    FilterMode filterMode;

    // Process wrapping
    inline Vector2f wrapCoord(const Vector2f& coord) const
    {
        if (wrapMode == WrapMode::NoWrap)
        {
            return coord;  // TGAImage::Get will return a specific color
        }
        else if (wrapMode == WrapMode::Repeat)
        {
            return Vector2f(coord.x - std::floor(coord.x), coord.y - std::floor(coord.y));
        }
        else if (wrapMode == WrapMode::MirroredRepeat)
        {
            int      xi = std::floor(coord.x), yi = std::floor(coord.y);
            Vector2f repeat = Vector2f(coord.x - xi, coord.y - yi);
            return Vector2f(xi % 2 == 0 ? repeat.x : 1.f - repeat.x,
                            yi % 2 == 0 ? repeat.y : 1.f - repeat.y);
        }
        else if (wrapMode == WrapMode::ClampToEdge)
        {
            return Clamp01(coord);
        }
        else
        {
            return coord;
        }
    }
};

#endif  // SRC_TEXTURE_H_
