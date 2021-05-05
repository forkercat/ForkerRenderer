//
// Created by Junhao Wang (@Forkercat) on 2021/5/5.
//

#ifndef SHADERS_SHADOW_H_
#define SHADERS_SHADOW_H_

#include "geometry.h"

class Buffer;

// Enable Perspective Correct Mapping (PCI)
#define PERSPECTIVE_CORRECT_INTERPOLATION

// Enable Shadow Mapping (Hard Shadow)
#define SHADOW_PASS

// Enable Soft Shadow
// #define SOFT_SHADOW_PCF  // Percentage-Closer Filtering (PCF)
#define SOFT_SHADOW_PCSS  // Percentage-Closer Soft Shadow (PCSS)

// Configurations
#define PCF_FILTER_SIZE 0.007
#define PCF_NUM_SAMPLES 64
#define PCSS_BLOCKER_SEARCH_NUM_SAMPLES 32
#define PCSS_BLOCKER_SEARCH_FILTER_SIZE 0.005
#define AREA_LIGHT_SIZE 2.5f

Float sampleShadowMap(const Buffer& shadowMap, const Vector2f& uv);

Float hardShadow(const Buffer& shadowMap, const Vector3f& shadowCoord, Float bias);

// PCF
Float PCF(const Buffer& shadowMap, const Vector3f& shadowCoord, Float bias,
          Float filterSize);

// PCSS
Float findAverageBlockDepth(const Buffer& shadowMap, const Vector3f& shadowCoord,
                            Float bias);
Float PCSS(const Buffer& shadowMap, const Vector3f& shadowCoord, Float bias);

Float calculateShadowVisibility(const Buffer&   shadowMap,
                                const Vector3f& positionLightSpaceNDC,
                                const Vector3f& normal, const Vector3f& lightDir);

#endif  // SHADERS_SHADOW_H_
