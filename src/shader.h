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
#include "model.h"
#include "tgaimage.h"

// Enable Perspective Correct Mapping (PCI)
#define PERSPECTIVE_CORRECT_MAPPING

// Enable Shadow Mapping (Hard Shadow)
#define SHADOW_PASS

// Enable Soft Shadow
// #define SOFT_SHADOW_PCF  // Percentage-Closer Filtering (PCF)
#define SOFT_SHADOW_PCSS  // Percentage-Closer Soft Shadow (PCSS)

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
    Matrix4x4f uProjectionMatrix;
    Matrix3x3f uNormalMatrix;
    PointLight uPointLight;

#ifdef SHADOW_PASS
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
#ifdef PERSPECTIVE_CORRECT_MAPPING
        Float oneOverW = 1.f / positionCS.w;
        v2fOneOverWs[vertIdx] = oneOverW;  // 1/w for 3 vertices
        positionVS *= oneOverW;
        texCoord *= oneOverW;
        normalVS *= oneOverW;
        if (mesh->GetModel()->HasTangents()) tangentVS *= oneOverW;
#ifdef SHADOW_PASS
        positionLightSpaceNDC *= oneOverW;
#endif
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
        const Material* material = mesh->GetMaterial();

        // Interpolation
        Point3f  positionVS = vPositionCorrectedVS * baryCoord;
        Vector2f texCoord = vTexCoordCorrected * baryCoord;
        Vector3f normalVS = vNormalCorrected * baryCoord;

        // PCI
#ifdef PERSPECTIVE_CORRECT_MAPPING
        Float w = 1.f / Dot(v2fOneOverWs, baryCoord);
        positionVS *= w;
        texCoord *= w;
        normalVS *= w;
#endif
        Vector3f N = Normalize(normalVS);
        Vector3f normal;

        if (mesh->GetModel()->HasTangents() && material->HasNormalMap())
        {
            Vector3f tangentVS = vTangentCorrected * baryCoord;
#ifdef PERSPECTIVE_CORRECT_MAPPING
            tangentVS *= w;
#endif
            Vector3f T = Normalize(tangentVS);
            // Normal (TBN Matrix)
            T = Normalize(T - Dot(T, N) * N);
            Vector3f   B = Normalize(Cross(N, T));
            Matrix3x3f TbnMatrix;
            TbnMatrix.SetCol(0, T).SetCol(1, B).SetCol(2, N);
            Vector3f sampledNormal = material->normalMap->Sample(texCoord);
            sampledNormal = Normalize(sampledNormal * 2.f - Vector3f(1.f));  // remap
            normal = Normalize(TbnMatrix * sampledNormal);
        }
        else
        {
            normal = N;
        }

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
#ifdef PERSPECTIVE_CORRECT_MAPPING
        positionLightSpaceNDC *= w;
#endif
        visibility = calculateShadowVisibility(positionLightSpaceNDC, normal, lightDir);
#endif

        // Blinn-Phong Shading
        gl_Color = calculateLight(lightDir, halfwayDir, normal, texCoord, visibility);

        return false;  // do not discard
    }

private:
    Color3 calculateLight(const Vector3f& lightDir, const Vector3f& halfwayDir,
                          const Vector3f& normal, const Vector2f& texCoord,
                          Float visibility)
    {
        const Material*           material = mesh->GetMaterial();
        const shared_ptr<Texture> diffuseMap = material->diffuseMap;
        const shared_ptr<Texture> specularMap = material->specularMap;
        const shared_ptr<Texture> ambientOcclusionMap = material->ambientOcclusionMap;

        Color3 lightColor = uPointLight.color;

        // Ambient
        Float ambi = material->HasAmbientOcclusionMap()
                         ? material->ambientOcclusionMap->SampleFloat(texCoord)
                         : 0.2f;

        // Diffuse
        Float diff = Max(0.f, Dot(lightDir, normal));

        // Specular
        Float spec = Max(0.f, Dot(halfwayDir, normal));
        if (material->HasSpecularMap())
        {
            Float specularity = specularMap->SampleFloat(texCoord);  // type 1
            // Float specularity = specularMap->SampleFloat(texCoord) * 255.f;  // type 2
            // spec *= specularity;  // type 1 - intensity
            spec = std::pow(spec, specularity + 5);  // type 2 - shininess
        }
        else
        {
            spec = std::pow(spec, 1.0);
        }

        // Color of Shading Component
        Color3 ambient, diffuse, specular;
        if (material->HasDiffuseMap())
        {
            Color3 diffuseColor = diffuseMap->Sample(texCoord);
            ambient = ambi * diffuseColor;
            diffuse = diff * diffuseColor;
        }
        else
        {
            ambient = ambi * material->ka;
            diffuse = diff * material->kd;
        }
        specular = material->ks * spec;

        ambient = ambient * lightColor;
        diffuse = diffuse * lightColor;
        specular = specular * lightColor;

        // Shadow Mapping
#ifdef SHADOW_PASS
        Float shadowIntensity = 0.6f;
        Float shadow = (1 - visibility) * shadowIntensity;
        visibility = 1 - shadow;
        diffuse *= visibility;
        specular *= visibility;
#endif

        // Combine
        Color3 color = ambient + diffuse + specular;
        return Clamp01(color);
    }
    /////////////////////////////////////////////////////////////////////////////////

#ifdef SHADOW_PASS
Float sampleShadowMap(const Buffer& shadowMap, const Vector2f& uv) const
    {
        // Fix region out of map
        if (uv.x < 0.f || uv.x > 1.f || uv.y < 0.f || uv.y > 1.f) return Infinity;

        int   w = uShadowBuffer.GetWidth() - 0.001f;
        int   h = uShadowBuffer.GetHeight() - 0.001f;
        int   u = (int)((float)w * uv.x);
        int   v = (int)((float)h * uv.y);
        Float depth = uShadowBuffer.GetValue(u, v);

        return depth < Epsilon ? 1.f : depth;  // fix background depth
    }

    // Hard Shadow
    Float hardShadow(const Buffer& shadowMap, const Vector3f& shadowCoord,
                     Float bias) const
    {
        Float visibility;
        Float sampledDepth = sampleShadowMap(uShadowBuffer, shadowCoord.xy);
        Float currentDepth = shadowCoord.z;
        visibility = (currentDepth <= sampledDepth + bias) ? 1.f : 0.f;
        return visibility;
    }

    // PCF
#define PCF_FILTER_SIZE 0.007
#define PCF_NUM_SAMPLES 64
    Float PCF(const Buffer& shadowMap, const Vector3f& shadowCoord, Float bias,
              Float filterSize) const
    {
        Float visibility = 0.f;
        Float currentDepth = shadowCoord.z;
        Float invPCFNumSamples = 1.f / (Float)PCF_NUM_SAMPLES;

        for (int i = 0; i < PCF_NUM_SAMPLES; ++i)
        {
            Vector2f uv = shadowCoord.xy + RandomVectorInUnitDisk().xy * filterSize;
            Float    sampleDepth = sampleShadowMap(shadowMap, uv);

            if (currentDepth <= sampleDepth + bias) visibility += invPCFNumSamples;
        }

        return visibility;
    }

    // PCSS
#define PCSS_BLOCKER_SEARCH_NUM_SAMPLES 32
#define PCSS_BLOCKER_SEARCH_FILTER_SIZE 0.005

    Float findAverageBlockDepth(const Buffer& shadowMap, const Vector3f& shadowCoord,
                                Float bias) const
    {
        Float blockerDepth = 0.f;
        Float numBlockers = 0.f;

        Float currentDepth = shadowCoord.z;

        for (int i = 0; i < PCSS_BLOCKER_SEARCH_NUM_SAMPLES; ++i)
        {
            Vector2f uv = shadowCoord.xy +
                          RandomVectorInUnitDisk().xy * PCSS_BLOCKER_SEARCH_FILTER_SIZE;
            Float sampleDepth = sampleShadowMap(shadowMap, uv);

            if (currentDepth > sampleDepth + bias)  // is blocker
            {
                blockerDepth += sampleDepth;
                numBlockers += 1.f;
            }
        }

        if (numBlockers < 1.f)  // return 0.0 if no blocker is found
            return 0.f;
        else
            return blockerDepth / numBlockers;
    }

#define AREA_LIGHT_SIZE 2.5f

    Float PCSS(const Buffer& shadowMap, const Vector3f& shadowCoord, Float bias) const
    {
        // 1. Average blocker depth
        Float dReceiver = shadowCoord.z;
        Float dBlocker = findAverageBlockDepth(shadowMap, shadowCoord, bias);

        if (dBlocker < Epsilon) return 1.f;

        // 2. Penumbra Size
        Float penumbraSize = (dReceiver - dBlocker) * AREA_LIGHT_SIZE / dBlocker;
        Float filterSize = PCF_FILTER_SIZE * penumbraSize;

        // 3. PCF Filtering
        return PCF(uShadowBuffer, shadowCoord, bias, filterSize);
    }

    // Calculate Shadow Component
    Float calculateShadowVisibility(const Vector3f& positionLightSpaceNDC,
                                    const Vector3f& normal,
                                    const Vector3f& lightDir) const
    {
        // Transform to [0, 1]
        Vector3f shadowCoord = positionLightSpaceNDC * 0.5f + Vector3f(0.5f);

        // Bias
        Float bias = Max(0.009f * (1.f - Dot(normal, lightDir)), 0.007f);
        Float visibility;

#ifdef SOFT_SHADOW_PCF
        // Percentage-Closer Filtering (PCF)
        visibility = PCF(uShadowBuffer, shadowCoord, bias, PCF_FILTER_SIZE);
#elif defined(SOFT_SHADOW_PCSS)
        // Percentage-Closer Soft Shadow (PCSS)
        visibility = PCSS(uShadowBuffer, shadowCoord, bias);
#else
        // Hard Shadow
        visibility = hardShadow(uShadowBuffer, shadowCoord, bias);
#endif

        return visibility;
    }
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
#endif
};

#endif  // SHADER_H_
