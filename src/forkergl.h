//
// Created by Junhao Wang (@Forkercat) on 2020/12/26.
//

#ifndef _FORKER_GL_H_
#define _FORKER_GL_H_

#include <vector>

#include "buffer.h"
#include "geometry.h"
#include "shader.h"

struct ForkerGL
{
    ForkerGL() = delete;

    enum RenderMode
    {
        Color,
        Shadow
    };

    // Buffers
    static Buffer FrameBuffer;
    static Buffer DepthBuffer;
    static Buffer ShadowBuffer;
    static Buffer ShadowDepthBuffer;

    static void InitFrameBuffers(int width, int height);
    static void InitShadowBuffers(int width, int height);

    // Update Status
    static void ClearColor(const Color3& color);
    static void Viewport(int x, int y, int w, int h);
    static void RenderMode(enum RenderMode mode);

    // Rasterization
    static void DrawTriangle(const Point4f ndcVerts[3], Shader& shader);
};

#endif  // _FORKER_GL_H_
