from falcor import *

def render_graph_ReSTIRGI():
    g = RenderGraph("ReSTIRGI")
    loadRenderPassLibrary("AccumulatePass.dll")
    loadRenderPassLibrary("GBuffer.dll")
    loadRenderPassLibrary('ReSTIRGIGBuffer.dll')
    loadRenderPassLibrary("ReSTIRGIPass.dll")
    loadRenderPassLibrary("RTXDIPass.dll")
    loadRenderPassLibrary("GlobalIlluminationPass.dll")
    loadRenderPassLibrary("ToneMapper.dll")

    VBufferRT = createPass("VBufferRT")
    g.addPass(VBufferRT, "VBufferRT")

    ReSTIRGIGBuffer = createPass('ReSTIRGIGBuffer', {'maxBounces': 1, 'useImportanceSampling': True})
    g.addPass(ReSTIRGIGBuffer, 'ReSTIRGIGBuffer')

    ReSTIRGIPass = createPass("ReSTIRGIPass")
    g.addPass(ReSTIRGIPass, "ReSTIRGIPass")

    RTXDIPass = createPass("RTXDIPass")
    g.addPass(RTXDIPass, "RTXDIPass")

    GlobalIlluminationPass = createPass("GlobalIlluminationPass")
    g.addPass(GlobalIlluminationPass, "GlobalIlluminationPass")

    AccumulatePass = createPass("AccumulatePass", {'enabled': False, 'precisionMode': AccumulatePrecision.Single})
    g.addPass(AccumulatePass, "AccumulatePass")

    ToneMapper = createPass("ToneMapper", {'autoExposure': False, 'exposureCompensation': 0.0})
    g.addPass(ToneMapper, "ToneMapper")

    g.addEdge('VBufferRT.vbuffer', 'ReSTIRGIGBuffer.vbuffer')
    g.addEdge('VBufferRT.viewW', 'ReSTIRGIGBuffer.viewW')

    g.addEdge("ReSTIRGIGBuffer.vPosW", "ReSTIRGIPass.vPosW")
    g.addEdge("ReSTIRGIGBuffer.vNormW", "ReSTIRGIPass.vNormW")
    g.addEdge("ReSTIRGIGBuffer.random", "ReSTIRGIPass.random")
    g.addEdge("ReSTIRGIGBuffer.sPosW", "ReSTIRGIPass.sPosW")
    g.addEdge("ReSTIRGIGBuffer.sNormW", "ReSTIRGIPass.sNormW")
    g.addEdge("ReSTIRGIGBuffer.vColor", "ReSTIRGIPass.vColor")
    g.addEdge("ReSTIRGIGBuffer.sColor", "ReSTIRGIPass.sColor")

    g.addEdge("VBufferRT.vbuffer", "ReSTIRGIPass.vbuffer")
    g.addEdge("VBufferRT.mvec", "ReSTIRGIPass.motionVectors")

    g.addEdge("VBufferRT.vbuffer", "RTXDIPass.vbuffer")
    g.addEdge("VBufferRT.mvec", "RTXDIPass.mvec")

    g.addEdge("RTXDIPass.color", "GlobalIlluminationPass.diColor")
    g.addEdge("ReSTIRGIPass.color", "GlobalIlluminationPass.giColor")

    g.addEdge("GlobalIlluminationPass.color", "AccumulatePass.input")

    g.addEdge("AccumulatePass.output", "ToneMapper.src")

    g.markOutput('ReSTIRGIGBuffer.vPosW')
    g.markOutput('ReSTIRGIGBuffer.vNormW')
    g.markOutput('ReSTIRGIGBuffer.vColor')
    g.markOutput('ReSTIRGIGBuffer.random')
    g.markOutput('ReSTIRGIGBuffer.sPosW')
    g.markOutput('ReSTIRGIGBuffer.sNormW')
    g.markOutput('ReSTIRGIGBuffer.vColor')
    g.markOutput('ReSTIRGIGBuffer.sColor')
    g.markOutput("ToneMapper.dst")

    return g

ReSTIRGI = render_graph_ReSTIRGI()
try: m.addGraph(ReSTIRGI)
except NameError: None
