//
// Created by Junhao Wang (@Forkercat) on 2021/5/5.
//

#pragma once

#include "geometry.h"

class Buffer;

// Enable Perspective Correct Mapping (PCI)
#define PERSPECTIVE_CORRECT_INTERPOLATION

// Enable Soft Shadow (Comment out both for hard shadow)
// #define SOFT_SHADOW_PCF  // Percentage-Closer Filtering (PCF)
#define SOFT_SHADOW_PCSS  // Percentage-Closer Soft Shadow (PCSS)

// Configurations
#define PCF_FILTER_SIZE 0.007
#define PCF_NUM_SAMPLES 64
#define PCSS_BLOCKER_SEARCH_NUM_SAMPLES 32
#define PCSS_BLOCKER_SEARCH_FILTER_SIZE 0.005
#define AREA_LIGHT_SIZE 2.5f

namespace Shadow
{
void SetShadowStatus(bool status);
bool GetShadowStatus();

Float SampleShadowMap(const Buffer& shadowMap, const Vector2f& uv);

Float HardShadow(const Buffer& shadowMap, const Vector3f& shadowCoord, Float bias);

// PCF
Float PCF(const Buffer& shadowMap, const Vector3f& shadowCoord, Float bias,
          Float filterSize);

// PCSS
Float FindAverageBlockDepth(const Buffer& shadowMap, const Vector3f& shadowCoord,
                            Float bias);
Float PCSS(const Buffer& shadowMap, const Vector3f& shadowCoord, Float bias);

Float CalculateShadowVisibility(const Buffer&   shadowMap,
                                const Vector3f& positionLightSpaceNDC,
                                const Vector3f& normal, const Vector3f& lightDir);
}  // namespace Shadow