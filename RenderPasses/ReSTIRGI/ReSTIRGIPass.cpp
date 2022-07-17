/***************************************************************************
 # Copyright (c) 2015-21, NVIDIA CORPORATION. All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions
 # are met:
 #  * Redistributions of source code must retain the above copyright
 #    notice, this list of conditions and the following disclaimer.
 #  * Redistributions in binary form must reproduce the above copyright
 #    notice, this list of conditions and the following disclaimer in the
 #    documentation and/or other materials provided with the distribution.
 #  * Neither the name of NVIDIA CORPORATION nor the names of its
 #    contributors may be used to endorse or promote products derived
 #    from this software without specific prior written permission.
 #
 # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS" AND ANY
 # EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 # IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 # PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 # CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 # EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 # PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 # PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 # OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 # (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 # OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **************************************************************************/
#include "ReSTIRGIPass.h"

const RenderPass::Info ReSTIRGIPass::kInfo { "ReSTIRGIPass", "Standalone pass for indirect lighting using ReSTIR GI." };

namespace
{
    // Shader location
    const std::string kPrepareSurfaceDataFile = "RenderPasses/ReSTIRGIPass/PrepareSurfaceData.cs.slang";
#if INITIAL_SAMPLING
    const std::string kInitialSamplingFile = "RenderPasses/ReSTIRGIPass/InitialSampling.cs.slang";
#endif
    const std::string kFinalShadingFile = "RenderPasses/ReSTIRGIPass/FinalShading.cs.slang";

    const std::string kShaderModel = "6_5";

    const std::string kInputVBuffer = "vbuffer";
    const std::string kInputMotionVectors = "motionVectors";
#if INITIAL_SAMPLING
    const std::string kInputVPosW = "vPosW";
    const std::string kInputVNormW = "vNormW";
    const std::string kInputVColor = "vColor";
    const std::string kInputSPosW = "sPosW";
    const std::string kInputSNormW = "sNormW";
    const std::string kInputSColor = "sColor";
    const std::string kInputRandom = "random";
#endif

    const Falcor::ChannelList kInputChannels =
    {
        //{ kInputVPosW,  "gVPosW",   "Visibile point position"   },
        //{ kInputVNormW, "gVNormW",  "Visibile point normal"     },
        //{ kInputSPosW,  "gSPosW",   "Sample point position"     },
        //{ kInputSNormW, "gSNormW",  "Sample point normal"       },
        //{ kInputSColor, "gSColor",  "Sample point radiance"     },
        { kInputVBuffer, "gVBuffer", "Visibility buffer in packed format", false, ResourceFormat::Unknown },
        { kInputMotionVectors, "gMotionVectors", "Motion vector buffer (float format)", true /* optional */, ResourceFormat::RG32Float },
#if INITIAL_SAMPLING
        { kInputVPosW,  "gVPosW",   "Visible point",                                false,  ResourceFormat::RGBA32Float },
        { kInputVNormW, "gVNormW",  "Visible surface normal",                       false,  ResourceFormat::RGBA32Float },
        { kInputVColor, "gVColor",  "Outgoing radiance at visible point in RGBA",   false,  ResourceFormat::RGBA32Float },
        { kInputSPosW,  "gSPosW",   "Sample point",                                 false,  ResourceFormat::RGBA32Float },
        { kInputSNormW, "gSNormW",  "Sample surface normal",                        false,  ResourceFormat::RGBA32Float },
        { "sColor", "gSColor",  "Outgoing radiance at sample point in RGBA",    false,  ResourceFormat::RGBA32Float },
        { kInputRandom, "gRandom",  "Random numbers used for path",                 false,  ResourceFormat::R32Float },
#endif
    };

    const Falcor::ChannelList kOutputChannels =
    {
        //{ "color",  "gColor",   "Final color",  true /* optional */, ResourceFormat::RGBA32Float },
        { "color",                  "gColor",                   "Final color",              true /* optional */, ResourceFormat::RGBA32Float },
        { "emission",               "gEmission",                "Emissive color",           true /* optional */, ResourceFormat::RGBA32Float },
        { "diffuseIllumination",    "gDiffuseIllumination",     "Diffuse illumination",     true /* optional */, ResourceFormat::RGBA32Float },
        { "diffuseReflectance",     "gDiffuseReflectance",      "Diffuse reflectance",      true /* optional */, ResourceFormat::RGBA32Float },
        { "specularIllumination",   "gSpecularIllumination",    "Specular illumination",    true /* optional */, ResourceFormat::RGBA32Float },
        { "specularReflectance",    "gSpecularReflectance",     "Specular reflectance",     true /* optional */, ResourceFormat::RGBA32Float },
        { "debug",                  "gDebug",                   "Debug output",             true /* optional */, ResourceFormat::RGBA32Float },
    };

    // Scripting options.
    const char* kOptions = "options";
    const char* kNumReSTIRInstances = "NumReSTIRInstances";
    const char kComputeDirect[] = "computeDirect";
}

// Don't remove this. it's required for hot-reload to function properly
extern "C" FALCOR_API_EXPORT const char* getProjDir()
{
    return PROJECT_DIR;
}

extern "C" FALCOR_API_EXPORT void getPasses(Falcor::RenderPassLibrary& lib)
{
    lib.registerPass(ReSTIRGIPass::kInfo, ReSTIRGIPass::create);
}

ReSTIRGIPass::SharedPtr ReSTIRGIPass::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    SharedPtr pPass = SharedPtr(new ReSTIRGIPass(dict));
    return pPass;
}

Dictionary ReSTIRGIPass::getScriptingDictionary()
{
    Dictionary d;
    d[kOptions] = mOptions;// mpScreenSpaceReSTIR->getOptions();
    d[kNumReSTIRInstances] = mNumReSTIRInstances;
    d[kComputeDirect] = mComputeDirect;
    return d;
}

RenderPassReflection ReSTIRGIPass::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;
    //reflector.addOutput("dst");
    //reflector.addInput("src");
    addRenderPassOutputs(reflector, kOutputChannels);
    addRenderPassInputs(reflector, kInputChannels);

    return reflector;
}

void ReSTIRGIPass::compile(RenderContext* pRenderContext, const CompileData& compileData)
{
    mFrameDim = compileData.defaultTexDims;
}

void ReSTIRGIPass::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    if (mNeedRecreateReSTIRInstances)
    {
        setScene(pRenderContext, mpScene);
    }

    // Clear outputs if no scene is loaded.
    if (!mpScene)
    {
        clearRenderPassChannels(pRenderContext, kOutputChannels, renderData);
        return;
    }

    const auto& pVBuffer = renderData[kInputVBuffer]->asTexture();
    const auto& pMotionVectors = renderData[kInputMotionVectors]->asTexture();
#if INITIAL_SAMPLING
    const auto& pVPosW = renderData[kInputVPosW]->asTexture();
    const auto& pVNormW = renderData[kInputVNormW]->asTexture();
    const auto& pVColor = renderData[kInputVColor]->asTexture();
    const auto& pSPosW = renderData[kInputSPosW]->asTexture();
    const auto& pSNormW = renderData[kInputSNormW]->asTexture();
    const auto& pSColor = renderData[kInputSColor]->asTexture();
    const auto& pRandom = renderData[kInputRandom]->asTexture();
#endif

    // Clear outputs if ReSTIR module is not initialized.
    if (mpScreenSpaceReSTIR.empty())
    {
        auto clear = [&](const ChannelDesc& channel)
        {
            auto pTex = renderData[channel.name]->asTexture();
            if (pTex)
            {
                pRenderContext->clearUAV(pTex->getUAV().get(), float4(0.f));
            }
        };
        for (const auto& channel : kOutputChannels)
        {
            clear(channel);
        }
        return;
    }

    auto& dict = renderData.getDictionary();

    // Update refresh flag if changes that affect the output have occured.
    if (mOptionsChanged)
    {
        auto flags = dict.getValue(kRenderPassRefreshFlags, Falcor::RenderPassRefreshFlags::None);
        flags |= Falcor::RenderPassRefreshFlags::RenderOptionsChanged;
        dict[Falcor::kRenderPassRefreshFlags] = flags;
        mOptionsChanged = false;
    }

    // Check if GBuffer has adjusted shading normals enabled.
    mGBufferAdjustShadingNormals = dict.getValue(Falcor::kRenderPassGBufferAdjustShadingNormals, false);

    for (size_t i = 0; i < mpScreenSpaceReSTIR.size(); ++i)
    {
        mpScreenSpaceReSTIR[i]->beginFrame(pRenderContext, mFrameDim);

        prepareSurfaceData(pRenderContext, pVBuffer, i);

#if INITIAL_SAMPLING
        initialSample(pRenderContext, pVPosW, pVNormW, pSPosW, pSNormW, pSColor, pRandom, i);
#endif

        mpScreenSpaceReSTIR[i]->updateReSTIRGI(pRenderContext, pMotionVectors);

        finalShading(pRenderContext, pVBuffer, pVColor, renderData, i);

        mpScreenSpaceReSTIR[i]->endFrame(pRenderContext);
    }

    auto copyTexture = [pRenderContext](Texture* pDst, const Texture* pSrc)
    {
        if (pDst && pSrc)
        {
            assert(pDst && pSrc);
            assert(pDst->getFormat() == pSrc->getFormat());
            assert(pDst->getWidth() == pSrc->getWidth() && pDst->getHeight() == pSrc->getHeight());
            pRenderContext->copyResource(pDst, pSrc);
        }
        else if (pDst)
        {
            pRenderContext->clearUAV(pDst->getUAV().get(), uint4(0, 0, 0, 0));
        }
    };
    // Copy debug output if available. (only support first ReSTIR instance for now)
    //if (const auto& pDebug = renderData["debug"]->asTexture())
    //{
    //    copyTexture(pDebug.get(), mpScreenSpaceReSTIR[0]->getDebugOutputTexture().get());
    //}
}

void ReSTIRGIPass::renderUI(Gui::Widgets& widget)
{
    mNeedRecreateReSTIRInstances = widget.var("Num ReSTIR Instances", mNumReSTIRInstances, 1, 8);

    bool dirty = false;

    dirty |= widget.checkbox("Evaluate direct illumination", mComputeDirect);
    widget.tooltip("Compute direct illumination.\nIf disabled only indirect is computed (when max bounces > 0).", true);

    if (!mpScreenSpaceReSTIR.empty() && mpScreenSpaceReSTIR[0])
    {
        mOptionsChanged = mpScreenSpaceReSTIR[0]->renderUI(widget);
        for (size_t i = 1; i < mpScreenSpaceReSTIR.size(); ++i)
        {
            mpScreenSpaceReSTIR[i]->copyRecompileStateFromOtherInstance(mpScreenSpaceReSTIR[0]);            

            if (mOptionsChanged || dirty)
            {
                mOptions = mpScreenSpaceReSTIR[i]->getOptions();
            }
        }
    }


    if (dirty)
    {
        mOptionsChanged = true;
    }
}

void ReSTIRGIPass::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
{
    mpScene = pScene;
    mpPrepareSurfaceData = nullptr;
    mpFinalShading = nullptr;

    if (!mpScreenSpaceReSTIR.empty())
    {
        //mOptions = mpScreenSpaceReSTIR->getOptions();
        mpScreenSpaceReSTIR.clear();
    }

    if (mpScene)
    {
        if (pScene->hasProceduralGeometry())
        {
            logError("This render pass does not support procedural primitives such as curves.");
        }

        mpScreenSpaceReSTIR.resize(mNumReSTIRInstances);
        for (int i = 0; i < mNumReSTIRInstances; i++)
        {
            mpScreenSpaceReSTIR[i] = ScreenSpaceReSTIR::create(mpScene, mOptions, mNumReSTIRInstances, i);
        }
    }
}

bool ReSTIRGIPass::onMouseEvent(const MouseEvent& mouseEvent)
{
    return !mpScreenSpaceReSTIR.empty() && mpScreenSpaceReSTIR[0] ? mpScreenSpaceReSTIR[0]->getPixelDebug()->onMouseEvent(mouseEvent) : false;
}

ReSTIRGIPass::ReSTIRGIPass(const Dictionary& dict)
    : RenderPass(kInfo)
{
    parseDictionary(dict);
}

void ReSTIRGIPass::parseDictionary(const Dictionary& dict)
{
    for (const auto& [key, value] : dict)
    {
        if (key == kOptions)
        {
            mOptions = value;
        }
        else if (key == kNumReSTIRInstances)
        {
            mNumReSTIRInstances = value;
        }
        else if (key == kComputeDirect)
        {
            mComputeDirect = value;
        }
        else
        {
            logWarning("Unknown field '" + key + "' in ScreenSpaceReSTIRPass dictionary");
        }
    }

    if (!mpScreenSpaceReSTIR.empty() && mpScreenSpaceReSTIR[0])
    {
        mpScreenSpaceReSTIR[0]->mOptions = mOptions;
    }
}

void ReSTIRGIPass::prepareSurfaceData(RenderContext* pRenderContext, const Texture::SharedPtr& pVBuffer, size_t instanceID)
{
    assert(!mpScreenSpaceReSTIR.empty() && mpScreenSpaceReSTIR[instanceID]);

    FALCOR_PROFILE("prepareSurfaceData");

    if (!mpPrepareSurfaceData)
    {
        Program::Desc desc;
        desc.addShaderLibrary(kPrepareSurfaceDataFile).setShaderModel(kShaderModel).csEntry("main");
        desc.addTypeConformances(mpScene->getTypeConformances());

        auto defines = mpScene->getSceneDefines();
        defines.add("GBUFFER_ADJUST_SHADING_NORMALS", mGBufferAdjustShadingNormals ? "1" : "0");
        mpPrepareSurfaceData = ComputePass::create(desc, defines, false);
        mpPrepareSurfaceData->setVars(nullptr);
    }

    mpPrepareSurfaceData->addDefine("GBUFFER_ADJUST_SHADING_NORMALS", mGBufferAdjustShadingNormals ? "1" : "0");

    mpPrepareSurfaceData["gScene"] = mpScene->getParameterBlock();

    auto var = mpPrepareSurfaceData["gPrepareSurfaceData"];
    //auto var = mpPrepareSurfaceData["CB"]["gPrepareSurfaceData"];

    var["vbuffer"] = pVBuffer;
    var["frameDim"] = mFrameDim;
    //mpScreenSpaceReSTIR[instanceID]->setShaderData(var["screenSpaceReSTIR"]);
    mpScreenSpaceReSTIR[instanceID]->setShaderDataRoot(mpPrepareSurfaceData->getRootVar());

    if (instanceID == 0 && mpFinalShading && mpScreenSpaceReSTIR[0]->mRequestParentRecompile)
    {
        mpFinalShading->setVars(nullptr);
        mpScreenSpaceReSTIR[0]->mRequestParentRecompile = false;
    }

    mpPrepareSurfaceData->execute(pRenderContext, mFrameDim.x, mFrameDim.y);
}

#if INITIAL_SAMPLING
void ReSTIRGIPass::initialSample(
    RenderContext* pRenderContext,
    const Texture::SharedPtr& pVPosW,
    const Texture::SharedPtr& pVNormW,
    const Texture::SharedPtr& pSPosW,
    const Texture::SharedPtr& pSNormW,
    const Texture::SharedPtr& pSColor,
    const Texture::SharedPtr& pRandom,
    size_t instanceID
)
{
    assert(!mpScreenSpaceReSTIR.empty() && mpScreenSpaceReSTIR[instanceID]);

    FALCOR_PROFILE("initialSample");

    if (!mpInitialSampling)
    {
        Program::Desc desc;
        desc.addShaderLibrary(kInitialSamplingFile).setShaderModel(kShaderModel).csEntry("main");
        desc.addTypeConformances(mpScene->getTypeConformances());

        auto defines = mpScene->getSceneDefines();
        defines.add("GBUFFER_ADJUST_SHADING_NORMALS", mGBufferAdjustShadingNormals ? "1" : "0");
        mpInitialSampling = ComputePass::create(desc, defines, false);
        mpInitialSampling->setVars(nullptr);
    }

    mpInitialSampling->addDefine("GBUFFER_ADJUST_SHADING_NORMALS", mGBufferAdjustShadingNormals ? "1" : "0");

    mpInitialSampling["gScene"] = mpScene->getParameterBlock();

    auto var = mpInitialSampling["gInitialSampling"];
    //auto var = mpPrepareSurfaceData["CB"]["gPrepareSurfaceData"];

    var["vPosW"] = pVPosW;
    var["vNormW"] = pVNormW;
    var["sPosW"] = pSPosW;
    var["sNormW"] = pSNormW;
    var["sColor"] = pSColor;
    var["random"] = pRandom;
    var["frameDim"] = mFrameDim;
    //mpScreenSpaceReSTIR[instanceID]->setShaderData(var["screenSpaceReSTIR"]);
    mpScreenSpaceReSTIR[instanceID]->setShaderDataRoot(mpInitialSampling->getRootVar());

    if (instanceID == 0 && mpFinalShading && mpScreenSpaceReSTIR[0]->mRequestParentRecompile)
    {
        mpFinalShading->setVars(nullptr);
        mpScreenSpaceReSTIR[0]->mRequestParentRecompile = false;
    }

    mpInitialSampling->execute(pRenderContext, mFrameDim.x, mFrameDim.y);
}
#endif

void ReSTIRGIPass::finalShading(RenderContext* pRenderContext, const Texture::SharedPtr& pVBuffer, const Texture::SharedPtr& pVColor, const RenderData& renderData, size_t instanceID)
{
    assert(!mpScreenSpaceReSTIR.empty() && mpScreenSpaceReSTIR[instanceID]);

    FALCOR_PROFILE("finalShading");

    if (!mpFinalShading)
    {
        Program::Desc desc;
        desc.addShaderLibrary(kFinalShadingFile).setShaderModel(kShaderModel).csEntry("main");
        desc.addTypeConformances(mpScene->getTypeConformances());

        auto defines = mpScene->getSceneDefines();
        defines.add("GBUFFER_ADJUST_SHADING_NORMALS", mGBufferAdjustShadingNormals ? "1" : "0");
        defines.add("COMPUTE_DIRECT", mComputeDirect ? "1" : "0");
        //defines.add("USE_ENV_BACKGROUND", mpScene->useEnvBackground() ? "1" : "0");
        defines.add(getValidResourceDefines(kOutputChannels, renderData));
        mpFinalShading = ComputePass::create(desc, defines, false);
        mpFinalShading->setVars(nullptr);
    }

    mpFinalShading->addDefine("GBUFFER_ADJUST_SHADING_NORMALS", mGBufferAdjustShadingNormals ? "1" : "0");
    //mpFinalShading->addDefine("USE_ENV_BACKGROUND", mpScene->useEnvBackground() ? "1" : "0");
    mpFinalShading->addDefine("_USE_LEGACY_SHADING_CODE", "0");

    // For optional I/O resources, set 'is_valid_<name>' defines to inform the program of which ones it can access.
    // TODO: This should be moved to a more general mechanism using Slang.
    mpFinalShading->getProgram()->addDefines(getValidResourceDefines(kOutputChannels, renderData));

    mpFinalShading["gScene"] = mpScene->getParameterBlock();

    auto var = mpFinalShading["gFinalShading"];
    //auto var = mpFinalShading["CB"]["gFinalShading"];

    var["vbuffer"] = pVBuffer;
    var["vColor"] = pVColor;
    var["frameDim"] = mFrameDim;
    var["numReSTIRInstances"] = mNumReSTIRInstances;
    var["ReSTIRInstanceID"] = instanceID;

    //mpScreenSpaceReSTIR[instanceID]->setShaderData(var["screenSpaceReSTIR"]);
    mpScreenSpaceReSTIR[instanceID]->setShaderDataRoot(mpFinalShading->getRootVar());

    // Bind output channels as UAV buffers.
    var = mpFinalShading->getRootVar();
    auto bind = [&](const ChannelDesc& channel)
    {
        Texture::SharedPtr pTex = renderData[channel.name]->asTexture();
        var[channel.texname] = pTex;
    };
    for (const auto& channel : kOutputChannels) bind(channel);

    mpFinalShading->execute(pRenderContext, mFrameDim.x, mFrameDim.y);
}

void ReSTIRGIPass::updateDict(const Dictionary& dict)
{
    parseDictionary(dict);
    mOptionsChanged = true;
    if (!mpScreenSpaceReSTIR.empty() && mpScreenSpaceReSTIR[0])
    {
        mpScreenSpaceReSTIR[0]->resetReservoirCount();
    }
}
