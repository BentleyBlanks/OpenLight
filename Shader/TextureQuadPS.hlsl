

struct PSInput
{
	float4 positionH : SV_POSITION;
	float2 texcoord:TEXCOORD;
};
float toSRGB(float v)
{
    if(v < 0.0031308f)
        return 12.92f * v;

    return 1.055f * pow(v,1.f/2.4f) - 0.055f;    
}

Texture2D tex : register(t0);
SamplerState sam :register(s0);
float4 PSMain(PSInput pin):SV_TARGET
{
	float4 rgba = tex.Sample(sam, pin.texcoord);
	return float4(toSRGB(rgba.r), toSRGB(rgba.g), toSRGB(rgba.b), 1.f);
}