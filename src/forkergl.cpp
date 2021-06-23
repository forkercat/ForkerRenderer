//
// Created by Junhao Wang (@Forkercat) on 2020/12/26.
//

#include "forkergl.h"

#include <thread>

#include "color.h"
#include "gshader.h"
#include "pbrshader.h"
#include "phongshader.h"
#include "scene.h"
#include "tgaimage.h"

// Texture Wrapping & Filtering
Texture::WrapMode   ForkerGL::TextureWrapping = Texture::WrapMode::NoWrap;
Texture::FilterMode ForkerGL::TextureFiltering = Texture::FilterMode::Nearest;

// Buffers
Buffer3f ForkerGL::FrameBuffer;  // Lighting Pass & Forward Pass
Buffer1f ForkerGL::DepthBuffer;
Buffer1f ForkerGL::ShadowBuffer;  // Shadow Pass
Buffer3f ForkerGL::NormalGBuffer;  // Geometry Pass
Buffer3f ForkerGL::WorldPosGBuffer;
Buffer3f ForkerGL::LightSpaceNDCPosGBuffer;
Buffer3f ForkerGL::AlbedoGBuffer;
Buffer3f ForkerGL::EmissiveGBuffer;
Buffer3f ForkerGL::ParamGBuffer;
Buffer1f ForkerGL::ShadingTypeGBuffer;

// Images
TGAImage ForkerGL::AntiAliasedImage;

// Matrix
Matrix4x4f viewportMatrix = Matrix4x4f::Identity();
Matrix4x4f lightSpaceMatrix = Matrix4x4f::Identity();

// Rendering
enum ForkerGL::RenderMode renderMode = ForkerGL::Forward;
enum ForkerGL::PassType   passType = ForkerGL::ForwardPass;

// Texture Wrap Mode & Filter Mode
void ForkerGL::TextureWrapMode(Texture::WrapMode wrapMode)
{
    ForkerGL::TextureWrapping = wrapMode;
}

void ForkerGL::TextureFilterMode(Texture::FilterMode filterMode)
{
    ForkerGL::TextureFiltering = filterMode;
}

// Buffer Initialization
void ForkerGL::InitFrameBuffer(int width, int height)
{
    FrameBuffer = Buffer3f(width, height, Buffer::Zero);
}

void ForkerGL::InitDepthBuffer(int width, int height)
{
    DepthBuffer = Buffer1f(width, height, Buffer::MaxPositive);
}

void ForkerGL::InitShadowBuffer(int width, int height)
{
    ShadowBuffer = Buffer1f(width, height, Buffer::Zero);
}

void ForkerGL::InitGeometryBuffers(int width, int height)
{
    NormalGBuffer = Buffer3f(width, height, Buffer::Zero);
    WorldPosGBuffer = Buffer3f(width, height, Buffer::Zero);
    if (Shadow::GetShadowStatus())
        LightSpaceNDCPosGBuffer = Buffer3f(width, height, Buffer::Zero);
    AlbedoGBuffer = Buffer3f(width, height, Buffer::Zero);
    EmissiveGBuffer = Buffer3f(width, height, Buffer::Zero);
    ParamGBuffer = Buffer3f(width, height, Buffer::Zero);
    ShadingTypeGBuffer = Buffer1f(width, height, Buffer::Zero);
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

void ForkerGL::SetLightSpaceMatrix(const Matrix4x4f& matrix)
{
    lightSpaceMatrix = matrix;
}

Matrix4x4f ForkerGL::GetLightSpaceMatrix()
{
    return lightSpaceMatrix;
}

void ForkerGL::SetRenderMode(enum RenderMode mode)
{
    renderMode = mode;
}

ForkerGL::RenderMode ForkerGL::GetRenderMode()
{
    return renderMode;
}

void ForkerGL::SetPassType(enum PassType type)
{
    passType = type;
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

    static BoundBox GenerateBoundBox(const Point2i points[3], T bufferWidth,
                                     T bufferHeight)
    {
        T minX = Clamp(Min3(points[0].x, points[1].x, points[2].x), 0, bufferWidth - 1);
        T minY = Clamp(Min3(points[0].y, points[1].y, points[2].y), 0, bufferHeight - 1);
        T maxX = Clamp(Max3(points[0].x, points[1].x, points[2].x), 0, bufferWidth - 1);
        T maxY = Clamp(Max3(points[0].y, points[1].y, points[2].y), 0, bufferHeight - 1);
        return BoundBox<T>(minX, minY, maxX, maxY);
    }
};

void ForkerGL::DrawTriangleSubTask(int xMin, int xMax, int yMin, int yMax,
                                   const Point2i points[3], Shader& shader,
                                   const Point3f& depths)
{
    for (int px = xMin; px <= xMax; ++px)
    {
        for (int py = yMin; py <= yMax; ++py)
        {
            Vector3f bary =
                Barycentric(points[0], points[1], points[2], Vector2i(px, py));

            // Inside Triangle Test
            if (bary.x < 0) continue;

            // Depth Test
            Float currentDepth = Dot(bary, depths);

            if (passType == ForwardPass)
            {
                if (currentDepth >= DepthBuffer.GetValue(px, py)) continue;
                DepthBuffer.SetValue(px, py, currentDepth);
            }
            else if (passType == GeometryPass)
            {
                if (currentDepth >= DepthBuffer.GetValue(px, py)) continue;
                DepthBuffer.SetValue(px, py, currentDepth);
            }
            else if (passType == LightingPass)
            {
                // Do nothing
            }
            else if (passType == ShadowPass)
            {
                if (currentDepth >= DepthBuffer.GetValue(px, py)) continue;
                DepthBuffer.SetValue(px, py, currentDepth);
            }

            // Fragment Shader
            Color3 frag;
            bool   discard = shader.ProcessFragment(bary, frag);
            if (discard) continue;

            if (passType == ForwardPass)
            {
                FrameBuffer.SetValue(px, py, frag);
            }
            else if (passType == GeometryPass)
            {
                // Do Nothing
                GShader& geometryShader = dynamic_cast<GShader&>(shader);
                NormalGBuffer.SetValue(px, py, geometryShader.outNormalWS);
                WorldPosGBuffer.SetValue(px, py, geometryShader.outPositionWS);
                if (Shadow::GetShadowStatus())
                    LightSpaceNDCPosGBuffer.SetValue(px, py, geometryShader.outLightSpaceNDC);
                AlbedoGBuffer.SetValue(px, py, geometryShader.outAlbedo);
                EmissiveGBuffer.SetValue(px, py, geometryShader.outEmissive);
                ParamGBuffer.SetValue(px, py, geometryShader.outParam);
                ShadingTypeGBuffer.SetValue(px, py, geometryShader.outShadingType);
            }
            else if (passType == LightingPass)
            {
                // Do nothing
            }
            else if (passType == ShadowPass)
            {
                ShadowBuffer.SetValue(px, py, frag.z);
            }
        }
    }
}

#define NUM_THREADS 4

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
    int w = (passType != ShadowPass) ? DepthBuffer.GetWidth() : ShadowBuffer.GetWidth();
    int h = (passType != ShadowPass) ? DepthBuffer.GetHeight() : ShadowBuffer.GetHeight();

    BoundBox<int> bbox = BoundBox<int>::GenerateBoundBox(points, w, h);

    if (true)  // single thread
    {
        DrawTriangleSubTask(bbox.MinX, bbox.MaxX, bbox.MinY, bbox.MaxY, points, shader,
                            depths);
    }
    else
    {
        int numTasks = bbox.MaxY - bbox.MinY + 1;

        if (numTasks < 5)
        {
            DrawTriangleSubTask(bbox.MinX, bbox.MaxX, bbox.MinY, bbox.MaxY, points,
                                shader, depths);
        }
        else
        {
            std::thread drawThreads[NUM_THREADS];

            int numTaskPerThread = std::round(numTasks / (Float)NUM_THREADS);



            // allocated threads
            // int numAllocatedThreads = 0;
            //
            // for (int t = 0; t < NUM_THREADS; ++t)
            // {
            //     int xStart = Max(bbox.MinX + t * numTaskPerThread, bbox.MinX);
            //     int xEnd = Min(xStart + numTaskPerThread - 1, bbox.MaxX);
            //
            //     drawThreads[t] = std::thread(DrawTriangleSubTask, xStart, xEnd, bbox.MinY, bbox.MaxY, std::ref(points), std::ref(shader), std::ref(depths));
            //     ++numAllocatedThreads;
            //
            //     if (xEnd == bbox.MaxX)
            //     {
            //         break;
            //     }
            // }
            //
            // for (int t = 0; t < numAllocatedThreads; ++t)
            // {
            //     drawThreads[t].join();
            // }

            int start = bbox.MinX;
            int end = bbox.MaxX;

            drawThreads[0] = std::thread(DrawTriangleSubTask, start, end / NUM_THREADS, bbox.MinY, bbox.MaxY, std::ref(points), std::ref(shader), std::ref(depths));

            drawThreads[1] = std::thread(DrawTriangleSubTask, end / NUM_THREADS + 1, end / NUM_THREADS * 2, bbox.MinY, bbox.MaxY, std::ref(points), std::ref(shader), std::ref(depths));

            drawThreads[2] = std::thread(DrawTriangleSubTask, end / NUM_THREADS * 2 + 1, end / NUM_THREADS * 3, bbox.MinY, bbox.MaxY, std::ref(points), std::ref(shader), std::ref(depths));

            drawThreads[3] = std::thread(DrawTriangleSubTask, end / NUM_THREADS * 3 + 1, end, bbox.MinY, bbox.MaxY, std::ref(points), std::ref(shader), std::ref(depths));

            drawThreads[0].join();
            drawThreads[1].join();
            drawThreads[2].join();
            drawThreads[3].join();

            // Join
            // for (int t = 0; t < NUM_THREADS; ++t)
            // {
            //     drawThreads[t].join();
            // }
        }
    }
}

void ForkerGL::DrawScreenSpacePixels(const Scene& scene)
{
    // Data Preparation
    Point3f eyePos = scene.GetCamera().GetPosition();
    Point3f lightPos = scene.GetPointLight().position;
    Color3  lightRadiance = scene.GetPointLight().color;
    int     screenWidth = ForkerGL::FrameBuffer.GetWidth();
    int     screenHeight = ForkerGL::FrameBuffer.GetHeight();

    // For Loop Each Pixel
    for (int y = 0; y < screenHeight; ++y)
    {
        for (int x = 0; x < screenWidth; ++x)
        {
            // Data Preparation
            Point3f  positionWS = ForkerGL::WorldPosGBuffer.GetValue(x, y);
            Vector3f normalWS = ForkerGL::NormalGBuffer.GetValue(x, y);
            Point3f  lightSpaceNDC = ForkerGL::LightSpaceNDCPosGBuffer.GetValue(x, y);
            Color3   albedo = ForkerGL::AlbedoGBuffer.GetValue(x, y);
            Color3   emissive = ForkerGL::EmissiveGBuffer.GetValue(x, y);
            Vector3f param = ForkerGL::ParamGBuffer.GetValue(x, y);
            Float    shadingType = ForkerGL::ShadingTypeGBuffer.GetValue(x, y);

            // Directions
            Vector3f lightDir = Normalize(lightPos - positionWS);
            Vector3f viewDir = Normalize(eyePos - positionWS);

            // Shadow Mapping
            Float visibility = 0.f;
            if (Shadow::GetShadowStatus())
            {
                visibility = Shadow::CalculateShadowVisibility(ForkerGL::ShadowBuffer, lightSpaceNDC, normalWS, lightDir);
            }

            Color3 color;
            if (shadingType < 0.5f)  // Non-PBR (Blinn-Phong Shading)
            {
                color = BlinnPhongShader::CalculateLight(lightDir, viewDir, normalWS, visibility, albedo, emissive, param, lightRadiance);
            }
            else  // PBR
            {
                Vector3f halfwayDir = Normalize(lightDir + viewDir);
                color = PBRShader::CalculateLight(lightDir, viewDir, halfwayDir, normalWS, visibility, albedo, emissive, param, lightRadiance);
            }

            ForkerGL::FrameBuffer.SetValue(x, y, color);
        }
    }
}