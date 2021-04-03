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
    std::string name;

    Material() = default;

    // clang-format off
    explicit  Material(const std::string& name)
        : name(name), ka(0.f), kd(0.f), ks(0.f), ke(0.f),
          diffuseMap(), specularMap(), normalMap(), ambientOcclusionMap(),
          hasDiffuseMap(false), hasSpecularMap(false), hasNormalMap(false),
          hasAmbientOcclusionMap(false)
    {
    }  // clang-format on

    Vector3f ka;
    Vector3f kd;
    Vector3f ks;
    Vector3f ke;

    TGAImage diffuseMap;
    TGAImage specularMap;
    TGAImage normalMap;
    TGAImage ambientOcclusionMap;

    bool hasDiffuseMap;
    bool hasSpecularMap;
    bool hasNormalMap;
    bool hasAmbientOcclusionMap;
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
            (m.diffuseMap.GetWidth() != 0) ? "o" : "x",
            (m.specularMap.GetWidth() != 0) ? "o" : "x",
            (m.normalMap.GetWidth() != 0) ? "o" : "x",
            (m.ambientOcclusionMap.GetWidth() != 0) ? "o" : "x",
            (Float)m.ka.x, (Float)m.ka.y, (Float)m.ka.z,
            (Float)m.kd.x, (Float)m.kd.y, (Float)m.kd.z,
            (Float)m.ks.x, (Float)m.ks.y, (Float)m.ks.z);
    // clang-format on
    out << buffer;
    return out;
}

#endif  // _MATERIAL_H_
