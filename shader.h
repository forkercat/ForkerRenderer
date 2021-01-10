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
    virtual Vec4f ProcessVertex(int faceIdx, int vertIdx) = 0;
    // Fragment Shader
    virtual bool ProcessFragment(const Vec3f& baryCoord, Vec3f& gl_Color) = 0;
};

/////////////////////////////////////////////////////////////////////////////////

struct BlinnPhongShader : public Shader
{
    // Interpolating Variables
    Mat3x3f varying_fragPosInEyeDivided;  // eye space
    Mat2x3f varying_texCoordsDivided;
    Mat3x3f varying_normalsDivided;

    // Vertex (out) -> Fragment (in)
    Mat3x3f v2f_fragPosInEye;
    Mat2x3f v2f_texCoords;
    Vec3f   v2f_oneOverWs;
    Vec3f   v2f_lightPosInEye;

    // Transformations
    Mat4f ufm_model;
    Mat4f ufm_view;
    Mat4f ufm_projection;
    Mat3f ufm_normalMatrix;

    // Uniform Variables
    PointLight ufm_pointLight;

#ifdef SHADOW_MAPPING
    Buffer  ufm_shadowBuffer;
    Mat4f   ufm_lightSpaceMatrix;
    Mat3x3f varying_fragPosInLightSpaceNdc;
#endif

    // Vertex Shader
    Vec4f ProcessVertex(int faceIdx, int vertIdx) override
    {
        // MVP
        Vec4f fragPosInWorld = ufm_model * Vec4f(mesh->Vert(faceIdx, vertIdx), 1.f);
        Vec4f fragPosInEye = ufm_view * fragPosInWorld;
        Vec4f fragPosInClip = ufm_projection * fragPosInEye;

        // TexCoord and Normal
        Vec2f texCoord = mesh->TexCoord(faceIdx, vertIdx);
        Vec3f normalInEye = ufm_normalMatrix * mesh->Normal(faceIdx, vertIdx);

        // Vertex -> Fragment
        v2f_fragPosInEye.SetCol(vertIdx, fragPosInEye.xyz);
        v2f_texCoords.SetCol(vertIdx, texCoord);
        v2f_lightPosInEye = (ufm_view * Vec4f(ufm_pointLight.Position, 1.f)).xyz;

        // PCI
#ifdef PERSPECTIVE_CORRECT_MAPPING
        Float oneOverW = 1.f / fragPosInClip.w;
        v2f_oneOverWs[vertIdx] = oneOverW;  // 1/w for 3 vertices
        fragPosInEye *= oneOverW;
        texCoord *= oneOverW;
        normalInEye *= oneOverW;
#endif
        // Varying
        varying_texCoordsDivided.SetCol(vertIdx, texCoord);
        varying_fragPosInEyeDivided.SetCol(vertIdx, fragPosInEye.xyz);
        varying_normalsDivided.SetCol(vertIdx, normalInEye);

        // Shadow Mapping
#ifdef SHADOW_MAPPING
        Vec4f fragPosInLightSpaceNdc = DivideByW(ufm_lightSpaceMatrix * fragPosInWorld);
        varying_fragPosInLightSpaceNdc.SetCol(vertIdx, fragPosInLightSpaceNdc.xyz);
#endif

        // Perspective Division
        return DivideByW(fragPosInClip);
    }

    /////////////////////////////////////////////////////////////////////////////////

    // Fragment Shader
    bool ProcessFragment(const Vec3f& baryCoord, Vec3f& gl_Color) override
    {
        // Interpolation
        Vec3f fragPosInEye = varying_fragPosInEyeDivided * baryCoord;
        Vec2f texCoord = varying_texCoordsDivided * baryCoord;
        Vec3f normalInEye = varying_normalsDivided * baryCoord;

        // PCI
#ifdef PERSPECTIVE_CORRECT_MAPPING
        Float w = 1.f / Dot(v2f_oneOverWs, baryCoord);
        fragPosInEye *= w;
        texCoord *= w;
        normalInEye *= w;
#endif
        normalInEye = Normalize(normalInEye);

        // Normal & Directions
        Vec3f normal = convertNormal(mesh->Normal(texCoord), normalInEye);  // normalized
        Vec3f lightDir = Normalize(v2f_lightPosInEye - fragPosInEye);
        Vec3f viewDir = Normalize(-fragPosInEye);  // eye pos is [0, 0, 0]
        Vec3f halfwayDir = Normalize(lightDir + viewDir);

        // Shadow Mapping
        Float shadow = 0.f;
#ifdef SHADOW_MAPPING
        Vec3f fragPosInLightSpace = varying_fragPosInLightSpaceNdc * baryCoord;
        shadow = calculateShadow(fragPosInLightSpace, normal, lightDir);
#endif

        // Blinn-Phong Shading
        gl_Color = calculateLight(lightDir, halfwayDir, normal, texCoord, shadow);

        return false;  // do not discard
    }

private:
    Vec3f calculateLight(const Vec3f& lightDir, const Vec3f& halfwayDir,
                         const Vec3f& normal, const Vec2f& texCoord, Float shadow)
    {
        Vec3f lightColor = ufm_pointLight.Color;
        Vec3f diffuseColor = mesh->DiffuseColor(texCoord);
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
        Vec3f ambient = mesh->GetKa() * ambi * diffuseColor * lightColor;
        Vec3f diffuse = mesh->GetKd() * diff * diffuseColor * lightColor;
        Vec3f specular = mesh->GetKs() * spec * lightColor;

        // Shadow Mapping
#ifdef SHADOW_MAPPING
        diffuse *= 1.f - shadow;
        specular *= 1.f - shadow;
#endif

        // Combine
        Vec3f color = ambient + diffuse + specular;
        return Clamp(color, 0.f, 1.f);
    }

    /////////////////////////////////////////////////////////////////////////////////

    // Convert Normal By TBN Matrix From Tangent Space To Eye Space
    Vec3f convertNormal(const Vec3f& normalInTangent, const Vec3f& normalInEye)
    {
        Vec3f edge1 = v2f_fragPosInEye.Col(1) - v2f_fragPosInEye.Col(0);
        Vec3f edge2 = v2f_fragPosInEye.Col(2) - v2f_fragPosInEye.Col(0);
        Vec2f deltaUv1 = v2f_texCoords.Col(1) - v2f_texCoords.Col(0);
        Vec2f deltaUv2 = v2f_texCoords.Col(2) - v2f_texCoords.Col(0);
        Mat3f TbnMatrix = MakeTbnMatrix(edge1, edge2, deltaUv1, deltaUv2, normalInEye);
        Vec3f normal = Normalize(TbnMatrix * normalInTangent);  // tangent -> eye
        return normal;
    }

    /////////////////////////////////////////////////////////////////////////////////

#ifdef SHADOW_MAPPING
    // Calculate Shadow Component
    Float calculateShadow(const Vec3f& fragPosInLightSpace, const Vec3f& normal,
                          const Vec3f& lightDir)
    {
        // Transform to [0, 1]
        Vec3f fragPosTransformed = fragPosInLightSpace * 0.5f + Vec3f(0.5f);

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
    Mat3x3f varying_fragPosInNdc;

    // Transformations
    Mat4f ufm_model;
    Mat4f ufm_lightSpace;

    DepthShader() : Shader() { }

    Vec4f ProcessVertex(int faceIdx, int vertIdx) override
    {
        Vec4f fragPosInClip =
            ufm_lightSpace * ufm_model * Vec4f(mesh->Vert(faceIdx, vertIdx), 1.f);
        Vec4f fragPosInNdc = DivideByW(fragPosInClip);
        varying_fragPosInNdc.SetCol(vertIdx, fragPosInNdc.xyz);
        return fragPosInNdc;
    }

    bool ProcessFragment(const Vec3f& baryCoord, Vec3f& gl_Color) override
    {
        // Interpolation
        Vec3f fragPosInNdc = varying_fragPosInNdc * baryCoord;
        gl_Color.z = fragPosInNdc.z * 0.5f + 0.5f;  // to [0, 1]
        return false;
    }
};

#endif  // _SHADER_H_
