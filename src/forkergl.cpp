//
// Created by Junhao Wang (@Forkercat) on 2020/12/26.
//

#include "forkergl.h"

// Buffers
Buffer ForkerGL::FrameBuffer;
Buffer ForkerGL::DepthBuffer;
Buffer ForkerGL::ShadowBuffer;
Buffer ForkerGL::ShadowDepthBuffer;

// Matrix
Matrix4f viewportMatrix = Matrix4f::Identity();

// RenderMode
enum ForkerGL::RenderMode renderMode = ForkerGL::Color;

// Buffer Initialization
void ForkerGL::InitFrameBuffers(int width, int height)
{
    FrameBuffer = Buffer(width, height, Buffer::Zero);
    DepthBuffer = Buffer(width, height, Buffer::MaxFloat32);
}

void ForkerGL::InitShadowBuffers(int width, int height)
{
    ShadowBuffer = Buffer(width, height, Buffer::Zero);
    ShadowDepthBuffer = Buffer(width, height, Buffer::MaxFloat32);
}

// Status Configuration
void ForkerGL::ClearColor(const TGAColor& color)
{
    FrameBuffer.PaintColor(color);
}

void ForkerGL::Viewport(int x, int y, int w, int h)
{
    viewportMatrix = Matrix4f::Identity();

    viewportMatrix[0][0] = w / 2.0f;
    viewportMatrix[1][1] = h / 2.0f;

    viewportMatrix[0][3] = x + w / 2.0f;
    viewportMatrix[1][3] = y + h / 2.0f;

    // for z
    viewportMatrix[2][2] = 1 / 2.0f;
    viewportMatrix[2][3] = 1 / 2.0f;
}

void ForkerGL::RenderMode(enum RenderMode mode)
{
    renderMode = mode;
}

// BoundBox Definition
template <typename T>
struct BoundBox
{
    T MinX, MinY, MaxX, MaxY;
    BoundBox(T minX, T minY, T maxX, T maxY)
        : MinX(minX), MinY(minY), MaxX(maxX), MaxY(maxY)
    {
    }

    static BoundBox GenerateBoundBox(const Vector2i points[3], T bufferWidth, T bufferHeight)
    {
        T minX = Clamp(Min3(points[0].x, points[1].x, points[2].x), 0, bufferWidth - 1);
        T minY = Clamp(Min3(points[0].y, points[1].y, points[2].y), 0, bufferHeight - 1);
        T maxX = Clamp(Max3(points[0].x, points[1].x, points[2].x), 0, bufferWidth - 1);
        T maxY = Clamp(Max3(points[0].y, points[1].y, points[2].y), 0, bufferHeight - 1);
        return BoundBox<T>(minX, minY, maxX, maxY);
    }
};

// Rasterization
void ForkerGL::DrawTriangle(const Vector4f ndcVerts[3], Shader& shader)
{
    // Viewport transformation
    Vector2i points[3];  // screen coordinates
    Vector3f depths;     // from 0 to 1
    for (int i = 0; i < 3; ++i)
    {
        Vector3f coord = (viewportMatrix * ndcVerts[i]).xyz;
        points[i] = Vector2i(coord.x, coord.y);
        depths[i] = coord.z;
    }

    // Bounding Box
    int w = (renderMode == Color) ? FrameBuffer.GetWidth() : ShadowBuffer.GetWidth();
    int h = (renderMode == Color) ? FrameBuffer.GetHeight() : ShadowBuffer.GetHeight();
    BoundBox<int> bbox = BoundBox<int>::GenerateBoundBox(points, w, h);

    for (int px = bbox.MinX; px <= bbox.MaxX; ++px)
    {
        for (int py = bbox.MinY; py <= bbox.MaxY; ++py)
        {
            Vector3f bary = Barycentric(points[0], points[1], points[2], Vector2i(px, py));

            // Inside Triangle Test
            if (bary.x < 0) continue;

            // Depth Test
            Float currentDepth = Dot(bary, depths);
            if (renderMode == Color)  // distinguish between render color or just depth value
            {
                if (currentDepth >= DepthBuffer.GetValue(px, py)) continue;
                DepthBuffer.SetValue(px, py, currentDepth);
            }
            else
            {
                if (currentDepth >= ShadowDepthBuffer.GetValue(px, py)) continue;
                ShadowDepthBuffer.SetValue(px, py, currentDepth);
            }

            // Fragment Shader
            Vector3f frag;
            bool  discard = shader.ProcessFragment(bary, frag);
            if (discard) continue;

            if (renderMode == Color)
                FrameBuffer.Set(px, py,
                                Vector3i(frag.r * 255, frag.g * 255, frag.b * 255));
            else
                ShadowBuffer.SetValue(px, py, frag.z);
        }
    }
}
