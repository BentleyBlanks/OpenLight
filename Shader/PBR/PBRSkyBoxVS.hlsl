#include"VertexType.hlsl"


cbuffer cbTrans
{
    float4x4 wvp;
    float4x4 world;
    float4x4 invTranspose;
};


// 渲染天空盒的 VS
SkyBoxPSInput SkyBoxMainVS(StandardVSInput vin)
{
    SkyBoxPSInput vout;
    vout.positionH = mul(float4(vin.positionL,1.f),wvp);
    // 天空球的局部坐标，用来采样纹理索引
    vout.positionL = vin.positionL;
    return vout;
}