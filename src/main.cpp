#include <spdlog/spdlog.h>
#include <spdlog/stopwatch.h>

#include "camera.h"
#include "depthshader.h"
#include "forkergl.h"
#include "geometry.h"
#include "light.h"
#include "model.h"
#include "pbrshader.h"
#include "phongshader.h"
#include "scene.h"
#include "tgaimage.h"

/////////////////////////////////////////////////////////////////////////////////

// #define ANTI_ALIASING
#define KERNEL_SIZE 2  // width of output will be (WIDTH / KERNEL_SIZE)

const int   WIDTH = 1280;
const int   HEIGHT = 800;
const Float RATIO = (Float)WIDTH / HEIGHT;

const Float CAMERA_NEAR_PLANE = 0.01f;
const Float CAMERA_FAR_PLANE = 20.f;

const Float SHADOW_VIEW_SIZE = 3.0f;
const Float SHADOW_NEAR_PLANE = 0.1f;
const Float SHADOW_FAR_PLANE = 20.f;

void     TimeElapsed(spdlog::stopwatch& sw, std::string note = "");
TGAImage SSAA(const TGAImage& image, int kernelSize);

int main(int argc, const char* argv[])
{
    // Input

    if (argc != 2)
    {
        std::cerr << "Required: 1 argument, but given " << argc - 1 << std::endl;
        std::cerr << "Usage: ./ForkerRenderer <scene file>" << std::endl;
        return 1;
    }

    std::string sceneFileName = argv[1];

    // Spdlog

    spdlog::set_pattern("[%^%l%$] %v");
    spdlog::set_level(spdlog::level::debug);
    spdlog::stopwatch stepStopwatch;
    spdlog::stopwatch totalStopwatch;

    // ForkerGL

    ForkerGL::Viewport(0, 0, WIDTH, HEIGHT);
    ForkerGL::TextureWrapMode(Texture::NoWrap);     // or Repeat, ClampedToEdge, etc
    ForkerGL::TextureFilterMode(Texture::Nearest);  // or Linear

    // Scene

    Scene scene(sceneFileName);

    TimeElapsed(stepStopwatch, "Scene Loaded");

    // Shadow Mapping

#ifdef SHADOW_PASS
    spdlog::info("");
    spdlog::info("Shadow Pass:");
    // Buffer Configuration
    ForkerGL::InitShadowBuffers(WIDTH, HEIGHT);
    ForkerGL::RenderMode(ForkerGL::ShadowPass);
    // Matrix
    Matrix4x4f viewMatrixSM =
        MakeLookAtMatrix(scene.GetPointLight().position, Vector3f(0.f));
    Matrix4x4f projMatrixSM = MakeOrthographicMatrix(
        -SHADOW_VIEW_SIZE * RATIO, SHADOW_VIEW_SIZE * RATIO, -SHADOW_VIEW_SIZE,
        SHADOW_VIEW_SIZE, SHADOW_NEAR_PLANE, SHADOW_FAR_PLANE);
    Matrix4x4f lightSpaceMatrix = projMatrixSM * viewMatrixSM;

    for (int i = 0; i < scene.GetModelCount(); ++i)
    {
        const auto& model = scene.GetModel(i);

        // Depth Shading
        DepthShader depthShader;
        depthShader.uModelMatrix = scene.GetModelMatrix(i);
        depthShader.uLightSpaceMatrix = lightSpaceMatrix;
        // Render
        model.Render(depthShader);
    }
    ForkerGL::ShadowBuffer.GenerateGrayImage(false).WriteTgaFile(
        "output/output_shadowmap.tga");
    TimeElapsed(stepStopwatch, "Shadow Mapping Finished");
#endif

    // Buffer Configuration

    ForkerGL::InitFrameBuffers(WIDTH, HEIGHT);
    ForkerGL::ClearColor(Color3(0.12f, 0.12f, 0.12f));
    ForkerGL::RenderMode(ForkerGL::ColorPass);

    spdlog::info("");
    for (int i = 0; i < scene.GetModelCount(); ++i)
    {
        const auto& model = scene.GetModel(i);

        if (!model.SupportPBR())
        {
            // Blinn-Phong Shading
            spdlog::info("Color Pass (Blinn-Phong Shading):");
            BlinnPhongShader bpShader;
            bpShader.uModelMatrix = scene.GetModelMatrix(i);
            bpShader.uViewMatrix = scene.GetCamera().GetViewMatrix();
            bpShader.uNormalMatrix = MakeNormalMatrix(bpShader.uModelMatrix);
            bpShader.uProjectionMatrix =
                (scene.GetProjectionType() == Camera::Orthographic)
                    ? scene.GetCamera().GetOrthographicMatrix(
                          -1.f * RATIO, 1.f * RATIO, -1.f, 1.f, CAMERA_NEAR_PLANE,
                          CAMERA_FAR_PLANE)
                    : scene.GetCamera().GetPerspectiveMatrix(
                          45.f, RATIO, CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE);

            // Shader Configuration
            bpShader.uPointLight = scene.GetPointLight();
            bpShader.uEyePos = scene.GetCamera().GetPosition();
#ifdef SHADOW_PASS
            bpShader.uShadowBuffer = ForkerGL::ShadowBuffer;
            bpShader.uLightSpaceMatrix = lightSpaceMatrix;
#endif
            // Render
            model.Render(bpShader);
        }
        else
        {
            // PBR Shading
            spdlog::info("Color Pass (PBR Shading):");
            PBRShader pbrShader;
            pbrShader.uModelMatrix = scene.GetModelMatrix(i);
            pbrShader.uViewMatrix = scene.GetCamera().GetViewMatrix();
            pbrShader.uNormalMatrix = MakeNormalMatrix(pbrShader.uModelMatrix);
            pbrShader.uProjectionMatrix =
                (scene.GetProjectionType() == Camera::Orthographic)
                    ? scene.GetCamera().GetOrthographicMatrix(
                          -1.f * RATIO, 1.f * RATIO, -1.f, 1.f, CAMERA_NEAR_PLANE,
                          CAMERA_FAR_PLANE)
                    : scene.GetCamera().GetPerspectiveMatrix(
                          45.f, RATIO, CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE);

            // Shader Configuration
            pbrShader.uPointLight = scene.GetPointLight();
            pbrShader.uEyePos = scene.GetCamera().GetPosition();
#ifdef SHADOW_PASS
            pbrShader.uShadowBuffer = ForkerGL::ShadowBuffer;
            pbrShader.uLightSpaceMatrix = lightSpaceMatrix;
#endif
            // Render
            model.Render(pbrShader);
        }
        TimeElapsed(stepStopwatch, "Model Rendered");
    }

    // Output Framebuffer Image
    TimeElapsed(totalStopwatch, "Total");
    TGAImage outputImage = ForkerGL::FrameBuffer.GenerateImage();
    outputImage.WriteTgaFile("output/output_framebuffer.tga", true);

    // Post-Processing: Anti-Aliasing
#ifdef ANTI_ALIASING  // clang-format off
    SSAA(outputImage, KERNEL_SIZE)
        .WriteTgaFile("output/output_framebuffer_SSAA.tga", true);  // clang-format on
#endif

    // Output Zbuffer Image
    ForkerGL::DepthBuffer.GenerateGrayImage(false).WriteTgaFile(
        "output/output_zbuffer.tga", true);

    return 0;
}

// Timer
void TimeElapsed(spdlog::stopwatch& sw, std::string note)
{
    spdlog::info("<Time Used: {:.6} Seconds ({})>", sw, note);
    // spdlog::info("------------------------------------------------------------");
    sw.reset();
}

// Anti-Aliasing
TGAImage SSAA(const TGAImage& image, int kernelSize)
{
    TGAImage antialiasingImage(image.GetWidth() / kernelSize,
                               image.GetHeight() / kernelSize, TGAImage::RGB);
    for (int x = 0; x < antialiasingImage.GetWidth(); ++x)
    {
        for (int y = 0; y < antialiasingImage.GetHeight(); ++y)
        {
            int xx = x * kernelSize;
            int yy = y * kernelSize;
            int R = 0, G = 0, B = 0;
            for (int i = 0; i < kernelSize; ++i)
            {
                for (int j = 0; j < kernelSize; ++j)
                {
                    TGAColor c = image.Get(xx + i, yy + j);
                    R += c.r;
                    G += c.g;
                    B += c.b;
                }
            }
            R /= (Float)kernelSize * kernelSize;
            G /= (Float)kernelSize * kernelSize;
            B /= (Float)kernelSize * kernelSize;
            TGAColor color(R, G, B);
            antialiasingImage.Set(x, y, color);
        }
    }
    return antialiasingImage;
}
