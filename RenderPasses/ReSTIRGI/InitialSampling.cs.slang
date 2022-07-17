/***************************************************************************
 # Copyright (c) 2015-21, NVIDIA CORPORATION. All rights reserved.
 #
 # NVIDIA CORPORATION and its licensors retain all intellectual property
 # and proprietary rights in and to this software, related documentation
 # and any modifications thereto.  Any use, reproduction, disclosure or
 # distribution of this software and related documentation without an express
 # license agreement from NVIDIA CORPORATION is strictly prohibited.
 **************************************************************************/
import Utils.Color.ColorHelpers;
import Rendering.ReSTIRGI.ScreenSpaceReSTIR;

/** Prepares ReSTIR samples
*/
struct InitialSampling
{
    Texture2D<float4> vPosW;
    Texture2D<float4> vNormW;
    Texture2D<float4> sPosW;
    Texture2D<float4> sNormW;
    Texture2D<float4> sColor;
    Texture2D<float> random;

    //ScreenSpaceReSTIR screenSpaceReSTIR;

    uint2 frameDim;

    void execute(uint2 pixel)
    {
        if (any(pixel >= frameDim)) return;

        gScreenSpaceReSTIR.setGIInitialSample(pixel, vPosW[pixel].xyz, vNormW[pixel].xyz, sPosW[pixel].xyz, sNormW[pixel].xyz, sColor[pixel].xyz, random[pixel]);
    }
};

ParameterBlock<InitialSampling> gInitialSampling;
//cbuffer CB
//{
//    PrepareSurfaceData gPrepareSurfaceData;
//};

[numthreads(16, 16, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    gInitialSampling.execute(dispatchThreadId.xy);
}
