/***************************************************************************
 # Copyright (c) 2015-21, NVIDIA CORPORATION. All rights reserved.
 #
 # NVIDIA CORPORATION and its licensors retain all intellectual property
 # and proprietary rights in and to this software, related documentation
 # and any modifications thereto.  Any use, reproduction, disclosure or
 # distribution of this software and related documentation without an express
 # license agreement from NVIDIA CORPORATION is strictly prohibited.
 **************************************************************************/
import Utils.Math.PackedFormats;

struct PackedSurfaceData
{
    uint3 pos;
    uint depth;
    uint normal;
    uint faceNormal;
    uint weightsAndLobes;
    uint _pad;
}

/** Per-pixel surface data.
*/
struct SurfaceData
{
    float3 pos;                 ///< Position in world-space.
    float depth;                ///< Depth from camera (-1 if invalid data).
    float3 normal;              ///< Shading normal in world-space.
    float3 faceNormal;          ///< Face normal in world-space.
    float diffuseWeight;        ///< Diffuse lobe weight.
    float specularWeight;       ///< Specular lobe weight.
    float specularRoughness;    ///< Specular lobe roughness (linear).
    uint  lobes;                ///< BSDF lobes

    static SurfaceData createFromPacked(const PackedSurfaceData packed)
    {
        SurfaceData surfaceData = {};
        surfaceData.unpack(packed);
        return surfaceData;
    }

    static SurfaceData createInvalid()
    {
        SurfaceData surfaceData = {};
        surfaceData.depth = -1.f;
        return surfaceData;
    }

    bool isValid()
    {
        return depth >= 0.f;
    }

    [mutating] void unpack(const PackedSurfaceData packed)
    {
        pos = asfloat(packed.pos);
        depth = asfloat(packed.depth);
        normal = decodeNormal2x16(packed.normal);
        faceNormal = decodeNormal2x16(packed.faceNormal);
        diffuseWeight = (packed.weightsAndLobes & 0xff) / float(0xff);
        specularWeight = ((packed.weightsAndLobes >> 8) & 0xff) / float(0xff);
        specularRoughness = ((packed.weightsAndLobes >> 16) & 0xff) / float(0xff);
        lobes = ((packed.weightsAndLobes >> 24) & 0xff);
    }

    PackedSurfaceData pack()
    {
        PackedSurfaceData packed = {};
        packed.pos = asuint(pos);
        packed.depth = asuint(depth);
        packed.normal = encodeNormal2x16(normal);
        packed.faceNormal = encodeNormal2x16(faceNormal);
        packed.weightsAndLobes = saturate(diffuseWeight) * 0xff;
        packed.weightsAndLobes |= uint((saturate(specularWeight) * 0xff)) << 8;
        packed.weightsAndLobes |= uint((saturate(specularRoughness) * 0xff)) << 16;
        packed.weightsAndLobes |= (lobes << 24);
        return packed;
    }
};

uint packNormalDepth(float4 normalDepth)
{
    return (encodeNormal2x8(normalDepth.xyz) << 16) | (f32tof16(normalDepth.w) & 0xffff);
}

float4 unpackNormalDepth(uint packed)
{
    return float4(decodeNormal2x8(packed >> 16), f16tof32(packed));
}

/***************************************************************************
 # Copyright (c) 2015-22, NVIDIA CORPORATION. All rights reserved.
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
//import Utils.Math.PackedFormats;
//import Utils.Math.FormatConversion;

//__exported import PackedTypes;

/** Stores data about primary hit points.
    To reduce register pressure, some of the fields are stored in packed format
    and decoded as being accessed through Slang properties.
*/
//struct SurfaceData
//{
//    // The following fields are stored in PackedSurfaceData.
//
//    float3 position;        ///< Position in world-space.
//    float viewDepth;        ///< Distance from viewer to surface (negative for invalid surface).
//    uint packedNormal;      ///< Packed surface normal in world-space (octahedral mapping).
//    uint packedWeights;     ///< Packed diffuse/specular reflectance and roughness.
//
//    // The following fields are *NOT* stored in PackedSurfaceData.
//
//    float3 viewDir;         ///< Direction to the viewer in world-space.
//
//    // True if surface data is valid.
//    property bool valid
//    {
//        get { return viewDepth >= 0.f; }
//    }
//
//    // Shading normal.
//    property float3 normal
//    {
//        get { return decodeNormal2x16(packedNormal); }
//        set { packedNormal = encodeNormal2x16(newValue); }
//    }
//
//    // Diffuse reflectance.
//    property float diffuse
//    {
//        get { return unpackUnorm8(packedWeights); }
//        set { packedWeights = (packedWeights & ~0xff) | packUnorm8(newValue); }
//    }
//
//    // Specular reflectance.
//    property float specular
//    {
//        get { return unpackUnorm8(packedWeights >> 8); }
//        set { packedWeights = (packedWeights & ~0xff00) | (packUnorm8(newValue) << 8); }
//    }
//
//    // Specular roughness.
//    property float roughness
//    {
//        get { return unpackUnorm8(packedWeights >> 16); }
//        set { packedWeights = (packedWeights & ~0xff0000) | (packUnorm8(newValue) << 16); }
//    }
//
//    // Diffuse lobe probability.
//    property float diffuseProb
//    {
//        get { return unpackUnorm8(packedWeights >> 24); }
//        set { packedWeights = (packedWeights & ~0xff000000) | (packUnorm8(newValue) << 24); }
//    }
//
//    /** Initialize an invalid surface.
//    */
//    __init()
//    {
//        this = {};
//        viewDepth = -1.f;
//    }
//
//    /** Initialize a surface.
//        \param[in] packed Packed surface data.
//        \param[in] viewDir View direction.
//    */
//    __init(const PackedSurfaceData packed, const float3 viewDir)
//    {
//        position = asfloat(packed.position);
//        viewDepth = asfloat(packed.viewDepth);
//        packedNormal = packed.normal;
//        packedWeights = packed.weights;
//        this.viewDir = viewDir;
//    }
//
//    /** Pack surface data.
//    */
//    PackedSurfaceData pack()
//    {
//        PackedSurfaceData packed = {};
//        packed.position = asuint(position);
//        packed.viewDepth = asuint(viewDepth);
//        packed.normal = packedNormal;
//        packed.weights = packedWeights;
//        return packed;
//    }
//};
