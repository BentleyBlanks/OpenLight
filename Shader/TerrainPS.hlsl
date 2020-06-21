#include "VertexType.hlsl"
Texture2D       diffuseMap                  :register(t0);
SamplerState    samLinear                   :register(s0);

float4 TerrainMainPS(StandardPSInput pin):SV_TARGET
{
    return diffuseMap.Sample(samLinear,pin.texcoord);
}