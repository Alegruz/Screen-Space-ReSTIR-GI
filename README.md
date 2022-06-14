# Screen-Space-ReSTIR-GI
ReSTIR GI Implementation on Falcor 5.1 based on the source code provided by Daqi Lin for his ReSTIR PT paper

[Original Source Code from ReSTIR PT](https://github.com/DQLin/ReSTIR_PT)

# Integration

1. Add the `Rendering/ReSTIRGI` folder into `Falcor/Source/Falcor/...` directory so that you have a `Falcor/Source/Falcor/Rendering/ReSTIRGI` directory.
2. Add the passes(`ReSTIR` and `ReSTIRGIBuffer` folders) in `RenderPasses` into `Falcor/Source/RenderPasses` directory so that you have a `Falcor/Source/RenderPasses/ReSTIRGI` and `Falcor/Source/RenderPasses/ReSTIRGIGBuffer` directories
3. Add the script `ReSTIRGIPassWithGBuffer` into `Falcor\Source\Mogwai\Data` directory.

Build the Falcor solution, run Mogwai and load the `ReSTIRGIPassWithGBuffer` script.

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