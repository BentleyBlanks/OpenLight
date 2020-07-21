Texture2D diffuseMap : register(t0);
Texture2D normalMap  : register(t1);
SamplerState sam :register(s0);

float4 DeferredLightingVSMain(StandardQuadPSInput pin):SV_TARGET
{
    static float3 pointDirs[3] = 
    {
        normalize(float3(1,1,0)),
        normalize(float3(0,1,1)),
        normalize(float3(1,0,1))    
    };
    static float3 pointColors[3]
    {
        normalize(float3(1,1,1)),
        normalize(float3(1,1,1)),
        normalize(float3(0,0,1))
    };
    float3 diffuse = diffuseMap.SamplerState(sam,pin.texcoord).xyz;
    float3 normal = normalMap.SamplerState(sam,pin.texcoord).xyz;
    normal = normalize(normal * 2.f - 1.f);

    float3 color = 0.f;
    [unroll]
    for(int i = 0;i<3;++i)
    {
        float factor = max(dot(pointDirs[i],normal),0.f);
        color += factor * diffuse * pointColors[i];
    }
    return float4(color,1.f);
}