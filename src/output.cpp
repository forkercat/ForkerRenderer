//
// Created by Junhao Wang (@Forkercat) on 2021/6/21.
//

#include "output.h"

#include "forkergl.h"

namespace Output
{

void OutputFrameBuffer()
{
    ForkerGL::FrameBuffer.GenerateImage().WriteTgaFile("output/framebuffer.tga");
}

void OutputZBuffer()
{
    ForkerGL::DepthBuffer.GenerateGrayImage().WriteTgaFile("output/zbuffer.tga");
}

void OutputShadowBuffer()
{
    if (ForkerGL::ShadowBuffer.GetWidth() != 0)
    {
        ForkerGL::ShadowBuffer.GenerateGrayImage().WriteTgaFile("output/shadowmap.tga");
    }
}

void OutputSSAAImage()
{
    if (ForkerGL::AntiAliasedImage.GetWidth() != 0)
    {
        ForkerGL::AntiAliasedImage.WriteTgaFile("output/framebuffer_SSAA.tga");
    }
}

void OutputDepthGBuffer()
{
    if (ForkerGL::DepthGBuffer.GetWidth() != 0)
    {
        ForkerGL::DepthGBuffer.GenerateGrayImage().WriteTgaFile(
            "output/gbuffer_depth.tga");
    }
}

void OutputNormalGBuffer()
{
    if (ForkerGL::NormalGBuffer.GetWidth() != 0)
    {
        ForkerGL::NormalGBuffer.GenerateImage().WriteTgaFile("output/gbuffer_normal.tga");
    }
}

void OutputWorldPosGBuffer()
{
    if (ForkerGL::WorldPosGBuffer.GetWidth() != 0)
    {
        ForkerGL::WorldPosGBuffer.GenerateImage().WriteTgaFile(
            "output/gbuffer_worldpos.tga");
    }
}

}  // namespace Output