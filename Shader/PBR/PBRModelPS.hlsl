#include"VertexType.hlsl"
#include"PBR.hlsl"


cbuffer cbCamera:register(b0)
{
    float4 cameraPositionW;
}

cbuffer cbMaterial:register(b1)
{
    // rgb: albedo
    //   a: alpha 透明度
    float4 materialAlbedo;
    // r: roughness 
    // g: metallic
    // b: ao
    float4 materialInfo;
}

cbuffer cbPointLights:register (b2)
{
    float4 lightPositions[4];
    float4 lightIntensitys[4];
    float4 lightSizeFloat;
}


SamplerState    samLinear                   :register(s0);
Texture2D       diffuseIrradianceMap        :register(t0);
Texture2D       specularPrefilterEnvMap     :register(t1);
Texture2D       specularPreBSDFMap          :register(t2);

float4 PBRModelMainPS(StandardPSInput pin):SV_TARGET
{
 //   return materialAlbedo;
    float3 surfaceNormal = normalize(pin.normalW);
    float3 surfacePosition = pin.positionW;
    float roughness = materialAlbedo.a;

    
    int lightSize = int(lightSizeFloat.r);

    float3 result = float3(0.f,0.f,0.f);
    for(int i =0;i<lightSize;++i)
    {
        result += CookTorrancePointLight(
            cameraPositionW.xyz,
            materialAlbedo.rgb,
            materialInfo.rgb,
            surfacePosition,
            surfaceNormal,
            lightPositions[i].xyz,
            lightIntensitys[i].xyz);
    }
    return float4(result,1.f);
}




float4 PBRModelIBLMainPS(StandardPSInput pin):SV_TARGET
{
    float3 surfaceNormal = normalize(pin.normalW);
    float3 surfacePosition = pin.positionW;

    float3 result = float3(0.f,0.f,0.f);
    result += CookTorranceIBL(
        cameraPositionW.xyz,
        materialAlbedo.rgb,
        materialInfo.rgb,
        surfacePosition,
        surfaceNormal,
        samLinear,
        diffuseIrradianceMap,
        specularPrefilterEnvMap,
        specularPreBSDFMap);

    
    return float4(result,1.f);
}

// IBL + Point Light
float4 PBRModelAllMainPS(StandardPSInput pin):SV_TARGET
{
    float3 surfaceNormal = normalize(pin.normalW);
    float3 surfacePosition = pin.positionW;

    float3 result = float3(0.f,0.f,0.f);

    // 添加 IBL 部分
    result += CookTorranceIBL(
        cameraPositionW.xyz,
        materialAlbedo.rgb,
        materialInfo.rgb,
        surfacePosition,
        surfaceNormal,
        samLinear,
        diffuseIrradianceMap,
        specularPrefilterEnvMap,
        specularPreBSDFMap);

    // 添加 点光源
    int lightSize = int(lightSizeFloat.r);

    for(int i =0;i<lightSize;++i)
    {
        result += CookTorrancePointLight(
            cameraPositionW.xyz,
            materialAlbedo.rgb,
            materialInfo.rgb,
            surfacePosition,
            surfaceNormal,
            lightPositions[i].xyz,
            lightIntensitys[i].xyz);
    }
    return float4(result,1.f);
}
