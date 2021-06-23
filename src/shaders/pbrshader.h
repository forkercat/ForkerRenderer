//
// Created by Junhao Wang (@Forkercat) on 2021/5/5.
//

#pragma once

#include "shader.h"

struct PBRShader : public Shader
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
        std::shared_ptr<const PBRMaterial> pbrMaterial = mesh->GetPBRMaterial();

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

        if (mesh->GetModel().HasTangents() && pbrMaterial->HasNormalMap())
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
            Vector3f sampledNormal = pbrMaterial->normalMap->Sample(texCoord);
            sampledNormal = Normalize(sampledNormal * 2.f - Vector3f(1.f));  // remap
            normal = Normalize(TbnMatrix * sampledNormal);
        }
        else
        {
            normal = N;
        }

        // Test normal
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

        // Physically-Based Shading
        // Texture Sampling
        std::shared_ptr<const Texture> baseColorMap = pbrMaterial->baseColorMap;
        std::shared_ptr<const Texture> metalnessMap = pbrMaterial->metalnessMap;
        std::shared_ptr<const Texture> roughnessMap = pbrMaterial->roughnessMap;
        std::shared_ptr<const Texture> aoMap = pbrMaterial->ambientOcclusionMap;
        std::shared_ptr<const Texture> emissiveMap = pbrMaterial->emissiveMap;

        Color3 albedo = pbrMaterial->HasBaseColorMap() ? baseColorMap->Sample(texCoord)
                                                       : pbrMaterial->albedo;
        Color3 emissive = pbrMaterial->HasEmssiveMap() ? emissiveMap->Sample(texCoord)
                                                       : pbrMaterial->ke;

        Float roughness = pbrMaterial->HasRoughnessMap()
                              ? roughnessMap->SampleFloat(texCoord)
                              : pbrMaterial->roughness;
        Float metalness = pbrMaterial->HasMetalnessMap()
                              ? metalnessMap->SampleFloat(texCoord)
                              : pbrMaterial->metalness;
        Float ao =
            pbrMaterial->HasAmbientOcclusionMap() ? aoMap->SampleFloat(texCoord) : 1.f;
        Vector3f param(ao, metalness, roughness);

        gl_Color = CalculateLight(lightDir, viewDir, halfwayDir, normal, visibility,
                                  albedo, emissive, param, uPointLight.color);

        return false;  // do not discard
    }

    static Color3 CalculateLight(const Vector3f& lightDir, const Vector3f& viewDir,
                                 const Vector3f& halfwayDir, const Vector3f& normal,
                                 Float visibility, const Color3& albedo,
                                 const Color3& emissive, const Vector3f& param,
                                 const Color3& lightRadiance)
    {
        // Gamma Correction (sRGB -> Linear Space)
        Color3 albedoLinear = Pow(albedo, Gamma);
        Color3 emissiveLinear = Pow(emissive, Gamma);

        Float ao = param.x;
        Float metalness = param.y;
        Float roughness = param.z;

        // Dot, Dot, Dot
        Float NdotV = Max(Dot(normal, viewDir), 0.f);
        Float NdotL = Max(Dot(normal, lightDir), 0.f);
        Float NdotH = Max(Dot(normal, halfwayDir), 0.f);
        Float HdotV = Max(Dot(halfwayDir, viewDir), 0.f);

        // Reflectance Equation
        Color3 F0 = Vector3f(0.04f);  // average base reflectivity
        F0 = Lerp(metalness, F0, albedoLinear);

        // Cook-Torrance BRDF
        Float    NDF = distributionGGX(NdotH, roughness);  // D
        Float    G = geometrySmith(NdotV, NdotL, roughness);
        Vector3f F = fresnelSchlick(HdotV, F0);
        Vector3f DGF = NDF * G * F;
        Float    denominator =
            4 * NdotV * NdotL + 0.001f;  // 0.001 to avoid division by zero

        // Specular
        Vector3f ks = F;
        Vector3f specular = DGF / denominator;

        // Diffuse
        Vector3f kd = Vector3f(1.f) - ks;
        kd *= 1.f - metalness;  // only non-metallic material has diffuse lighting

        // BRDF
        Vector3f brdf = kd * albedoLinear * InvPi + specular;

        // Outgoing Radiance
        Color3 Lo = brdf * lightRadiance * NdotL;

        // Shadow Mapping
        if (Shadow::GetShadowStatus())
        {
            Float shadowIntensity = 0.6f;
            Float shadow = (1 - visibility) * shadowIntensity;
            visibility = 1 - shadow;
            Lo *= visibility;
        }

        Color3 color = Lo;

        // Ambient
        Color3 ambient = Color3(0.03) * albedoLinear * ao;
        color += ambient;

        // Emissive
        color += emissiveLinear;

        // HDR Tonemapping
        color = color / (color + Color3(1.f));

        // Gamma Correction
        color = Pow(color, InvGamma);

        return Clamp01(color);
    }

private:
    static Float distributionGGX(Float NdotH, Float roughness)
    {
        Float a = roughness * roughness;  // roughness^2 is better
        Float a2 = a * a;
        Float NdotH2 = NdotH * NdotH;

        Float denominator = (NdotH2 * (a2 - 1.f) + 1.f);
        Float denominator2 = denominator * denominator;
        Float NDF = a2 * InvPi / denominator2;
        return NDF;
    }

    static Float geometrySchlickGGX(Float NdotV, Float roughness)
    {
        Float a = roughness + 1.f;
        Float k = a * a / 8.f;
        return NdotV / (NdotV * (1 - k) + k);
    }

    static Float geometrySmith(Float NdotV, Float NdotL, Float roughness)
    {
        // Geometry Obstruction
        Float ggx1 = geometrySchlickGGX(NdotV, roughness);
        // Geometry Shadowing
        Float ggx2 = geometrySchlickGGX(NdotL, roughness);
        return ggx1 * ggx2;
    }

    static Vector3f fresnelSchlick(Float cosTheta, const Vector3f& f0)
    {
        Float OneMinusHdotV = Max(1.f - cosTheta, 0.f);
        return f0 + (Vector3f(1.f) - f0) * Pow(OneMinusHdotV, 5.f);
    }
};
