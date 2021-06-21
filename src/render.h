//
// Created by Junhao Wang (@Forkercat) on 2021/6/21.
//

#pragma once

#include "camera.h"
#include "depthshader.h"
#include "forkergl.h"
#include "geometry.h"
#include "light.h"
#include "model.h"
#include "pbrshader.h"
#include "phongshader.h"
#include "geometryshader.h"
#include "scene.h"

namespace Render
{
// Configure
void Preconfigure(const Scene& scene);

// Pass
void DoShadowPass(const Scene& scene);
void DoGeometryPass(const Scene& scene);
void DoLightingPass(const Scene& scene);

// Anti-Aliasing
void DoSSAA();
}  // namespace Render
