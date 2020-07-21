
#include "VertexType.hlsl"

struct PSOut
{
    float4 diffuse:SV_TARGET0;
    float4 normal: SV_TARGET1;
};

Texture2D tex : register(t0);
SamplerState sam :register(s0);
PSOut PSMain(StandardPSInput pin)
{
	PSOut pout;
    pout.diffuse = tex.SamplerState(sam,pin.texcoord);
    pout.normal  = float4(normalize(pin.normalW) * 0.5f + 0.5f,1.f);
    return pout;
}