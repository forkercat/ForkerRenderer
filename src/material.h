//
// Created by Junhao Wang (@Forkercat) on 2021/1/6.
//

#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include <spdlog/fmt/ostr.h>

#include <iostream>
#include <string>

#include "geometry.h"
#include "tgaimage.h"

class Material
{
public:
    std::string Name;

    Material() = default;

    // clang-format off
    explicit  Material(const std::string& name)
        : Name(name), Ka(0.f), Kd(0.f), Ks(0.f), Ke(0.f),
          DiffuseMap(), SpecularMap(), NormalMap(), AmbientOcclusionMap()
    {
    }  // clang-format on

    Vector3f Ka;
    Vector3f Kd;
    Vector3f Ks;
    Vector3f Ke;

    TGAImage DiffuseMap;
    TGAImage SpecularMap;
    TGAImage NormalMap;
    TGAImage AmbientOcclusionMap;
};

inline std::ostream& operator<<(std::ostream& out, const Material& m)
{
    char buffer[1024];
    // clang-format off
    sprintf(buffer,
            "map_Kd[%s] map_Ks[%s] map_Bump[%s] map_Ao[%s] | "
            "Ka(%3.2f, %3.2f, %3.2f), "
            "Kd(%3.2f, %3.2f, %3.2f), "
            "Ks(%3.2f, %3.2f, %3.2f)",
            (m.DiffuseMap.GetWidth() != 0) ? "o" : "x",
            (m.SpecularMap.GetWidth() != 0) ? "o" : "x",
            (m.NormalMap.GetWidth() != 0) ? "o" : "x",
            (m.AmbientOcclusionMap.GetWidth() != 0) ? "o" : "x",
            (Float)m.Ka.x, (Float)m.Ka.y, (Float)m.Ka.z,
            (Float)m.Kd.x, (Float)m.Kd.y, (Float)m.Kd.z,
            (Float)m.Ks.x, (Float)m.Ks.y, (Float)m.Ks.z);
    // clang-format on
    out << buffer;
    return out;
}

#endif  // _MATERIAL_H_
