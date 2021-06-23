#include <spdlog/spdlog.h>
#include <spdlog/stopwatch.h>

#include <iostream>

#include "output.h"
#include "render.h"
#include "utility.h"

/////////////////////////////////////////////////////////////////////////////////

static spdlog::stopwatch stepStopwatch;

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

    // Render
    Render::Render(scene);

    // Output
    Output::OutputFrameBuffer();
    Output::OutputSSAAImage();
    Output::OutputShadowBuffer();
    Output::OutputZBuffer();
    Output::OutputNormalGBuffer();  // if not empty
    Output::OutputWorldPosGBuffer();
    Output::OutputAlbedoGBuffer();
    Output::OutputParamGBuffer();
    Output::OutputShadingTypeGBuffer();

    return 0;
}
