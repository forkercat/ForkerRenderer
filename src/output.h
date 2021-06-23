//
// Created by Junhao Wang (@Forkercat) on 2021/6/21.
//

#pragma once

namespace Output
{
void OutputFrameBuffer();
void OutputZBuffer();
void OutputShadowBuffer();
void OutputSSAAImage();
// G-Buffers
void OutputNormalGBuffer();
void OutputWorldPosGBuffer();
void OutputAlbedoGBuffer();
void OutputParamGBuffer();
void OutputShadingTypeGBuffer();
}  // namespace Output
