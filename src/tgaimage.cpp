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

TGAImage::TGAImage() : m_Data(), m_Width(0), m_Height(0), m_Bytespp(0)
{
}

TGAImage::TGAImage(int w, int h, int bpp)
    : m_Data(w * h * bpp, 0), m_Width(w), m_Height(h), m_Bytespp(bpp)
{
}

TGAImage::TGAImage(const TGAImage& img)
{
    m_Width = img.m_Width;
    m_Height = img.m_Height;
    m_Bytespp = img.m_Bytespp;
    m_Data = img.m_Data;
}

TGAImage& TGAImage::operator=(const TGAImage& img)
{
    if (this != &img)
    {
        m_Width = img.m_Width;
        m_Height = img.m_Height;
        m_Bytespp = img.m_Bytespp;
        m_Data = img.m_Data;
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
    m_Width = header.width;
    m_Height = header.height;
    m_Bytespp = header.bitsperpixel >> 3;  // divided by 8
    if (m_Width <= 0 || m_Height <= 0 ||
        (m_Bytespp != GRAYSCALE && m_Bytespp != RGB && m_Bytespp != RGBA))
    {
        in.close();
        spdlog::error("bad bpp (or width/height) value");
        return false;
    }

    size_t nbytes = m_Bytespp * m_Width * m_Height;
    m_Data = std::vector<std::uint8_t>(nbytes, 0);

    if (3 == header.datatypecode || 2 == header.datatypecode)
    {
        in.read(reinterpret_cast<char*>(m_Data.data()), nbytes);
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
    size_t   pixelcount = m_Width * m_Height;
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
                in.read(reinterpret_cast<char*>(colorbuffer.bgra), m_Bytespp);
                if (!in.good())
                {
                    spdlog::error("an error occurred while reading the header");
                    return false;
                }
                for (int t = 0; t < m_Bytespp; t++)
                    m_Data[currentbyte++] = colorbuffer.bgra[t];
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
            in.read(reinterpret_cast<char*>(colorbuffer.bgra), m_Bytespp);
            if (!in.good())
            {
                spdlog::error("an error occurred while reading the header");
                return false;
            }
            for (int i = 0; i < chunkheader; i++)
            {
                for (int t = 0; t < m_Bytespp; t++)
                    m_Data[currentbyte++] = colorbuffer.bgra[t];
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
    spdlog::info("Output TGA File: {}", filename);
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
    header.bitsperpixel = m_Bytespp << 3;
    header.width = m_Width;
    header.height = m_Height;
    header.datatypecode = (m_Bytespp == GRAYSCALE ? (rle ? 11 : 3) : (rle ? 10 : 2));
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
        out.write(reinterpret_cast<const char*>(m_Data.data()),
                  m_Width * m_Height * m_Bytespp);
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
    size_t             npixels = m_Width * m_Height;
    size_t             curpix = 0;
    while (curpix < npixels)
    {
        size_t       chunkstart = curpix * m_Bytespp;
        size_t       curbyte = curpix * m_Bytespp;
        std::uint8_t run_length = 1;
        bool         raw = true;
        while (curpix + run_length < npixels && run_length < max_chunk_length)
        {
            bool succ_eq = true;
            for (int t = 0; succ_eq && t < m_Bytespp; t++)
            {
                succ_eq = (m_Data[curbyte + t] == m_Data[curbyte + t + m_Bytespp]);
            }
            curbyte += m_Bytespp;
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
        out.write(reinterpret_cast<const char*>(m_Data.data() + chunkstart),
                  (raw ? run_length * m_Bytespp : m_Bytespp));
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
    if (!m_Data.size() || x < 0 || y < 0 || x >= m_Width || y >= m_Height)
    {
        return TGAColor(0, 0, 0);
    }
    return TGAColor(m_Data.data() + (x + y * m_Width) * m_Bytespp, m_Bytespp);
}

void TGAImage::Set(int x, int y, const TGAColor& c)
{
    if (!m_Data.size() || x < 0 || y < 0 || x >= m_Width || y >= m_Height) return;
    memcpy(m_Data.data() + (x + y * m_Width) * m_Bytespp, c.bgra, m_Bytespp);
}

int TGAImage::GetBytespp() const
{
    return m_Bytespp;
}

int TGAImage::GetWidth() const
{
    return m_Width;
}

int TGAImage::GetHeight() const
{
    return m_Height;
}

void TGAImage::FlipHorizontally()
{
    if (!m_Data.size()) return;
    int half = m_Width >> 1;
    for (int i = 0; i < half; i++)
    {
        for (int j = 0; j < m_Height; j++)
        {
            TGAColor c1 = Get(i, j);
            TGAColor c2 = Get(m_Width - 1 - i, j);
            Set(i, j, c2);
            Set(m_Width - 1 - i, j, c1);
        }
    }
}

void TGAImage::FlipVertically()
{
    if (!m_Data.size()) return;
    size_t                    bytes_per_line = m_Width * m_Bytespp;
    std::vector<std::uint8_t> line(bytes_per_line, 0);
    int                       half = m_Height >> 1;
    for (int j = 0; j < half; j++)
    {
        size_t l1 = j * bytes_per_line;
        size_t l2 = (m_Height - 1 - j) * bytes_per_line;
        std::copy(m_Data.begin() + l1, m_Data.begin() + l1 + bytes_per_line, line.begin());
        std::copy(m_Data.begin() + l2, m_Data.begin() + l2 + bytes_per_line,
                  m_Data.begin() + l1);
        std::copy(line.begin(), line.end(), m_Data.begin() + l2);
    }
}

std::uint8_t* TGAImage::Buffer()
{
    return m_Data.data();
}

void TGAImage::Clear()
{
    m_Data = std::vector<std::uint8_t>(m_Width * m_Height * m_Bytespp, 0);
}

void TGAImage::Scale(int w, int h)
{
    if (w <= 0 || h <= 0 || !m_Data.size()) return;
    std::vector<std::uint8_t> tdata(w * h * m_Bytespp, 0);
    int                       nscanline = 0;
    int                       oscanline = 0;
    int                       erry = 0;
    size_t                    nlinebytes = w * m_Bytespp;
    size_t                    olinebytes = m_Width * m_Bytespp;
    for (int j = 0; j < m_Height; j++)
    {
        int errx = m_Width - w;
        int nx = -m_Bytespp;
        int ox = -m_Bytespp;
        for (int i = 0; i < m_Width; i++)
        {
            ox += m_Bytespp;
            errx += w;
            while (errx >= (int)m_Width)
            {
                errx -= m_Width;
                nx += m_Bytespp;
                memcpy(tdata.data() + nscanline + nx, m_Data.data() + oscanline + ox,
                       m_Bytespp);
            }
        }
        erry += h;
        oscanline += olinebytes;
        while (erry >= (int)m_Height)
        {
            if (erry >= (int)m_Height << 1)  // it means we jump over a scanline
                memcpy(tdata.data() + nscanline + nlinebytes, tdata.data() + nscanline,
                       nlinebytes);
            erry -= m_Height;
            nscanline += nlinebytes;
        }
    }
    m_Data = tdata;
    m_Width = w;
    m_Height = h;
}