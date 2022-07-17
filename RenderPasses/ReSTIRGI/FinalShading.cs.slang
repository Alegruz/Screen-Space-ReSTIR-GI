/***************************************************************************
 # Copyright (c) 2015-21, NVIDIA CORPORATION. All rights reserved.
 #
 # NVIDIA CORPORATION and its licensors retain all intellectual property
 # and proprietary rights in and to this software, related documentation
 # and any modifications thereto.  Any use, reproduction, disclosure or
 # distribution of this software and related documentation without an express
 # license agreement from NVIDIA CORPORATION is strictly prohibited.
 **************************************************************************/
import LoadShadingData;
import Scene.RaytracingInline;
//import Rendering.Materials.StandardMaterial;
import Rendering.Materials.Microfacet;
import Rendering.ReSTIRGI.ScreenSpaceReSTIR;
import Utils.Sampling.TinyUniformSampleGenerator;

RWTexture2D<float4> gColor;
RWTexture2D<float4> gEmission;
RWTexture2D<float4> gDiffuseIllumination;
RWTexture2D<float4> gDiffuseReflectance;
RWTexture2D<float4> gSpecularIllumination;
RWTexture2D<float4> gSpecularReflectance;
RWTexture2D<float4> gDebug;

#define is_valid(name) (is_valid_##name != 0)
static const bool kComputeDirect = COMPUTE_DIRECT;

/** Perform shading with final samples from ReSTIR.
*/
struct FinalShading
{
    // Static configuration.
    //static const bool kUseEnvBackground = USE_ENV_BACKGROUND;

    Texture2D<PackedHitInfo> vbuffer;
    Texture2D<float4> vColor;

    //ScreenSpaceReSTIR screenSpaceReSTIR;
    //StandardMaterial standardMaterial;

    uint2 frameDim;

    uint numReSTIRInstances;
    uint ReSTIRInstanceID;

    void execute(uint2 pixel)
    {
        if (any(pixel >= frameDim)) return;

        float3 color = {};
        float3 emission = {};
        float3 diffuseReflectance = {};
        float3 diffuseIllumination = {};
        float3 specularReflectance = {};
        float3 specularIllumination = {};
        float hitT = 10000.f; // TODO: Remove magic number also used in SpatioTemporalResampling pass.

        ShadingData sd;
        let lod = ExplicitLodTextureSampler(0.f); // TODO: Implement texture level-of-detail.
        if (loadShadingData(pixel, frameDim, gScene.camera, vbuffer, lod, sd))
        {
            // Create BSDF instance and query its properties.
            let bsdf = gScene.materials.getBSDF(sd, lod);
            let bsdfProperties = bsdf.getProperties(sd);

            // Get final sample from ReSTIR.
            float3 dir;
            float distance;
            float3 Li;
            uint baseIndex = 0;
            uint sampleIndex = 0;
            uint elementCount = 0;

            float3 creationPoint;
            float3 creationNormal;
            float3 samplePosition;
            float3 sampleNormal;
            float3 sampleRadiance;

            float3 vcolor;
            //if (kComputeDirect)
            //{
            vcolor = float3(vColor[pixel].x, vColor[pixel].y, vColor[pixel].z);
            //}

            float avgWeight;
            gScreenSpaceReSTIR.getGIFinalSample(pixel, 0, creationPoint, creationNormal, samplePosition, sampleNormal, sampleRadiance, avgWeight);

            // Evaluate diffuse and specular lobes.
            //if (valid)
            //{
            //TinyUniformSampleGenerator sg = TinyUniformSampleGenerator(pixel, gScreenSpaceReSTIR.frameIndex);
            color = avgWeight * sampleRadiance;
            //color = samplePosition;
            //if (kComputeDirect)
            //{
            //    color += vcolor;
            //}

            //}

            //float3 diffuse = {};
            //float3 specular = {};
            //if (valid)
            //{
            //    TinyUniformSampleGenerator sg = TinyUniformSampleGenerator(pixel, gScreenSpaceReSTIR.frameIndex);
            //
            //    sd.mtl.setActiveLobes((uint)LobeType::DiffuseReflection);
            //    diffuse = bsdf.eval(sd, dir, sg) * Li;
            //
            //    sd.mtl.setActiveLobes((uint)LobeType::SpecularReflection);
            //    specular = bsdf.eval(sd, dir, sg) * Li;
            //
            //    hitT = distance;
            //}
            //
            //emission = bsdfProperties.emission;
            //color = diffuse + specular + emission;

            // Demodulate diffuse reflectance (albedo) from diffuse color.
            diffuseReflectance = bsdfProperties.diffuseReflectionAlbedo;
            float3 diffuseFactor = diffuseReflectance <= 0.f ? 0.f : 1.f / diffuseReflectance;
            diffuseIllumination = color * diffuseFactor;

            // Demodulate preintegrated specular reflectance from specular color.
            /*
            specularReflectance += approxSpecularIntegralGGX(sd.specular, sd.linearRoughness * sd.linearRoughness, sd.NdotV);
            float3 specularFactor = specularReflectance <= 0.f ? 0.f : 1.f / specularReflectance;
            specularIllumination += specular * specularFactor;
            */
            // Demodulate preintegrated specular reflectance from specular color.
            float NdotV = saturate(dot(sd.N, sd.V));
            float ggxAlpha = bsdfProperties.roughness * bsdfProperties.roughness;
            specularReflectance = approxSpecularIntegralGGX(bsdfProperties.specularReflectance, ggxAlpha, NdotV);
            float3 specularFactor = specularReflectance <= 0.f ? 0.f : 1.f / specularReflectance;
            specularIllumination = color * specularFactor;
        }
        else
        {
            //if (kUseEnvBackground)
            //{
            //    float3 dir = gScene.camera.computeRayPinhole(pixel, frameDim).dir;
            //    emission = gScene.envMap.eval(dir);
            //    color = emission;
            //}
        }

        // Write active outputs.

        if (ReSTIRInstanceID == 0)
        {
            if (is_valid(gColor)) gColor[pixel] = float4(color, 1.f) / numReSTIRInstances;
            if (is_valid(gEmission)) gEmission[pixel] = float4(emission, 1.f) / numReSTIRInstances;
            if (is_valid(gDiffuseReflectance)) gDiffuseReflectance[pixel] = float4(diffuseReflectance, 1.f) / numReSTIRInstances;
            if (is_valid(gDiffuseIllumination)) gDiffuseIllumination[pixel] = float4(diffuseIllumination, hitT) / numReSTIRInstances;
            if (is_valid(gSpecularReflectance)) gSpecularReflectance[pixel] = float4(specularReflectance, 1.f) / numReSTIRInstances;
            if (is_valid(gSpecularIllumination)) gSpecularIllumination[pixel] = float4(specularIllumination, hitT) / numReSTIRInstances;
        }
        else
        {
            if (is_valid(gColor)) gColor[pixel] += float4(color, 1.f) / numReSTIRInstances;
            if (is_valid(gEmission)) gEmission[pixel] += float4(emission, 1.f) / numReSTIRInstances;
            if (is_valid(gDiffuseReflectance)) gDiffuseReflectance[pixel] += float4(diffuseReflectance, 1.f) / numReSTIRInstances;
            if (is_valid(gDiffuseIllumination)) gDiffuseIllumination[pixel] += float4(diffuseIllumination, hitT) / numReSTIRInstances;
            if (is_valid(gSpecularReflectance)) gSpecularReflectance[pixel] += float4(specularReflectance, 1.f) / numReSTIRInstances;
            if (is_valid(gSpecularIllumination)) gSpecularIllumination[pixel] += float4(specularIllumination, hitT) / numReSTIRInstances;
        }
    }
};

ParameterBlock<FinalShading> gFinalShading;
//cbuffer CB
//{
//    FinalShading gFinalShading;
//};

[numthreads(16, 16, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    gFinalShading.execute(dispatchThreadId.xy);
}
