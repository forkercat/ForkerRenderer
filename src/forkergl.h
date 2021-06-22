//
// Created by Junhao Wang (@Forkercat) on 2020/12/26.
//

#pragma once

#include <vector>

#include "buffer.h"
#include "geometry.h"
#include "shader.h"
#include "texture.h"

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
    static Buffer FrameBuffer;
    static Buffer DepthBuffer;
    static Buffer ShadowBuffer;
    static Buffer ShadowDepthBuffer;
    static Buffer NormalGBuffer;  // G-Buffers
    static Buffer WorldPosGBuffer;
    static Buffer AlbedoGBuffer;
    static Buffer EmissiveGBuffer;
    static Buffer ParamGBuffer;
    static Buffer ShadingTypeGBuffer;

    // Images
    static TGAImage AntiAliasedImage;

    static void InitFrameBuffer(int width, int height);
    static void InitDepthBuffer(int width, int height);
    static void InitShadowBuffer(int width, int height);
    static void InitShadowDepthBuffer(int width, int height);
    static void InitGeometryBuffers(int width, int height);

    // Update Status
    static void       ClearColor(const Color3& color);
    static void       Viewport(int x, int y, int w, int h);
    static void       SetLightSpaceMatrix(const Matrix4x4f& matrix);
    static Matrix4x4f GetLightSpaceMatrix();
    static void       SetRenderMode(enum RenderMode mode);
    static RenderMode       GetRenderMode();
    static void             SetPassType(enum PassType type);

    // Rasterization
    static void DrawTriangle(const Point4f ndcVerts[3], Shader& shader);

private:
    static void DrawTriangleSubTask(int xMin, int xMax, int yMin, int yMax,
                                    const Point2i points[3], Shader& shader,
                                    const Point3f& depths);
};
