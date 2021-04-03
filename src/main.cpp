#include <spdlog/spdlog.h>
#include <spdlog/stopwatch.h>

#include "camera.h"
#include "check.h"
#include "forkergl.h"
#include "geometry.h"
#include "light.h"
#include "model.h"
#include "shader.h"
#include "tgaimage.h"

using std::make_shared;
using std::shared_ptr;

/////////////////////////////////////////////////////////////////////////////////

// #define ANTI_ALIASING_SSAA
#define KERNEL_SIZE 1  // width of output will be (WIDTH / KERNEL_SIZE)

const int   WIDTH = 1024;
const int   HEIGHT = 1024;
const Float RATIO = (Float)WIDTH / HEIGHT;

const Float CAMERA_NEAR_PLANE = 0.1f;
const Float CAMERA_FAR_PLANE = 20.f;
const Float SHADOW_NEAR_PLANE = 0.1f;
const Float SHADOW_FAR_PLANE = 10.f;

void     TimeElapsed(spdlog::stopwatch& sw, std::string note = "");
TGAImage SSAA(const TGAImage& image, int kernelSize);

int main(int argc, const char* argv[])
{
    // Input

    if (argc < 2 || argc > 4)
    {
        std::cerr << "Required: 1 - 3 arguments, but given " << argc - 1 << std::endl;
        std::cerr << "Usage: ./ForkerRenderer obj/model/model.obj"
                     " <RotateDegreeOnY = 0.0> <Scale = 1.0>"
                  << std::endl;
        return 1;
    }
    std::string modelFilename = argv[1];
    Float       rotateDegreeOnY = (argc >= 3) ? std::stof(argv[2]) : 0.f;
    Float       uniformScale = (argc >= 4) ? std::stof(argv[3]) : 1.f;
    if (uniformScale == 0.f)
    {
        std::cerr << "Scale factor should not be 0" << std::endl;
        return -1;
    }

    // Spdlog

    spdlog::set_level(spdlog::level::debug);
    spdlog::stopwatch stepStopwatch;
    spdlog::stopwatch totalStopwatch;

    // Model

    std::vector<shared_ptr<Model>> models;
    std::vector<Matrix4x4f>        modelMatrices;

    models.push_back(make_shared<Model>("obj/plane/plane.obj"));  // default plane
    modelMatrices.push_back(MakeModelMatrix(Vector3f(0, -1, 0), 0, 2.f));

    models.push_back(make_shared<Model>(modelFilename, true, true));
    modelMatrices.push_back(MakeModelMatrix(Vector3f(0), rotateDegreeOnY, uniformScale));

    TimeElapsed(stepStopwatch, "Model Loaded");

    // Camera

    Camera                 camera(0, 2, 5);  // LookAt = (0,0,0)
    Camera::ProjectionType projectionType = Camera::Perspective;
    ForkerGL::Viewport(0, 0, WIDTH, HEIGHT);

    // Light

    PointLight pointLight(0, 2, 5);

    // Shadow Mapping

#ifdef SHADOW_PASS
    spdlog::info("");
    spdlog::info("Shadow Pass:");
    // Buffer Configuration
    ForkerGL::InitShadowBuffers(WIDTH, HEIGHT);
    ForkerGL::RenderMode(ForkerGL::ShadowPass);
    // Matrix
    Matrix4x4f viewShadowMapping = MakeLookAtMatrix(pointLight.position, Vector3f(0.f));
    Matrix4x4f projShadowMapping = MakeOrthographicMatrix(
        -2.f * RATIO, 2.f * RATIO, -2.f, 2.f, SHADOW_NEAR_PLANE, SHADOW_FAR_PLANE);
    Matrix4x4f lightSpaceMatrix = projShadowMapping * viewShadowMapping;

    for (int i = 0; i < models.size(); ++i)
    {
        auto model = models[i];

        // Depth Shading
        DepthShader depthShader;
        depthShader.uModelMatrix = modelMatrices[i];
        depthShader.uLightSpaceMatrix = lightSpaceMatrix;
        // Render
        model->Render(depthShader);
    }
    ForkerGL::ShadowBuffer.GenerateGrayImage(false).WriteTgaFile(
        "output/output_shadowmap.tga");
    TimeElapsed(stepStopwatch, "Shadow Mapping Finished");
#endif

    // Buffer Configuration

    ForkerGL::InitFrameBuffers(WIDTH, HEIGHT);
    ForkerGL::ClearColor(Color3(0.12f, 0.12f, 0.12f));
    ForkerGL::RenderMode(ForkerGL::ColorPass);

    // Blinn-Phong Shading
    spdlog::info("");
    spdlog::info("Color Pass:");
    for (int i = 0; i < models.size(); ++i)
    {
        auto model = models[i];

        BlinnPhongShader bpShader;
        bpShader.uModelMatrix = modelMatrices[i];
        bpShader.uViewMatrix = camera.GetViewMatrix();
        bpShader.uNormalMatrix =
            MakeNormalMatrix(bpShader.uViewMatrix * bpShader.uModelMatrix);
        bpShader.uProjectionMatrix =
            (projectionType == Camera::Orthographic)
                ? camera.GetOrthographicMatrix(-1.f * RATIO, 1.f * RATIO, -1.f, 1.f,
                                               CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE)
                : camera.GetPerspectiveMatrix(45.f, RATIO, CAMERA_NEAR_PLANE,
                                              CAMERA_FAR_PLANE);

        // Shader Configuration
        bpShader.uPointLight = pointLight;
#ifdef SHADOW_PASS
        bpShader.uShadowBuffer = ForkerGL::ShadowBuffer;
        bpShader.uLightSpaceMatrix = lightSpaceMatrix;
#endif

        // Render
        model->Render(bpShader);
        TimeElapsed(stepStopwatch, "Model Rendered");
    }

    // Output Framebuffer Image
    TimeElapsed(totalStopwatch, "Total");
    TGAImage outputImage = ForkerGL::FrameBuffer.GenerateImage();
    outputImage.WriteTgaFile("output/output_framebuffer.tga", true);

    // Post-Processing: Anti-Aliasing
#ifdef ANTI_ALIASING_SSAA  // clang-format off
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
