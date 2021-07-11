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
    ForkerGL::DepthBuffer.GenerateImage().WriteTgaFile("output/zbuffer.tga");
}

void OutputShadowBuffer()
{
    if (ForkerGL::ShadowBuffer.GetWidth() != 0)
    {
        ForkerGL::ShadowBuffer.GenerateImage().WriteTgaFile("output/shadowmap.tga");
    }
}

void OutputSSAAImage()
{
    if (ForkerGL::AntiAliasedImage.GetWidth() != 0)
    {
        ForkerGL::AntiAliasedImage.WriteTgaFile("output/framebuffer_SSAA.tga");
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

void OutputAlbedoGBuffer()
{
    if (ForkerGL::AlbedoGBuffer.GetWidth() != 0)
    {
        ForkerGL::AlbedoGBuffer.GenerateImage().WriteTgaFile(
            "output/gbuffer_albedo.tga");
    }
}

void OutputParamGBuffer()
{
    if (ForkerGL::ParamGBuffer.GetWidth() != 0)
    {
        ForkerGL::ParamGBuffer.GenerateImage().WriteTgaFile("output/gbuffer_param.tga");
    }
}

void OutputShadingTypeGBuffer()
{
    if (ForkerGL::ShadingTypeGBuffer.GetWidth() != 0)
    {
        ForkerGL::ShadingTypeGBuffer.GenerateImage().WriteTgaFile("output/gbuffer_shading_type.tga");
    }
}

void OutputAmbientOcclusionGBuffer()
{
    if (ForkerGL::AmbientOcclusionGBuffer.GetWidth() != 0)
    {
        ForkerGL::AmbientOcclusionGBuffer.GenerateImage().WriteTgaFile("output/gbuffer_ambient_occlusion.tga");
    }
}

}  // namespace Output