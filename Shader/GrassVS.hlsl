#include "VertexType.hlsl"


cbuffer cbGrassInfo:register(b0)
{
    float4      cameraPositionW;
    float4      grassSize;
    float4      windTime;
    float4x4    vp;
}


GrassGSInput GrassMainVS(StandardVSInput vin)
{
    GrassGSInput vout;
    vout.positionW  = vin.positionL;
    vout.sizeW      = grassSize.xy;
    return vout;
}