#include"VertexType.hlsl"

Texture2D envMap        :register(t0);
SamplerState samLinear  :register(s0);


// 辅助函数
float2 dir2uv(float3 dir)
{
    // 根据局部光线的 theta phi 计算 uv
    float theta = acos(dir.y);
    float phi = atan2(dir.x, -dir.z); 
    
    if (phi < 0.f) phi += 6.283185307178f;
    float2 uv = float2(phi * 0.15915494309189533577,-theta * 0.31830988618379067154);

    return uv;
}

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


