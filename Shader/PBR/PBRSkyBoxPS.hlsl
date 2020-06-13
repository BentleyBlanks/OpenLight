#include<VertexType.hlsl>
#include<PBR.hlsl>

SamplerState samLinear;
Texture2D envMap;


float4 SkyBoxMainPS(SkyBoxPSInput pin):SV_TARGET
{
    float4 color = float4(0.f,0.f,0.f,0.f);
    
    // 计算索引
    // 局部光线
    float3 localDir  = normalize(pin.positionL);

    // 根据局部光线的 theta phi 计算 uv
    float2 uv = dir2uv(localDir);

    color = envMap.SampleLevel(samLinear,uv,0);
    return color;
}


