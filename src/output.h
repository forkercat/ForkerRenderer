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
void OutputDepthGBuffer();
void OutputNormalGBuffer();
void OutputWorldPosGBuffer();
}  // namespace Output
