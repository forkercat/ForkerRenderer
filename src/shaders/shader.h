//
// Created by Junhao Wang (@Forkercat) on 2020/12/26.
//

#pragma once

#include <utility>

#include "buffer.h"
#include "color.h"
#include "geometry.h"
#include "light.h"
#include "mesh.h"
#include "model.h"
#include "tgaimage.h"
#include "shadow.h"

// Abstract Class
struct Shader
{
    std::shared_ptr<const Mesh> mesh;

    Shader() : mesh(nullptr) { }
    virtual ~Shader() { }

    // Use Shader Program (set which mesh to shade on)
    void Use(std::shared_ptr<const Mesh> m) { mesh = m; }
    // Vertex Shader
    virtual Point4f ProcessVertex(int faceIdx, int vertIdx) = 0;
    // Fragment Shader
    virtual bool ProcessFragment(const Vector3f& baryCoord, Color3& gl_Color) = 0;
};
