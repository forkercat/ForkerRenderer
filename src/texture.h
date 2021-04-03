//
// Created by Junhao Wang (@Forkercat) on 2021/4/3.
//

#ifndef SRC_TEXTURE_H_
#define SRC_TEXTURE_H_

#include "color.h"
#include "geometry.h"
#include "tgaimage.h"

class Texture
{
public:
    Texture(const TGAImage& img)
        : width(img.GetWidth()), height(img.GetHeight()), image(img)
    {
    }

    inline int GetWidth() const { return width; }
    inline int GetHeight() const { return height; }

    inline Color3 Sample(const Vector2f& coord) const
    {
        int w = width - 1, h = height - 1;
        const TGAColor& color = image.Get(coord.s * w, coord.t * h);
        return Color3(color.r / 255.f, color.g / 255.f, color.b / 255.f);
    }

    inline Float SampleFloat(const Vector2f& coord) const
    {
        int w = width - 1, h = height - 1;
        const TGAColor& color = image.Get(coord.s * w, coord.t * h);
        return color[0] / 255.f;
    }

private:
    // Private Data
    int      width;
    int      height;
    TGAImage image;
};

#endif  // SRC_TEXTURE_H_
