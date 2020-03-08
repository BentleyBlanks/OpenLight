struct VSOutput
{
	float4 positionH : SV_POSITION;
	float2 texcoord : TEXCOORD;
};

VSOutput VSMain(float4 position : POSITION, float2 texcoord:TEXCOORD)
{
	VSOutput vout ;
	vout.positionH = position;
	vout.texcoord = texcoord;
	return vout;
}
