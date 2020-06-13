#include<PBR.hlsl>

RWTexture2D<float4>  irradianceMap      :register(u0);
// 用来预计算 Envrionment Map Irradiance
cbuffer cbIBLInfo:register(b0)
{
    // r: wi 半球上的 Theta 采样数 [0,PI/2]
    // g: wi 半球上的 Phi   采样数 [0,2PI]
    // b: 给定半球上的 Theta 步幅   
    // a: 给定半球上的 Phi   步幅
    float4 IBLDiffuseInfoVec;

    // r: 给定半球上的 Theta 步幅
    // g: 给定半球上的 Phi   步幅
    // b: wi 的总采样数
    // a: roughness
    float4 IBLSpecularInfoVec;

    // r: NdotV 的步幅
    // g: roughness 的步幅
    // b: 采样数
    float4 IBLBSDFInfoVec;

};
Texture2D            envMap             :register(t0);
SamplerState         samLinear          :register(s0);




float radicalInverseVdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
float2 hammersley(uint i, uint N)
{
    return float2(float(i)/float(N), radicalInverseVdC(i));
} 

float3 importanceSampleGGX(float2 Xi, float3 N, float roughness)
{
    const float PI = 3.1415926f;
    float a = roughness*roughness;

    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

    // from spherical coordinates to cartesian coordinates
    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space vector to world-space sample vector
    float3 up        = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent   = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}


// 


// x: i 代表 theta 索引
// y: j 代表 phi   索引
[numthreads(16,16,1)]
void precomputeDiffuseIrradianceMainCS(
    int3 dispatchThreadID : SV_DispatchThreadID)
{
    const float TWOPI = 6.283185307178f;
    const float PI = 3.1415926f;
    const float PI_2 = 1.5707963f;
    // 根据 i j 构建半球法线
    int i = dispatchThreadID.x;
    int j = dispatchThreadID.y;

    // 构建 N
    float thetaN = i * IBLDiffuseInfoVec.b;
    float phiN = j * IBLDiffuseInfoVec.a;
    float sinThetaN = sin(thetaN);

    float3 up = float3(0,1,0);
    float3 normal = float3(sinThetaN * cos(phiN), cos(thetaN), sinThetaN * sin(phiN));
    float3 right = cross(up,normal);
    up  = cross(normal,right);

    // 构建入射光线 wi
    float deltaTheta = PI_2 / IBLDiffuseInfoVec.r;
    float deltaPhi = TWOPI / IBLDiffuseInfoVec.g;
    
    // 计算 irradiance
    float3 irradiance = 0.f;
    int nSamples = 0;
    for(float theta = deltaTheta * 0.5f; theta < PI_2; theta += deltaTheta)
    {
        for(float phi = deltaPhi * 0.5f ; phi < TWOPI; phi += deltaPhi)
        {   
            // theta 为 wi 与 N 的夹角
            float sinTheta = sin(theta);
            float3 localWi = float3(sinTheta * cos(phi),sinTheta * sin(phi),cos(theta));
            float3 globalWi = localWi.x * right + localWi.y * up + localWi.z * normal;
            float2 uv = dir2uv(globalWi);
            irradiance += envMap.SampleLevel(samLinear,uv,0).rgb * cos(theta) * sinTheta ;
            ++nSamples;
        }
    }
    irradiance = PI * irradiance / nSamples;
    irradianceMap[dispatchThreadID.xy] = float4(irradiance,1.f);
}


// 预滤波 HDR 环境贴图
// \int_Li(wi,n)dwi
// x 代表 theta 索引
// y 代表 phi   索引
[numthreads(16, 16, 1)]
void precomputeGGXPrefilterEnvMapMainCS(
    int3 dispatchThreadID:SV_DispatchThreadID)
{
    const float TWOPI = 6.283185307178f;
    const float PI = 3.1415926f;
    const float PI_2 = 1.5707963f;
    // 根据 i j 构建半球法线
    int x = dispatchThreadID.x;
    int y = dispatchThreadID.y;

    // 构建 N
    float thetaN = x * IBLSpecularInfoVec.r;
    float phiN = y * IBLSpecularInfoVec.g;
    float sinThetaN = sin(thetaN);
    float3 up = float3(0,1,0);
    float3 normal = float3(sinThetaN * cos(phiN), cos(thetaN), sinThetaN * sin(phiN));
    float3 right = cross(up,normal);

    float3 R,V,N;
    R = V = N = normal;

    float sumWeight = 0;
    float3 prefilteredColor = 0;

    // 根据 Hammersley 序列生成采样方向
    int sampleCount = (int)IBLSpecularInfoVec.b;
    float roughness = IBLSpecularInfoVec.a;
    for(int i =0;i<sampleCount;++i)
    {
        // 生成 wi
        float2 random = hammersley(i, sampleCount);
        float3 H = importanceSampleGGX(random, N, roughness);
//      float3 L = normalize(2.0 * V.dot(H) * H - V);
        float3 L = normalize(2.0 * dot(V,H) * H - V);
        float cosL = max(dot(N,L), 0.0f);
        if (cosL > 0.0)
        {
            float2 uv = dir2uv(L);
            
            // 根据 UE4 ，使用 cos 值当做权值
            prefilteredColor += envMap.SampleLevel(samLinear,uv,0).rgb * cosL;
            sumWeight += cosL;
        }
    }

    irradianceMap[dispatchThreadID.xy] = float4(prefilteredColor / sumWeight,1.f);
}


// 预计算 BSDF 部分
// x 代表 NdotV 的索引
// y 代表 roughness 的索引
[numthreads(16,16,1)]
void precomputeGGXBSDFMainCS(
    int3 dispatchThreadID:SV_DispatchThreadID
)
{
    int x = dispatchThreadID.x;
    int y = dispatchThreadID.y;
    float NdotV = x * IBLBSDFInfoVec.r;
    float roughness = y * IBLBSDFInfoVec.g;
    int sampleCount = int(IBLBSDFInfoVec.b);

    
    float kIBL = roughness * roughness * 0.5f;

    // N 假设 (0,0,1)
    // 根据 dot(N,V)的值 构建 V
    float3 N = float3(0,0,1);
    float3 V;
    V.x = sqrt(1.0 - NdotV*NdotV);
    V.y = 0.0;
    V.z = NdotV;
    float A = 0,B=0;
    for (int i = 0; i < sampleCount; ++i)
    {
        float2 random = hammersley(i, sampleCount);
        // H = normalize(L + V)
        // 根据 V H 计算 L
        float3 H = importanceSampleGGX(random, N, roughness);
        float3 L  = normalize(2.0 * dot(V, H) * H - V);
        float NdotL = max(L.z, 0.0f);
        float NdotH = max(H.z, 0.0f);
        float VdotH = max(dot(V,H), 0.0f);
        if (NdotL > 0.0f) 
        {
            float G = (Gem(NdotV,kIBL) * Gem(NdotL,kIBL)).r;
            float GVisibility = (G * VdotH) / (NdotV * NdotH);
            float Fc = pow(1.0f - VdotH, 5.0f);
            A += (1.0f - Fc) * GVisibility;
            B += Fc * GVisibility;
        }

    }
    irradianceMap[dispatchThreadID.xy] = float4(float2(A,B)/sampleCount,0,0);
}