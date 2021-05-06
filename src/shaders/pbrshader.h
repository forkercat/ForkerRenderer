//
// Created by Junhao Wang (@Forkercat) on 2021/5/5.
//

#ifndef SHADERS_PBRSHADER_H_
#define SHADERS_PBRSHADER_H_

#include "shader.h"

struct PBRShader : public Shader
{
    // Interpolated Variables
    Matrix3x3f vPositionCorrectedVS;  // eye space
    Matrix2x3f vTexCoordCorrected;
    Matrix3x3f vNormalCorrected;
    Matrix3x3f vTangentCorrected;

    // Vertex (out) -> Fragment (in)
    Matrix3x3f v2fPositionsVS;
    Matrix2x3f v2fTexCoords;
    Vector3f   v2fOneOverWs;
    Point3f    v2fLightPositionVS;

    // Uniform Variables
    Matrix4x4f uModelMatrix;
    Matrix4x4f uViewMatrix;
    Matrix4x4f uProjectionMatrix;
    Matrix3x3f uNormalMatrix;
    PointLight uPointLight;

    // For Shadow Pass
    Buffer     uShadowBuffer;
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
        Vector3f normalVS = uNormalMatrix * mesh->Normal(faceIdx, vertIdx);
        Vector3f tangentVS;
        if (mesh->GetModel()->HasTangents())
        {
            tangentVS = uNormalMatrix * mesh->Tangent(faceIdx, vertIdx);
        }

        // Vertex -> Fragment
        v2fPositionsVS.SetCol(vertIdx, positionVS.xyz);
        v2fTexCoords.SetCol(vertIdx, texCoord);
        v2fLightPositionVS = (uViewMatrix * Vector4f(uPointLight.position, 1.f)).xyz;

        // Shadow Mapping
#ifdef SHADOW_PASS
        Point4f positionLightSpaceNDC = uLightSpaceMatrix * positionWS;
        positionLightSpaceNDC /= positionLightSpaceNDC.w;
#endif

        // PCI
#ifdef PERSPECTIVE_CORRECT_INTERPOLATION
        Float oneOverW = 1.f / positionCS.w;
        v2fOneOverWs[vertIdx] = oneOverW;  // 1/w for 3 vertices
        positionVS *= oneOverW;
        texCoord *= oneOverW;
        normalVS *= oneOverW;
        if (mesh->GetModel()->HasTangents()) tangentVS *= oneOverW;
#ifdef SHADOW_PASS
        positionLightSpaceNDC *= oneOverW;
#endif  // SHADOW_PASS
#endif
        // Varying
        vTexCoordCorrected.SetCol(vertIdx, texCoord);
        vPositionCorrectedVS.SetCol(vertIdx, positionVS.xyz);
        vNormalCorrected.SetCol(vertIdx, normalVS);
        if (mesh->GetModel()->HasTangents()) vTangentCorrected.SetCol(vertIdx, tangentVS);
#ifdef SHADOW_PASS
        vPositionLightSpaceNDC.SetCol(vertIdx, positionLightSpaceNDC.xyz);
#endif

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
        Point3f  positionVS = vPositionCorrectedVS * baryCoord;
        Vector2f texCoord = vTexCoordCorrected * baryCoord;
        Vector3f normalVS = vNormalCorrected * baryCoord;

        // PCI
#ifdef PERSPECTIVE_CORRECT_INTERPOLATION
        Float w = 1.f / Dot(v2fOneOverWs, baryCoord);
        positionVS *= w;
        texCoord *= w;
        normalVS *= w;
#endif

        Vector3f N = Normalize(normalVS);
        Vector3f normal;

        if (mesh->GetModel()->HasTangents() && pbrMaterial->HasNormalMap())
        {
            Vector3f tangentVS = vTangentCorrected * baryCoord;
#ifdef PERSPECTIVE_CORRECT_INTERPOLATION
            tangentVS *= w;
#endif
            Vector3f T = Normalize(tangentVS + Vector3f(0.001));  // avoid zero division
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
        Vector3f lightDir = Normalize(v2fLightPositionVS - positionVS);
        Vector3f viewDir = Normalize(-positionVS);  // eye pos is [0, 0, 0]
        Vector3f halfwayDir = Normalize(lightDir + viewDir);

        // Shadow Mapping
        Float visibility = 0.f;
#ifdef SHADOW_PASS
        Point3f positionLightSpaceNDC = vPositionLightSpaceNDC * baryCoord;
#ifdef PERSPECTIVE_CORRECT_INTERPOLATION
        positionLightSpaceNDC *= w;
#endif
        visibility = calculateShadowVisibility(uShadowBuffer, positionLightSpaceNDC,
                                               normal, lightDir);
#endif

        // Physically-Based Shading
        gl_Color =
            calculateLight(lightDir, viewDir, halfwayDir, normal, texCoord, visibility);

        return false;  // do not discard
    }

private:
    Color3 calculateLight(const Vector3f& lightDir, const Vector3f& viewDir,
                          const Vector3f& halfwayDir, const Vector3f& normal,
                          const Vector2f& texCoord, Float visibility)
    {
        // Dot, Dot, Dot
        Float NdotV = Max(Dot(normal, viewDir), 0.f);
        Float NdotL = Max(Dot(normal, lightDir), 0.f);
        Float NdotH = Max(Dot(normal, halfwayDir), 0.f);
        Float HdotV = Max(Dot(halfwayDir, viewDir), 0.f);

        // Texture Sampling
        std::shared_ptr<const PBRMaterial> pbrMaterial = mesh->GetPBRMaterial();
        std::shared_ptr<const Texture>     baseColorMap = pbrMaterial->baseColorMap;
        std::shared_ptr<const Texture>     metalnessMap = pbrMaterial->metalnessMap;
        std::shared_ptr<const Texture>     roughnessMap = pbrMaterial->roughnessMap;
        std::shared_ptr<const Texture>     aoMap = pbrMaterial->ambientOcclusionMap;
        std::shared_ptr<const Texture>     emissiveMap = pbrMaterial->emissiveMap;

        Color3 albedo = pbrMaterial->HasBaseColorMap() ? baseColorMap->Sample(texCoord)
                                                       : pbrMaterial->albedo;
        Color3 emissive =
            pbrMaterial->HasEmssiveMap() ? emissiveMap->Sample(texCoord) : Color3(0.f);

        Float roughness = pbrMaterial->HasRoughnessMap()
                              ? roughnessMap->SampleFloat(texCoord)
                              : pbrMaterial->roughness;
        Float metalness = pbrMaterial->HasMetalnessMap()
                              ? metalnessMap->SampleFloat(texCoord)
                              : pbrMaterial->metalness;
        Float ao =
            pbrMaterial->HasAmbientOcclusionMap() ? aoMap->SampleFloat(texCoord) : 1.f;

        // Input Radiance
        Color3 radiance = uPointLight.color;  // ignore distance attenuation
        radiance = Color3(10.0);

        // Gamma Correction (sRGB -> Linear Space)
        albedo = Pow(albedo, Gamma);
        emissive = Pow(emissive, Gamma);

        // Reflectance Equation
        Color3 F0 = Vector3f(0.04f);  // average base reflectivity
        F0 = Lerp(metalness, F0, albedo);

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
        Vector3f brdf = kd * albedo * InvPi + specular;

        // Outgoing Radiance
        Color3 Lo = brdf * radiance * NdotL;

        // Shadow Mapping
#ifdef SHADOW_PASS
        Float shadowIntensity = 0.6f;
        Float shadow = (1 - visibility) * shadowIntensity;
        visibility = 1 - shadow;
        Lo *= visibility;
#endif
        Color3 color = Lo;

        // Ambient
        Color3 ambient = Color3(0.03) * albedo * ao;
        color += ambient;

        // Emissive
        color += emissive;

        // HDR Tonemapping
        color = color / (color + Color3(1.f));

        // Gamma Correction
        color = Pow(color, InvGamma);

        return Clamp01(color);
    }

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

#endif  // SHADERS_PBRSHADER_H_
