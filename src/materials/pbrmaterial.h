//
// Created by Junhao Wang (@Forkercat) on 2021/5/5.
//

#pragma once

#include <spdlog/fmt/ostr.h>

#include "geometry.h"
#include "stringprint.h"
#include "texture.h"

class PBRMaterial
{
public:
    std::string name;

    PBRMaterial() = default;

    // clang-format off
    explicit PBRMaterial(std::string name)
        : name(std::move(name)), ka(0.f), ke(0.f), albedo(1.f), roughness(0.f), metalness(0.f),
          baseColorMap(nullptr), roughnessMap(nullptr), metalnessMap(nullptr),
          ambientOcclusionMap(nullptr), emissiveMap(nullptr)
    { }
    // clang-format on
    Vector3f ka;
    Vector3f ke;
    Vector3f albedo;
    Float    roughness;
    Float    metalness;

    std::shared_ptr<Texture> baseColorMap;
    std::shared_ptr<Texture> roughnessMap;
    std::shared_ptr<Texture> metalnessMap;
    std::shared_ptr<Texture> ambientOcclusionMap;
    std::shared_ptr<Texture> normalMap;
    std::shared_ptr<Texture> emissiveMap;

    inline bool HasBaseColorMap() const { return baseColorMap != nullptr; }
    inline bool HasRoughnessMap() const { return roughnessMap != nullptr; }
    inline bool HasMetalnessMap() const { return metalnessMap != nullptr; }
    inline bool HasAmbientOcclusionMap() const { return ambientOcclusionMap != nullptr; }
    inline bool HasNormalMap() const { return normalMap != nullptr; }
    inline bool HasEmssiveMap() const { return emissiveMap != nullptr; }
};

inline std::ostream& operator<<(std::ostream& out, const PBRMaterial& m)
{
    out << StringPrintf(
        "map_Kd[%s] map_Pr[%s] map_Pm[%s] map_Ao[%s] map_Bump[%s] map_Ke[%s] | "
        "Ka(%3.2f, %3.2f, %3.2f) Ke(%3.2f, %3.2f, %3.2f) Albedo(%3.2f, %3.2f, %3.2f) Roughness(%3.2f) Metalness(%3.2f)",
        m.baseColorMap ? "o" : "x", m.roughnessMap ? "o" : "x",
        m.metalnessMap ? "o" : "x", m.ambientOcclusionMap ? "o" : "x",
        m.normalMap ? "o" : "x", m.emissiveMap ? "o" : "x",
        (Float)m.ka.x, (Float)m.ka.y, (Float)m.ka.z,
        (Float)m.ke.x, (Float)m.ke.y, (Float)m.ke.z,
        (Float)m.albedo.x, (Float)m.albedo.y, (Float)m.albedo.z,
        m.roughness, m.metalness);
    return out;
}
