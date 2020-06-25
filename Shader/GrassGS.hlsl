
#include "VertexType.hlsl"
cbuffer cbGrassInfo:register(b0)
{
    float4      cameraPositionW;
    float4      grassSize;
    float4      windTime;
    float4  	maxDepth;
    float4x4    vp;
}

Texture2D perlinNoiseMap :register(t0);
SamplerState samPoint:register(s0);
/*
    输入一颗草的中心点
    以该中心点开始构建面片
*/
[maxvertexcount(30)]
void GrassMainGS(point GrassGSInput gin[1],
    inout TriangleStream<GrassGSOutput> triStream)
{
    float3 root         = gin[0].positionW;
    float2 halfSize     = gin[0].sizeW * 0.5f;
    
    // 构建顶点
    float3 up      = float3(0,1,0);
    float3 look    = float3(cameraPositionW.x,root.y,cameraPositionW.z) - root;
    float lookDist = length(look);
    look /= lookDist;
    float3 right   = normalize(cross(up,look));
    look           = normalize(cross(right,up));
//    float3 look = float3(0,0,1);
//    float3 right = float3(1,0,0);

    GrassGSOutput v[12];
    v[0].positionW = root - right * halfSize.x;
    v[0].normalW   = look;
    v[0].tangentW  = right;
    v[0].texcoord  = float2(0.f,0.f);
    v[0].positionH = mul(float4(v[0].positionW,1.f),vp);
    v[1].positionW = root + right * halfSize.x;
    v[1].normalW   = look;
    v[1].tangentW  = right;
    v[1].texcoord  = float2(0.99f,0.f);
    v[1].positionH = mul(float4(v[1].positionW,1.f),vp);
    

    [unroll]
    for(int i=1;i<6;++i)
    {
        float windOffset = sin(windTime.x );
        int baseIndex              = i * 2;
        v[baseIndex + 0].positionW = v[0].positionW + i * float3(0.f,halfSize.y,0.f);
        v[baseIndex + 0].positionW += right * 0.2f * i * windOffset;
        v[baseIndex + 0].normalW   = look;
        v[baseIndex + 0].tangentW  = right;
        v[baseIndex + 0].texcoord  = v[0].texcoord + i * float2(0.f,0.199f);
        v[baseIndex + 0].positionH = mul(float4(v[baseIndex + 0].positionW,1.f),vp);

        v[baseIndex + 1].positionW = v[1].positionW + i * float3(0.f,halfSize.y,0.f);
        v[baseIndex + 1].positionW += right * 0.2f * i * windOffset;
        v[baseIndex + 1].normalW   = look;
        v[baseIndex + 1].tangentW  = right;
        v[baseIndex + 1].texcoord  = v[1].texcoord + i * float2(0.f,0.199f);
        v[baseIndex + 1].positionH = mul(float4(v[baseIndex + 1].positionW,1.f),vp);
    }
    for(int j =0;j<5;++j)
    {
        int baseIndex = j*2;
        triStream.Append(v[baseIndex + 0]);
        triStream.Append(v[baseIndex + 2]);
        triStream.Append(v[baseIndex + 1]);
        triStream.RestartStrip();        

        triStream.Append(v[baseIndex + 2]);
        triStream.Append(v[baseIndex + 3]);
        triStream.Append(v[baseIndex + 1]);
        triStream.RestartStrip();     
    }
}

[maxvertexcount(30)]
void GrassCullMainGS(point GrassGSInput gin[1],
    inout TriangleStream<GrassGSOutput> triStream)
{
    float3 root         = gin[0].positionW;
    float2 halfSize     = gin[0].sizeW * 0.5f;
    float  texStride    = 0.199f;

    float lodDist = maxDepth.x;
    // 构建顶点
    float3 up      = float3(0,1,0);
    float3 look    = float3(cameraPositionW.x,root.y,cameraPositionW.z) - root;
    float lookDist = length(look);
    look /= lookDist;
    float3 right   = normalize(cross(up,look));
    look           = normalize(cross(right,up));
//    float3 look = float3(0,0,1);
//    float3 right = float3(1,0,0);

    int quadCount = 5;
    float windStride = 1;;
    if (lookDist > lodDist * 0.5f)
    {
        quadCount  = 1;
        halfSize.y = halfSize.y * 3.f;
        texStride  = texStride * 5.f;
        windStride = 5.f;
    }
    else if (lookDist > lodDist * 0.2f)
    {
        quadCount  = 3;
        halfSize.y = halfSize.y * 1.5f;
        texStride  = texStride * 1.667f;
        windStride = 1.667f;
    }
//     quadCount  = 1;
//    halfSize.y = halfSize.y * 5.f;
//     texStride  = texStride * 5.f;
//     windStride = 5.f;
    float2 noise = perlinNoiseMap.SampleLevel(samPoint,gin[0].texcoord,0.f).xy;
    noise = normalize(noise * 2.f - 1.f);
    float3 windDir = float3(noise.x,0.f,noise.y);
    GrassGSOutput v[12];
    [unroll]
    for (int pi = 0; pi < 6; ++pi)
    {
        int baseIndex = pi * 2;
    
        v[baseIndex + 0].positionW = root - right * halfSize.x;
        v[baseIndex + 0].normalW   = look;
        v[baseIndex + 0].tangentW  = right;
        v[baseIndex + 0].texcoord  = float2(0.f,0.f);
        v[baseIndex + 0].positionH = mul(float4(v[0].positionW,1.f),vp);
        v[baseIndex + 1].positionW = root + right * halfSize.x;
        v[baseIndex + 1].normalW   = look;
        v[baseIndex + 1].tangentW  = right;
        v[baseIndex + 1].texcoord  = float2(0.99f,0.f);
        v[baseIndex + 1].positionH = mul(float4(v[1].positionW,1.f),vp);
    }
  
    [unroll]
    for(int i=1;i<= quadCount;++i)
    {
        float windOffset = sin(windTime.x );
//        float windOffset = 1.f;
        int baseIndex              = i * 2;
        v[baseIndex + 0].positionW = v[0].positionW + i * float3(0.f,halfSize.y,0.f);
        v[baseIndex + 0].positionW += windDir * 0.2f * i * windStride * windOffset;
        v[baseIndex + 0].normalW   = look;
        v[baseIndex + 0].tangentW  = right;
        v[baseIndex + 0].texcoord  = v[0].texcoord + i * float2(0.f,texStride);
        v[baseIndex + 0].positionH = mul(float4(v[baseIndex + 0].positionW,1.f),vp);

        v[baseIndex + 1].positionW = v[1].positionW + i * float3(0.f,halfSize.y,0.f);
        v[baseIndex + 1].positionW += windDir * 0.2f * i * windStride * windOffset;
        v[baseIndex + 1].normalW   = look;
        v[baseIndex + 1].tangentW  = right;
        v[baseIndex + 1].texcoord  = v[1].texcoord + i * float2(0.f,texStride);
        v[baseIndex + 1].positionH = mul(float4(v[baseIndex + 1].positionW,1.f),vp);
    }

    [unroll]
    for(int j =0;j< quadCount;++j)
    {
        int baseIndex = j*2;
        triStream.Append(v[baseIndex + 0]);
        triStream.Append(v[baseIndex + 2]);
        triStream.Append(v[baseIndex + 1]);
        triStream.RestartStrip();        

        triStream.Append(v[baseIndex + 2]);
        triStream.Append(v[baseIndex + 3]);
        triStream.Append(v[baseIndex + 1]);
        triStream.RestartStrip();     
    }
}