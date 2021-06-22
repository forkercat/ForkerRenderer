//
// Created by Junhao Wang (@Forkercat) on 2021/6/19.
//

#pragma once

#include "shader.h"

struct GShader : public Shader
{
    // Interpolated Variables
    Matrix3x3f vPositionCorrectedWS;  // world space
    Matrix2x3f vTexCoordCorrected;
    Matrix3x3f vNormalCorrectedWS;
    Matrix3x3f vTangentCorrectedWS;

    // Vertex (out) -> Fragment (in)
    Matrix3x3f v2fPositionsWS;
    Matrix2x3f v2fTexCoords;
    Vector3f   v2fOneOverWs;

    // Uniform Variables
    Matrix4x4f uModelMatrix;
    Matrix4x4f uViewMatrix;
    Matrix4x4f uProjectionMatrix;
    Matrix3x3f uNormalMatrix;

    // Extra Output
    Vector3f outNormalWS;
    Vector3f outPositionWS;
    Color3   outAlbedo;  // Diffuse
    Color3   outEmissive;
    Vector3f outParam;        // Specular
    Float    outShadingType;  // PBR or Non-PBR

    Point4f ProcessVertex(int faceIdx, int vertIdx) override
    {
        // MVP
        Point4f positionWS = uModelMatrix * Point4f(mesh->Vert(faceIdx, vertIdx), 1.f);
        Point4f positionVS = uViewMatrix * positionWS;
        Point4f positionCS = uProjectionMatrix * positionVS;

        // TexCoord, Normal, Tangent
        Vector2f texCoord = mesh->TexCoord(faceIdx, vertIdx);
        Vector3f normalWS = uNormalMatrix * mesh->Normal(faceIdx, vertIdx);
        Vector3f tangentWS;
        if (mesh->GetModel().HasTangents())
        {
            tangentWS = uNormalMatrix * mesh->Tangent(faceIdx, vertIdx);
        }

        // Vertex -> Fragment
        v2fPositionsWS.SetCol(vertIdx, positionWS.xyz);
        v2fTexCoords.SetCol(vertIdx, texCoord);

        // PCI
#ifdef PERSPECTIVE_CORRECT_INTERPOLATION
        Float oneOverW = 1.f / positionCS.w;
        v2fOneOverWs[vertIdx] = oneOverW;  // 1/w for 3 vertices
        positionWS *= oneOverW;
        texCoord *= oneOverW;
        normalWS *= oneOverW;
        if (mesh->GetModel().HasTangents()) tangentWS *= oneOverW;
#endif

        // Varying
        vTexCoordCorrected.SetCol(vertIdx, texCoord);
        vPositionCorrectedWS.SetCol(vertIdx, positionWS.xyz);
        vNormalCorrectedWS.SetCol(vertIdx, normalWS);
        if (mesh->GetModel().HasTangents())
            vTangentCorrectedWS.SetCol(vertIdx, tangentWS);

        // Perspective Division
        Point4f positionNDC = positionCS / positionCS.w;
        return positionNDC;
    }
    /////////////////////////////////////////////////////////////////////////////////

    bool ProcessFragment(const Vector3f& baryCoord, Color3& gl_Color) override
    {
        std::shared_ptr<const Material> material = mesh->GetMaterial();

        // Interpolation
        Point3f  positionWS = vPositionCorrectedWS * baryCoord;
        Vector2f texCoord = vTexCoordCorrected * baryCoord;
        Vector3f normalWS = vNormalCorrectedWS * baryCoord;

        // PCI
#ifdef PERSPECTIVE_CORRECT_INTERPOLATION
        Float w = 1.f / Dot(v2fOneOverWs, baryCoord);
        positionWS *= w;
        texCoord *= w;
        normalWS *= w;
#endif
        Vector3f N = Normalize(normalWS);  // World Space
        Vector3f normal;

        if (mesh->GetModel().HasTangents() && material->HasNormalMap())
        {
            Vector3f tangentWS = vTangentCorrectedWS * baryCoord;
#ifdef PERSPECTIVE_CORRECT_INTERPOLATION
            tangentWS *= w;
#endif
            Vector3f T = Normalize(tangentWS + Vector3f(0.001));  // avoid zero division
            // Normal (TBN Matrix)
            T = Normalize(T - Dot(T, N) * N);
            Vector3f   B = Normalize(Cross(N, T));
            Matrix3x3f TbnMatrix;
            TbnMatrix.SetCol(0, T).SetCol(1, B).SetCol(2, N);
            Vector3f sampledNormal = material->normalMap->Sample(texCoord);
            sampledNormal = Normalize(sampledNormal * 2.f - Vector3f(1.f));  // remap
            normal = Normalize(TbnMatrix * sampledNormal);  // World Space
        }
        else
        {
            normal = N;
        }

        // Output Geometry Info
        outNormalWS = normal;
        outPositionWS = positionWS;

        if (mesh->GetModel().SupportPBR())
        {
            // PBR Material
            std::shared_ptr<const PBRMaterial> pbrMaterial = mesh->GetPBRMaterial();
            std::shared_ptr<const Texture>     baseColorMap = pbrMaterial->baseColorMap;
            std::shared_ptr<const Texture>     metalnessMap = pbrMaterial->metalnessMap;
            std::shared_ptr<const Texture>     roughnessMap = pbrMaterial->roughnessMap;
            std::shared_ptr<const Texture>     aoMap = pbrMaterial->ambientOcclusionMap;
            std::shared_ptr<const Texture>     emissiveMap = pbrMaterial->emissiveMap;

            Color3 albedo = pbrMaterial->HasBaseColorMap()
                                ? baseColorMap->Sample(texCoord)
                                : pbrMaterial->albedo;
            Color3 emissive = pbrMaterial->HasEmssiveMap() ? emissiveMap->Sample(texCoord)
                                                           : material->ke;
            Float  roughness = pbrMaterial->HasRoughnessMap()
                                   ? roughnessMap->SampleFloat(texCoord)
                                   : pbrMaterial->roughness;
            Float  metalness = pbrMaterial->HasMetalnessMap()
                                   ? metalnessMap->SampleFloat(texCoord)
                                   : pbrMaterial->metalness;
            Float  ao = pbrMaterial->HasAmbientOcclusionMap()
                            ? aoMap->SampleFloat(texCoord)
                            : 1.f;
            outAlbedo = albedo;
            outEmissive = emissive;
            outParam = Vector3f(ao, metalness, roughness);
            outShadingType = 1.f;  // PBR
        }
        else
        {
            // Material
            std::shared_ptr<const Material> material = mesh->GetMaterial();
            std::shared_ptr<const Texture>  diffuseMap = material->diffuseMap;
            std::shared_ptr<const Texture>  specularMap = material->specularMap;
            std::shared_ptr<const Texture>  emissiveMap = material->emissiveMap;

            Color3 emissive =
                material->HasEmissiveMap() ? emissiveMap->Sample(texCoord) : material->ke;
            Color3 diffuseColor =
                material->HasDiffuseMap() ? diffuseMap->Sample(texCoord) : material->kd;
            Float ambient = material->ka.r;
            Float specular = material->ks.r;
            Float shininess = material->HasSpecularMap() ? specularMap->SampleFloat(texCoord) + 5 : 1.f;

            outAlbedo = diffuseColor;
            outEmissive = emissive;
            outParam = Vector3f(ambient, specular, shininess);
            outShadingType = 0.f;  // Non-PBR
        }

        return false;  // do not discard
    }
};