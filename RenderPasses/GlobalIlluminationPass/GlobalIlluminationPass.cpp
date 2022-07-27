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
#include "GlobalIlluminationPass.h"

const RenderPass::Info GlobalIlluminationPass::kInfo { "GlobalIlluminationPass", "Add two input into one." };

// Don't remove this. it's required for hot-reload to function properly
extern "C" FALCOR_API_EXPORT const char* getProjDir()
{
    return PROJECT_DIR;
}

extern "C" FALCOR_API_EXPORT void getPasses(Falcor::RenderPassLibrary& lib)
{
    lib.registerPass(GlobalIlluminationPass::kInfo, GlobalIlluminationPass::create);
}

namespace
{
    const char kShaderFile[] = "RenderPasses/GlobalIlluminationPass/GlobalIlluminationPass.3d.slang";
    const std::string kShaderModel = "6_5";

    const std::string kInputDiColor = "diColor";
    const std::string kInputGiColor = "giColor";

    const ChannelList kInputChannels =
    {
        { kInputDiColor,    "gDiColor",     "Direct Illumination Color" },
        { kInputGiColor,    "gGiColor",     "Global Illumination Color" },
    };

    const ChannelList kOutputChannels =
    {
        { "color",          "gColor", "Output color (sum of direct and indirect)", false, ResourceFormat::RGBA32Float },
    };
}

GlobalIlluminationPass::SharedPtr GlobalIlluminationPass::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    SharedPtr pPass = SharedPtr(new GlobalIlluminationPass());
    return pPass;
}

Dictionary GlobalIlluminationPass::getScriptingDictionary()
{
    return Dictionary();
}

RenderPassReflection GlobalIlluminationPass::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;

    addRenderPassInputs(reflector, kInputChannels);
    addRenderPassOutputs(reflector, kOutputChannels);
    //reflector.addOutput("dst");
    //reflector.addInput("src");
    return reflector;
}

void GlobalIlluminationPass::compile(RenderContext* pRenderContext, const CompileData& compileData)
{
    mFrameDim = compileData.defaultTexDims;
}

void GlobalIlluminationPass::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    // Clear outputs if no scene is loaded.
    if (!mpScene)
    {
        clearRenderPassChannels(pRenderContext, kOutputChannels, renderData);
        return;
    }

    // renderData holds the requested resources
    // auto& pTexture = renderData["src"]->asTexture();
    const auto& pGiColor = renderData[kInputDiColor]->asTexture();
    const auto& pDiColor = renderData[kInputGiColor]->asTexture();

    if (!mpShader)
    {
        Program::Desc desc;
        desc.addShaderLibrary(kShaderFile).setShaderModel(kShaderModel).csEntry("main");
        desc.addTypeConformances(mpScene->getTypeConformances());

        auto defines = mpScene->getSceneDefines();
        mpShader = ComputePass::create(desc, defines, false);
        mpShader->setVars(nullptr);
    }

    auto var = mpShader["gGIPass"];
    var["giColor"] = pGiColor;
    var["diColor"] = pDiColor;
    var["frameDim"] = mFrameDim;

    // Bind output channels as UAV buffers.
    var = mpShader->getRootVar();
    auto bind = [&](const ChannelDesc& channel)
    {
        Texture::SharedPtr pTex = renderData[channel.name]->asTexture();
        var[channel.texname] = pTex;
    };
    for (const auto& channel : kOutputChannels) bind(channel);

    mpShader->execute(pRenderContext, mFrameDim.x, mFrameDim.y);
}

void GlobalIlluminationPass::renderUI(Gui::Widgets& widget)
{
}

void GlobalIlluminationPass::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
{
    mpScene = pScene;
    mpShader = nullptr;
}
