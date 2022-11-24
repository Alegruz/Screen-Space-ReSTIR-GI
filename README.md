# Screen-Space-ReSTIR-GI
ReSTIR GI Implementation on Falcor 5.1 based on the source code provided by Daqi Lin for his ReSTIR PT paper

[Original Source Code from ReSTIR PT](https://github.com/DQLin/ReSTIR_PT)

# Integration

1. Add the `Rendering/ReSTIRGI` folder into `Falcor/Source/Falcor/...` directory so that you have a `Falcor/Source/Falcor/Rendering/ReSTIRGI` directory.
2. Add the passes(`ReSTIR` and `ReSTIRGIBuffer` folders) in `RenderPasses` into `Falcor/Source/RenderPasses` directory so that you have a `Falcor/Source/RenderPasses/ReSTIRGI` and `Falcor/Source/RenderPasses/ReSTIRGIGBuffer` directories
3. Add the script `ReSTIRGIPassWithGBuffer` into `Falcor\Source\Mogwai\Data` directory.

Build the Falcor solution, run Mogwai and load the `ReSTIRGIPassWithGBuffer` script.

# 1. Introduction

Global illumination research focuses on methods for computing light transport along some of the paths photons draw. Algorithms that solve the full rendering equation can generate stunning photorealistic images. Those methods, however, are too computationally expensive for real-time applications.

So, why Ray-Traced Global Illumination(RTGI)?

RTGI resembles feature-film image quality. Ray tracing using path tracing with Rendering equation generates correct solution with global illumination.

Recent trends focuses on hybrid rendering
* G-buffer is rendered using rasterization
* Direct lighting and post-processing is done using compute shaders
* GI, reflections, and transparency & translucency are done using pure ray tracing

## ReSTIR GI: Path Resampling for Real-Time Path Tracing

* 2021.06.24. CGF HPG
* Effective path sampling algorithm for indirect lighting that is suitable to highly parallel GPU architectures
Screen-space spatio-temporal resampling
* Resamples multi-bounce indirect lighting paths obtained by path tracing

# 2. Implementation Details

* Uses NVIDIA Falcor 5.1

Initial sampling is implemented with Ray-Traced GBuffer `ReSTIRGIPassWithGBuffer`. GBuffer samples the position, normal, radiance of the visible point, and the sample point scattered from the visible point and its PDF.

Initial Sampling:
1. for each pixel q do  
  1. Retrieve visible point x<sub>v</sub> and normal n<sub>v</sub> from GBuffer (`ReSTIRGIPassWithGBuffer.rt.slang`)
  2. Sample random ray direction &omega;<sub>i</sub> from source PDF p<sub>1</sub> (`ReSTIRGIPassWithGBuffer.rt.slang`)
  3. Trace ray to find sample point x<sub>s</sub> and normal n<sub>s</sub> (`ReSTIRGIPassWithGBuffer.rt.slang`)
  4. Estimate outgoing radiance L<sub>o</sub> at x<sub>s</sub> (`ReSTIRGIPassWithGBuffer.rt.slang`)
  5. InitialSampleBuffer[q] ← SAMPLE(x<sub>v</sub>, n<sub>v</sub>, x<sub>s</sub>, n<sub>s</sub>, L<sub>o</sub>) (`InitialSampling.cs.slang`)

Spatiotemporal resampling is implemented `GIResampling` compute shader.

Temporal resampling (`GIResampling.cs.slang`):

1. for each pixel q do
  1. S ← InitialSampleBuffer[q]
  2. R ← TemporalReservoirBuffer[q]
  3. w ← p^<sub>q</sub>(S) / p<sub>q</sub>(S)
  4. R.UPDATE(S, w)
  5. R.W ← R.w / (R.M · p^(R.z))
  6. TemporalReservoirBuffer[q] ← R

Spatial resampling (`GIResampling.cs.slang`):

1. for each pixel q do
  1. R<sub>s</sub> ← SpatialReservoirBuffer[q]
  2. Q ← q
  3. for s = 1 to maxIterations do
    1. Randomly choose a neighbor pixel q<sub>n</sub>
    2. Calculate geometric similarity betwen q and q<sub>n</sub>
    3. if similarity is lower than the given threshold then
      1. continue
    4. R<sub>n</sub> ← SpatialReservoirBuffer[q<sub>n</sub>]
    5. Calculate |J<sub>q<sub>n</sub> → q</sub>|
    6. p^'<sub>q</sub> ← p^<sub>q</sub>(R<sub>n</sub>.z) / |J<sub>q<sub>n</sub> → q</sub>|
    7. if R<sub>n</sub>'s sample point is not visible to x<sub>v</sub> at q then
      1. p^'<sub>q</sub> ← 0
    8. R<sub>s</sub>.MERGE(R<sub>n</sub>, p^'<sub>q</sub>)
    9. Q ← Q ∩ q<sub>n</sub>
  4. Z ← 0
  5. for each q<sub>n</sub> in Q do
    1. if p^<sub>q<sub>n</sub></sub>(R.z) > 0 then
      1. Z ← Z + R<sub>n</sub>.M
  6. R<sub>s</sub>.W ← R<sub>s</sub>.w / (Z · p^<sub>q</sub>(R<sub>s</sub>.z))
  7. SpatialReservoirBuffer[q] ← R<sub>s</sub>

Final shading (`FinalShading.cs.slang`)

1. for each pixel q do
  1. radiance, weight ← Reservoir[q]
  2. pixel radiance ← radiance × weight

# 3. Results

Hardware:
* CPU: 11th Gen Intel(R) Core(TM) i5-11300H @ 3.10GHz (8 CPUs), ~3.1GHz
* Memory: 16384 MB RAM
* DirectX Version: DirectX 12
* Display:
  * Device: NVIDIA GeForce RTX 3060 Laptop GPU
  * Approx. Total Memory: 14053 MB

MinimalPathTracer without direct illumination took approximately 35~36 ms per frame (28 FPS)
ReSTIR GI took approximately 44~51 ms per frame (22 FPS)

Comparing the results, ReSTIR GI provides better image quality with just a tiny bit of fps drop.

# License from the Original Source Code

Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions
are met:
  * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the distribution.
  * Neither the name of NVIDIA CORPORATION nor the names of its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.