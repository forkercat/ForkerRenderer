//
// Created by Junhao Wang (@Forkercat) on 2021/6/21.
//

#include "output.h"

#include "forkergl.h"

namespace Output
{

void OutputFrameBuffer()
{
    ForkerGL::FrameBuffer.GenerateImage().WriteTgaFile("output/output_framebuffer.tga");
}

void OutputZBuffer()
{
    ForkerGL::DepthBuffer.GenerateGrayImage(false).WriteTgaFile(
        "output/output_zbuffer.tga");
}

void OutputShadowBuffer()
{
    if (ForkerGL::ShadowBuffer.GetWidth() != 0)
    {
        ForkerGL::ShadowBuffer.GenerateGrayImage(false).WriteTgaFile(
            "output/output_shadowmap.tga");
    }
}

void OutputSSAAImage()
{
    if (ForkerGL::AntiAliasedImage.GetWidth() != 0)
    {
        ForkerGL::AntiAliasedImage.WriteTgaFile("output/output_framebuffer_SSAA.tga");
    }
}

}  // namespace Output