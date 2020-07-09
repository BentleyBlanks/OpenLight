#include "VertexType.hlsl"


cbuffer cbGrassInfo:register(b0)
{
    float4      cameraPositionW;
    /*
		xy	: grassSize
		z	: wind amplitude
	*/
    float4      grassSize;
    /*
		x	: runTime
		yz	: direction of wind
		w	: speed     of wind
	*/
    float4      windTime;
    float4  	maxDepth;
    float4x4    vp;
}


GrassGSInput GrassMainVS(StandardVSInput vin)
{
    GrassGSInput vout;
    vout.positionW      = vin.positionL;
    float2 windDir      = normalize(float2(windTime.y,windTime.z));
    float windSpeed     = windTime.w;
    vout.texcoord       = vin.texcoord + windTime.x * windSpeed * windDir;
    vout.sizeW          = grassSize.xy;
    return vout;
}