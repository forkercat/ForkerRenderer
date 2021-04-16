//
// Created by Junhao Wang (@Forkercat) on 2020/12/18.
// Reference: ssloy/TinyRenderer (mostly unmodified)
//

#ifndef _TGAIMAGE_H_
#define _TGAIMAGE_H_

#include <fstream>
#include <ostream>
#include <vector>

#include "check.h"
#include "geometry.h"

#pragma pack(push, 1)
struct TGA_Header
{
    std::uint8_t  idlength{};
    std::uint8_t  colormaptype{};
    std::uint8_t  datatypecode{};
    std::uint16_t colormaporigin{};
    std::uint16_t colormaplength{};
    std::uint8_t  colormapdepth{};
    std::uint16_t x_origin{};
    std::uint16_t y_origin{};
    std::uint16_t width{};
    std::uint16_t height{};
    std::uint8_t  bitsperpixel{};
    std::uint8_t  imagedescriptor{};
};
#pragma pack(pop)

struct TGAColor
{
    union
    {
        struct
        {
            std::uint8_t b, g, r, a;
        };
        std::uint8_t bgra[4] = { 0, 0, 0, 0 };
    };
    std::uint8_t bytespp;

    TGAColor() = default;

    TGAColor(std::uint8_t R, std::uint8_t G, std::uint8_t B)
        : bgra{ B, G, R, 0 }, bytespp(3)
    {
    }

    TGAColor(std::uint8_t R, std::uint8_t G, std::uint8_t B, std::uint8_t A)
        : bgra{ B, G, R, A }, bytespp(4)
    {
    }

    TGAColor(std::uint8_t v) : bgra{ v, 0, 0, 0 }, bytespp(1) { }

    TGAColor(const std::uint8_t* p, std::uint8_t bpp) : bgra{ 0, 0, 0, 0 }, bytespp(bpp)
    {
        for (int i = 0; i < bpp; ++i)
        {
            bgra[i] = p[i];
        }
    }

    std::uint8_t  operator[](const int i) const { return bgra[i]; }
    std::uint8_t& operator[](const int i) { return bgra[i]; }

    TGAColor& operator=(const TGAColor& c)
    {
        if (this != &c)  // not equal itself
        {
            for (int i = 0; i < 4; i++)
                bgra[i] = c.bgra[i];
            bytespp = c.bytespp;
        }
        return *this;
    }

    template <typename T>
    TGAColor operator*(const T& intensity) const
    {
        TGAColor ret = *this;
        double   clamped = Clamp01(intensity);
        for (int i = 0; i < 4; i++)
            ret.bgra[i] *= clamped;
        return ret;
    }
};

inline std::ostream& operator<<(std::ostream& out, const TGAColor& c)
{
    out << "TGAColor( " << (int)c.r << ", " << (int)c.g << ", " << (int)c.b
        << " | a: " << (int)c.a << ", bytespp: " << c.bytespp << " )";
    return out;
}

inline TGAColor operator+(const TGAColor& c1, const TGAColor& c2)
{
    CHECK(c1.bytespp == c2.bytespp);
    TGAColor ret(c1);
    ret.r = std::max(0, std::min(255, ret.r + c2.r));
    ret.g = std::max(0, std::min(255, ret.g + c2.g));
    ret.b = std::max(0, std::min(255, ret.b + c2.b));
    return ret;
}

/////////////////////////////////////////////////////////////////////////////////

class TGAImage
{
public:
    enum Format
    {
        GRAYSCALE = 1,
        RGB = 3,
        RGBA = 4
    };
    TGAImage();
    TGAImage(int w, int h, int bpp);
    TGAImage(const TGAImage& img);

    bool ReadTgaFile(const std::string filename);
    bool WriteTgaFile(const std::string filename, bool vFlip = true,
                      bool rle = true) const;

    TGAColor  Get(int x, int y) const;
    void      Set(int x, int y, const TGAColor& c);
    TGAImage& operator=(const TGAImage& img);

    void FlipHorizontally();
    void FlipVertically();
    void Scale(int w, int h);

    int           GetWidth() const;
    int           GetHeight() const;
    int           GetBytespp() const;
    std::uint8_t* Buffer();
    void          Clear();

protected:
    std::vector<std::uint8_t> m_Data;
    int                       m_Width;
    int                       m_Height;
    int                       m_Bytespp;

    bool loadRleData(std::ifstream& in);
    bool unloadRleData(std::ofstream& out) const;
};

#endif  // _TGAIMAGE_H_
