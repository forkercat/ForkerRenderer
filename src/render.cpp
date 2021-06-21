//
// Created by Junhao Wang (@Forkercat) on 2021/6/21.
//

#include "render.h"

#include <spdlog/spdlog.h>

// #define ANTI_ALIASING

static const Float s_ShadowViewSize = 3.0f;
static const Float s_ShadowNearPlane = 0.1f;
static const Float s_ShadowFarPlane = 20.f;

static const Float s_CameraNearPlane = 0.01f;
static const Float s_CameraFarPlane = 20.f;

/* Width of output will be (WIDTH / KERNEL_SIZE) */
static const Float s_SSAA_KernelSize = 2;

namespace Render
{

inline int GetWidth(const Scene& scene)
{
#ifdef ANTI_ALIASING
    return scene.GetWidth() * s_SSAA_KernelSize;
#else
    return scene.GetWidth();
#endif
}

inline int GetHeight(const Scene& scene)
{
#ifdef ANTI_ALIASING
    return scene.GetHeight() * s_SSAA_KernelSize;
#else
    return scene.GetHeight();
#endif
}

void Preconfigure(const Scene& scene)
{
    ForkerGL::Viewport(0, 0, GetWidth(scene), GetHeight(scene));
    ForkerGL::TextureWrapMode(Texture::NoWrap);     // or Repeat, ClampedToEdge, etc
    ForkerGL::TextureFilterMode(Texture::Nearest);  // or Linear
}

void DoShadowPass(const Scene& scene)
{
    spdlog::info("Shadow Pass:");
#ifdef SHADOW_PASS
    spdlog::info("  [Status] Enabled");
    // Buffer Configuration
    ForkerGL::InitShadowBuffer(GetWidth(scene), GetHeight(scene));
    ForkerGL::InitShadowDepthBuffer(GetWidth(scene), GetHeight(scene));
    ForkerGL::RenderMode(ForkerGL::ShadowPass);

    // Matrix
    Matrix4x4f viewMatrixSM =
        MakeLookAtMatrix(scene.GetPointLight().position, Vector3f(0.f));
    Matrix4x4f projMatrixSM = MakeOrthographicMatrix(
        -s_ShadowViewSize * scene.GetRatio(), s_ShadowViewSize * scene.GetRatio(),
        -s_ShadowViewSize, s_ShadowViewSize, s_ShadowNearPlane, s_ShadowFarPlane);
    ForkerGL::SetLightSpaceMatrix(projMatrixSM * viewMatrixSM);

    for (int i = 0; i < scene.GetModelCount(); ++i)
    {
        const auto& model = scene.GetModel(i);

        // Depth Shading
        DepthShader depthShader;
        depthShader.uModelMatrix = scene.GetModelMatrix(i);
        depthShader.uLightSpaceMatrix = ForkerGL::GetLightSpaceMatrix();
        // Render
        model.Render(depthShader);
    }
#else
    spdlog::info("  [Status] Disabled");
#endif
}

void DoGeometryPass(const Scene& scene)
{
    ForkerGL::InitGeometryBuffers(GetWidth(scene), GetHeight(scene));
    ForkerGL::InitDepthBuffer(GetWidth(scene), GetHeight(scene));
    ForkerGL::RenderMode(ForkerGL::GeometryPass);

    Float ratio = scene.GetRatio();

    for (int i = 0; i < scene.GetModelCount(); ++i)
    {
        const auto& model = scene.GetModel(i);

        // Geometry Pass
        spdlog::info("Geometry Pass:");
        GeometryShader geometryShader;
        geometryShader.uModelMatrix = scene.GetModelMatrix(i);
        geometryShader.uViewMatrix = scene.GetCamera().GetViewMatrix();
        geometryShader.uNormalMatrix = MakeNormalMatrix(geometryShader.uModelMatrix);
        geometryShader.uProjectionMatrix =
            (scene.GetProjectionType() == Camera::Orthographic)
                ? scene.GetCamera().GetOrthographicMatrix(-1.f * ratio, 1.f * ratio, -1.f,
                                                          1.f, s_CameraNearPlane,
                                                          s_CameraFarPlane)
                : scene.GetCamera().GetPerspectiveMatrix(45.f, ratio, s_CameraNearPlane,
                                                         s_CameraFarPlane);
        // Render
        model.Render(geometryShader);
    }
}

void DoLightingPass(const Scene& scene)
{
    ForkerGL::InitFrameBuffer(GetWidth(scene), GetHeight(scene));
    ForkerGL::InitDepthBuffer(GetWidth(scene), GetHeight(scene));
    ForkerGL::ClearColor(Color3(0.12f, 0.12f, 0.12f));
    ForkerGL::RenderMode(ForkerGL::LightingPass);

    Float ratio = scene.GetRatio();

    for (int i = 0; i < scene.GetModelCount(); ++i)
    {
        const auto& model = scene.GetModel(i);

        if (!model.SupportPBR())
        {
            // Blinn-Phong Shading
            spdlog::info("Lighting Pass (Blinn-Phong):");
            BlinnPhongShader bpShader;
            bpShader.uModelMatrix = scene.GetModelMatrix(i);
            bpShader.uViewMatrix = scene.GetCamera().GetViewMatrix();
            bpShader.uNormalMatrix = MakeNormalMatrix(bpShader.uModelMatrix);
            bpShader.uProjectionMatrix =
                (scene.GetProjectionType() == Camera::Orthographic)
                    ? scene.GetCamera().GetOrthographicMatrix(
                          -1.f * ratio, 1.f * ratio, -1.f, 1.f, s_CameraNearPlane,
                          s_CameraFarPlane)
                    : scene.GetCamera().GetPerspectiveMatrix(
                          45.f, ratio, s_CameraNearPlane, s_CameraFarPlane);

            // Shader Configuration
            bpShader.uPointLight = scene.GetPointLight();
            bpShader.uEyePos = scene.GetCamera().GetPosition();
#ifdef SHADOW_PASS
            bpShader.uLightSpaceMatrix = ForkerGL::GetLightSpaceMatrix();
#endif
            // Render
            model.Render(bpShader);
        }
        else
        {
            // PBR Shading
            spdlog::info("Lighting Pass (PBR):");
            PBRShader pbrShader;
            pbrShader.uModelMatrix = scene.GetModelMatrix(i);
            pbrShader.uViewMatrix = scene.GetCamera().GetViewMatrix();
            pbrShader.uNormalMatrix = MakeNormalMatrix(pbrShader.uModelMatrix);
            pbrShader.uProjectionMatrix =
                (scene.GetProjectionType() == Camera::Orthographic)
                    ? scene.GetCamera().GetOrthographicMatrix(
                          -1.f * ratio, 1.f * ratio, -1.f, 1.f, s_CameraNearPlane,
                          s_CameraFarPlane)
                    : scene.GetCamera().GetPerspectiveMatrix(
                          45.f, ratio, s_CameraNearPlane, s_CameraFarPlane);

            // Shader Configuration
            pbrShader.uPointLight = scene.GetPointLight();
            pbrShader.uEyePos = scene.GetCamera().GetPosition();
#ifdef SHADOW_PASS
            pbrShader.uLightSpaceMatrix = ForkerGL::GetLightSpaceMatrix();
#endif
            // Render
            model.Render(pbrShader);
        }
    }
}

void DoSSAA()
{
    spdlog::info("Anti-Aliasing (SSAA):");
#ifdef ANTI_ALIASING
    spdlog::info("  [Status] Enabled");
    const TGAImage& framebufferImage = ForkerGL::FrameBuffer.GenerateImage();
    TGAImage        antiAliasedImage(framebufferImage.GetWidth() / s_SSAA_KernelSize,
                              framebufferImage.GetHeight() / s_SSAA_KernelSize,
                              TGAImage::RGB);

    spdlog::info("  [Kernel Size] {}", s_SSAA_KernelSize);
    spdlog::info("  [Sampling Size] {} x {}", framebufferImage.GetWidth(),
                 framebufferImage.GetHeight());
    spdlog::info("  [Output Size] {} x {}", antiAliasedImage.GetWidth(),
                 antiAliasedImage.GetHeight());

    for (int x = 0; x < antiAliasedImage.GetWidth(); ++x)
    {
        for (int y = 0; y < antiAliasedImage.GetHeight(); ++y)
        {
            int xx = x * s_SSAA_KernelSize;
            int yy = y * s_SSAA_KernelSize;
            int R = 0, G = 0, B = 0;
            for (int i = 0; i < s_SSAA_KernelSize; ++i)
            {
                for (int j = 0; j < s_SSAA_KernelSize; ++j)
                {
                    TGAColor c = framebufferImage.Get(xx + i, yy + j);
                    R += c.r;
                    G += c.g;
                    B += c.b;
                }
            }
            R /= (Float)s_SSAA_KernelSize * s_SSAA_KernelSize;
            G /= (Float)s_SSAA_KernelSize * s_SSAA_KernelSize;
            B /= (Float)s_SSAA_KernelSize * s_SSAA_KernelSize;
            TGAColor color(R, G, B);
            antiAliasedImage.Set(x, y, color);
        }
    }

    ForkerGL::AntiAliasedImage = antiAliasedImage;
#else
    spdlog::info("  [Status] Disabled");
#endif
}

}  // namespace Render
