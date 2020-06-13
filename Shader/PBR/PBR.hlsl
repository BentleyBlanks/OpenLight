
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

float2 dir2uvIBL(float3 dir)
{
    float theta = acos(dir.y);
    float phi = atan2(dir.x, dir.z); 
    if (phi < 0.f) phi += 6.283185307178f;
    float2 uv = float2(theta * 0.31830988618379067154,phi * 0.15915494309189533577);
    return uv;
}


float3 Fresnel(
    in float3 h,
    in float3 v,
    in float3 F0)
{
    // Fresnel-Schlick approximation
    float dotHV = max(dot(h,v),0.f);
    return F0 + (1-F0) * pow(1- dotHV,5.f);
}

float3 FresnelSchlickRoughness(float cosTheta, 
    float3 F0, 
    float roughness)
{
    float3 t = float3(1- roughness,1-roughness,1-roughness);

    return F0 + (max(t,F0) - F0) * pow(1.0 - cosTheta, 5.0);
}   

float3 GGX(float cosThetaH,float roughness)
{
    const float PI = 3.1415926f;
    // GGX Distritubution
    float r = roughness * roughness;

    float r2 = r * r;
    float h2 = cosThetaH * cosThetaH;
    float t = h2 * (r2 - 1) + 1;
    return r2 / (PI * t *t);
}

float3 Gem(float cosThetaV,float k)
{
    return cosThetaV / (cosThetaV * (1-k) + k);
}


float3 CookTorrancePointLight(
    float3 cameraPosition,
    float3 surfaceAlbedo,
    // r: roughness 
    // g: metallic
    // b: ao
    float3 surfaceInfo,
    float3 surfacePosition,
    float3 surfaceNormal,
    float3 lightPosition,
    float3 lightIntensity)
{
    const float PI = 3.1415926f;
    
    float roughness = surfaceInfo.r;
    float metallic = surfaceInfo.g;
    float ao = surfaceInfo.b;


    float3 result = float3(0.f,0.f,0.f);

    
    float3 L = lightPosition - surfacePosition;
    float3 V = cameraPosition - surfacePosition;
    float LDist = length(L);
    float VDist = length(V);
    if(LDist == 0.f || VDist == 0.f)
        return float3(0.f,0.f,0.f);
    L /= LDist;
    V /= VDist;
    float3 H = normalize(L + V);

    float3 attenuation = 1.f / (LDist * LDist);
    // diffuse
    float cosTheta = max(dot(L,surfaceNormal),0.f);
    float3 BSDFLambertian = (surfaceAlbedo  / PI) * cosTheta;

    // Cook-Torrance
    float cosThetaV = max(dot(V,surfaceNormal), 0.f);
    float cosThetaL = max(dot(L,surfaceNormal), 0.f);
    float cosThetaH = max(dot(H,surfaceNormal), 0.f);
    float3 F0 = float3(0.04f,0.04f,0.04f);
    F0 = (1.f - metallic) * F0 + metallic * surfaceAlbedo;

    float kdirect = (roughness + 1 ) * (roughness + 1) * 0.125f;

    float3 D = GGX(cosThetaH,roughness);
    float3 G = Gem(cosThetaV,kdirect) * Gem(cosThetaL,kdirect);
    float3 F = Fresnel(H,V,F0);
    float3 BSDFCookTorrance = (D * F * G) / (4 * cosThetaV + 1e-4);

    float3 ks = F;
    float3 kd = 1.f - ks; 
    kd *= (1.f - metallic);

    result = (kd* BSDFLambertian + BSDFCookTorrance) * lightIntensity * attenuation;

    return result;
}






float3 CookTorranceIBL(
    float3 cameraPosition,
    float3 surfaceAlbedo,
    // r: roughness 
    // g: metallic
    // b: ao
    float3 surfaceInfo,
    float3 surfacePosition,
    float3 surfaceNormal,
    in SamplerState samLinear,
    in Texture2D diffuseIrradianceMap,
    in Texture2D specularPrefilterEnvMap,
    in Texture2D specularPreBSDFMap)
{


    

    const float PI = 3.1415926f;
    
    float roughness = surfaceInfo.r;
    float metallic = surfaceInfo.g;
    float ao = surfaceInfo.b;

    float3 F0 = float3(0.04f,0.04f,0.04f);
    F0 = (1.f - metallic) * F0 + metallic * surfaceAlbedo;
    
    float3 N = surfaceNormal;
    float3 V = normalize(cameraPosition - surfacePosition);

    // OIT 时，当 实现夹角 小于 0 时 翻转 N
    if( dot(N,V) <= 0.f )
        N = -N;


    // 由于没有采样过程
    // 无法得到计算 Fresnel 的 半向量 H
    // 用 N 和 V 来近似，同时用考虑 roughness 的 Fresnel 来缓解该问题 (F 代替 F0)
    float3 ks = FresnelSchlickRoughness(max(dot(N,V),0.f),F0,roughness);
    float3 kd = 1.f - ks;
    kd  = kd * (1 - metallic);

    
    // 获取 diffuse 部分
    float3 diffuseIrradiance = diffuseIrradianceMap.SampleLevel(samLinear,dir2uvIBL(surfaceNormal),0.f).rgb;
    float3 diffuse = surfaceAlbedo * diffuseIrradiance;

    // 获取 specular 部分 
    const int MipmapLevelMaxIndex = 4;
    float3 R =  reflect(-V,N);
    float3 prefilter = specularPrefilterEnvMap.SampleLevel(samLinear,dir2uvIBL(R),roughness * MipmapLevelMaxIndex).rgb;
    float NdotV = clamp(dot(N,V),0.f,0.99f);
    float2 envBRDF = specularPreBSDFMap.SampleLevel(samLinear,float2(max(NdotV,0.f),roughness),0.f).rg;
    float3 specular = prefilter * (ks * envBRDF.x + envBRDF.y);

    float3 ambient  = (kd * diffuse + specular) * ao;

    return ambient;
}