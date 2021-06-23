//
// Created by Junhao Wang (@Forkercat) on 2021/5/5.
//

#include "shadow.h"

#include "buffer.h"

static bool s_IsShadowOn = true;

namespace Shadow
{
void SetShadowStatus(bool status)
{
    s_IsShadowOn = status;
}

bool GetShadowStatus()
{
    return s_IsShadowOn;
}

Float SampleShadowMap(const Buffer1f& shadowMap, const Vector2f& uv)
{
    // Fix region out of map
    if (uv.x < 0.f || uv.x > 1.f || uv.y < 0.f || uv.y > 1.f) return Infinity;

    int   w = shadowMap.GetWidth() - 0.001f;
    int   h = shadowMap.GetHeight() - 0.001f;
    int   u = (int)((float)w * uv.x);
    int   v = (int)((float)h * uv.y);
    Float depth = shadowMap.GetValue(u, v);

    return depth < Epsilon ? 1.f : depth;  // fix background depth
}

// Hard Shadow
Float HardShadow(const Buffer1f& shadowMap, const Vector3f& shadowCoord, Float bias)
{
    Float visibility;
    Float sampledDepth = SampleShadowMap(shadowMap, shadowCoord.xy);
    Float currentDepth = shadowCoord.z;
    visibility = (currentDepth <= sampledDepth + bias) ? 1.f : 0.f;
    return visibility;
}

Float PCF(const Buffer1f& shadowMap, const Vector3f& shadowCoord, Float bias,
          Float filterSize)
{
    Float visibility = 0.f;
    Float currentDepth = shadowCoord.z;
    Float invPCFNumSamples = 1.f / (Float)PCF_NUM_SAMPLES;

    for (int i = 0; i < PCF_NUM_SAMPLES; ++i)
    {
        Vector2f uv = shadowCoord.xy + RandomVectorInUnitDisk().xy * filterSize;
        Float    sampleDepth = SampleShadowMap(shadowMap, uv);

        if (currentDepth <= sampleDepth + bias) visibility += invPCFNumSamples;
    }

    return visibility;
}

Float FindAverageBlockDepth(const Buffer1f& shadowMap, const Vector3f& shadowCoord,
                            Float bias)
{
    Float blockerDepth = 0.f;
    Float numBlockers = 0.f;

    Float currentDepth = shadowCoord.z;

    for (int i = 0; i < PCSS_BLOCKER_SEARCH_NUM_SAMPLES; ++i)
    {
        Vector2f uv = shadowCoord.xy +
                      RandomVectorInUnitDisk().xy * PCSS_BLOCKER_SEARCH_FILTER_SIZE;
        Float sampleDepth = SampleShadowMap(shadowMap, uv);

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

Float PCSS(const Buffer1f& shadowMap, const Vector3f& shadowCoord, Float bias)
{
    // 1. Average blocker depth
    Float dReceiver = shadowCoord.z;
    Float dBlocker = FindAverageBlockDepth(shadowMap, shadowCoord, bias);

    if (dBlocker < Epsilon) return 1.f;

    // 2. Penumbra Size
    Float penumbraSize = (dReceiver - dBlocker) * AREA_LIGHT_SIZE / dBlocker;
    Float filterSize = PCF_FILTER_SIZE * penumbraSize;

    // 3. PCF Filtering
    return PCF(shadowMap, shadowCoord, bias, filterSize);
}

// Calculate Shadow Component
Float CalculateShadowVisibility(const Buffer1f&   shadowMap,
                                const Vector3f& positionLightSpaceNDC,
                                const Vector3f& normal, const Vector3f& lightDir)
{
    // Transform to [0, 1]
    Vector3f shadowCoord = positionLightSpaceNDC * 0.5f + Vector3f(0.5f);

    // Bias
    Float bias = Max(0.009f * (1.f - Dot(normal, lightDir)), 0.007f);
    Float visibility;

#ifdef SOFT_SHADOW_PCF
    // Percentage-Closer Filtering (PCF)
    visibility = PCF(shadowMap, shadowCoord, bias, PCF_FILTER_SIZE);
#elif defined(SOFT_SHADOW_PCSS)
    // Percentage-Closer Soft Shadow (PCSS)
    visibility = PCSS(shadowMap, shadowCoord, bias);
#else
    // Hard Shadow
    visibility = HardShadow(shadowMap, shadowCoord, bias);
#endif

    return visibility;
}
}  // namespace Shadow