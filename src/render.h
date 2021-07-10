//
// Created by Junhao Wang (@Forkercat) on 2021/6/21.
//

#pragma once

#include "camera.h"
#include "depthshader.h"
#include "forkergl.h"
#include "geometry.h"
#include "gshader.h"
#include "light.h"
#include "model.h"
#include "pbrshader.h"
#include "phongshader.h"
#include "scene.h"
#include "shadow.h"

namespace Render
{
// Configure
void Preconfigure(const Scene& scene);

// Render
void Render(const Scene& scene);

// Pass
void DoShadowPass(const Scene& scene);
void DoForwardPass(const Scene& scene);
void DoGeometryPass(const Scene& scene);
void DoLightingPass(const Scene& scene);

// SSAO
void DoSSAO(const Scene& scene);

// Anti-Aliasing
void DoSSAA(const Scene& scene);
}  // namespace Render
