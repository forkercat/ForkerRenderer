//
// Created by Junhao Wang (@Forkercat) on 2020/12/26.
//

#pragma once

#include "buffer.h"
#include "geometry.h"
#include "shader.h"
#include "texture.h"

class Scene;
class TGAImage;

struct ForkerGL
{
    ForkerGL() = delete;

    enum RenderMode
    {
        Forward,
        Deferred
    };

    enum PassType
    {
        ForwardPass,
        GeometryPass,
        LightingPass,
        ShadowPass
    };

    // Texture Wrap Mode & Filter Mode
    static Texture::WrapMode   TextureWrapping;
    static Texture::FilterMode TextureFiltering;

    static void TextureWrapMode(Texture::WrapMode wrapMode);
    static void TextureFilterMode(Texture::FilterMode filterMode);

    // Buffers
    static Buffer3f FrameBuffer;
    static Buffer1f DepthBuffer;
    static Buffer1f ShadowBuffer;
    static Buffer3f NormalGBuffer;  // G-Buffers
    static Buffer3f WorldPosGBuffer;
    static Buffer3f LightSpaceNDCPosGBuffer;
    static Buffer3f AlbedoGBuffer;
    static Buffer3f EmissiveGBuffer;
    static Buffer3f ParamGBuffer;
    static Buffer1f ShadingTypeGBuffer;
    static Buffer1f AmbientOcclusionGBuffer;  // SSAO

    // Images
    static TGAImage AntiAliasedImage;

    static void InitFrameBuffer(int width, int height);
    static void InitDepthBuffer(int width, int height);
    static void InitShadowBuffer(int width, int height);
    static void InitGeometryBuffers(int width, int height);

    // Update Status
    static void       ClearColor(const Color3& color);
    static void       SetViewportMatrix(int x, int y, int w, int h);
    static Matrix4x4f GetViewportMatrix();
    static void       SetViewProjectionMatrix(const Matrix4x4f& matrix);
    static Matrix4x4f GetViewProjectionMatrix();
    static void       SetLightSpaceMatrix(const Matrix4x4f& matrix);
    static Matrix4x4f GetLightSpaceMatrix();
    static void       SetRenderMode(enum RenderMode mode);
    static RenderMode GetRenderMode();
    static void       SetPassType(enum PassType type);

    // Rasterization
    static void DrawTriangle(const Point4f ndcVerts[3], Shader& shader);
    static void DrawScreenSpacePixels(const Scene& scene);

private:
    static void DrawTriangleSubTask(int xMin, int xMax, int yMin, int yMax,
                                    const Point2i points[3], Shader& shader,
                                    const Point3f& depths);
};
