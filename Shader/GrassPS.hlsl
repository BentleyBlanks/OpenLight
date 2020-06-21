#include"VertexType.hlsl"

Texture2D diffuseMap    :register(t0);
Texture2D alphaMap      :register(t1);
SamplerState samLinear  :register(s0);

float4 GrassMainPS(GrassGSOutput pin):SV_TARGET
{
    float4 diffuse = diffuseMap.Sample(samLinear,pin.texcoord);
    float4 alpha   = alphaMap.Sample(samLinear,pin.texcoord);
    clip(alpha.r - 0.1f);
    return float4(diffuse.rgb,alpha.r);
}