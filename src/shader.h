//
// Created by Junhao Wang (@Forkercat) on 2020/12/26.
//

#ifndef SHADER_H_
#define SHADER_H_

#include "buffer.h"
#include "color.h"
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
    virtual Point4f ProcessVertex(int faceIdx, int vertIdx) = 0;
    // Fragment Shader
    virtual bool ProcessFragment(const Vector3f& baryCoord, Color3& gl_Color) = 0;
};

/////////////////////////////////////////////////////////////////////////////////

struct BlinnPhongShader : public Shader
{
    // Interpolating Variables
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
    Matrix4x4f uProjMatrix;
    Matrix3x3f uNormalMatrix;
    PointLight uPointLight;

#ifdef SHADOW_MAPPING
    Buffer     uShadowBuffer;
    Matrix4x4f uLightSpaceMatrix;
    Matrix3x3f vPositionLightSpaceNDC;
#endif

    // Vertex Shader
    Point4f ProcessVertex(int faceIdx, int vertIdx) override
    {
        // MVP
        Point4f positionWS = uModelMatrix * Point4f(mesh->Vert(faceIdx, vertIdx), 1.f);
        Point4f positionVS = uViewMatrix * positionWS;
        Point4f positionCS = uProjMatrix * positionVS;

        // TexCoord, Normal, Tangent
        Vector2f texCoord = mesh->TexCoord(faceIdx, vertIdx);
        Vector3f normalVS = uNormalMatrix * mesh->Normal(faceIdx, vertIdx);
        Vector3f tangentVS = uNormalMatrix * mesh->Tangent(faceIdx, vertIdx);

        // Vertex -> Fragment
        v2fPositionsVS.SetCol(vertIdx, positionVS.xyz);
        v2fTexCoords.SetCol(vertIdx, texCoord);
        v2fLightPositionVS = (uViewMatrix * Vector4f(uPointLight.position, 1.f)).xyz;

        // PCI
#ifdef PERSPECTIVE_CORRECT_MAPPING
        Float oneOverW = 1.f / positionCS.w;
        v2fOneOverWs[vertIdx] = oneOverW;  // 1/w for 3 vertices
        positionVS *= oneOverW;
        texCoord *= oneOverW;
        normalVS *= oneOverW;
        tangentVS *= oneOverW;
#endif
        // Varying
        vTexCoordCorrected.SetCol(vertIdx, texCoord);
        vPositionCorrectedVS.SetCol(vertIdx, positionVS.xyz);
        vNormalCorrected.SetCol(vertIdx, normalVS);
        vTangentCorrected.SetCol(vertIdx, tangentVS);

        // Shadow Mapping
#ifdef SHADOW_MAPPING
        Vector4f positionLightSpaceNDC = uLightSpaceMatrix * positionWS;
        positionLightSpaceNDC /= positionLightSpaceNDC.w;
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
        // Interpolation
        Point3f  positionVS = vPositionCorrectedVS * baryCoord;
        Vector2f texCoord = vTexCoordCorrected * baryCoord;
        Vector3f normalVS = vNormalCorrected * baryCoord;
        Vector3f tangentVS = vTangentCorrected * baryCoord;

        // PCI
#ifdef PERSPECTIVE_CORRECT_MAPPING
        Float w = 1.f / Dot(v2fOneOverWs, baryCoord);
        positionVS *= w;
        texCoord *= w;
        normalVS *= w;
        tangentVS *= w;
#endif
        Vector3f N = Normalize(normalVS);
        Vector3f T = Normalize(tangentVS);

        // Normal (TBN Matrix)
        T = Normalize(T - Dot(T, N) * N);
        Vector3f   B = Normalize(Cross(N, T));
        Matrix3x3f TbnMatrix;
        TbnMatrix.SetCol(0, T).SetCol(1, B).SetCol(2, N);
        Vector3f normal = Normalize(TbnMatrix * mesh->Normal(texCoord));

        // Directions
        Vector3f lightDir = Normalize(v2fLightPositionVS - positionVS);
        Vector3f viewDir = Normalize(-positionVS);  // eye pos is [0, 0, 0]
        Vector3f halfwayDir = Normalize(lightDir + viewDir);

        // Shadow Mapping
        Float shadow = 0.f;
#ifdef SHADOW_MAPPING
        Point3f positionLightSpaceNDC = vPositionLightSpaceNDC * baryCoord;
        shadow = calculateShadow(positionLightSpaceNDC, normal, lightDir);
#endif

        // Blinn-Phong Shading
        gl_Color = calculateLight(lightDir, halfwayDir, normal, texCoord, shadow);

        return false;  // do not discard
    }

private:
    Color3 calculateLight(const Vector3f& lightDir, const Vector3f& halfwayDir,
                          const Vector3f& normal, const Vector2f& texCoord, Float shadow)
    {
        Color3 lightColor = uPointLight.color;
        Color3 diffuseColor = mesh->DiffuseColor(texCoord);
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
        Color3 ambient = mesh->GetKa() * ambi * diffuseColor * lightColor;
        Color3 diffuse = mesh->GetKd() * diff * diffuseColor * lightColor;
        Color3 specular = mesh->GetKs() * spec * lightColor;

        // Shadow Mapping
#ifdef SHADOW_MAPPING
        diffuse *= 1.f - shadow;
        specular *= 1.f - shadow;
#endif

        // Combine
        Color3 color = ambient + diffuse + specular;
        return Clamp01(color);
    }
    /////////////////////////////////////////////////////////////////////////////////

#ifdef SHADOW_MAPPING
    // Calculate Shadow Component
    Float calculateShadow(const Point3f& positionLightSpaceNDC, const Vector3f& normal,
                          const Vector3f& lightDir) const
    {
        // Transform to [0, 1]
        Point3f position01 = positionLightSpaceNDC * 0.5f + Point3f(0.5f);

        // Bias
        Float bias = Max(0.009f * (1.f - Dot(normal, lightDir)), 0.007f);
        // Float bias = 0.008f;
        Float currentDepth = position01.z;
        Float shadow;

#ifdef SOFT_SHADOW_MAPPING_PCF
        // Percentage-Closer Filtering (PCF)
        int size = 4;
        for (int xoffset = -size; xoffset <= size; ++xoffset)
        {
            for (int yoffset = -size; yoffset <= size; ++yoffset)
            {
                int w = uShadowBuffer.GetWidth();
                int h = uShadowBuffer.GetHeight();
                int xpos = w * position01.x + xoffset;
                int ypos = h * position01.y + yoffset;
                xpos = Clamp(xpos, 0, w - 1);
                ypos = Clamp(ypos, 0, h - 1);
                Float pcfClosestDepth = uShadowBuffer.GetValue(xpos, ypos);
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
    Matrix3x3f vPositionNDC;

    // Transformations
    Matrix4x4f uModelMatrix;
    Matrix4x4f uLightSpaceMatrix;

    DepthShader() : Shader() { }

    Point4f ProcessVertex(int faceIdx, int vertIdx) override
    {
        Point4f positionCS =
            uLightSpaceMatrix * uModelMatrix * Point4f(mesh->Vert(faceIdx, vertIdx), 1.f);
        Point4f positionNDC = positionCS / positionCS.w;
        vPositionNDC.SetCol(vertIdx, positionNDC.xyz);
        return positionNDC;
    }

    bool ProcessFragment(const Vector3f& baryCoord, Color3& gl_Color) override
    {
        // Interpolation
        Point3f positionNDC = vPositionNDC * baryCoord;
        gl_Color.z = positionNDC.z * 0.5f + 0.5f;  // to [0, 1]
        return false;
    }
};

#endif  // SHADER_H_
