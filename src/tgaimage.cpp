//
// Created by Junhao Wang (@Forkercat) on 2020/12/18.
// Reference: ssloy/TinyRenderer (mostly unmodified)
//

#include "tgaimage.h"

#include <math.h>
#include <spdlog/spdlog.h>
#include <string.h>
#include <time.h>

#include <fstream>

/////////////////////////////////////////////////////////////////////////////////

TGAImage::TGAImage() : data(), width(0), height(0), bytespp(0)
{
}

TGAImage::TGAImage(int w, int h, int bpp)
    : data(w * h * bpp, 0), width(w), height(h), bytespp(bpp)
{
}

TGAImage::TGAImage(const TGAImage& img)
{
    width = img.width;
    height = img.height;
    bytespp = img.bytespp;
    data = img.data;
}

TGAImage& TGAImage::operator=(const TGAImage& img)
{
    if (this != &img)
    {
        width = img.width;
        height = img.height;
        bytespp = img.bytespp;
        data = img.data;
    }
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////

bool TGAImage::ReadTgaFile(const std::string filename)
{
    std::ifstream in;
    in.open(filename, std::ios::binary);
    if (!in.is_open())
    {
        spdlog::error("can't open file: {}", filename);
        in.close();
        return false;
    }
    TGA_Header header;
    in.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!in.good())
    {
        in.close();
        spdlog::error("an error occurred while reading the header");
        return false;
    }
    width = header.width;
    height = header.height;
    bytespp = header.bitsperpixel >> 3;  // divided by 8
    if (width <= 0 || height <= 0 ||
        (bytespp != GRAYSCALE && bytespp != RGB && bytespp != RGBA))
    {
        in.close();
        spdlog::error("bad bpp (or width/height) value");
        return false;
    }

    size_t nbytes = bytespp * width * height;
    data = std::vector<std::uint8_t>(nbytes, 0);

    if (3 == header.datatypecode || 2 == header.datatypecode)
    {
        in.read(reinterpret_cast<char*>(data.data()), nbytes);
        if (!in.good())
        {
            in.close();
            spdlog::error("an error occurred while reading the data");
            return false;
        }
    }
    else if (10 == header.datatypecode || 11 == header.datatypecode)
    {
        if (!loadRleData(in))
        {
            in.close();
            spdlog::error("an error occurred while reading the data");
            return false;
        }
    }
    else
    {
        in.close();
        spdlog::error("unknown file format: {}", (int)header.datatypecode);
        return false;
    }
    if (!(header.imagedescriptor & 0x20)) FlipVertically();
    if (header.imagedescriptor & 0x10) FlipHorizontally();
    // spdlog::info("{}x{}/{}", width, height, bytespp);
    in.close();
    return true;
}

/////////////////////////////////////////////////////////////////////////////////

bool TGAImage::loadRleData(std::ifstream& in)
{
    size_t   pixelcount = width * height;
    size_t   currentpixel = 0;
    size_t   currentbyte = 0;
    TGAColor colorbuffer;
    do
    {
        std::uint8_t chunkheader = 0;
        chunkheader = in.get();
        if (!in.good())
        {
            spdlog::error("an error occurred while reading the data");
            return false;
        }
        if (chunkheader < 128)
        {
            chunkheader++;
            for (int i = 0; i < chunkheader; i++)
            {
                in.read(reinterpret_cast<char*>(colorbuffer.bgra), bytespp);
                if (!in.good())
                {
                    spdlog::error("an error occurred while reading the header");
                    return false;
                }
                for (int t = 0; t < bytespp; t++)
                    data[currentbyte++] = colorbuffer.bgra[t];
                currentpixel++;
                if (currentpixel > pixelcount)
                {
                    spdlog::error("Too many pixels read");
                    return false;
                }
            }
        }
        else
        {
            chunkheader -= 127;
            in.read(reinterpret_cast<char*>(colorbuffer.bgra), bytespp);
            if (!in.good())
            {
                spdlog::error("an error occurred while reading the header");
                return false;
            }
            for (int i = 0; i < chunkheader; i++)
            {
                for (int t = 0; t < bytespp; t++)
                    data[currentbyte++] = colorbuffer.bgra[t];
                currentpixel++;
                if (currentpixel > pixelcount)
                {
                    spdlog::error("too many pixels read");
                    return false;
                }
            }
        }
    } while (currentpixel < pixelcount);
    return true;
}

/////////////////////////////////////////////////////////////////////////////////

bool TGAImage::WriteTgaFile(const std::string filename, bool vFlip, bool rle) const
{
    std::uint8_t  developer_area_ref[4] = { 0, 0, 0, 0 };
    std::uint8_t  extension_area_ref[4] = { 0, 0, 0, 0 };
    std::uint8_t  footer[18] = { 'T', 'R', 'U', 'E', 'V', 'I', 'S', 'I', 'O',
                                'N', '-', 'X', 'F', 'I', 'L', 'E', '.', '\0' };
    std::ofstream out;
    out.open(filename, std::ios::binary);
    if (!out.is_open())
    {
        spdlog::error("can't open file: {}", filename);
        out.close();
        return false;
    }
    TGA_Header header;
    memset((void*)&header, 0, sizeof(header));
    header.bitsperpixel = bytespp << 3;
    header.width = width;
    header.height = height;
    header.datatypecode = (bytespp == GRAYSCALE ? (rle ? 11 : 3) : (rle ? 10 : 2));
    header.imagedescriptor = vFlip ? 0x00 : 0x20;  // top-left or bottom-left origin
    out.write(reinterpret_cast<char*>(&header), sizeof(header));
    if (!out.good())
    {
        out.close();
        spdlog::error("can't dump the tga file");
        return false;
    }
    if (!rle)
    {
        out.write(reinterpret_cast<const char*>(data.data()), width * height * bytespp);
        if (!out.good())
        {
            spdlog::error("can't unload raw data");
            out.close();
            return false;
        }
    }
    else
    {
        if (!unloadRleData(out))
        {
            out.close();
            spdlog::error("can't unload rle data");
            return false;
        }
    }
    out.write(reinterpret_cast<const char*>(developer_area_ref),
              sizeof(developer_area_ref));
    if (!out.good())
    {
        spdlog::error("can't dump the tga file");
        out.close();
        return false;
    }
    out.write(reinterpret_cast<const char*>(extension_area_ref),
              sizeof(extension_area_ref));
    if (!out.good())
    {
        spdlog::error("can't dump the tga file");
        out.close();
        return false;
    }
    out.write(reinterpret_cast<const char*>(footer), sizeof(footer));
    if (!out.good())
    {
        spdlog::error("can't dump the tga file");
        out.close();
        return false;
    }
    out.close();
    return true;
}

// TODO: it is not necessary to break a raw chunk for two equal pixels (for the matter of
// the resulting size)
bool TGAImage::unloadRleData(std::ofstream& out) const
{
    const std::uint8_t max_chunk_length = 128;
    size_t             npixels = width * height;
    size_t             curpix = 0;
    while (curpix < npixels)
    {
        size_t       chunkstart = curpix * bytespp;
        size_t       curbyte = curpix * bytespp;
        std::uint8_t run_length = 1;
        bool         raw = true;
        while (curpix + run_length < npixels && run_length < max_chunk_length)
        {
            bool succ_eq = true;
            for (int t = 0; succ_eq && t < bytespp; t++)
            {
                succ_eq = (data[curbyte + t] == data[curbyte + t + bytespp]);
            }
            curbyte += bytespp;
            if (1 == run_length)
            {
                raw = !succ_eq;
            }
            if (raw && succ_eq)
            {
                run_length--;
                break;
            }
            if (!raw && !succ_eq)
            {
                break;
            }
            run_length++;
        }
        curpix += run_length;
        out.put(raw ? run_length - 1 : run_length + 127);
        if (!out.good())
        {
            spdlog::error("can't dump the tga file");
            return false;
        }
        out.write(reinterpret_cast<const char*>(data.data() + chunkstart),
                  (raw ? run_length * bytespp : bytespp));
        if (!out.good())
        {
            spdlog::error("can't dump the tga file");
            return false;
        }
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////////

TGAColor TGAImage::Get(int x, int y) const
{
    if (!data.size() || x < 0 || y < 0 || x >= width || y >= height)
    {
        return TGAColor(0, 0, 0);
    }
    return TGAColor(data.data() + (x + y * width) * bytespp, bytespp);
}

void TGAImage::Set(int x, int y, const TGAColor& c)
{
    if (!data.size() || x < 0 || y < 0 || x >= width || y >= height) return;
    memcpy(data.data() + (x + y * width) * bytespp, c.bgra, bytespp);
}

int TGAImage::GetBytespp() const
{
    return bytespp;
}

int TGAImage::GetWidth() const
{
    return width;
}

int TGAImage::GetHeight() const
{
    return height;
}

void TGAImage::FlipHorizontally()
{
    if (!data.size()) return;
    int half = width >> 1;
    for (int i = 0; i < half; i++)
    {
        for (int j = 0; j < height; j++)
        {
            TGAColor c1 = Get(i, j);
            TGAColor c2 = Get(width - 1 - i, j);
            Set(i, j, c2);
            Set(width - 1 - i, j, c1);
        }
    }
}

void TGAImage::FlipVertically()
{
    if (!data.size()) return;
    size_t                    bytes_per_line = width * bytespp;
    std::vector<std::uint8_t> line(bytes_per_line, 0);
    int                       half = height >> 1;
    for (int j = 0; j < half; j++)
    {
        size_t l1 = j * bytes_per_line;
        size_t l2 = (height - 1 - j) * bytes_per_line;
        std::copy(data.begin() + l1, data.begin() + l1 + bytes_per_line, line.begin());
        std::copy(data.begin() + l2, data.begin() + l2 + bytes_per_line,
                  data.begin() + l1);
        std::copy(line.begin(), line.end(), data.begin() + l2);
    }
}

std::uint8_t* TGAImage::Buffer()
{
    return data.data();
}

void TGAImage::Clear()
{
    data = std::vector<std::uint8_t>(width * height * bytespp, 0);
}

void TGAImage::Scale(int w, int h)
{
    if (w <= 0 || h <= 0 || !data.size()) return;
    std::vector<std::uint8_t> tdata(w * h * bytespp, 0);
    int                       nscanline = 0;
    int                       oscanline = 0;
    int                       erry = 0;
    size_t                    nlinebytes = w * bytespp;
    size_t                    olinebytes = width * bytespp;
    for (int j = 0; j < height; j++)
    {
        int errx = width - w;
        int nx = -bytespp;
        int ox = -bytespp;
        for (int i = 0; i < width; i++)
        {
            ox += bytespp;
            errx += w;
            while (errx >= (int)width)
            {
                errx -= width;
                nx += bytespp;
                memcpy(tdata.data() + nscanline + nx, data.data() + oscanline + ox,
                       bytespp);
            }
        }
        erry += h;
        oscanline += olinebytes;
        while (erry >= (int)height)
        {
            if (erry >= (int)height << 1)  // it means we jump over a scanline
                memcpy(tdata.data() + nscanline + nlinebytes, tdata.data() + nscanline,
                       nlinebytes);
            erry -= height;
            nscanline += nlinebytes;
        }
    }
    data = tdata;
    width = w;
    height = h;
}