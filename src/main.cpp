#include <spdlog/spdlog.h>
#include <spdlog/stopwatch.h>

#include <iostream>

#include "output.h"
#include "render.h"
#include "utility.h"

/////////////////////////////////////////////////////////////////////////////////

static spdlog::stopwatch stepStopwatch;
static spdlog::stopwatch totalStopwatch;

static void InitSpdLog()
{
    spdlog::set_pattern("[%^%l%$] %v");
    spdlog::set_level(spdlog::level::debug);
}

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
    InitSpdLog();

    // Scene
    Scene scene(sceneFileName);
    TimeElapsed(stepStopwatch, "Scene Loaded");

    // Preconfiguration
    Render::Preconfigure(scene);

    // Shadow Pass
    Render::DoShadowPass(scene);
    TimeElapsed(stepStopwatch, "Shadow Pass");

    // Geometry Pass
    Render::DoGeometryPass(scene);
    TimeElapsed(stepStopwatch, "Geometry Pass");

    // Lighting Pass
    Render::DoLightingPass(scene);
    TimeElapsed(stepStopwatch, "Lighting Pass");

    // Anti-Aliasing
    Render::DoSSAA();
    TimeElapsed(stepStopwatch, "Anti-Aliasing");

    // Output
    Output::OutputFrameBuffer();
    Output::OutputSSAAImage();
    Output::OutputShadowBuffer();
    Output::OutputZBuffer();
    Output::OutputDepthGBuffer();
    Output::OutputNormalGBuffer();
    Output::OutputWorldPosGBuffer();
    TimeElapsed(totalStopwatch, "Total");

    return 0;
}
