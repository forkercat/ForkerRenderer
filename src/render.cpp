//
// Created by Junhao Wang (@Forkercat) on 2021/6/21.
//

#include "render.h"

#include <spdlog/spdlog.h>

static const Float s_ShadowViewSize = 3.0f;
static const Float s_ShadowNearPlane = 0.1f;
static const Float s_ShadowFarPlane = 20.f;

static const Float s_CameraNearPlane = 0.01f;
static const Float s_CameraFarPlane = 20.f;

static spdlog::stopwatch stepStopwatch;

namespace Render
{

inline int GetWidth(const Scene& scene)
{
    return scene.IsSSAAOn() ? scene.GetWidth() * scene.GetSSAAKernelSize()
                            : scene.GetWidth();
}

inline int GetHeight(const Scene& scene)
{
    return scene.IsSSAAOn() ? scene.GetHeight() * scene.GetSSAAKernelSize()
                            : scene.GetHeight();
}

void Preconfigure(const Scene& scene)
{
    ForkerGL::SetViewportMatrix(0, 0, GetWidth(scene), GetHeight(scene));
    ForkerGL::TextureWrapMode(Texture::NoWrap);     // or Repeat, ClampedToEdge, etc
    ForkerGL::TextureFilterMode(Texture::Nearest);  // or Linear
}

void Render(const Scene& scene)
{
    // Shadow Pass
    Render::DoShadowPass(scene);

    // Forward Rendering
    if (ForkerGL::GetRenderMode() == ForkerGL::Forward)
    {
        DoForwardPass(scene);
    }
    // Deferred Rendering
    else
    {
        DoGeometryPass(scene);
        DoLightingPass(scene);
    }
    // Anti-Aliasing
    DoSSAA(scene);
}

void DoShadowPass(const Scene& scene)
{
    spdlog::info("Shadow Pass:");

    if (Shadow::GetShadowStatus())
    {
        spdlog::info("  [Status] Enabled");
        // Buffer Configuration
        ForkerGL::InitShadowBuffer(GetWidth(scene), GetHeight(scene));
        ForkerGL::InitDepthBuffer(GetWidth(scene), GetHeight(scene));
        ForkerGL::SetPassType(ForkerGL::ShadowPass);

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
    }
    else
    {
        spdlog::info("  [Status] Disabled");
    }
    TimeElapsed(stepStopwatch, "Shadow Pass");
}

void DoForwardPass(const Scene& scene)
{
    ForkerGL::InitFrameBuffer(GetWidth(scene), GetHeight(scene));
    ForkerGL::InitDepthBuffer(GetWidth(scene), GetHeight(scene));
    ForkerGL::ClearColor(Color3(0.12f, 0.12f, 0.12f));
    ForkerGL::SetPassType(ForkerGL::ForwardPass);

    Float             ratio = scene.GetRatio();
    const Matrix4x4f& viewMatrix = scene.GetCamera().GetViewMatrix();
    const Matrix4x4f& projectionMatrix =
        (scene.GetProjectionType() == Camera::Orthographic)
            ? scene.GetCamera().GetOrthographicMatrix(-1.f * ratio, 1.f * ratio, -1.f,
                                                      1.f, s_CameraNearPlane,
                                                      s_CameraFarPlane)
            : scene.GetCamera().GetPerspectiveMatrix(45.f, ratio, s_CameraNearPlane,
                                                     s_CameraFarPlane);
    ForkerGL::SetViewProjectionMatrix(projectionMatrix * viewMatrix);

    for (int i = 0; i < scene.GetModelCount(); ++i)
    {
        const auto& model = scene.GetModel(i);

        if (!model.SupportPBR())
        {
            // Blinn-Phong Shading
            spdlog::info("Forward Pass (Blinn-Phong):");
            BlinnPhongShader bpShader;
            bpShader.uModelMatrix = scene.GetModelMatrix(i);
            bpShader.uViewMatrix = viewMatrix;
            bpShader.uProjectionMatrix = projectionMatrix;
            bpShader.uNormalMatrix = MakeNormalMatrix(bpShader.uModelMatrix);
            // Shader Configuration
            bpShader.uPointLight = scene.GetPointLight();
            bpShader.uEyePos = scene.GetCamera().GetPosition();
            if (Shadow::GetShadowStatus())
                bpShader.uLightSpaceMatrix = ForkerGL::GetLightSpaceMatrix();
            // Render
            model.Render(bpShader);
        }
        else
        {
            // PBR Shading
            spdlog::info("Forward Pass (PBR):");
            PBRShader pbrShader;
            pbrShader.uModelMatrix = scene.GetModelMatrix(i);
            pbrShader.uViewMatrix = viewMatrix;
            pbrShader.uProjectionMatrix = projectionMatrix;
            pbrShader.uNormalMatrix = MakeNormalMatrix(pbrShader.uModelMatrix);
            // Shader Configuration
            pbrShader.uPointLight = scene.GetPointLight();
            pbrShader.uEyePos = scene.GetCamera().GetPosition();
            if (Shadow::GetShadowStatus())
                pbrShader.uLightSpaceMatrix = ForkerGL::GetLightSpaceMatrix();
            // Render
            model.Render(pbrShader);
        }
    }
    TimeElapsed(stepStopwatch, "Forward Pass");
}

void DoGeometryPass(const Scene& scene)
{
    ForkerGL::InitGeometryBuffers(GetWidth(scene), GetHeight(scene));
    ForkerGL::InitDepthBuffer(GetWidth(scene), GetHeight(scene));
    ForkerGL::SetPassType(ForkerGL::GeometryPass);

    Float             ratio = scene.GetRatio();
    const Matrix4x4f& viewMatrix = scene.GetCamera().GetViewMatrix();
    const Matrix4x4f& projectionMatrix =
        (scene.GetProjectionType() == Camera::Orthographic)
            ? scene.GetCamera().GetOrthographicMatrix(-1.f * ratio, 1.f * ratio, -1.f,
                                                      1.f, s_CameraNearPlane,
                                                      s_CameraFarPlane)
            : scene.GetCamera().GetPerspectiveMatrix(45.f, ratio, s_CameraNearPlane,
                                                     s_CameraFarPlane);
    ForkerGL::SetViewProjectionMatrix(projectionMatrix *
                                      viewMatrix);  // used in lighting pass

    for (int i = 0; i < scene.GetModelCount(); ++i)
    {
        const auto& model = scene.GetModel(i);

        // Geometry Pass
        spdlog::info("Geometry Pass (Deferred):");
        GShader geometryShader;
        geometryShader.uModelMatrix = scene.GetModelMatrix(i);
        geometryShader.uViewMatrix = viewMatrix;
        geometryShader.uProjectionMatrix = projectionMatrix;
        geometryShader.uNormalMatrix = MakeNormalMatrix(geometryShader.uModelMatrix);
        geometryShader.uLightSpaceMatrix = ForkerGL::GetLightSpaceMatrix();
        // Render
        model.Render(geometryShader);
    }
    TimeElapsed(stepStopwatch, "Geometry Pass");
}

void DoLightingPass(const Scene& scene)
{
    spdlog::info("Lighting Pass (Deferred):");

    ForkerGL::InitFrameBuffer(GetWidth(scene), GetHeight(scene));
    ForkerGL::SetPassType(ForkerGL::LightingPass);
    ForkerGL::ClearColor(Color3(0.12f, 0.12f, 0.12f));  // this does not work

    // SSAO Effect
    if (scene.IsSSAOOn())
    {
        DoSSAO(scene);
    }

    ForkerGL::DrawScreenSpacePixels(scene);

    TimeElapsed(stepStopwatch, "Lighting Pass");
}

void DoSSAO(const Scene& scene)
{
    spdlog::info("* Applying SSAO in deferred shading...");

    // SSAO Parameters
    const int   numSample = 32;
    const Float kernelScale = 1.f / numSample;

    const Float radius = 0.075f;
    const bool rangeCheckEnabled = true;
    const Float rangeCheckRadius = 0.01f;  // based on radius

    int screenWidth = ForkerGL::FrameBuffer.GetWidth();
    int screenHeight = ForkerGL::FrameBuffer.GetHeight();

    for (int y = 0; y < screenHeight; ++y)
    {
        for (int x = 0; x < screenWidth; ++x)
        {
            Point3f  positionWS = ForkerGL::WorldPosGBuffer.GetValue(x, y);
            Vector3f normalWS = ForkerGL::NormalGBuffer.GetValue(x, y);
            Float    fragDepth = ForkerGL::DepthBuffer.GetValue(x, y);

            Float occlusion = 0.f;
            for (int s = 0; s < numSample; ++s)
            {
                Vector3f sampledDirection = RandomVectorInHemisphere(normalWS);
                Float    scale = sampledDirection.Length();
                scale = Lerp(0.1f, 1.0f, scale * scale);
                sampledDirection *= scale;

                Point3f sampledPositionWS = positionWS + sampledDirection * radius;
                Point4f sampledPositionCS = ForkerGL::GetViewProjectionMatrix() *
                                            Vector4f(sampledPositionWS, 1.f);
                Point4f sampledPositionNDC = sampledPositionCS / sampledPositionCS.w;
                Point3f sampledPositionSS =
                    (ForkerGL::GetViewportMatrix() * sampledPositionNDC).xyz;

                Point2i sampledScreenPosition =
                    Point2i(sampledPositionSS.x, sampledPositionSS.y);
                Float sampledDepth = sampledPositionSS.z;
                Float cachedDepth = ForkerGL::DepthBuffer.GetValue(
                    sampledScreenPosition.x, sampledScreenPosition.y);

                const Float bias = 0.0005f;

                if (sampledDepth >= cachedDepth + bias)
                {
                    if (rangeCheckEnabled)
                    {
                        Float rangeCheck =
                            std::abs(fragDepth - cachedDepth) < rangeCheckRadius ? 1.f
                                                                                 : 0.f;
                        // Another way
                        // Float rangeCheck = Smoothstep(
                        //     0.f, 1.f, rangeCheckRadius / std::abs(cachedDepth -
                        //     fragDepth));
                        occlusion += kernelScale * rangeCheck;
                    }
                    else
                    {
                        occlusion += kernelScale;
                    }
                }
            }

            occlusion = 1.f - occlusion;

            occlusion = Pow(occlusion, 3);

            ForkerGL::AmbientOcclusionGBuffer.SetValue(x, y, occlusion);
        }
    }

    ForkerGL::AmbientOcclusionGBuffer.TwoPassGaussianBlurDenoised();
}

void DoSSAA(const Scene& scene)
{
    spdlog::info("Anti-Aliasing (SSAA):");
    if (scene.IsSSAAOn())
    {
        spdlog::info("  [Status] Enabled");
        int kernelSize = scene.GetSSAAKernelSize();
        int kernelSizeSquared = kernelSize * kernelSize;

        const TGAImage& framebufferImage = ForkerGL::FrameBuffer.GenerateImage();
        TGAImage        antiAliasedImage(framebufferImage.GetWidth() / kernelSize,
                                  framebufferImage.GetHeight() / kernelSize,
                                  TGAImage::RGB);

        spdlog::info("  [Kernel Size] {}", kernelSize);
        spdlog::info("  [Sampling Size] {} x {}", framebufferImage.GetWidth(),
                     framebufferImage.GetHeight());
        spdlog::info("  [Output Size] {} x {}", antiAliasedImage.GetWidth(),
                     antiAliasedImage.GetHeight());

        for (int x = 0; x < antiAliasedImage.GetWidth(); ++x)
        {
            for (int y = 0; y < antiAliasedImage.GetHeight(); ++y)
            {
                int xx = x * kernelSize;
                int yy = y * kernelSize;
                int R = 0, G = 0, B = 0;
                for (int i = 0; i < kernelSize; ++i)
                {
                    for (int j = 0; j < kernelSize; ++j)
                    {
                        TGAColor c = framebufferImage.Get(xx + i, yy + j);
                        R += c.r;
                        G += c.g;
                        B += c.b;
                    }
                }
                R /= (Float)kernelSizeSquared;
                G /= (Float)kernelSizeSquared;
                B /= (Float)kernelSizeSquared;
                TGAColor color(R, G, B);
                antiAliasedImage.Set(x, y, color);
            }
        }

        ForkerGL::AntiAliasedImage = antiAliasedImage;
    }
    else
    {
        spdlog::info("  [Status] Disabled");
    }
    TimeElapsed(stepStopwatch, "Anti-Aliasing");
}

}  // namespace Render
