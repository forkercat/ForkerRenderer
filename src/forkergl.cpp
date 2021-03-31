//
// Created by Junhao Wang (@Forkercat) on 2020/12/26.
//

#include "forkergl.h"

#include "color.h"

// Buffers
Buffer ForkerGL::FrameBuffer;
Buffer ForkerGL::DepthBuffer;
Buffer ForkerGL::ShadowBuffer;
Buffer ForkerGL::ShadowDepthBuffer;

// Matrix
Matrix4x4f viewportMatrix = Matrix4x4f::Identity();

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
void ForkerGL::ClearColor(const Color3& color)
{
    FrameBuffer.PaintColor(color);
}

void ForkerGL::Viewport(int x, int y, int w, int h)
{
    viewportMatrix = Matrix4x4f::Identity();

    viewportMatrix[0][0] = w / 2.f;
    viewportMatrix[1][1] = h / 2.f;

    viewportMatrix[0][3] = x + w / 2.f;
    viewportMatrix[1][3] = y + h / 2.f;

    // for z
    viewportMatrix[2][2] = 1 / 2.f;
    viewportMatrix[2][3] = 1 / 2.f;
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

    static BoundBox GenerateBoundBox(const Point2i points[3], T bufferWidth, T bufferHeight)
    {
        T minX = Clamp(Min3(points[0].x, points[1].x, points[2].x), 0, bufferWidth - 1);
        T minY = Clamp(Min3(points[0].y, points[1].y, points[2].y), 0, bufferHeight - 1);
        T maxX = Clamp(Max3(points[0].x, points[1].x, points[2].x), 0, bufferWidth - 1);
        T maxY = Clamp(Max3(points[0].y, points[1].y, points[2].y), 0, bufferHeight - 1);
        return BoundBox<T>(minX, minY, maxX, maxY);
    }
};

// Rasterization
void ForkerGL::DrawTriangle(const Point4f ndcVerts[3], Shader& shader)
{
    // Viewport transformation
    Point2i points[3];  // screen coordinates
    Point3f depths;     // from 0 to 1
    for (int i = 0; i < 3; ++i)
    {
        Point3f coord = (viewportMatrix * ndcVerts[i]).xyz;
        points[i] = Point2i(coord.x, coord.y);
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
            Color3 frag;
            bool   discard = shader.ProcessFragment(bary, frag);
            if (discard) continue;

            if (renderMode == Color)
                FrameBuffer.Set(px, py, frag);
            else
                ShadowBuffer.SetValue(px, py, frag.z);
        }
    }
}
