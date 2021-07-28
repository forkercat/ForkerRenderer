//
// Created by Junhao Wang (@Forkercat) on 2021/1/6.
//

#pragma once

#include <spdlog/fmt/ostr.h>

#include "geometry.h"
#include "stringprint.h"
#include "texture.h"

class Material
{
public:
    std::string name;

    Material() = default;

    // clang-format off
    explicit Material(std::string name)
        : name(std::move(name)), ka(0.f), kd(0.f), ks(0.f), ke(0.f),
          diffuseMap(nullptr), specularMap(nullptr), normalMap(nullptr), emissiveMap(nullptr)
    {
    }  // clang-format on

    Vector3f ka;
    Vector3f kd;
    Vector3f ks;
    Vector3f ke;

    std::shared_ptr<Texture> diffuseMap;
    std::shared_ptr<Texture> specularMap;
    std::shared_ptr<Texture> normalMap;
    std::shared_ptr<Texture> emissiveMap;

    inline bool HasDiffuseMap() const { return diffuseMap != nullptr; }
    inline bool HasSpecularMap() const { return specularMap != nullptr; }
    inline bool HasNormalMap() const { return normalMap != nullptr; }
    inline bool HasEmissiveMap() const { return emissiveMap != nullptr; }
};

inline std::ostream& operator<<(std::ostream& out, const Material& m)
{
    out << StringPrintf(
            "map_Kd[%s] map_Ks[%s] map_Ke[%s] map_Bump[%s] | "
            "Ka(%3.2f, %3.2f, %3.2f), "
            "Kd(%3.2f, %3.2f, %3.2f), "
            "Ks(%3.2f, %3.2f, %3.2f)",
            "Ke(%3.2f, %3.2f, %3.2f)",
            m.diffuseMap ? "o" : "x",
            m.specularMap ? "o" : "x",
            m.emissiveMap ? "o" : "x",
            m.normalMap ? "o" : "x",
            (Float)m.ka.x, (Float)m.ka.y, (Float)m.ka.z,
            (Float)m.kd.x, (Float)m.kd.y, (Float)m.kd.z,
            (Float)m.ks.x, (Float)m.ks.y, (Float)m.ks.z,
            (Float)m.ke.x, (Float)m.ke.y, (Float)m.ke.z);
    return out;
}