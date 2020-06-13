#include"VertexType.hlsl"
#include"PBR.hlsl"

cbuffer cbTrans:register(b0)
{
    float4x4        wvp;
    float4x4        world;
    float4x4        invTranspose;
};

StandardPSInput PBRModelMainVS(StandardVSInput vin)
{
    StandardPSInput vout;

    vout.positionH = mul(float4(vin.positionL,1.f),wvp);
    vout.positionW = mul(float4(vin.positionL,1.f),world).xyz;
    vout.normalW = normalize(mul(float4(vin.normalL,0.f),invTranspose).xyz);
    vout.tangentW = normalize(mul(float4(vin.tangentL,0.f),invTranspose).xyz);
    vout.texcoord = vin.texcoord;
    return vout;
}

