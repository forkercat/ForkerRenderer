//
// Created by Junhao Wang (@Forkercat) on 2021/5/5.
//

#pragma once

#include "shader.h"

struct BlinnPhongShader : public Shader
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
    PointLight uPointLight;
    Point3f    uEyePos;

    // For Shadow Pass
    Matrix4x4f uLightSpaceMatrix;
    Matrix3x3f vPositionLightSpaceNDC;

    // Vertex Shader
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

        // Shadow Mapping
        bool    isShadowOn = Shadow::GetShadowStatus();
        Point4f positionLightSpaceNDC;
        if (isShadowOn)
        {
            positionLightSpaceNDC = uLightSpaceMatrix * positionWS;
            positionLightSpaceNDC /= positionLightSpaceNDC.w;
        }

        // PCI
#ifdef PERSPECTIVE_CORRECT_INTERPOLATION
        Float oneOverW = 1.f / positionCS.w;
        v2fOneOverWs[vertIdx] = oneOverW;  // 1/w for 3 vertices
        positionWS *= oneOverW;
        texCoord *= oneOverW;
        normalWS *= oneOverW;
        if (mesh->GetModel().HasTangents()) tangentWS *= oneOverW;
        if (isShadowOn) positionLightSpaceNDC *= oneOverW;
#endif
        // Varying
        vTexCoordCorrected.SetCol(vertIdx, texCoord);
        vPositionCorrectedWS.SetCol(vertIdx, positionWS.xyz);
        vNormalCorrectedWS.SetCol(vertIdx, normalWS);
        if (mesh->GetModel().HasTangents())
            vTangentCorrectedWS.SetCol(vertIdx, tangentWS);
        if (isShadowOn) vPositionLightSpaceNDC.SetCol(vertIdx, positionLightSpaceNDC.xyz);

        // Perspective Division
        Point4f positionNDC = positionCS / positionCS.w;
        return positionNDC;
    }

    /////////////////////////////////////////////////////////////////////////////////

    // Fragment Shader
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

        // normal = normal * 0.5f + Vector3f(0.5f);
        // gl_Color = normal;
        // return false;

        // Directions
        Vector3f lightDir = Normalize(uPointLight.position - positionWS);
        Vector3f viewDir = Normalize(uEyePos - positionWS);
        Vector3f halfwayDir = Normalize(lightDir + viewDir);

        // Shadow Mapping
        Float visibility = 0.f;
        if (Shadow::GetShadowStatus())
        {
            Point3f positionLightSpaceNDC = vPositionLightSpaceNDC * baryCoord;
#ifdef PERSPECTIVE_CORRECT_INTERPOLATION
            positionLightSpaceNDC *= w;
#endif
            visibility = Shadow::CalculateShadowVisibility(
                ForkerGL::ShadowBuffer, positionLightSpaceNDC, normal, lightDir);
        }

        // Blinn-Phong Shading
        std::shared_ptr<const Texture> diffuseMap = material->diffuseMap;
        std::shared_ptr<const Texture> specularMap = material->specularMap;
        std::shared_ptr<const Texture> emissiveMap = material->emissiveMap;

        Color3 diffuseColor =
            material->HasDiffuseMap() ? diffuseMap->Sample(texCoord) : material->kd;
        Color3 emissive =
            material->HasEmissiveMap() ? emissiveMap->Sample(texCoord) : material->ke;
        Float shininess = 1.f;
        if (material->HasSpecularMap())
            shininess = specularMap->SampleFloat(texCoord) + 5;
        Vector3f param(material->ka.x, material->ks.x, shininess);

        gl_Color = CalculateLight(lightDir, halfwayDir, normal, visibility, diffuseColor,
                                  emissive, param, uPointLight.color);

        return false;  // do not discard
    }

    static Color3 CalculateLight(const Vector3f& lightDir, const Vector3f& halfwayDir,
                                 const Vector3f& normal, Float visibility,
                                 const Color3& diffuseColor, const Color3& emissive,
                                 const Vector3f& param, const Color3& lightColor)
    {
        // Gamma Correction (sRGB -> Linear Space)
        Color3 diffuseColorLinear = Pow(diffuseColor, Gamma);
        Color3 emissiveLinear = Pow(emissive, Gamma);

        Float ao = param.x;
        Float ks = param.y;
        Float shininess = param.z;

        // Diffuse
        Float diff = Max(0.f, Dot(lightDir, normal));

        // Specular
        Float spec = std::pow(Max(0.f, Dot(halfwayDir, normal)), shininess);

        // Color of Shading Component
        Color3 ambient = Color3(0.3f) * diffuseColorLinear * ao;
        Color3 diffuse = diffuseColorLinear * diff * ao;
        Color3 specular = Color3(ks) * spec;

        // Shadow Mapping
        if (Shadow::GetShadowStatus())
        {
            Float shadowIntensity = 0.6f;
            Float shadow = (1 - visibility) * shadowIntensity;
            visibility = 1 - shadow;
            diffuse *= visibility;
            specular *= visibility;
        }

        // Combine
        Color3 color = ambient + (diffuse + specular + emissiveLinear) * lightColor;

        // HDR Tonemapping
        color = color / (color + Color3(1.f));

        // Gamma Correction
        color = Pow(color, InvGamma);

        return Clamp01(color);
    }
};