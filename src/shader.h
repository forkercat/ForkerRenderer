//
// Created by Junhao Wang (@Forkercat) on 2020/12/26.
//

#ifndef _SHADER_H_
#define _SHADER_H_

#include "buffer.h"
#include "geometry.h"
#include "light.h"
#include "mesh.h"
#include "tgaimage.h"

// Enable Perspective Correct Mapping (PCI) If Using Perspective Projection
#define PERSPECTIVE_CORRECT_MAPPING

// Enable Shadow Mapping
#define SHADOW_MAPPING
#define SOFT_SHADOW_MAPPING_PCF

// Abstract Class
struct Shader
{
    const Mesh* mesh;

    Shader() : mesh(nullptr) { }

    // Use Shader Program (set which mesh to shade on)
    void Use(const Mesh* m) { mesh = m; }
    // Vertex Shader
    virtual Vector4f ProcessVertex(int faceIdx, int vertIdx) = 0;
    // Fragment Shader
    virtual bool ProcessFragment(const Vector3f& baryCoord, Vector3f& gl_Color) = 0;
};

/////////////////////////////////////////////////////////////////////////////////

struct BlinnPhongShader : public Shader
{
    // Interpolating Variables
    Matrix3x3f varying_fragPosInEyeDivided;  // eye space
    Matrix2x3f varying_texCoordsDivided;
    Matrix3x3f varying_normalsDivided;
    Matrix3x3f varying_tangentsDivided;

    // Vertex (out) -> Fragment (in)
    Matrix3x3f v2f_fragPosInEye;
    Matrix2x3f v2f_texCoords;
    Vector3f   v2f_oneOverWs;
    Vector3f   v2f_lightPosInEye;

    // Transformations
    Matrix4f ufm_model;
    Matrix4f ufm_view;
    Matrix4f ufm_projection;
    Matrix3f ufm_normalMatrix;

    // Uniform Variables
    PointLight ufm_pointLight;

#ifdef SHADOW_MAPPING
    Buffer  ufm_shadowBuffer;
    Matrix4f ufm_lightSpaceMatrix;
    Matrix3x3f varying_fragPosInLightSpaceNdc;
#endif

    // Vertex Shader
    Vector4f ProcessVertex(int faceIdx, int vertIdx) override
    {
        // MVP
        Vector4f fragPosInWorld = ufm_model * Vector4f(mesh->Vert(faceIdx, vertIdx), 1.f);
        Vector4f fragPosInEye = ufm_view * fragPosInWorld;
        Vector4f fragPosInClip = ufm_projection * fragPosInEye;

        // TexCoord, Normal, Tangent
        Vector2f texCoord = mesh->TexCoord(faceIdx, vertIdx);
        Vector3f normalInEye = ufm_normalMatrix * mesh->Normal(faceIdx, vertIdx);
        Vector3f tangentInEye = ufm_normalMatrix * mesh->Tangent(faceIdx, vertIdx);

        // Vertex -> Fragment
        v2f_fragPosInEye.SetCol(vertIdx, fragPosInEye.xyz);
        v2f_texCoords.SetCol(vertIdx, texCoord);
        v2f_lightPosInEye = (ufm_view * Vector4f(ufm_pointLight.Position, 1.f)).xyz;

        // PCI
#ifdef PERSPECTIVE_CORRECT_MAPPING
        Float oneOverW = 1.f / fragPosInClip.w;
        v2f_oneOverWs[vertIdx] = oneOverW;  // 1/w for 3 vertices
        fragPosInEye *= oneOverW;
        texCoord *= oneOverW;
        normalInEye *= oneOverW;
        tangentInEye *= oneOverW;
#endif
        // Varying
        varying_texCoordsDivided.SetCol(vertIdx, texCoord);
        varying_fragPosInEyeDivided.SetCol(vertIdx, fragPosInEye.xyz);
        varying_normalsDivided.SetCol(vertIdx, normalInEye);
        varying_tangentsDivided.SetCol(vertIdx, tangentInEye);

        // Shadow Mapping
#ifdef SHADOW_MAPPING
        Vector4f fragPosInLightSpaceNdc = DivideByW(ufm_lightSpaceMatrix * fragPosInWorld);
        varying_fragPosInLightSpaceNdc.SetCol(vertIdx, fragPosInLightSpaceNdc.xyz);
#endif

        // Perspective Division
        return DivideByW(fragPosInClip);
    }

    /////////////////////////////////////////////////////////////////////////////////

    // Fragment Shader
    bool ProcessFragment(const Vector3f& baryCoord, Vector3f& gl_Color) override
    {
        // Interpolation
        Vector3f fragPosInEye = varying_fragPosInEyeDivided * baryCoord;
        Vector2f texCoord = varying_texCoordsDivided * baryCoord;
        Vector3f normalInEye = varying_normalsDivided * baryCoord;
        Vector3f tangentInEye = varying_tangentsDivided * baryCoord;

        // PCI
#ifdef PERSPECTIVE_CORRECT_MAPPING
        Float w = 1.f / Dot(v2f_oneOverWs, baryCoord);
        fragPosInEye *= w;
        texCoord *= w;
        normalInEye *= w;
        tangentInEye *= w;
#endif
        Vector3f N = Normalize(normalInEye);
        Vector3f T = Normalize(tangentInEye);

        // Normal (TBN Matrix)
        T = Normalize(T - Dot(T, N) * N);
        Vector3f B = Normalize(Cross(N, T));
        Matrix3f TbnMatrix;
        TbnMatrix.SetCol(0, T).SetCol(1, B).SetCol(2, N);
        Vector3f normal = Normalize(TbnMatrix * mesh->Normal(texCoord));

        // Directions
        Vector3f lightDir = Normalize(v2f_lightPosInEye - fragPosInEye);
        Vector3f viewDir = Normalize(-fragPosInEye);  // eye pos is [0, 0, 0]
        Vector3f halfwayDir = Normalize(lightDir + viewDir);

        // Shadow Mapping
        Float shadow = 0.f;
#ifdef SHADOW_MAPPING
        Vector3f fragPosInLightSpace = varying_fragPosInLightSpaceNdc * baryCoord;
        shadow = calculateShadow(fragPosInLightSpace, normal, lightDir);
#endif

        // Blinn-Phong Shading
        gl_Color = calculateLight(lightDir, halfwayDir, normal, texCoord, shadow);

        return false;  // do not discard
    }

private:
    Vector3f calculateLight(const Vector3f& lightDir, const Vector3f& halfwayDir,
                         const Vector3f& normal, const Vector2f& texCoord, Float shadow)
    {
        Vector3f lightColor = ufm_pointLight.Color;
        Vector3f diffuseColor = mesh->DiffuseColor(texCoord);
        // Float specularity = mesh->SpecularIntensity(texCoord);  // type 1
        Float specularity = mesh->SpecularShininess(texCoord);  // type 2
        Float aoIntensity = mesh->AmbientOcclusionIntensity(texCoord);

        // Contribution of Shading Component
        Float ambi = aoIntensity;
        Float diff = Max(0.f, Dot(lightDir, normal));
        Float spec = Max(0.f, Dot(halfwayDir, normal));
        // spec *= specularity;  // type 1 - intensity
        spec = std::pow(spec, specularity + 5);  // type 2 - shininess

        // Color of Shading Component
        Vector3f ambient = mesh->GetKa() * ambi * diffuseColor * lightColor;
        Vector3f diffuse = mesh->GetKd() * diff * diffuseColor * lightColor;
        Vector3f specular = mesh->GetKs() * spec * lightColor;

        // Shadow Mapping
#ifdef SHADOW_MAPPING
        diffuse *= 1.f - shadow;
        specular *= 1.f - shadow;
#endif

        // Combine
        Vector3f color = ambient + diffuse + specular;
        return Clamp01(color);
    }
    /////////////////////////////////////////////////////////////////////////////////

#ifdef SHADOW_MAPPING
    // Calculate Shadow Component
    Float calculateShadow(const Vector3f& fragPosInLightSpace, const Vector3f& normal,
                          const Vector3f& lightDir)
    {
        // Transform to [0, 1]
        Vector3f fragPosTransformed = fragPosInLightSpace * 0.5f + Vector3f(0.5f);

        // Bias
        Float bias = Max(0.009f * (1.f - Dot(normal, lightDir)), 0.007f);
        // Float bias = 0.008f;
        Float currentDepth = fragPosTransformed.z;
        Float shadow;

#ifdef SOFT_SHADOW_MAPPING_PCF
        // Percentage-Closer Filtering (PCF)
        int size = 4;
        for (int xoffset = -size; xoffset <= size; ++xoffset)
        {
            for (int yoffset = -size; yoffset <= size; ++yoffset)
            {
                int w = ufm_shadowBuffer.GetWidth();
                int h = ufm_shadowBuffer.GetHeight();
                int xpos = w * fragPosTransformed.x + xoffset;
                int ypos = h * fragPosTransformed.y + yoffset;
                xpos = Clamp(xpos, 0, w - 1);
                ypos = Clamp(ypos, 0, h - 1);
                Float pcfClosestDepth = ufm_shadowBuffer.GetValue(xpos, ypos);
                shadow += (currentDepth - bias > pcfClosestDepth) ? 1.f : 0.f;
            }
        }
        shadow /= (Float)((2 * size + 1) * (2 * size + 1));
#else
        // Regular Shadowing
        int   xpos = ufm_shadowBuffer.GetWidth() * fragPosTransformed.x;
        int   ypos = ufm_shadowBuffer.GetHeight() * fragPosTransformed.y;
        Float closestDepth = ForkerGL::ShadowBuffer.Get(xpos, ypos);
        shadow = (currentDepth - bias > closestDepth) ? 1.f : 0.f;
#endif
        return shadow;
    }
#endif
};

/////////////////////////////////////////////////////////////////////////////////

// Depth Shader For Shadow Mapping
struct DepthShader : public Shader
{
    // Interpolation
    Matrix3x3f varying_fragPosInNdc;

    // Transformations
    Matrix4f ufm_model;
    Matrix4f ufm_lightSpace;

    DepthShader() : Shader() { }

    Vector4f ProcessVertex(int faceIdx, int vertIdx) override
    {
        Vector4f fragPosInClip =
            ufm_lightSpace * ufm_model * Vector4f(mesh->Vert(faceIdx, vertIdx), 1.f);
        Vector4f fragPosInNdc = DivideByW(fragPosInClip);
        varying_fragPosInNdc.SetCol(vertIdx, fragPosInNdc.xyz);
        return fragPosInNdc;
    }

    bool ProcessFragment(const Vector3f& baryCoord, Vector3f& gl_Color) override
    {
        // Interpolation
        Vector3f fragPosInNdc = varying_fragPosInNdc * baryCoord;
        gl_Color.z = fragPosInNdc.z * 0.5f + 0.5f;  // to [0, 1]
        return false;
    }
};

#endif  // _SHADER_H_
