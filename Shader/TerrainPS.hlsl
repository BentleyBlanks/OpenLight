#include "VertexType.hlsl"
Texture2D       diffuseMap                  :register(t0);
SamplerState    samLinear                   :register(s0);

float4 TerrainMainPS(StandardPSInput pin):SV_TARGET
{
    return diffuseMap.Sample(samLinear,pin.texcoord);
}

struct PSOut
{
    float4 diffuse:SV_TARGET0;
    float4 normal: SV_TARGET1;
};

PSOut TerrainGBufferMainPS(StandardPSInput pin)
{
	PSOut pout;
    pout.diffuse = tex.SamplerState(sam,pin.texcoord);
    pout.normal  = float4(normalize(pin.normalW) * 0.5f + 0.5f,1.f);
    return pout;
}